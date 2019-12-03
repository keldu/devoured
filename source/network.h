#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <optional>
#include <functional>

namespace dvr {
	class UnixSocketAddress;
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

	class Connection;
	enum class ConnectionState {
		Broken,
		ReadReady,
		WriteReady
	};
	class IConnectionStateObserver {
	public:

		virtual ~IConnectionStateObserver() = default;

		virtual void notify(Connection& c, ConnectionState mask) = 0;
	};

	typedef uint64_t ConnectionId;
	class Connection : public IFdObserver {
	private:
		EventPoll& poll;
		const ConnectionId connection_id;
		IConnectionStateObserver& observer;
		
		int file_desc;
		bool is_broken;

		// non blocking helpers
		// write buffering
		bool write_ready;
		std::vector<uint8_t> write_buffer;

		// read buffering 
		bool read_ready;

		std::queue<std::vector<uint8_t>> ready_reads;
		size_t read_offset;
		size_t next_message_size;
		std::vector<uint8_t> read_buffer;

		void onReadyWrite();
		void onReadyRead();
	public:
		Connection(EventPoll& p, int fd, IConnectionStateObserver& obsrv);
		~Connection();

		void notify(uint32_t mask) override;

		void write(std::vector<uint8_t>& buffer);
		bool hasWriteQueued() const;
		
		std::optional<std::vector<uint8_t>> read();
		bool hasReadQueued() const;

		const int& fd() const;
		bool broken() const;
		const ConnectionId& id() const;
	};

	enum class ServerState {
		Accept
	};
	class Server;
	class IServerStateObserver {
	public:
		virtual ~IServerStateObserver() = default;
		virtual void notify(Server& server, ServerState state) = 0;
	};
	class Server : public IFdObserver {
	private:
		int file_desc;
		EventPoll& event_poll;
		IServerStateObserver& observer;
	public:
		Server(EventPoll& p, int fd, IServerStateObserver& srv);

		void notify(uint32_t mask) override;

		std::unique_ptr<Connection> accept(IConnectionStateObserver& obsrv);
	};

	class UnixSocketAddress {
	private:
		EventPoll& poll;
		std::string bind_address;

		int socketBind();
	public:
		UnixSocketAddress(EventPoll& p, const std::string& unix_address);

		std::unique_ptr<Server> listen();
		std::unique_ptr<Connection> connect();

		int getFD() const;
		const std::string& getPath() const;
	};
	
	class Network {
	private:
		EventPoll ev_poll;
	public:
		Network();

		void poll();

		std::unique_ptr<Server> listen(const std::string& address, IServerStateObserver& obsrv);
		std::unique_ptr<Connection> connect(const std::string& address, IConnectionStateObserver& obsrv);

		std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& unix_path);
	};
	
	std::optional<MessageRequest> asyncReadRequest(Connection& connection);
	bool asyncWriteRequest(Connection& connection, const MessageRequest& request);
	
	std::optional<MessageResponse> asyncReadResponse(Connection& connection);
	bool asyncWriteResponse(Connection& connection, const MessageResponse& request);
}
