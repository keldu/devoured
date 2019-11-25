#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <functional>

namespace dvr {
	class UnixSocketAddress;
	class EventPoll;
	class Stream;

	class StreamReadableSignal {
	public:
		virtual ~StreamReadableSignal() = default;

		virtual void notify(StreamCallback& callback) = 0;
	};

	class FdObserver {
	private:
		int file_descriptor;
		EventPoll& poll;
		Stream& stream;

		friend struct FdObserverCmp;
		friend class Stream;
	public:
		FdObserver(EventPoll& p, int fd);
		~FdObserver();

		void becomesWritable();
		void becomesReadable();
	};

	class EventPoll {
	private:
		std::map<int, FdObserver*> observers;

		friend class FdObserver;
	public:
		void poll();

		void subscribe(FdObserver& obv);
		void unsubscribe(FdObserver& obv);
	};
	
	class Stream{
	private:
		std::queue<std::unique_ptr<std::vector<uint8_t>>> write_buffer_queue;
		std::vector<uint8_t> read_buffer;
		StreamReadableSignal* signalee;

		int file_descriptor;
		FdObserver observer;
		EventPoll& poll;

		friend struct StreamCmp;
		friend class UnixSocketAddress;
		friend class StreamAcceptor;
	public:
		Stream(int fd, EventPoll& p, StreamCallback& call);

		bool operator<(const Stream& b) const {
			return file_descriptor < b.file_descriptor;
		}

		void write(std::unique_ptr<std::vector<uint8_t>>&& buffer);
		size_t read(StreamReadableSignal& readable);
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
		EventPoll& poll;
	public:
		StreamAcceptor(const std::string& sp, int fd, EventPoll& p);
		~StreamAcceptor();

		std::unique_ptr<Stream> accept();
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
	
	class NetworkEvent;
	class NetworkTrigger {
	private:
		NetworkEvent& event;
	public:
		NetworkTrigger(NetworkEvent& ev);

		void trigger();
	};

	class NetworkEvent {
	private:
		std::function function;
	public:
		void execute();
		template<typename Function>
		void then(std::function func){
			function = func;
		}
	};

	class Network {
	private:
		EventPoll poll;
	public:
		Network();

		void poll();

		std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& unix_path);
	};

}
