#include "ProcessStream.h"
#include <memory>

#include <sys/socket.h>

using dvr::ProcessStream;

int createProcessAndStream(std::unique_ptr<ProcessStream>& process){
	int sockets[2];
	socketpair(PF_UNIX, SOCK_STREAM, AF_UNIX, sockets);

	return 0;
}

int main(int argc, char** argv){

	std::unique_ptr<ProcessStream> process;
	/*
		Even though I know that copy on write is implemented for linux when executing fork
		I still want to stay as low as possible in the stack. I will maintain a little bit
		C style language while handling fork requests.
	*/
	int rv = createProcessAndStream(process);

	return 0;
}