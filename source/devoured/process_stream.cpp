#include "process_stream.h"

#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

namespace dvr {
	ProcessStream::ProcessStream(const std::string& ef, int pid, std::unique_ptr<OutputStream> in, std::unique_ptr<InputStream> out, std::unique_ptr<InputStream> err):
		exec_file{ef},
		process_id{pid},
		std_in{std::move(in)},
		std_out{std::move(out)},
		std_err{std::move(err)}
	{
	}

	OutputStream& ProcessStream::in(){
		return *std_in;
	}
	
	InputStream& ProcessStream::out(){
		return *std_out;
	}
	
	InputStream& ProcessStream::err(){
		return *std_err;
	}
	
	int ProcessStream::getPID() const {
		return process_id;
	}
	
	std::unique_ptr<ProcessStream> createProcessStream(const std::string& exec_file, AsyncIoProvider& provider, IStreamStateObserver& observer){
		std::unique_ptr<ProcessStream> process{nullptr};
		int fds[3][2];

		// Creating each pipe
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
				return nullptr;
			}
		}

		// Swapping the output streams
		// fd[i][0] is the input/writable side when created by pipe and fd[i][1] is the output/readable side.
		// Because the process gets fd[i][1], the writable side has to be swapped.
		for(uint8_t i : {1,2} ){
			int temp = fds[i][0];
			fds[i][0] = fds[i][1];
			fds[i][1] = temp;
		}

		int pid = fork();
		if( pid < 0 ){
			for(uint8_t i = 0; i < 3; ++i){
				for(uint8_t j = 0; j < 2; ++j){
					close(fds[i][j]);
				}
			}
		}else if( pid == 0 ){
			/* 
			 * replaces all default streams with the following fds 
			 * and closes the parent fds
			 */
			for(uint8_t i = 0; i < 3; ++i){
				close(fds[i][0]);
				dup2(fds[i][1],i);
			}
			
			// TODO
			// replace the exec with a handling loop which
			// waits for a start signal by the core loop
			// It should be possible that specific signals are not forwarded to the service.
			// For SIGTERM handling etc
			// 
			// Or just keep a record of service information and create the process stream only when needed.
			execlp("terraria", "terraria", NULL);
		}else{
			for(uint8_t i = 0; i < 3; ++i){
				close(fds[i][1]);
			}

			auto in = provider.wrapOutputFd(fds[0][0], observer);
			auto out = provider.wrapInputFd(fds[1][0], observer);
			auto err = provider.wrapInputFd(fds[2][0], observer);

			process = std::make_unique<ProcessStream>(exec_file, pid, std::move(in), std::move(out), std::move(err));
		}
		return process;
	}
}
