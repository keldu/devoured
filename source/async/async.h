#pragma once

#include <memory>
#include <optional>
#include <cstdint>
#include <variant>
#include <functional>

using std::size_t;

#include "async-magic.h"

/*
 * Credits to capnproto which I don't want to depend on, but
 * rather would have a smaller dependecy base. Plus I'm designing Belts
 * as a belt and not a one time thing.
 */

namespace dvr {
	/*
	 *
	 */
	template<typename Func, typename T>
	using BeltForResult = ReduceBelts<ReturnType<Func, T>>;

	/*
	 * Capn'proto Belt class
	 */
	template<typename T>
	class Belt : public BeltBase {
	public:
		/*
		 * Construct a fulfilled promise
		 */
		Belt(FixVoid<T> value);
		/*
		 * Construct a broken promise
		 */
		Belt(const Error& error);
		Belt(decltype(nullptr)){}

		/*
		 * I don't want to use the whole set of functor hacks just to get a more generel definition, especially since I want to use only lambdas anyway...
		 * I think I have to define the default function from PropagateError later on.
		 * Because this intereferes with the ability to use Belt<void> as a possible promise.
		 * I want to have the T&&, but void&& isn't possible
		 *
		 * TODO change PropagateError to support std::function and don't let it act as a functor
		 */
		template<typename Func>
		BeltForResult<Func, T> then(std::function<ReturnType<Func, T>>&& func, std::function<void(Error&&)>&& error_func);

		/*
		 * This allows the attachment of any tuple of objects to a promise to ensure
		 * object lifetime while the promise exists
		 */
		template<typename... Attachments>
		void attach(Attachments&&... attachments);
	};

	class EventLoop;
	class Event {
	private:
		EventLoop& loop;
	protected:
		virtual std::optional<std::unique_ptr<Event>> fire() = 0;
	public:
		Event(EventLoop& loop);
		virtual ~Event() = default;

		void armDepthFirst();
		void armBreadthFirst();
		void armLast();
		void disarm();
	};

 	/*
	 * Base class which a promise inherits
	 */
	class BeltNode {
	public:
		enum class State : uint8_t {
			WORKING = 1,
			OUTPUT_READY = 2,
			INPUT_READY = 4
		};
	private:
		BeltNode* parent;
		State state;
	public:
		BeltNode();
		virtual ~BeltNode() = default;

		/*
		 * Do I want to do it this way?
		 * I want promise nodes to be desconstruct themselves,
		 * when their dependency is fulfilled.
		 */
		
		void setParentPtr(BeltNode& node);

		virtual void set(ErrorOrValue&& error) = 0;

		virtual bool hasResult() const = 0;
	};

	template<typename T, typename DepT>
	class TransformBeltNode final : public BeltNode{
	private:
		std::function<ReturnType<T,DepT>> function;

		std::unique_ptr<BeltNode> dependency;
	public:
		TransformBeltNode(std::unique_ptr<BeltNode>&& depend, std::function<ReturnType<T,DepT>>&& func, std::function<ReturnType<void, Error&&>> err_func );
		TransformBeltNode(std::unique_ptr<BeltNode>&& depend, std::function<ReturnType<T,DepT>>&& func );
		~TransformBeltNode();

		void dropDependency();

		bool isReady() const;
	};

	template<typename T>
	class AdapterBeltNode final : public BeltNode {
	private:
		// BeltFulfiller<T>* fulfiller;
	public:
		AdapterBeltNode();
		~AdapterBeltNode();

		void fulfill(T&& value);
		void fail(Error&& error);
	};

	/*
	 * This is my epoll implementation interface
	 * Abstract for other implementations
	 */
	class EventPoll {
	public:
		virtual ~EventPoll() = default;

		virtual void wait() = 0;
		virtual void poll() = 0;
	};

	class EventLoop {
	private:
		EventPoll* ev_poll;
		bool running;

		friend class WaitScope;

		void enterScope();
		void leaveScope();
	public:
		EventLoop();
		EventLoop(EventPoll& poll);

		void turn();
		void poll();

		bool isRunnable() const;
		void setRunnable(bool run);
	};

	class WaitScope {
	private:
		EventLoop& loop;
	public:
		WaitScope(EventLoop& loop);
		~WaitScope();

		void poll();
		void wait();
	};

	class AsyncInputStream {
	public:
		virtual ~AsyncInputStream() = default;

		virtual Belt<size_t> read(uint8_t* buffer, size_t length) = 0;
		virtual void closeRead() = 0;
	};

	class AsyncOutputStream {
	public:
		virtual ~AsyncOutputStream() = default;

		virtual Belt<size_t> write(uint8_t* buffer, size_t length ) = 0;
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

	/*
	 * Template Implementation / Inline Block
	 */

	
}
