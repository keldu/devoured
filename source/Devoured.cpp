#include "Devoured.h"

#include "Config.h"

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
		while(isActive()){
			;
		}
	}

	std::unique_ptr<Devoured> createContext(int argc, char** argv){
		std::unique_ptr<Devoured> context;

		const Config config = parseConfig(argc, argv);


		switch(config.mode){
			case Config::Mode::INVALID:{
				std::cerr<<"Invalid Config"<<std::endl;
				context = std::make_unique<InvalidDevoured>();
				break;
			}
			case Config::Mode::SERVICE:{
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