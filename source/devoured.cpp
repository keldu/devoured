#include "devoured.h"
#include <iostream>
#include <chrono>
#include <list>
#include <thread>

#include "arguments/parameter.h"
#include "signal_handler.h"

namespace dvr {
	class ServiceDevoured final : public Devoured {
	private:
		Network network;
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
				next_update += std::chrono::milliseconds{1000};

				// Update all subsystems like network ...
				network.poll();
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

			//unix_socket_address = std::make_unique<UnixSocketAddress>(poll, socket_path);
			//TODO: Check correct file path somewhere

			control_server = network.listen(socket_path);
			if(!control_server){
				stop();
			}
		}

		Config config;

		std::unique_ptr<Server> control_server;
		std::list<std::unique_ptr<Connection>> control_streams;
	};
	
	class InvalidDevoured final : public Devoured {
	public:
		InvalidDevoured():
			Devoured(false, -1)
		{}
	protected:
		void loop(){}
	};

	class SpawnDevoured final : public Devoured {
	public:
		SpawnDevoured():
			Devoured(true, 0)
		{}
	protected:
		void loop(){
			while(isActive()){
			}
		}
	};

	class StatusDevoured final : public Devoured, public IConnectionStateObserver {
	private:
		Network network;
	public:
		StatusDevoured():
			Devoured(true, 0)
		{}

		void notify(Connection& conn, ConnectionState state) override {
			(void)conn;
			(void)state;
		}
	protected:
		void loop(){
			setup();
			while(isActive()){
			}
		}
	private:
		void setup(){
			auto connection = network.connect("/tmp/devoured/default", *this);
			MessageRequest msg{
				0,
				static_cast<uint8_t>(Parameter::Mode::STATUS),
				"",
				""
			};
			if(!asyncWriteRequest(*connection, msg)){
				stop();
			}
		}
	};

	Devoured::Devoured(bool act, int sta):
		active{act},
		status{sta}
	{
		register_signal_handlers();
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

	std::unique_ptr<Devoured> createContext(int argc, char** argv){
		std::unique_ptr<Devoured> context;
		const Parameter parameter = parseParams(argc, argv);

		switch(parameter.mode){
			case Parameter::Mode::INVALID:{
				context = std::make_unique<InvalidDevoured>();
				break;
			}
			case Parameter::Mode::DAEMON:{
				context = std::make_unique<ServiceDevoured>("config.toml");
				break;
			}
			case Parameter::Mode::CREATE:{
				context = std::make_unique<SpawnDevoured>();
				break;
			}
			case Parameter::Mode::STATUS: {
				context = std::make_unique<StatusDevoured>();
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
