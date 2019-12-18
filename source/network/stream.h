#pragma once

#include <cstdint>

#include "fd_observer.h"

using std::size_t;

namespace dvr {
	class InputStream {
	public:
		virtual ~InputStream() = default;

		virtual size_t read(uint8_t* buffer, size_t length) = 0;
	};

	class OutputStream {
	public:
		virtual ~OutputStream() = default;

		virtual size_t write(uint8_t* buffer, size_t length) = 0;
	};

	class IoStream : public InputStream, public OutputStream, public IFdObserver {
	public:
		virtual ~IoStream() = default;
	};
}
