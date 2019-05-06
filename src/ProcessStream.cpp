#include "ProcessStream.h"

namespace dvr {
	ProcessStream::ProcessStream(int fd, int pid):
		file_descriptor{fd},
		process_id{pid}
	{

	}
}