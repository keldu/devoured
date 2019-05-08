#include "ProcessStream.h"

#include <sys/socket.h>
#include <unistd.h>

namespace dvr {
	ProcessStream::ProcessStream(int pid, std::array<int,3> fds):
		process_id{pid},
		file_descriptors{fds}
	{

	}
int createProcessAndStream(std::unique_ptr<ProcessStream>& process){
	int fds[2][3];

	//Creating each pipe
	for(int8_t i = 0; i < 3; ++i){
		int rv = pipe(fds[i]);
		if( rv == -1 ){
			std::cerr<<"Failed to create pipe "<<std::to_string(i)<<std::endl;
			//Closing previously opened pipes
			for(int8_t j = i - 1; j >= 0; --j){
				for(int8_t k = 0; k < 2; ++k){
					close(fds[j][k]);
				}
			}
			return -2;
		}
	}

	int pid = fork();
	if( pid == -1 ){
		for(uint8_t i = 0; i < 3; ++i){
			for(uint8_t j = 0; j < 2; ++j){
				close(fds[i][j]);
			}
		}
	}else if( pid == 0 ){
		/* replaces all default streams with the following fds 
		 * and closes the parent fds
		 */
		for(uint8_t i = 0; i < 3; ++i){
			close(fds[i][0]);
			dup2(fds[i][1],i);
		}
		
		//replace the exec with a handling loop which
		//waits for a start signal by the core loop
		//It should be possible that specific signals are not forwarded to the service.
		//For SIGTERM handling etc
		//Also it should be possible that a program goes to sleep whenever possible.
		execlp("terraria", "terraria", NULL);
	}else{
		for(uint8_t i = 0; i < 3; ++i){
			close(fds[i][1]);
		}
		process = std::make_unique<ProcessStream>(pid,std::array<int,3>{fds[0][0],fds[1][0],fds[2][0]});
	}

	return pid;
}
}
