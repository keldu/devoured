#include "Devoured.h"

#include "Parameter.h"

#include <iostream>

namespace dvr {
	Devoured::Devoured(bool act, int sta):
		active{act},
		status{sta}
	{

	}

	int Devoured::run(){
		loop();
		
		return status;
	}

	void Devoured::stop(){
		active = false;
	}

	bool Devoured::isActive()const{
		return active;
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
		Config conf = readConf();
		setupControl(conf);

		while(isActive()){
			
		}
	}

	ServiceDevoured::Config ServiceDevoured::readConf(){
		Config conf;
		conf.control_path = "/var/run/terraria";
		return conf;
	}

	void ServiceDevoured::setupControl(ServiceDevoured::Config& conf){
		std::unique_ptr<UnixSocketAddress> socket_address = std::make_unique<UnixSocketAddress>(conf.control_path);
		//TODO: Check correct file path somewhere

		control_acceptor = socket_address->listen();
		if(!control_acceptor){
			stop();
		}
	}

	std::unique_ptr<Devoured> createContext(int argc, char** argv){
		std::unique_ptr<Devoured> context;

		const Parameter config = parseParams(argc, argv);

		switch(config.mode){
			case Parameter::Mode::INVALID:{
				std::cerr<<"Invalid Config"<<std::endl;
				context = std::make_unique<InvalidDevoured>();
				break;
			}
			case Parameter::Mode::SERVICE:{
				std::cerr<<"Service Context case, but not output not ready"<<std::endl;
				context = std::make_unique<ServiceDevoured>(config.devour.value(),config.target);
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