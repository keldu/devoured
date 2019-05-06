#pragma once

#include <string>
#include <vector>
#include <memory>

namespace dvr {
	class ProcessStream{
	public:
		ProcessStream(int fd, int pid);

		int getFD();
		int getPID();
	private:
		int file_descriptor;
		int process_id;
	};
}