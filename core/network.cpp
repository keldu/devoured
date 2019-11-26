#include "network.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

#include <iostream>

const size_t read_buffer_size = 4096;

namespace dvr {
	IFdObserver::IFdObserver(EventPoll& p, int file_d, uint8_t msk):
		poll{p},
		fd{file_d},
		mask{msk}
	{
		poll.subscribe(*this);
	}

	IFdObserver::~IFdObserver(){
		poll.unsubscribe(*this);
	}

	void EventPoll::poll(){
		// Epoll
	}

	void EventPoll::subscribe(IFdObserver& obv){
		observers.insert(std::make_pair(obv.fd, &obv));
	}

	void EventPoll::unsubscribe(IFdObserver& obv){
		observers.erase(obv.fd);
	}

	Stream::Stream(int fd):
		file_descriptor{fd}
	{}

	int Stream::fd(){
		return file_descriptor;
	}

	StreamAcceptor::StreamAcceptor(const std::string& sp, int fd):
		socket_path{sp},
		file_descriptor{fd}
	{
	}

	StreamAcceptor::~StreamAcceptor()
	{
		if(file_descriptor != -1){
			unlink(socket_path.c_str());
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
		int file_descriptor = socket( AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0 );

		if(file_descriptor == -1){
			std::cerr<<"Couldn't create socket at "<<bind_address<<std::endl;
			return nullptr;
		}

		int status;

		struct ::sockaddr_un local;
		int len;
		strncpy(local.sun_path, bind_address.c_str(), 108);
		local.sun_family = AF_UNIX;

		struct ::stat stats;
		status = ::stat(local.sun_path,&stats);

		if(status >= 0){
			std::cerr<<"Socket path exists already: "<<local.sun_path<<std::endl;
			return nullptr;
		}
		
		len = strlen(local.sun_path) + sizeof(local.sun_family);

		status = ::bind(file_descriptor, (struct ::sockaddr*)&local, len);

		if( status == -1){
			std::cerr<<"Socket path is malformed: "<<local.sun_path<<std::endl;
			return nullptr;
		}

		::listen(file_descriptor, SOMAXCONN);

		return std::make_unique<StreamAcceptor>(bind_address, file_descriptor);
	}

	int StreamAcceptor::fd(){
		return file_descriptor;
	}

	Connection::Connection(EventPoll& p, std::unique_ptr<Stream>&& str):
		IFdObserver(p, str->fd(), 0),
		poll{p},
		stream{std::move(str)}
	{}

	void Connection::notify(uint8_t mask){
		(void)mask;
	}

	Server::Server(EventPoll& p, std::unique_ptr<StreamAcceptor>&& acc):
		IFdObserver(p, acc->fd(), 0),
		acceptor{std::move(acc)}
	{}

	void Server::notify(uint8_t mask){
		(void) mask;
	}

	std::unique_ptr<Stream> UnixSocketAddress::connect(){
		return 0L;
	}

	const std::string& UnixSocketAddress::getPath()const{
		return bind_address;
	}
}
