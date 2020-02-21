#pragma once

#include <utility>
#include <algorithm>
#include <memory>
#include <variant>

namespace dvr {

class Error {
private:

public:
	Error();
	Error(const std::string& msg);
	Error(const std::string& msg, int64_t ec);

	const std::string& message() const;
	int64_t code() const;
	bool failed() const;
};

class ErrorOrValue {
	public:
		virtual ~ErrorOrValue() = default;
};

template<typename T>
class ErrorOr : public ErrorOrValue {
private:
	std::variant<T, Error> value_or_error;
public:
	ErrorOr(const T& value);
	ErrorOr(T&& value);

	ErrorOr(const Error& error);
	ErrorOr(Error&& error);

	void visit(std::function<void(T&)> value_visitor);
	void visit(std::function<void(T&)> value_visitor, std::function<void(const Error& error)> error_visitor);
};

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
 * Why not use std::function with std::function<ReturnType>?
 */
template <typename Func, typename T>
struct ReturnType_ { typedef decltype( std::declval<Func>()(std::declval<T>())) Type; };

template <typename Func>
struct ReturnType_<Func, void> { typedef decltype( std::declval<Func>()()) Type; };

template <typename Func, typename T>
using ReturnType = typename ReturnType_<Func, T>::Type;

// Usage hints for the return type
// template<typename Func, typename T>
// std::function<ReturnType<Func,T>>
// std::function<ReturnType<void, int>> 
// std::function<void(int)>

/*
 * Unsure if I can keep this
 */

struct Void {};
template<typename T> struct FixVoid_ {typedef T Type; };
template<> struct FixVoid_<void> { typedef Void Type; };
template<typename T> using FixVoid = typename FixVoid_<T>::Type;


template<typename T> struct UnfixVoid_ {typedef T Type; };
template<> struct UnfixVoid_<Void> { typedef void Type; };
template<typename T> using UnfixVoid = typename UnfixVoid_<T>::Type;

class PromiseNode;
/*
class NeverDone {
public:
	template<typename T>
	operator Promise<T>() const;
};
*/

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
