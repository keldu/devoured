#include "error_callback.h"

namespace dvr {
	StreamError::StreamError(const std::string& msg, int64_t c):
		error_msg{msg},
		error_code{c}
	{}

	const std::string& StreamError::message() const {
		return error_msg;
	}

	int64_t StreamError::code() const {
		return error_code;
	}

	bool StreamError::isCritical() const {
		return error_code < 0 && error_code >= -100;
	}

	bool StreamError::isRecoverable() const {
		return error_code < -100;
	}
}
