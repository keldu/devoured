#include "network.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <endian.h>

#include <unistd.h>
#include <errno.h>
#include <cstring>

#include <cassert>
#include <iostream>

const size_t read_buffer_size = 4096;
const size_t write_buffer_size = 4096;

namespace dvr {
	IFdOwner::IFdOwner(EventPoll& p, int file_d, int flags_d, uint32_t msk):
		poll{p},
		file_desc{file_d},
		event_mask{msk}
	{
		poll.subscribe(*this);
	}

	IFdOwner::~IFdOwner(){
		poll.unsubscribe(*this);
	}

	int IFdOwner::fd()const{
		return file_desc;
	}

	uint32_t IFdOwner::mask()const{
		return event_mask;
	}
	
	const size_t max_events = 256;

	thread_local EventPoll* thread_local_event_poll = nullptr;

	class EventPoll::Impl {
	private:
		std::map<int, IFdOwner*> observers;

		int epoll_fd;
		bool broken;

		::epoll_event events[max_events];
	public:
		Impl():
			broken{false}
		{
			epoll_fd = epoll_create1(0);
			if(epoll_fd < 0){
				broken = true;
			}
		}

		~Impl(){
			if(epoll_fd > 0){
				close(epoll_fd);
			}
		}

		bool poll(){
			if(broken){
				return true;
			}
			int nfds = ::epoll_wait(epoll_fd, events, max_events, -1);
			if(nfds < 0){
				return broken = true;
			}

			for(int n = 0; n < nfds; ++n){
				std::cout<<"FD "<<events[n].data.fd<<std::endl;
				IFdOwner* owner = reinterpret_cast<IFdOwner*>(events[n].data.ptr);
				owner->notify(events[n].events);
			}

			return broken;
		}

		void subscribe(IFdOwner& obsv){
			if(!broken){
				int fd = obsv.fd();
				assert(fd >= 0);
				::epoll_event event;
				memset(&event, 0, sizeof(event));
				event.events = obsv.mask() | EPOLLET;
				event.data.fd = fd;
				event.data.ptr = &obsv;
				
				if(::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)){
					broken = true;
					return;
				}
			}
		}

