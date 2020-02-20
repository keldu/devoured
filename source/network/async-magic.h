#pragma once

#include <utility>
#include <algorithm>
#include <memory>

namespace dvr {

/*
 * TODO Error and ErrorOrValue define
 *
 */

template<typename t>
class Promise;

class AsyncEvent;

template<typename T>
Promise<T> reducePromiseType(T*, ...);
template<typename T>
Promise<T> reducePromiseType(Promise<T>*, ...);
template<typename T, typename Reduced = decltype(T::reducePromise(std::declval<Promise<T>>()))>
Reduced reducePromiseType(T*, bool);

template <typename T>
using ReducePromises = decltype(reducePromiseType((T*)nullptr, false));

class PropagateError {
	public:
		class Bottom {
		private:
			Error error;
		public:
			Bottom(Error&& error_p):error{std::move(error_p)}{}

			Error asError(){return std::move(error);}
		};

		Bottom operator()(Error&& err){
			return Bottom{std::move(err)};
		}

		Bottom operator()(const Error& err){
			Error error = err;
			return Bottom{std::move(error)};
		}
};

/*
 * Inspired by capn'proto hacky template magic
 */
template <typename Func, typename T>
struct ReturnType_ { typedef decltype( std::declval<Func>()(std::declval<T>())) Type; };

template <typename Func>
struct ReturnType_<Func, void> { typedef decltype( std::declval<Func>()()) Type; };

template <typename Func, typename T>
using ReturnType = typename ReturnType_<Func, T>::Type;

class PromiseNode;

/*
 * Base should never be deconstructed as PromiseBase, so the virtual destructor isn't necessary.
 * Since Promise and PromiseNode are friend classes the destructor is private
 */
class PromiseBase {
private:
	std::unique_ptr<PromiseNode> node;

	PromiseBase();
	PromiseBase(std::unique_ptr<PromiseNode>&& node_p);/*:node{std::move(node_p)}{}*/
	~PromiseBase();
	
	template<typename T>
	friend class Promise;
	friend class PromiseNode;
public:
};
}
