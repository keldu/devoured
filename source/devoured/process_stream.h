#pragma once

#include <array>
#include <memory>
#include <vector>

#include "network/event_poll.h"
#include "network/stream.h"

namespace dvr {
	class ProcessStream {
	private:
		pid_t process_id;
		
		std::array<int,3> file_descriptors;

		/* TODO
		 * will replace file_descriptors
		 */
		std::unique_ptr<OutputStream> std_in;
		std::unique_ptr<InputStream> std_out;
		std::unique_ptr<InputStream> std_err;
	public:
		ProcessStream(int pid, std::array<int,3>&& fds, EventPoll& event_poll);

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
