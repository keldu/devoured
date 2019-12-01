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

#include <iostream>

// Don't remember the value of 2^16 - 1 so 1024 * 64 - 1 will have to do
const size_t read_buffer_size = 1024 * 64 - 1;

const uint16_t max_message_size = 4095 - 2;
const uint16_t max_target_size = 255;
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

	IFdObserver::IFdObserver(EventPoll& p, int file_d, uint8_t msk):
		poll{p},
		file_desc{file_d},
		mask{msk}
	{
		poll.subscribe(*this);
	}

	IFdObserver::~IFdObserver(){
		poll.unsubscribe(*this);
	}

	int IFdObserver::fd()const{
		return file_desc;
	}

	EventPoll::EventPoll()
	{
		epoll_fd = epoll_create1(0);
		if( epoll_fd < 0){
			//TODO error handling
		}
	}

	void EventPoll::poll(){
		// Epoll
	}

	void EventPoll::subscribe(IFdObserver& obv){
		observers.insert(std::make_pair(obv.file_desc, &obv));
	}

	void EventPoll::unsubscribe(IFdObserver& obv){
		observers.erase(obv.file_desc);
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
		//TODO Missing errno check in every error state

		if(file_descriptor < 0){
			std::cerr<<"Couldn't create socket: "<<(::strerror(errno))<<std::endl;
			return nullptr;
		}

		int status;

		struct ::sockaddr_un local;
		local.sun_family = AF_UNIX;
		size_t len = (sizeof(local.sun_path)-1) < bind_address.size() ? (sizeof(local.sun_path)-1): bind_address.size();
		::strncpy(local.sun_path, bind_address.c_str(), len);

		status = ::connect(file_descriptor, (struct ::sockaddr*)&local, sizeof(local));
		if( status != 0){
			std::cerr<<"Couldn't connect to socket: "<<local.sun_path<<std::endl;
			return nullptr;
		}

		return std::make_unique<Stream>(file_descriptor);
	}


	int StreamAcceptor::fd(){
		return file_descriptor;
	}

	Connection::Connection(EventPoll& p, std::unique_ptr<Stream>&& str):
		IFdObserver(p, str->fd(), 0),
		poll{p},
		stream{std::move(str)}
	{}

	/*
	 * Based on EPoll notification accept, read or write in a non blocking way
	 * and queue reads and/or handle writes
	 */
	void Connection::notify(uint8_t mask){
		(void) mask;
	}

	void Connection::write(std::vector<uint8_t>& buffer){
		(void) buffer;
	}

	bool Connection::hasWriteQueued() const {
		return !write_buffer_queue.empty();
	}

	std::vector<uint8_t> Connection::grabRead(){
		std::vector<uint8_t> front;
		if(read_buffer_queue.empty()){
			return front;
		}
		front = std::move(read_buffer_queue.front());
		read_buffer_queue.pop();
		return front;
	}

	bool Connection::hasReadQueued() const {
		return !read_buffer_queue.empty();
	}

	Server::Server(EventPoll& p, std::unique_ptr<StreamAcceptor>&& acc):
		IFdObserver(p, acc->fd(), 0),
		acceptor{std::move(acc)}
	{}

	void Server::notify(uint8_t mask){
		(void) mask;
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

	std::unique_ptr<Connection> Network::connect(const std::string& address){
		auto unix_addr = parseUnixAddress(address);
		if(!unix_addr){
			return nullptr;
		}
		auto stream = unix_addr->connect();
		if(!stream){
			return nullptr;
		}
		return std::make_unique<Connection>(ev_poll, std::move(stream));
	}

	std::unique_ptr<UnixSocketAddress> Network::parseUnixAddress(const std::string& unix_path){
		return std::make_unique<UnixSocketAddress>(ev_poll, unix_path);
	}

	size_t deserialize(uint8_t* buffer, uint16_t& value){
		uint16_t& buffer_value = *reinterpret_cast<uint16_t*>(buffer);
		value = le16toh(buffer_value);

		return 2;
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
			auto buffer = connection.grabRead();
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
			auto buffer = connection.grabRead();
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
