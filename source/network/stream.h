#pragma once

#include <cstdint>
#include <memory>
#include <streambuf>

#include "fd_observer.h"

using std::size_t;

namespace dvr {
	typedef uint64_t StreamId;
	
	class InputStream {
	public:
		virtual ~InputStream() = default;

		virtual size_t read(uint8_t* buffer, size_t length) = 0;
		virtual void closeRead() = 0;
	};

	class OutputStream {
	public:
		virtual ~OutputStream() = default;

		virtual size_t write(uint8_t* buffer, size_t length) = 0;
		virtual void closeWrite() = 0;
	};

	class IoStream;
	enum class StreamState {
		ReadReady,
		ReadClosed,
		WriteReady,
		WriteClosed
	};
	class IStreamStateObserver {
	public:
		virtual ~IStreamStateObserver() = default;
		virtual void notify(IoStream& stream, StreamState hint) = 0;
	};

	class IoStream : public InputStream, public OutputStream, public IFdObserver {
	private:
		const StreamId id;
		int file_desc;

		bool read_enabled;
		bool write_enabled;
	public:
		IoStream(EventPoll& poll, uint32_t mask, int fd);

		const int& fd() const;

		void closeWrite() override;
		void closeRead() override;

		size_t write(uint8_t* buffer, size_t length) override;
		size_t read(uint8_t* buffer, size_t length) override;
	};

	struct SocketPair {
		std::unique_ptr<InputStream> input;
		std::unique_ptr<OutputStream> output;
	};
}
