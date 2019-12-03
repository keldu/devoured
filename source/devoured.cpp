#include "devoured.h"
#include <iostream>
#include <chrono>
#include <list>
#include <thread>
#include <sstream>
#include <map>

#include <unistd.h>
#include <sys/types.h>

#include "arguments/parameter.h"
#include "signal_handler.h"

namespace dvr {
	// TODO integrate in non static way. Maybe integrate this in the devoured base class
	static uid_t user_id = 0;
	static std::string user_id_string = "";

	class ServiceDevoured final : public Devoured, public IConnectionStateObserver, public IServerStateObserver {
	private:
		Network network;

		/*
		 * Key - ConnId - Connection ID
		 * Value - the connection ptr
		 */
		std::map<ConnectionId, std::unique_ptr<Connection>> connection_map;
	public:
		ServiceDevoured(const std::string& f):
			Devoured(true, 0),
			next_update{std::chrono::steady_clock::now()},
			config_path{f}
		{}

		std::chrono::steady_clock::time_point next_update;

		void notify(Connection& conn, ConnectionState state) override {
			switch(state){
				case ConnectionState::Broken:{
					connection_map.erase(conn.id());
					std::cout<<"Connection unregistered in DaemonDevoured"<<std::endl;
				}
				break;
			}
		}

		void notify(Server& server, ServerState state) override {
			if( state == ServerState::Accept ){
				auto connection = server.accept(*this);
				ConnectionId id = connection->id();
				connection_map.insert(std::make_pair(id, std::move(connection)));
				std::cout<<"Connection registered in DaemonDevoured"<<std::endl;
			}
		}
	protected:
		void loop() override {
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
			socket_path += config.control_name + user_id_string;

			// unix_socket_address = std::make_unique<UnixSocketAddress>(poll, socket_path);
			// TODO: Check correct file path somewhere

			control_server = network.listen(socket_path,*this);
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
		void loop()override{
			setup();
			while(isActive()){
			}
		}
	private:
		void setup(){
			auto connection = network.connect(std::string{"/tmp/devoured/default"}+ user_id_string, *this);
			MessageRequest msg{
				0,
				static_cast<uint8_t>(Parameter::Mode::STATUS),
				"",
				""
			};
			if(connection){
				if(!asyncWriteRequest(*connection, msg)){
					stop();
				}
			}else{
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
		user_id = ::getuid();
		std::stringstream ss;
		ss<<"-"<<user_id;
		user_id_string = ss.str();

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
