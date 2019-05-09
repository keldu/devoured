#pragma once

#include <array>
#include <memory>

namespace dvr {
	class ProcessStream{
	public:
		ProcessStream(int pid, std::array<int,3> fds);

		/*
		 *	returns the file descriptor from the parent side which replaced
		 *	stdin 0,
		 *	stdout 1,
		 *	stderr 2
		 */
		int getFD(int from_fd) const;
		/*
		 *	return the process id of the child
		 */
		int getPID() const;
	private:
		int process_id;
		std::array<int,3> file_descriptors;
	};

	int createProcessStream(std::unique_ptr<ProcessStream>& process_stream);
}
