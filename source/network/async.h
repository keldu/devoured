#pragma once

#include <memory>
#include <optional>
#include <cstdint>
#include <streambuf>
#include <functional>

using std::size_t;

namespace dvr {
	class Error {
	private:
		const int64_t error_code;
		const std::string error_message;
	public:
		Error(int64_t ec, const std::string& em):
			error_code{ec},
			error_message{em}
		{}

		int64_t code() const {
			return error_code;
		}

		const std::string& message() const {
			return error_message;
		}
	};

	template<typename T>
	class ErrorOr {
	private:
		std::function<void(T)> call;
		std::function<void(const Error& error)> err_call;
	public:
		ErrorOr(std::function<void(T)> call, std::function<void(const Error&)> errors):
			call{std::move(call)},
			err_call{std::move(errors)}
		{}

		void set(T value){
			call(std::move(value));
		}

		void fail(const Error& error){
			err_call(error);
		}
	};

	class AsyncInputStream {
	public:
		virtual ~AsyncInputStream() = default;

		virtual void read(uint8_t* buffer, size_t length, ErrorOr<size_t> async ) = 0;
		virtual void closeRead() = 0;
	};

	class AsyncOutputStream {
	public:
		virtual ~AsyncOutputStream() = default;

		virtual void write(uint8_t* buffer, size_t length, ErrorOr<size_t> async ) = 0;
		virtual void closeWrite() = 0;
	};

	class AsyncIoStream : public AsyncInputStream, public AsyncOutputStream {
	public:
	};

	class StreamListener {
	public:
		virtual void accept(ErrorOr<std::unique_ptr<AsyncIoStream>> async) = 0;
	};

	class NetworkAddress {
	public:
		virtual ~NetworkAddress() = default;

		virtual void listen(ErrorOr<std::unique_ptr<StreamListener>> async) = 0;
		virtual void connect(ErrorOr<std::unique_ptr<AsyncIoStream>> async) = 0;
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
		virtual void parseAddress(ErrorOr<std::unique_ptr<AsyncIoStream>> async, const std::string& addr, int32_t port_hint = 0) = 0;
	};

	struct AsyncIoContext {
		std::unique_ptr<AsyncIoProvider> provider;
		EventPoll& event_poll;
	};

	AsyncIoContext setupAsyncIo();
}
