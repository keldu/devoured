#pragma once

#include <array>
#include <memory>
#include <vector>

#include "network/async.h"

namespace dvr {
	class ProcessStream {
	private:
		pid_t process_id;
		
		/* TODO
		 * will replace file_descriptors
		 */
		std::unique_ptr<AsyncOutputStream> std_in;
		std::unique_ptr<AsyncInputStream> std_out;
		std::unique_ptr<AsyncInputStream> std_err;
	public:
		ProcessStream(int pid, std::unique_ptr<AsyncOutputStream> in, std::unique_ptr<AsyncInputStream> out, std::unique_ptr<AsyncInputStream> err);

		/*
		 *	returns the file descriptor streams from the parent side which replaced
		 *	stdin 0,
		 *	stdout 1,
		 *	stderr 2
		 */
		AsyncOutputStream& in() const;
		AsyncInputStream& out() const;
		AsyncInputStream& err() const;
		/*
		 *	return the process id of the child
		 */
		pid_t getPID() const;
	};

	std::unique_ptr<ProcessStream> createProcessStream(const std::string& exec_file, const std::vector<std::string>& arguments, EventPoll& ev_poll);
}
