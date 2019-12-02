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

// Don't remember the value of 2^16 - 1 so 1024 * 64 - 1 will have to do
const size_t read_buffer_size = 4096;

const uint16_t max_message_size = 4095 - 2;
const uint16_t max_target_size = 255;
const size_t message_length_size = 2;
/*
 * So This is the rest of the max size. We have 
 * 3 uint16_t values and
 * 2 uint8_t values
 * Subtrace all these and the other max sizes and you get the resulting protocol
 * This should be guaranteed to be > 0.
 * Check with a calculator if you change this or sth similar :)
 */

const uint16_t request_static_size = 4 * 64;
const uint16_t max_request_content_size = max_message_size - max_target_size - request_static_size;
// Is by coincidence the same
const uint16_t response_static_size = 4 * 64;
const uint16_t max_response_content_size = max_message_size - max_target_size - response_static_size;


namespace dvr {
	MessageRequest::MessageRequest(uint16_t rid_p, uint8_t type_p, const std::string& target_p, const std::string& content_p):
		request_id{rid_p},
		type{type_p},
		target{target_p},
		content{content_p}
	{}

	IFdObserver::IFdObserver(EventPoll& p, int file_d, uint32_t msk):
		poll{p},
		file_desc{file_d},
		event_mask{msk}
	{
		poll.subscribe(*this);
	}

	IFdObserver::~IFdObserver(){
		poll.unsubscribe(*this);
	}

	int IFdObserver::fd()const{
		return file_desc;
	}

	uint32_t IFdObserver::mask()const{
		return event_mask;
	}
	
	const size_t max_events = 256;

	class EventPoll::Impl {
	private:
		std::map<int, IFdObserver*> observers;

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
				int fd_event = events[n].data.fd;
				auto finder = observers.find(fd_event);
				if(finder == observers.end()){
					return broken = true;
				}
				finder->second->notify(events[n].events);
			}

