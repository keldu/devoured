#include <memory>
#include <iostream>

#include <sys/socket.h>
#include <unistd.h>

#include "ProcessStream.h"

using dvr::ProcessStream;

int createProcessAndStream(std::unique_ptr<ProcessStream>& process){
	int sockets[2];
	int rv = socketpair(PF_UNIX, SOCK_STREAM, AF_UNIX, sockets);
	if ( rv != -1 ){
		std::cerr<<"Failed to create socket"<<std::endl;
		return -2;
	}

	int pid = fork();
	if( pid == -1 ){
		close(sockets[0]);
		close(sockets[1]);
		return -1;
	}else if( pid == 0){
		close(sockets[0]);

		dup2(sockets[1],0);
		dup2(sockets[1],1);

		execlp("terraria", "terraria", NULL);
	}else{
		close(sockets[1]);
		process = std::make_unique<ProcessStream>(sockets[0],pid);
	}

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