#pragma once

#include <array>
#include <memory>

namespace dvr {
	class ProcessStream{
	public:
		ProcessStream(const std::string& ef, int pid, std::array<int,3> fds);

		/*
		 *	returns the file descriptor from the parent side which replaced
		 *	stdin 0,
		 *	stdout 1,
		 *	stderr 2
		 */
		const std::array<int,3>& getFD() const;
		/*
		 *	return the process id of the child
		 */
		int getPID() const;
	private:
		int process_id;
		std::array<int,3> file_descriptors;
		std::string exec_file;
	};

	std::pair<std::unique_ptr<ProcessStream>,int> createProcessStream(const std::string& exec_file);
}