			return broken;
		}

		void subscribe(IFdObserver& obsv){
			if(!broken){
				int fd = obsv.fd();
				::epoll_event event;
				event.events = obsv.mask();
				event.data.fd = fd;
				
				if(::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)){
					broken = true;
					return;
				}

				observers.insert(std::make_pair(fd, &obsv));
			}
		}

		void unsubscribe(IFdObserver& obsv){
			if(!broken){
				int fd = obsv.fd();
				::epoll_event event;
				event.events = obsv.mask();
				event.data.fd = fd;

				if(::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event)){
					broken = true;
					return;
				}

				observers.erase(fd);
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

	void EventPoll::subscribe(IFdObserver& obv){
		impl->subscribe(obv);
	}

	void EventPoll::unsubscribe(IFdObserver& obv){
		impl->unsubscribe(obv);
	}

	Stream::Stream(int fd):
		file_descriptor{fd}
	{}

	size_t Stream::write(uint8_t* buffer, size_t length){
		ssize_t len = send(file_descriptor, buffer, length, 0);
		if(len < 0){
			close(file_descriptor);
			file_descriptor = -1;
			return 0;
		}
		return static_cast<size_t>(len);
	}

	size_t Stream::read(uint8_t* buffer, size_t length){
		ssize_t len = recv(file_descriptor, buffer, length, 0);
		if(len < 0){
			close(file_descriptor);
			file_descriptor = -1;
			return 0;
		}
		return static_cast<size_t>(len);
	}

	const int& Stream::fd() const{
		return file_descriptor;
	}

	bool Stream::broken() const {
		return file_descriptor < 0;
	}

	StreamAcceptor::StreamAcceptor(const std::string& sp, int fd):
		socket_path{sp},
		file_descriptor{fd}
	{
	}

	StreamAcceptor::~StreamAcceptor()
	{
		if(file_descriptor >= 0){
			::unlink(socket_path.c_str());
		}
	}

	std::unique_ptr<Stream> StreamAcceptor::accept(){
		std::unique_ptr<Stream> stream;

		struct ::sockaddr_storage addr;
		socklen_t addr_len = sizeof(addr);

		int accepted_fd = ::accept4(file_descriptor, reinterpret_cast<struct ::sockaddr*>(&addr), &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
		if(accepted_fd >= 0){
			stream = std::make_unique<Stream>(accepted_fd);
		}else {
			//TODO Error checking
			//int error = errno;
		}

		return stream;
	}

	UnixSocketAddress::UnixSocketAddress(EventPoll& p, const std::string& unix_addr):
		poll{p},
		bind_address{unix_addr}
	{}

	std::unique_ptr<StreamAcceptor> UnixSocketAddress::listen(){
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

		status = ::bind(file_descriptor, (struct ::sockaddr*)&local, sizeof(local));
		if( status != 0){
			std::cerr<<"Couldn't bind socket: "<<local.sun_path<<std::endl;
			return nullptr;
		}

		::listen(file_descriptor, SOMAXCONN);

		return std::make_unique<StreamAcceptor>(bind_address, file_descriptor);
	}
	
	std::unique_ptr<Stream> UnixSocketAddress::connect(){
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

		return std::make_unique<Stream>(file_descriptor);
	}


	int StreamAcceptor::fd(){
		return file_descriptor;
	}
	
	size_t deserialize(uint8_t* buffer, uint16_t& value){
		uint16_t& buffer_value = *reinterpret_cast<uint16_t*>(buffer);
		value = le16toh(buffer_value);

		return 2;
	}

	Connection::Connection(EventPoll& p, std::unique_ptr<Stream>&& str, IConnectionStateObserver& obsrv):
		IFdObserver(p, str->fd(), EPOLLIN | EPOLLOUT),
		poll{p},
		stream{std::move(str)},
		observer{obsrv},
		write_ready{true},
		read_ready{true},
		read_offset{0},
		next_message_size{0}
	{
		read_buffer.resize(read_buffer_size);
	}

	Connection::~Connection(){
	}

	/*
	 * Based on EPoll notification accept, read or write in a non blocking way
	 * and queue reads and/or handle writes
	 */
	void Connection::notify(uint32_t mask){
		if( mask & EPOLLOUT ){
			onReadyWrite();
		}
		if( mask & EPOLLIN ){
			onReadyRead();
		}
	}

	void Connection::write(std::vector<uint8_t>& buffer){
		if(!write_ready || (write_ready && !write_buffer.empty())){
			write_buffer.insert(std::end(write_buffer), std::begin(buffer),std::end(buffer));
		}
		if(write_buffer.empty()){
			write_buffer = std::move(buffer);
		}
		if(write_ready){
			onReadyWrite();
		}
	}

	bool Connection::hasWriteQueued() const {
		return !write_buffer.empty();
	}

	void Connection::onReadyWrite(){
		size_t n = stream->write(write_buffer.data(), write_buffer.size());
		if(n==0){
			if( stream->broken() ){
				observer.notify(*this, ConnectionState::Broken);
			}
			return;
		}
		assert(write_buffer.size() > n);
		size_t remaining = write_buffer.size() - n;
		for(size_t i = 0; i < remaining; ++i){
			write_buffer[i] = write_buffer[i+n];
		}
		write_buffer.resize(remaining);
		write_ready = false;
	}

	void Connection::onReadyRead(){
		size_t remaining = read_buffer.size() - read_offset;
		size_t n = stream->read(&read_buffer[read_offset], remaining);
		read_ready = false;
		read_offset += n;
		if( read_offset >= message_length_size ){
			if( next_message_size < message_length_size ){
				uint16_t tmp_msg_size;
				deserialize(&read_buffer[read_offset], tmp_msg_size);
				next_message_size = tmp_msg_size;
			}
			if(read_offset > next_message_size){
				std::vector<uint8_t> next_message;
				next_message.assign(read_buffer.begin(), read_buffer.begin()+next_message_size);
				size_t remaining = read_offset - next_message_size;
				for(size_t i = 0; i < remaining; ++i){
					read_buffer[i] = read_buffer[i+next_message_size];
				}
				read_offset -= next_message_size;
				next_message_size = 0;
				ready_reads.push(std::move(next_message));
			}
		}
	}

	std::optional<std::vector<uint8_t>> Connection::read(){
		if(ready_reads.empty()){
			if( read_ready ){
				onReadyRead();
				return read();
			}
			return std::nullopt;
		}
		auto front = std::move(ready_reads.front());
		ready_reads.pop();
		return front;
	}

	bool Connection::hasReadQueued() const{
		return !ready_reads.empty();
	}

	bool Connection::broken() const {
		return stream->broken();
	}

	Server::Server(EventPoll& p, std::unique_ptr<StreamAcceptor>&& acc):
		IFdObserver(p, acc->fd(), EPOLLIN),
		acceptor{std::move(acc)}
	{}

	void Server::notify(uint32_t mask){
		if(mask & EPOLLIN){
			
		}
	}

	const std::string& UnixSocketAddress::getPath()const{
		return bind_address;
	}

	Network::Network(){
	}

	void Network::poll(){
		ev_poll.poll();
	}

	std::unique_ptr<Server> Network::listen(const std::string& address){
		auto unix_addr = parseUnixAddress(address);
		if(!unix_addr){
			return nullptr;
		}
		auto acceptor = unix_addr->listen();
		if(!acceptor){
			return nullptr;
		}
		return std::make_unique<Server>(ev_poll, std::move(acceptor));
	}

	std::unique_ptr<Connection> Network::connect(const std::string& address, IConnectionStateObserver& obsrv){
		auto unix_addr = parseUnixAddress(address);
		if(!unix_addr){
			return nullptr;
		}
		auto stream = unix_addr->connect();
		if(!stream){
			return nullptr;
		}
		return std::make_unique<Connection>(ev_poll, std::move(stream), obsrv);
	}

	std::unique_ptr<UnixSocketAddress> Network::parseUnixAddress(const std::string& unix_path){
		return std::make_unique<UnixSocketAddress>(ev_poll, unix_path);
	}


	size_t deserialize(uint8_t* buffer, std::string& value){
		uint16_t val_size = 0;
		size_t shift = deserialize(buffer, val_size);
		value.resize(val_size);
		for(size_t i = 0; i < val_size; ++i){
			value[i] = buffer[shift+i];
		}
		return shift + val_size;
	}

	std::optional<MessageRequest> asyncReadRequest(Connection& connection){
		if(connection.hasReadQueued()){
			MessageRequest msg;
			auto opt_buffer = connection.read();
			auto& buffer = *opt_buffer;
			if( buffer.size() >= 2 ){
				uint16_t msg_size;
				size_t shift = deserialize(&buffer[0], msg_size);
				if( msg_size < max_message_size ){
					shift += deserialize(&buffer[shift], msg.request_id);
					msg.type = buffer[shift++];
					shift += deserialize(&buffer[shift], msg.target);
					if( msg.target.size() < max_target_size ){
						shift += deserialize(&buffer[shift], msg.content);
						if( msg.content.size() < max_request_content_size ){
							return msg;
						}
					}
				}
			}
		}
		return std::nullopt;
	}

	// endian.h conversion
	size_t serialize(uint8_t* buffer, uint16_t value){
		uint16_t& val_buffer = *reinterpret_cast<uint16_t*>(buffer);
		val_buffer = htole16(value);
		return 2;
	}

	size_t serialize(uint8_t* buffer, const std::string& value){
		size_t shift = serialize(buffer, static_cast<uint16_t>(value.size()));
		for(size_t i = 0; i < value.size(); ++i){
			buffer[shift + i] = static_cast<uint8_t>(value[i]);
		}

		return shift + value.size();
	}

	bool asyncWriteRequest(Connection& connection, const MessageRequest& request){
		const size_t ct_size = request.content.size();
		const size_t tg_size = request.target.size();
		const size_t msg_size = ct_size + tg_size + request_static_size;

		if( msg_size < max_message_size && ct_size < max_request_content_size && tg_size < max_target_size ){
			std::vector<uint8_t> buffer;
			buffer.resize(msg_size);

			size_t shift = serialize(&buffer[0], request.request_id);
			buffer[shift++] = request.type;
			shift += serialize(&buffer[shift], request.target);
			shift += serialize(&buffer[shift], request.content);

			connection.write(buffer);
			return true;
		}else{
			return false;
		}
	}

	std::optional<MessageResponse> asyncReadResponse(Connection& connection){
		if(connection.hasReadQueued()){
			MessageResponse msg;
			auto opt_buffer = connection.read();
			auto& buffer = *opt_buffer;
			if( buffer.size() >= 2 ){
				uint16_t msg_size;
				size_t shift = deserialize(&buffer[0], msg_size);
				if( msg_size < max_message_size ){
					shift += deserialize(&buffer[shift], msg.request_id);
					msg.return_code = buffer[shift++];
					shift += deserialize(&buffer[shift], msg.target);
					if( msg.target.size() < max_target_size ){
						shift += deserialize(&buffer[shift], msg.content);
						if( msg.content.size() < max_request_content_size ){
							return msg;
						}
					}
				}
			}
		}
		return std::nullopt;
	}
	
	bool asyncWriteResponse(Connection& connection, const MessageResponse& request){
		const size_t ct_size = request.content.size();
		const size_t tg_size = request.target.size();
		const size_t msg_size = ct_size + tg_size + request_static_size;

		if( msg_size < max_message_size && ct_size < max_request_content_size && tg_size < max_target_size ){
			std::vector<uint8_t> buffer;
			buffer.resize(msg_size);

			size_t shift = serialize(&buffer[0], request.request_id);
			buffer[shift++] = request.return_code;
			shift += serialize(&buffer[shift], request.target);
			shift += serialize(&buffer[shift], request.content);

			connection.write(buffer);
			return true;
		}else{
			return false;
		}
	}
}
