#pragma once

#include <memory>
#include <optional>
#include <cstdint>
#include <variant>
#include <functional>

using std::size_t;

#include "async-magic.h"

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

		bool failed() const {
			return error_code != 0;
		}
	};

	template<typename T>
	class ErrorOr {
	private:
		std::variant<Error, T> error_or_value;
	public:
		void visit(std::function<void(T&)> value_func);
		void visit(std::function<void(T&)> value_func, std::function<void(const Error&)> error_func);
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
