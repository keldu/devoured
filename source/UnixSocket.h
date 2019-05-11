#pragma once

#include <future>
#include <memory>

namespace dvr {
	class UnixSocketAddress;

	class Stream{
	public:
		Stream(int fd);

	private:
		int file_descriptor;
	};

	class StreamAcceptor {
	public:
		StreamAcceptor(UnixSocketAddress& addr);

		std::unique_ptr<Stream> accept();
	private:
		UnixSocketAddress* address;
	};

	class UnixSocketAddress {
	public:
		UnixSocketAddress(const std::string& unix_address);

		std::unique_ptr<StreamAcceptor> listen();
		std::unique_ptr<Stream> connect();

		int getFD() const;
	private:
		int file_descriptor;
		std::string bind_address;
	};
}