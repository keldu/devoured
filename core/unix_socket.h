#pragma once

#include <memory>
#include <set>

namespace dvr {
	class UnixSocketAddress;
	class Stream;

	class FdObserver {
	public:
		void select();

		void observe(int fd);
		void forget(int fd);
	private:
		std::set<Stream*> streams;
	};

	class Stream{
	public:
		Stream(int fd);

		bool operator<(const Stream& b) const {
			return file_descriptor < b.file_descriptor;
		}

	private:
		int file_descriptor;
	};

	class StreamAcceptor {
	public:
		StreamAcceptor(const std::string& sp, int fd);
		~StreamAcceptor();

		std::unique_ptr<Stream> accept();
	private:
		std::string socket_path;
		int file_descriptor;
	};

	class UnixSocketAddress {
	public:
		UnixSocketAddress(const std::string& unix_address);

		std::unique_ptr<StreamAcceptor> listen();
		std::unique_ptr<Stream> connect();

		int getFD() const;
		const std::string& getPath() const;
	private:
		std::string bind_address;
	};
}