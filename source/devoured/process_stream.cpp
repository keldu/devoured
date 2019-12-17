#include "process_stream.h"

#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

namespace dvr {
	ProcessStream::ProcessStream(int pid, const std::array<int,3>& fds):
		process_id{pid},
		file_descriptors{fds}
	{

	}

	pid_t ProcessStream::getPID() const {
		return process_id;
	}
	
	const std::array<int,3>& ProcessStream::getFds() const {
		return file_descriptors;
	}
	
	std::unique_ptr<ProcessStream> createProcessStream(const std::string& exec_file, const std::vector<std::string>& arguments){
		std::unique_ptr<ProcessStream> process{nullptr};
		int fds[2][3];

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
			std::vector<char*> arg_array;
			arg_array.resize(arguments.size()+2);
			arg_array.front() = const_cast<char*>(exec_file.data());
			// Fill argument array except for the front and back
			for(size_t i = 1; (i+1) < arg_array.size(); ++i){
				arg_array.at(i) = {const_cast<char*>(exec_file.data())};
			}
			arg_array.back() = nullptr;

			::execvp(exec_file.data(), arg_array.data());
		}else{
			for(uint8_t i = 0; i < 3; ++i){
				close(fds[i][1]);
			}
			process = std::make_unique<ProcessStream>(pid, std::array<int,3>{fds[0][0],fds[1][0],fds[2][0]});
		}
		return process;
	}
}
