#pragma once

#include <memory>
#include <optional>
#include <cstdint>
#include <variant>
#include <functional>

using std::size_t;

#include "async-magic.h"

namespace dvr {
	/*
	 *
	 */
	template<typename Func, typename T>
	using PromiseForResult = ReducePromises<ReturnType<Func, T>>;

	template<typename T>
	class Promise : PromiseBase {
	public:
		/*
		 * Construct a fulfilled promise
		 */
		Promise(FixVoid<T> value);
		/*
		 * Construct a broken promise
		 */
		Promise(const Error& error);

		Promise(decltype(nullptr)){}

		template<typename Func, typename ErrorFunc = PropagateError>
		PromiseForResult<Func, T> then(std::function<ReturnType<Func,T>>&& func, std::function<void(Error&&)>&& error_func = PropagateError());

		Promise<void> ignoreResult() { return then([](T&&){});}

		template<typename... Attachments>
		void attach(Attachments&&... attachments);
	};

	class AsyncInputStream {
	public:
		virtual ~AsyncInputStream() = default;

		virtual void read(uint8_t* buffer, size_t length) = 0;
		virtual void closeRead() = 0;
	};

	class AsyncOutputStream {
	public:
		virtual ~AsyncOutputStream() = default;

		virtual void write(uint8_t* buffer, size_t length ) = 0;
		virtual void closeWrite() = 0;
	};

	class AsyncIoStream : public AsyncInputStream, public AsyncOutputStream {
	public:
	};

	class StreamListener {
	public:
		virtual void accept() = 0;
	};

	class NetworkAddress {
	public:
		virtual ~NetworkAddress() = default;

		virtual std::unique_ptr<StreamListener> listen() = 0;
		virtual std::unique_ptr<AsyncIoStream> connect() = 0;
	};

	class EventPoll {
	public:
		virtual ~EventPoll() = default;

		virtual void wait() = 0;
		virtual void poll() = 0;
	};

	class AsyncIoProvider {
	public:
		virtual ~AsyncIoProvider() = default;

		virtual EventPoll& eventPoll() = 0;
		virtual std::unique_ptr<NetworkAddress> parseAddress(const std::string& addr, int32_t port_hint = 0) = 0;
	};

	struct AsyncIoContext {
		std::unique_ptr<AsyncIoProvider> provider;
		EventPoll& event_poll;
	};

	AsyncIoContext setupAsyncIo();
}

#include "async-inl.h"
