#include "network.h"

namespace dvr {
}

/*
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
	UnixSocketAddress::UnixSocketAddress(EventPoll& p, const std::string& unix_addr):
		poll{p},
		bind_address{unix_addr}
	{}

	std::unique_ptr<Server> UnixSocketAddress::listen(IStreamStateObserver& obsrv){
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

#ifndef NDEBUG
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
	
	std::unique_ptr<Connection> UnixSocketAddress::connect(IoStateObserver<Connection>& obsrv){
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

		return std::make_unique<Connection>(poll, file_descriptor, obsrv);
	}

	static ConnectionId next_connection_id = 0;

	Connection::Connection(EventPoll& p, int fd, IoStateObserver<Connection>& obsrv):
		IFdObserver(p, fd, EPOLLIN | EPOLLOUT),
		poll{p},
		connection_id{++next_connection_id},
		observer{obsrv},
		is_broken{false},
		write_ready{true},
		already_written{0},
		read_ready{true},
		already_read{0}
	{
		read_buffer.resize(read_buffer_size);
	}

	Connection::~Connection(){
		close();
	}

	void Connection::close(){
		if(broken()){
			return;
		}
		is_broken = true;
		read_ready = false;
		write_ready = false;
		observer.notify(*this, IoState::Broken);
	}

	 * Based on EPoll notification accept, read or write in a non blocking way
	 * and queue reads and/or handle writes
	void Connection::notify(uint32_t mask){
		if(broken()){
			return;
		}
		if( mask & EPOLLOUT ){
			write_ready = true;
			onReadyWrite();
			observer.notify(*this, IoState::WriteReady);
		}
		if( mask & EPOLLIN ){
			read_ready = true;
			onReadyRead();
			observer.notify(*this, IoState::ReadReady);
		}
	}

	void Connection::write(std::vector<uint8_t>&& buffer){
		if(write_buffer.empty()){
			write_buffer = std::move(buffer);
		}else if (write_buffer.size() + buffer.size() <= write_buffer_size){
			write_buffer.insert(std::end(write_buffer), std::begin(buffer),std::end(buffer));
		}else{

		}
		if(write_ready){
			onReadyWrite();
		}
	}

	bool Connection::hasWriteQueued() const {
		return !write_buffer.empty();
	}

	void Connection::onReadyWrite(){
		while(write_ready && write_buffer.size() > 0){
			ssize_t n = ::send(file_desc, write_buffer.data(), write_buffer.size(), MSG_NOSIGNAL);
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

	void Connection::onReadyRead(){
		if(broken()){
			return;
		}
		do {
			// Not yet read into the buffer
			size_t remaining = read_buffer.size() - already_read;
			ssize_t n = ::recv(file_desc, &read_buffer[already_read], remaining, 0);
			
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

	std::optional<uint8_t*> Connection::read(size_t n){
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

	void Connection::consumeRead(size_t n){
		size_t remaining = n < read_buffer.size()?(read_buffer.size()-n):(0);
		for(size_t i = 0; i < remaining; ++i){
			read_buffer[i] = read_buffer[i+n];
		}
		assert(already_read>=n);
		already_read -= n;
	}

	bool Connection::hasReadQueued() const{
		return !read_buffer.empty();
	}

	bool Connection::broken() const {
		return is_broken;
	}

	const ConnectionId& Connection::id()const{
		return connection_id;
	}

	Server::Server(EventPoll& p, int fd, const std::string& addr, IServerStateObserver& obs):
		IFdObserver(p, fd, EPOLLIN),
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

	std::unique_ptr<Connection> Server::accept(IoStateObserver<Connection>& obsrv){
		struct ::sockaddr_storage addr;
		socklen_t addr_len = sizeof(addr);

		int accepted_fd = ::accept4(fd(), reinterpret_cast<struct ::sockaddr*>(&addr), &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

		if(accepted_fd<0) {
			//TODO Error checking
			//int error = errno;
			return nullptr;
		}

		return std::make_unique<Connection>(event_poll, accepted_fd, obsrv);
	}

	const std::string& UnixSocketAddress::getPath()const{
		return bind_address;
	}

	void Pipe::Input::notify(uint32_t mask){
	}
	
	void Pipe::Output::notify(uint32_t mask){
	}

	Network::Network(EventPoll& event_poll_p):
		event_poll{event_poll_p}
	{
	}

	std::unique_ptr<Server> Network::listen(const std::string& address, IServerStateObserver& obsrv){
		auto unix_addr = parseUnixAddress(address);
		if(!unix_addr){
			return nullptr;
		}
		auto acceptor = unix_addr->listen(obsrv);
		if(!acceptor){
			return nullptr;
		}
		return acceptor;
	}

	std::unique_ptr<Connection> Network::connect(const std::string& address, IoStateObserver<Connection>& obsrv){
		auto unix_addr = parseUnixAddress(address);
		if(!unix_addr){
			return nullptr;
		}
		auto stream = unix_addr->connect(obsrv);
		if(!stream){
			return nullptr;
		}
		return stream;
	}

	std::unique_ptr<UnixSocketAddress> Network::parseUnixAddress(const std::string& unix_path){
		return std::make_unique<UnixSocketAddress>(event_poll, unix_path);
	}
}
*/