		void unsubscribe(IFdOwner& obsv){
			if(!broken){
				std::cout<<"Unsubscribed observer"<<std::endl;
				int fd = obsv.fd();
				assert(fd >= 0);
				::epoll_event event;
				event.events = obsv.mask() | EPOLLET;

				if(::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr)){
					broken = true;
					return;
				}
			}
		}
	};

	EventPoll::EventPoll():
		impl{std::make_unique<EventPoll::Impl>()}
	{
	}
	EventPoll::~EventPoll(){}

	bool EventPoll::poll(){
		return impl->poll();
	}

	void EventPoll::subscribe(IFdOwner& obv){
		impl->subscribe(obv);
	}

	void EventPoll::unsubscribe(IFdOwner& obv){
		impl->unsubscribe(obv);
	}

	void EventPoll::enterScope(){
		assert(thread_local_event_poll == nullptr);
		thread_local_event_poll = this;
	}

	void EventPoll::leaveScope(){
		assert(thread_local_event_poll == this);
		thread_local_event_poll = nullptr;
	}
	
	class FdStream final: public IFdOwner, public IoStream {
	private:
		EventPoll& poll;
		IStreamStateObserver& observer;
		
		bool is_broken;

		// non blocking helpers
		// write buffering
		bool write_ready;
		size_t already_written;
		std::vector<uint8_t> write_buffer;

		// read buffering 
		bool read_ready;
		size_t already_read;
		std::vector<uint8_t> read_buffer;
		//
		void onReadyWrite(){
			while(write_ready && write_buffer.size() > 0){
				ssize_t n = ::send(fd(), write_buffer.data(), write_buffer.size(), MSG_NOSIGNAL);
				if(n<0){
					if(errno != EAGAIN){
						close();
					}
					write_ready = false;
					return;
				}else if (n==0){
					close();
					write_ready = false;
					return;
				}

				already_written += n;
			
				size_t remaining = write_buffer.size() - already_written;
				for(size_t i = 0; i < remaining; ++i){
					write_buffer[i] = write_buffer[already_written+i];
				}
				write_buffer.resize(remaining);
				remaining = 0;
			}
		}
		void onReadyRead(){
			if(broken()){
				return;
			}
			do {
				// Not yet read into the buffer
				size_t remaining = read_buffer.size() - already_read;
				ssize_t n = ::recv(fd(), &read_buffer[already_read], remaining, 0);

				if(n < 0){
					if(errno != EAGAIN){
						close();
					}
					read_ready = false;
					return;
				}else if(n == 0){
					close();
					read_ready = false;
				}
				already_read += static_cast<size_t>(n);
			}while(read_ready);
		}
	public:
		FdStream(EventPoll& p, int fd, int flags, IStreamStateObserver& obsrv):
			IFdOwner(p, fd, flags, EPOLLIN | EPOLLOUT),
			poll{p},
			observer{obsrv},
			is_broken{false},
			write_ready{true},
			already_written{0},
			read_ready{true},
			already_read{0}
		{
			read_buffer.resize(read_buffer_size);
		}
		~FdStream(){
			close();
		}

		void notify(uint32_t mask) override {
			if( broken() ){
				return;
			}
			if( mask & EPOLLOUT ){
				write_ready = true;
				onReadyWrite();
				observer.notify(*this, IoStreamState::WriteReady);
			}
			if( mask & EPOLLIN ){
				read_ready = true;
				onReadyRead();
				observer.notify(*this, IoStreamState::ReadReady);
			}
		}

		/*
		 * move buffer to the writeQueue
		 */
		void write(std::vector<uint8_t>&& buffer) override {
			if(write_buffer.empty()){
				write_buffer = std::move(buffer);
			}else /*if (write_buffer.size() + buffer.size() <= write_buffer_size)*/{
				write_buffer.insert(std::end(write_buffer), std::begin(buffer),std::end(buffer));
			}/*else{
			}*/
			if(write_ready){
				onReadyWrite();
			}
		}
		bool hasWriteQueued() const override {
			return !write_buffer.empty();
		}
		
		/* 
		 * get front buffer in read and the already read hint.
		 */
		std::optional<uint8_t*> read(size_t n) override {
			if(already_read < n){
				if( read_ready ){
					onReadyRead();
					return read(n);
				}
				return std::nullopt;
			}
			//auto front = std::move(read_reads.front());
			//ready_reads.pop();
			return read_buffer.data();
		}
		void consumeRead(size_t n) override {
			size_t remaining = n < read_buffer.size()?(read_buffer.size()-n):(0);
			for(size_t i = 0; i < remaining; ++i){
				read_buffer[i] = read_buffer[i+n];
			}
			assert(already_read>=n);
			already_read -= n;
		}
		
		bool hasReadQueued() const override {
			return !read_buffer.empty();
		}

		/*
		 * checks if stream is broken or not
		 */
		void close() {
			if(broken()){
				return;
			}
			is_broken = true;
			read_ready = false;
			write_ready = false;
			std::cerr<<"Test"<<std::endl;
			observer.notify(*this, IoStreamState::Broken);
		}
		bool broken() const {
			return is_broken;
		}

		int id() const override {
			return fd();
		}
	};
	
	UnixSocketAddress::UnixSocketAddress(EventPoll& p, const std::string& unix_addr):
		poll{p},
		bind_address{unix_addr}
	{}

	std::unique_ptr<Server> UnixSocketAddress::listen(IServerStateObserver& obsrv){
		int file_descriptor = ::socket( AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0 );
		//TODO Missing errno check in every state

		if(file_descriptor < 0){
			std::cerr<<"Couldn't create socket: "<<(::strerror(errno))<<std::endl;
			return nullptr;
		}

		int status;

		struct ::sockaddr_un local;
		local.sun_family = AF_UNIX;
		size_t len = (sizeof(local.sun_path)-1) < bind_address.size() ? (sizeof(local.sun_path)-1): bind_address.size();
		::strncpy(local.sun_path, bind_address.c_str(), len);
		local.sun_path[len] = 0;

#ifdef NDEBUG
		::unlink(local.sun_path);
#endif

		status = ::bind(file_descriptor, (struct ::sockaddr*)&local, sizeof(local));
		if( status != 0){
			std::cerr<<"Couldn't bind socket: "<<local.sun_path<<std::endl;
			return nullptr;
		}

		::listen(file_descriptor, SOMAXCONN);

		return std::make_unique<Server>(poll, file_descriptor, bind_address, obsrv);
	}
	
	std::unique_ptr<IoStream> UnixSocketAddress::connect(IStreamStateObserver& obsrv){
		int file_descriptor = ::socket( AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0 );

		if(file_descriptor < 0){
			std::cerr<<"Couldn't create socket: "<<(::strerror(errno))<<std::endl;
			return nullptr;
		}

		int status;

		struct ::sockaddr_un local;
		local.sun_family = AF_UNIX;
		size_t len = (sizeof(local.sun_path)-1) < bind_address.size() ? (sizeof(local.sun_path)-1): bind_address.size();
		::strncpy(local.sun_path, bind_address.c_str(), len);
		local.sun_path[len] = 0;

		status = ::connect(file_descriptor, (struct ::sockaddr*)&local, sizeof(local));
		if( status != 0){
			std::cerr<<"Couldn't connect to socket at "<<local.sun_path<<" : "<<(::strerror(errno))<<std::endl;
			return nullptr;
		}

		return std::make_unique<FdStream>(poll, file_descriptor, 0, obsrv);
	}

	/*
	 * Based on EPoll notification accept, read or write in a non blocking way
	 * and queue reads and/or handle writes
	 */
	Server::Server(EventPoll& p, int fd, const std::string& addr, IServerStateObserver& obs):
		IFdOwner(p, fd, 0, EPOLLIN),
		event_poll{p},
		address{addr},
		observer{obs}
	{
	}

	Server::~Server(){
		::unlink(address.c_str());
	}

	void Server::notify(uint32_t mask){
		if(mask & EPOLLIN){
			observer.notify(*this, ServerState::Accept);
		}
	}

	std::unique_ptr<IoStream> Server::accept(IStreamStateObserver& obsrv){
		struct ::sockaddr_storage addr;
		socklen_t addr_len = sizeof(addr);

		int accepted_fd = ::accept4(fd(), reinterpret_cast<struct ::sockaddr*>(&addr), &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

		if(accepted_fd<0) {
			//TODO Error checking
			//int error = errno;
			return nullptr;
		}

		return std::make_unique<FdStream>(event_poll, accepted_fd, 0, obsrv);
	}

	const std::string& UnixSocketAddress::getPath()const{
		return bind_address;
	}

	class UnixAsyncIoProvider : public AsyncIoProvider {
	private:
		EventPoll events;
		WaitScope wait_scope;
	public:
		UnixAsyncIoProvider():
			wait_scope{events}
		{}

		EventPoll& eventPoll() {
			return events;
		}

		WaitScope& waitScope() {
			return wait_scope;
		}

		std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& path) override {
			// TODO check socket address validity
			if( false ){
				return nullptr;
			}
			return std::make_unique<UnixSocketAddress>(events, path);
		}

		std::unique_ptr<InputStream> wrapInputFd(int fd, IStreamStateObserver& obsrv, uint32_t flags = 0) override {
			return std::make_unique<FdStream>(events, fd, flags, obsrv);
		}
		
		std::unique_ptr<OutputStream> wrapOutputFd(int fd, IStreamStateObserver& obsrv, uint32_t flags = 0) override {
			return std::make_unique<FdStream>(events, fd, flags, obsrv);
		}
	};

	WaitScope::WaitScope(EventPoll& p):
		e_poll{p}
	{
		e_poll.enterScope();
	}

	WaitScope::~WaitScope(){
		e_poll.leaveScope();
	}

	void WaitScope::poll(){
		e_poll.poll();
	}

	AsyncIoContext setupAsyncIo(){
		std::unique_ptr<UnixAsyncIoProvider> provider = std::make_unique<UnixAsyncIoProvider>();
		auto& event_poll = provider->eventPoll();
		auto& wait_scope = provider->waitScope();
		return {std::move(provider), event_poll, wait_scope};
	}
}
