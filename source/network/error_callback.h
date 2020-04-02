#pragma once

#include <functional>
#include <string>
#include <cstdint>

namespace dvr {
	/*
	 * Error callback helper for stream
	 */
	class StreamError {
	private:
		std::string error_msg;
		int64_t error_code;
	public:
		StreamError(const std::string& msg, int64_t c);

		const std::string& message() const;
		int64_t code() const;

		bool isCritical() const;
		bool isRecoverable() const;
	};

	struct Void {};
	template<typename S, typename T>
	class StreamErrorOrValueCallback {
	private:
		std::function<void(S&,const StreamError&)> err_callback;
		std::function<void(S&,T&&)> val_callback;
	public:
		StreamErrorOrValueCallback(
			std::function<void(S&,const StreamError&)>&& err_func,
			std::function<void(S&,T&&)>&& func
		);
	
		/*
		 * Use the void struct for empty fulfillments
		 */
		void set(S& source, T&& value);
		void fail(S& source, const StreamError& error);
	};

	template<typename S, typename T>
	StreamErrorOrValueCallback<S,T>::StreamErrorOrValueCallback(
		std::function<void(S&,const StreamError&)>&& err_func,
		std::function<void(S&,T&&)>&& func
	):
		err_callback{std::move(err_func)},
		val_callback{std::move(func)}
	{}

	template<typename S, typename T>
	void StreamErrorOrValueCallback<S,T>::set(S& source, T&& value){
		val_callback(source, std::move(value));
	}

	template<typename S, typename T>
	void StreamErrorOrValueCallback<S,T>::fail(S& source, const StreamError& error){
		err_callback(source, error);
	}
}
