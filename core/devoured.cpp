#include "devoured.h"

#include <iostream>

#include "arguments/parameter.h"
#include "signal_handler.h"

namespace dvr {
	Devoured::Devoured(bool act, int sta):
		active{act},
		status{sta}
	{
		signal(SIGINT, signal_handler);
	}

	int Devoured::run(){
		loop();
		
		return status;
	}

	void Devoured::stop(){
		active = false;
	}

	bool Devoured::isActive()const{
		return active && (shutdown_requested != 1);
	}

	int Devoured::getStatus()const{
		return status;
	}

	InvalidDevoured::InvalidDevoured():
		Devoured(false, -1)
	{

	}

	void InvalidDevoured::loop(){
	}

	ServiceDevoured::ServiceDevoured(const std::string& f, const std::string& t):
		Devoured(true, 0),
		config_path{f},
		target{t}
	{

	}

	void ServiceDevoured::loop(){
		setup();

		while(isActive()){
			
		}
	}

	void ServiceDevoured::setup(){
		config = parseConfig(config_path);
		setupControlInterface();
	}

	void ServiceDevoured::setupControlInterface(){
		std::string socket_path = config.control_iloc;
		if(target.empty()){
			socket_path += config.control_name;
		}else{
			socket_path += target;
		}
		unix_socket_address = std::make_unique<UnixSocketAddress>(socket_path);
		//TODO: Check correct file path somewhere

		control_acceptor = unix_socket_address->listen();
		if(!control_acceptor){
			stop();
		}
	}

	std::unique_ptr<Devoured> createContext(int argc, char** argv){
		std::unique_ptr<Devoured> context;

		const Parameter parameter = parseParams(argc, argv);

		switch(parameter.mode){
			case Parameter::Mode::INVALID:{
				context = std::make_unique<InvalidDevoured>();
				break;
			}
			case Parameter::Mode::SERVICE:{
				context = std::make_unique<ServiceDevoured>(parameter.devour.value(),parameter.target);
				break;
			}
			default:{
				std::cerr<<"Unimplemented case"<<std::endl;
				context = std::make_unique<InvalidDevoured>();
				break;
			}
		}

		return context;
	}
}