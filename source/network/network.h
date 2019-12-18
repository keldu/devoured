#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <optional>
#include <functional>

#include "fd_observer.h"
#include "stream.h"

namespace dvr {
	class UnixSocketAddress;
	class IFdObserver;


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
		size_t already_written;
		std::vector<uint8_t> write_buffer;

		// read buffering 
		bool read_ready;
		size_t already_read;
		std::vector<uint8_t> read_buffer;
		//
		void onReadyWrite();
		void onReadyRead();
	public:
		Connection(EventPoll& p, int fd, IConnectionStateObserver& obsrv);
		~Connection();

		void notify(uint32_t mask) override;

		/*
		 * move buffer to the writeQueue
		 */
		void write(std::vector<uint8_t>&& buffer);
		bool hasWriteQueued() const;
		
		/* 
		 * get front buffer in read and the already read hint.
		 */
		std::optional<uint8_t*> read(size_t n);
		void consumeRead(size_t n);
		bool hasReadQueued() const;

		/*
		 * checks if stream is broken or not
		 */
		const int& fd() const;
		void close();
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
		const std::string address;
		IServerStateObserver& observer;
	public:
		Server(EventPoll& p, int fd, const std::string& addr, IServerStateObserver& srv);
		~Server();

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

		std::unique_ptr<Server> listen(IServerStateObserver& obsrv);
		std::unique_ptr<Connection> connect(IConnectionStateObserver& obsrv);

		int getFD() const;
		const std::string& getPath() const;
	};
	
	class Network {
	private:
		EventPoll& event_poll;
		
		std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& unix_path);
	public:
		Network(EventPoll& event_poll_p);

		std::unique_ptr<Server> listen(const std::string& address, IServerStateObserver& obsrv);
		std::unique_ptr<Connection> connect(const std::string& address, IConnectionStateObserver& obsrv);

		std::unique_ptr<IoStream> connect(const std::string& address, IStreamStateObserver& obsrv);
	};
}
