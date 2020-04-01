#pragma once

#include <array>
#include <memory>

#include "network/network.h"

namespace dvr {
	class ProcessStream{
	private:
		std::string exec_file;
		const int process_id;
		
		std::unique_ptr<OutputStream> std_in;
		std::unique_ptr<InputStream> std_out;
		std::unique_ptr<InputStream> std_err;
	public:
		ProcessStream(const std::string& ef, int pid, std::unique_ptr<OutputStream> in, std::unique_ptr<InputStream> out, std::unique_ptr<InputStream> err);

		/*
		 *	returns the file descriptor from the parent side which replaced
		 *	stdin 0,
		 *	stdout 1,
		 *	stderr 2
		 */
		OutputStream& in();
		InputStream& out();
		InputStream& err();
		/*
		 *	return the process id of the child
		 */
		int getPID() const;

		/*
		* kill or stop the process
		*/
		bool stop();
		bool kill();
	};

	std::unique_ptr<ProcessStream> createProcessStream(const std::string& exec_file, const std::vector<std::string>& arguments, const std::string& working_directory, AsyncIoProvider& provider, IStreamStateObserver& obsrv);
}
