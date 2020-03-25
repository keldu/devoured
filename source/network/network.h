#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <optional>
#include <functional>
#include <chrono>

#include "error_callback.h"

namespace dvr {
	class UnixSocketAddress;
	class IFdOwner;

	class EventPoll {
	private:
		// PImpl Pattern, because of platform specific implementations
		class Impl;
		std::unique_ptr<Impl> impl;
	public:
		EventPoll();
		~EventPoll();

		void enterScope();
		void leaveScope();

		bool poll();
		bool wait(std::chrono::steady_clock::duration duration);

		void subscribe(IFdOwner* obsv, int fd, uint32_t mask);
		void unsubscribe(int fd);
	};

	class IFdOwner {
	private:
		EventPoll& poll;
		const int file_desc;
		uint32_t event_mask;

		friend class EventPoll;
	public:
		IFdOwner(EventPoll& poll, int fd, int flags, uint32_t mask);
		virtual ~IFdOwner();

		virtual void notify(uint32_t mask) = 0;

		int fd() const;
		uint32_t mask() const;
	};

	class IoStream;
	enum class IoStreamState {
		Broken,
		ReadReady,
		WriteReady
	};
	class IStreamStateObserver {
	public:

		virtual ~IStreamStateObserver() = default;

		virtual void notify(IoStream& c, IoStreamState mask) = 0;
	};

	class OutputStream {
	public:
		virtual ~OutputStream() = default;

		/*
		 * 
		 */
		virtual void write(std::vector<uint8_t>&& buffer) = 0;
		virtual bool hasWriteQueued() const = 0;
	};
	
	class InputStream {
	public:
		virtual ~InputStream() = default;

		/*
		 * Is technically a peek
		 */
		virtual std::optional<uint8_t*> read(size_t n) = 0;
		virtual void consumeRead(size_t n) = 0;
		virtual bool hasReadQueued() const = 0;
	};

	typedef uint64_t IoStreamId;
	/*
	 * This should be divided into IoStream and FdStream, where
	 * IoStream is an interface
	 * FdStream is the child class hidden in network.cpp
	 * Exposing is ok here, because it's not a library, but I still would prefer
	 * to move it.
	 * Meant for the future when everything works
	 */
	class IoStream : public InputStream, public OutputStream {
	public:
		virtual ~IoStream() = default;

		virtual int id() const = 0;

		virtual void write(std::vector<uint8_t>&& buffer) = 0;
		virtual bool hasWriteQueued() const = 0;

		virtual std::optional<uint8_t*> read(size_t n) = 0;
		virtual void consumeRead(size_t n) = 0;
		virtual bool hasReadQueued() const = 0;
	};
	
	class Server : public IFdOwner {
	private:
		EventPoll& event_poll;
		const std::string address;
		StreamErrorOrValueCallback<Server, Void> observer;
	public:
		Server(EventPoll& p, int fd, const std::string& addr, StreamErrorOrValueCallback<Server, Void>&& obsrv);
		~Server();

		void notify(uint32_t mask) override;

		std::unique_ptr<IoStream> accept(IStreamStateObserver& obsrv);
	};

	class UnixSocketAddress {
	private:
		EventPoll& poll;
		std::string bind_address;
	public:
		UnixSocketAddress(EventPoll& p, const std::string& unix_address);

		std::unique_ptr<Server> listen(StreamErrorOrValueCallback<Server, Void>&& obsrv);
		std::unique_ptr<IoStream> connect(IStreamStateObserver& obsrv);

		const std::string& getPath() const;
	};
	
	class WaitScope;
	class AsyncIoProvider {
	public:
		virtual ~AsyncIoProvider() = default;

		virtual std::unique_ptr<UnixSocketAddress> parseUnixAddress(const std::string& path) = 0;

		virtual std::unique_ptr<InputStream> wrapInputFd(int fd, IStreamStateObserver& obsrv, uint32_t flags = 0) = 0;
		virtual std::unique_ptr<OutputStream> wrapOutputFd(int fd, IStreamStateObserver& obsrv, uint32_t flags = 0) = 0;
	};

	class WaitScope {
	private:
		EventPoll& e_poll;
	public:
		WaitScope(EventPoll& poll);
		~WaitScope();

		void poll();
		void wait(std::chrono::steady_clock::duration duration);
	};

	struct AsyncIoContext {
		std::unique_ptr<AsyncIoProvider> provider;
		EventPoll& event_poll;
		WaitScope& wait_scope;
	};

	AsyncIoContext setupAsyncIo();
}
