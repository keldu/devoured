#include "service.h"

namespace dvr {
	Service::Service(const ServiceConfig& config, AsyncIoProvider& provider):
		config{config},
		provider{provider},
		process{nullptr},
		state{State::OFF}
	{
	}

	Service::~Service(){
	}

	Service::Service(Service&& service):
		config{service.config},
		provider{service.provider},
		process{std::move(service.process)},
		state{service.state}
	{
		service.state = State::TERMINATED;
	}

	void Service::start(){
		if(state==State::ON){
			if(!process){
				state = State::TERMINATED;
			}
			return;
		}
		state = State::ON;

		process = createProcessStream(config.start_command, config.arguments, config.working_directory, provider, 
			StreamErrorOrValueCallback<IoStream, IoStreamState>{
				std::function<void(IoStream&,const StreamError&)>{[this](IoStream& source, const StreamError& value){
					// TODO Handle Error
				}},
				std::function<void(IoStream&,IoStreamState&&)>{[this](IoStream& source, IoStreamState&& value){
					notify(source, std::move(value));
				}}
			}
		);
		if(!process){
			state = State::TERMINATED;
			return;
		}
	}
	// TODO handle stop correctly
	void Service::stop(){
		if(process){
		}else{
		}
	}

	Service createService(const ServiceConfig& config, AsyncIoProvider& provider){
		return Service{config, provider};
	}
}
