#include "ProcessStream.h"

namespace dvr {
	ProcessStream::ProcessStream(int pid, std::array<int,3> fds):
		process_id{pid},
		file_descriptors{fds}
	{

	}
}
