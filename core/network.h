#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <optional>
#include <functional>

namespace dvr {
	/* 
	 * Don't change the Message types with out changing the corresponding max size in network.cpp
	 * I'm wondering if there exists some kind of template magic to autogenerate the sizes
	 * with some helper classes
	 */
	class MessageRequest {
	public:
		MessageRequest() = default;
		MessageRequest(uint16_t, uint8_t, const std::string&, const std::string&);
		uint16_t request_id;
		uint8_t type;
		std::string target;
		std::string content;
	};
	/*
	 * Same here as above
	 */
	class MessageResponse {
	public:
		MessageResponse() = default;
		MessageResponse(uint16_t, uint8_t, const std::string&, const std::string&);
		uint16_t request_id;
		uint8_t return_code;
		std::string target;
		std::string content;
	};

	bool serializeMessageRequest(std::vector<uint8_t>& buffer, MessageRequest& request);
	bool deserializeMessageRequest(std::vector<uint8_t>& buffer, MessageRequest& request);

	bool serializeMessageResponse(std::vector<uint8_t>& buffer, MessageResponse& response);
	bool deserializeMessageResponse(std::vector<uint8_t>& buffer, MessageResponse& response);

	class UnixSocketAddress;
	class Stream;
	class IFdObserver;

	class EventPoll {
	private:
		// Impl Pattern
		class Impl;
		std::unique_ptr<Impl> impl;
	public:
		EventPoll();
		~EventPoll();

		bool poll();

		void subscribe(IFdObserver& obsv);
		void unsubscribe(IFdObserver& obsv);
	};

	class IFdObserver {
	private:
		EventPoll& poll;
		int file_desc;
		uint32_t event_mask;

		friend class EventPoll;
	public:
		IFdObserver(EventPoll& poll, int fd, uint32_t mask);
		virtual ~IFdObserver();

		virtual void notify(uint32_t mask) = 0;

		int fd() const;
		uint32_t mask() const;
	};

	class Stream{
	private:
		int file_descriptor;
		
		friend struct StreamCmp;
	public:
		Stream(int fd);

		size_t write(uint8_t* buffer, size_t n);
		size_t read(uint8_t* buffer, size_t n);

		int fd();
	};
	
	struct StreamCmp {
		bool operator()(Stream* a, Stream* b)const{
			return a->file_descriptor < b->file_descriptor;
		}
	};
	
	class StreamAcceptor {
	private:
		std::string socket_path;
		int file_descriptor;
	public:
		StreamAcceptor(const std::string& sp, int fd);
		~StreamAcceptor();

		std::unique_ptr<Stream> accept();

		int fd();
	};

	class Connection;
	class IConnectionObserver {
	public:
		virtual ~IConnectionObserver() = default;

		virtual void notifyCreate(Connection& c) = 0;
		virtual void notifyDestroy(Connection& c) = 0;
	};

	class Connection : public IFdObserver {
	private:
		EventPoll& poll;
		std::unique_ptr<Stream> stream;

		// non blocking helpers
		// write buffering
		std::queue<std::vector<uint8_t>> write_buffer_queue;

		// read buffering 
		std::queue<std::vector<uint8_t>> read_buffer_queue;

		uint16_t next_message_size;
		std::vector<uint8_t> incomplete_message;
	public:
		Connection(EventPoll& p, std::unique_ptr<Stream>&& str/*, IConnectionObserver& obsrv*/);

		void notify(uint32_t mask) override;

		void write(std::vector<uint8_t>& buffer);
		bool hasWriteQueued() const;
		
		std::vector<uint8_t> grabRead();
		bool hasReadQueued() const;
	};

	class Server : public IFdObserver {
	private:
		std::unique_ptr<StreamAcceptor> acceptor;
	public:
		Server(EventPoll& p, std::unique_ptr<StreamAcceptor>&& acc);

		void notify(uint32_t mask) override;
	};

	class UnixSocketAddress {
	private:
		EventPoll& poll;
		std::string bind_address;

		int socketBind();
	public:
		UnixSocketAddress(EventPoll& p, const std::string& unix_address);

		std::unique_ptr<StreamAcceptor> listen();
		std::unique_ptr<Stream> connect();

		int getFD() const;
		const std::string& getPath() const;
	};
	
	class Network {
	private:
		EventPoll ev_poll;
	public:
		Network();

		void poll();

		std::unique_ptr<Server> listen(const std::string& address);
		std::unique_ptr<Connection> connect(const std::string& address);

		std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& unix_path);
	};
	
	std::optional<MessageRequest> asyncReadRequest(Connection& connection);
	bool asyncWriteRequest(Connection& connection, const MessageRequest& request);
	
	std::optional<MessageResponse> asyncReadResponse(Connection& connection);
	bool asyncWriteResponse(Connection& connection, const MessageResponse& request);
}
