#include "devoured.h"

#include <iostream>
#include <chrono>
#include <thread>

#include "arguments/parameter.h"
#include "signal_handler.h"

namespace dvr {
	class ServiceDevoured final : public Devoured {
	private:
		EventPoll poll;
	public:
		ServiceDevoured(const std::string& f):
			Devoured(true, 0),
			next_update{std::chrono::steady_clock::now()},
			config_path{f}
		{}

		std::chrono::steady_clock::time_point next_update;
	protected:
		void loop(){
			setup();
			while(isActive()){
				std::this_thread::sleep_until(next_update);
				next_update += std::chrono::milliseconds{100};


			}
		}

		std::string config_path;
	private:
		void setup(){
			config = parseConfig(config_path);
			
			setupControlInterface();
		}
		void setupControlInterface(){
			std::string socket_path = config.control_iloc;
			socket_path += config.control_name;

			unix_socket_address = std::make_unique<UnixSocketAddress>(poll, socket_path);
			//TODO: Check correct file path somewhere

			control_acceptor = unix_socket_address->listen();
			if(!control_acceptor){
				stop();
			}
		}

		Config config;

		std::unique_ptr<UnixSocketAddress> unix_socket_address;
		std::unique_ptr<StreamAcceptor> control_acceptor;
		std::set<Stream> control_streams;
	};

	Devoured::Devoured(bool act, int sta):
		active{act},
		status{sta}
	{
		register_signal_handlers();
		//signal(SIGINT, signal_handler);
	}

	int Devoured::run(){
		loop();
		return status;
	}

	void Devoured::stop(){
		active = false;
	}

	bool Devoured::isActive()const{
		return active && !shutdown_requested();
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

	std::unique_ptr<Devoured> createContext(int argc, char** argv){
		std::unique_ptr<Devoured> context;
		const Parameter parameter = parseParams(argc, argv);

		switch(parameter.mode){
			case Parameter::Mode::INVALID:{
				context = std::make_unique<InvalidDevoured>();
				break;
			}
			case Parameter::Mode::SERVICE:{
				context = std::make_unique<ServiceDevoured>("config.toml");
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
