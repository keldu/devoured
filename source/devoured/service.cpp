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
		service.state = State::BROKEN;
	}

	void Service::start(){
		if(state==State::ON){
			if(!process){
				state = State::BROKEN;
			}
			return;
		}
		state = State::ON;

		// TODO add provider somehow
		process = createProcessStream(config.start_command, config.arguments, config.working_directory, provider, *this);
		if(!process){
			state = State::BROKEN;
			return;
		}
	}

	void Service::stop(){
	}

	Service createService(const ServiceConfig& config, AsyncIoProvider& provider){
		return Service{config, provider};
	}
}
