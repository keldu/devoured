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

template<typename T>
class ErrorOr;

/*
 * Intended as passing around helper.
 * Not every function needs to know which templated
 * type this is. This may result in less code possibly depending
 * on the compiler or the cpp standard
 */
class ErrorOrValue {
public:
	virtual ~ErrorOrValue() = default;

	template<typename T>
	ErrorOr<T>& as();

	template<typename T>
	const ErrorOr<T>& as() const;
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
class Belt;

class AsyncEvent;

template<typename T>
Belt<T> reduceBeltType(T*, ...);
template<typename T>
Belt<T> reduceBeltType(Belt<T>*, ...);
template<typename T, typename Reduced = decltype(T::reduceBelt(std::declval<Belt<T>>()))>
Reduced reduceBeltType(T*, bool);

template <typename T>
using ReduceBelts = decltype(reduceBeltType((T*)nullptr, false));

/*
 * Since no functors are used for propagation. Just set the return value to the error
 */
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
 * For me it's simpler to use and I don't have to play around with
 * function memory addresses
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
 * Unsure if I can should keep this
 */

struct Void {};
template<typename T> struct FixVoid_ {typedef T Type; };
template<> struct FixVoid_<void> { typedef Void Type; };
template<typename T> using FixVoid = typename FixVoid_<T>::Type;


template<typename T> struct UnfixVoid_ {typedef T Type; };
template<> struct UnfixVoid_<Void> { typedef void Type; };
template<typename T> using UnfixVoid = typename UnfixVoid_<T>::Type;

class BeltNode;
/*
class NeverDone {
public:
	template<typename T>
	operator Belt<T>() const;
};
*/

/*
 * Base should never be deconstructed as BeltBase, so the virtual destructor isn't necessary.
 * Since Belt and BeltNode are friend classes the destructor is private
 */

class BeltBase {
private:
	/*
	 * Every promise has a promise node
	 */
	std::unique_ptr<BeltNode> node;

	BeltBase();
	BeltBase(std::unique_ptr<BeltNode>&& node_p);/*:node{std::move(node_p)}{}*/
	~BeltBase();
	
	template<typename T>
	friend class Belt;
	friend class BeltNode;
public:
};
}
