#pragma once

#include <array>
#include <memory>
#include <vector>

#include "network/network.h"

namespace dvr {
	class ProcessStream : public IFdObserver{
	private:
		pid_t process_id;
		std::array<int,3> file_descriptors;
	public:
		ProcessStream(int pid, const std::array<int,3>& fds);

		/*
		 *	returns the file descriptor from the parent side which replaced
		 *	stdin 0,
		 *	stdout 1,
		 *	stderr 2
		 */
		const std::array<int,3>& getFds() const;
		/*
		 *	return the process id of the child
		 */
		pid_t getPID() const;
	};

	std::unique_ptr<ProcessStream> createProcessStream(const std::string& exec_file, const std::vector<std::string>& arguments, EventPoll& ev_poll);
}
