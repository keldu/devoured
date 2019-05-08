#pragma once

#include <string>
#include <array>
#include <memory>

namespace dvr {
	class ProcessStream{
	public:
		ProcessStream(int pid, std::array<int,3> fds);

		int getInFD();
		int getOutFD();
		int getErrFD();
		int getPID();
	private:
		int process_id;
		std::array<int,3> file_descriptors;
	};
}
