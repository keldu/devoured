#include "service.h"

namespace dvr {
	Service::Service(const ServiceConfig& config, AsyncIoProvider& provider):
		config{config},
		provider{provider},
		process{nullptr},
		state{State::OFF}
	{
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
		process = nullptr; //createProcessStream("/usr/bin/ping",{"8.8.8.8"}, provider, *this);
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
