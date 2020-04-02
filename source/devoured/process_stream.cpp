#include "process_stream.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

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

	bool ProcessStream::stop() {
		int rv = ::kill(process_id, SIGTERM);
		return rv < 0;
	}

	bool ProcessStream::kill() {
		int rv = ::kill(process_id, SIGKILL);
		return rv < 0;
	}
	
	std::unique_ptr<ProcessStream> createProcessStream(const std::string& exec_file, const std::vector<std::string>& arguments, const std::string& working_directory, AsyncIoProvider& provider, StreamErrorOrValueCallback<IoStream,IoStreamState>&& observer){
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
			int rc = chdir(working_directory.c_str());
			if( rc < 0){
				exit(1);
			}
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
			// Fill argument array except for the front and back
			
			arg_array.reserve(arguments.size()+2);
			arg_array.push_back(const_cast<char*>(exec_file.data()));
			for(auto& arg : arguments){
				arg_array.push_back(const_cast<char*>(arg.data()));
			}
			arg_array.push_back(nullptr);
			
			int rv = ::execvp(exec_file.data(), arg_array.data());

			if(rv < 0){
				for(uint8_t i = 0; i < 3; ++i){
					close(fds[i][1]);
				}
			}
			/*
			 * If the creation of the environemnt fails, then just kill the child.
			 */
			exit(-1);
		}else{
			for(uint8_t i = 0; i < 3; ++i){
				close(fds[i][1]);
			}

			auto in_obsrv = std::move(observer);
			auto out_obsrv = in_obsrv;
			auto err_obsrv = in_obsrv;

			auto in = provider.wrapOutputFd(fds[0][0], std::move(in_obsrv));
			auto out = provider.wrapInputFd(fds[1][0], std::move(out_obsrv));
			auto err = provider.wrapInputFd(fds[2][0], std::move(err_obsrv));

			process = std::make_unique<ProcessStream>(exec_file, pid, std::move(in), std::move(out), std::move(err));
		}
		return process;
	}
}
