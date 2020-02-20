#pragma once

#include <memory>

#include "network/async.h"

namespace dvr {
	class ConnectionContext {
	private:
		/*
		 * Not too sure about this callback. I want promises, but don't want a capn'proto dependency :/
		 */
		ErrorOr<bool> callback;
		std::unique_ptr<AsyncIoStream> stream;
		std::vector<uint8_t> read_buffer;
	public:
	};
}
