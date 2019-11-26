#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <functional>

namespace dvr {
	class UnixSocketAddress;
	class Stream;
	class IFdObserver;

	class EventPoll {
	private:
		std::map<int, IFdObserver*> observers;
	public:
		void poll();

		void subscribe(IFdObserver& obsv);
		void unsubscribe(IFdObserver& obsv);
	};

	class IFdObserver {
	private:
		EventPoll& poll;
		int fd;
		uint8_t mask;

		friend class EventPoll;
	public:
		IFdObserver(EventPoll& poll, int fd, uint8_t mask);
		virtual ~IFdObserver();

		virtual void notify(uint8_t mask) = 0;
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

	class Connection : public IFdObserver {
	private:
		EventPoll& poll;
		std::unique_ptr<Stream> stream;
	public:
		Connection(EventPoll& p, std::unique_ptr<Stream>&& str);

		void notify(uint8_t mask) override;
	};

	class Server : public IFdObserver {
	private:
		std::unique_ptr<StreamAcceptor> acceptor;
	public:
		Server(EventPoll& p, std::unique_ptr<StreamAcceptor>&& acc);

		void notify(uint8_t mask) override;
	};

	class UnixSocketAddress {
	private:
		EventPoll& poll;
		std::string bind_address;
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

		void listen(const std::string& address);
		std::unique_ptr<Connection> connect(const std::string& address);

		std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& unix_path);
	};

}
