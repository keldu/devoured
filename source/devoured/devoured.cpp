#include "devoured.h"

#include <iostream>
#include <chrono>
#include <list>
#include <thread>
#include <sstream>
#include <map>
#include <functional>
#include <cassert>
#include <unistd.h>
#include <sys/types.h>

#include "arguments/parameter.h"
#include "signal_handler.h"
#include "network/protocol.h"
#include "network/event_poll.h"

#include "service.h"

namespace dvr {
	const std::chrono::milliseconds sleep_interval{100};
	const std::chrono::seconds single_request_timeout{10};

	// TODO integrate in non static way. Maybe integrate this in the devoured base class
	static uid_t user_id = 0;
	static std::string user_id_string = "";

	class DaemonDevoured final : public Devoured, public IoStateObserver<Connection>, public IServerStateObserver {
	private:
		EventPoll event_poll;
		Network network;

		std::map<Devoured::Mode, std::function<void(Connection&,const MessageRequest&)>> request_handlers;
		/*
		 * Key - ConnId - Connection ID
		 * Value - the connection ptr
		 */
		std::map<ConnectionId, std::unique_ptr<Connection>> connection_map;

		/*
		 * Key - Target - String
		 * Value - Target Configuration and State
		 */
		std::map<std::string, int> targets;
		
		std::chrono::steady_clock::time_point next_update;
		
		Config config;

		std::unique_ptr<Server> control_server;
		std::list<std::unique_ptr<Connection>> control_streams;
		
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

		void handleStatus(Connection& connection, const MessageRequest& req){
			std::cout<<"Handling status messages"<<std::endl;
			auto t_find = targets.find(req.target);
			if(t_find != targets.end()){

			}else if(req.target.empty()){
				std::string content;
				if(targets.empty()){
					content = "Currently no service registered";
				}else{
					std::stringstream ss;
					for(auto iter = targets.begin(); iter != targets.end(); ++iter){
						ss<<iter->first<<": "<<"set status message here"<<"\n";
						ss<<"\tMaybe add aditional info here\n"<<std::endl;
					}
					content = ss.str();
				}
				MessageResponse resp{
					req.request_id,
					static_cast<uint8_t>(ReturnCode::OK),
					req.target,
					content
				};
				if(!asyncWriteResponse(connection, resp)){
					std::cerr<<"Response in error mode"<<std::endl;
					stop();
				}
			}else if(req.target == "devoured"){
				MessageResponse resp{
					req.request_id,
					static_cast<uint8_t>(ReturnCode::OK),
					req.target,
					"Devoured feels ok. Thanks for asking"
				};
				if(!asyncWriteResponse(connection, resp)){
					std::cerr<<"Response in error mode"<<std::endl;
					stop();
				}
			}else{
				MessageResponse resp{
					req.request_id,
					static_cast<uint8_t>(ReturnCode::NOSERVICE),
					req.target,
					"No matching service found"
				};
				if(!asyncWriteResponse(connection, resp)){
					std::cerr<<"Response in error mode"<<std::endl;
					stop();
				}
			}
		}

		void handleManage(Connection& connection, const MessageRequest& req){
			std::cout<<"Handling manage messages"<<std::endl;

		}
	protected:
		void loop() override {
			setup();
			while(isActive()){
				std::this_thread::sleep_until(next_update);
				next_update += sleep_interval;

				// Update all subsystems like network ...
				event_poll.poll();
			}
		}

		std::string config_path;
	public:
		DaemonDevoured(const std::string& f):
			Devoured(true, 0),
			network{event_poll},
			request_handlers{
				{Devoured::Mode::STATUS,std::bind(&DaemonDevoured::handleStatus, this, std::placeholders::_1, std::placeholders::_2)},
				{Devoured::Mode::MANAGE,std::bind(&DaemonDevoured::handleManage, this, std::placeholders::_1, std::placeholders::_2)}
			},
			next_update{std::chrono::steady_clock::now()},
			config_path{f}
    	{}
    
		void notify(Connection& conn, IoState state) override {
			switch(state){
				case IoState::Broken:{
					connection_map.erase(conn.id());
					std::cout<<"Connection unregistered in DaemonDevoured"<<std::endl;
				}
				break;
				case IoState::ReadReady:{
					auto opt_msg = asyncReadRequest(conn);
					if(opt_msg){
						auto& msg = *opt_msg;
						auto func_find = request_handlers.find(static_cast<Devoured::Mode>(msg.type));
						if(func_find != request_handlers.end()){
							func_find->second(conn,msg);
						}
					}
				}
				break;
			}
		}

		void notify(Server& server, ServerState state) override {
			if( state == ServerState::Accept ){
				auto connection = server.accept(*this);
				if(connection){
					ConnectionId id = connection->id();
					connection_map.insert(std::make_pair(id, std::move(connection)));
					std::cout<<"Connection registered in DaemonDevoured"<<std::endl;
				}
			}
		}
	};
	
	class InvalidDevoured final : public Devoured {
	public:
		InvalidDevoured():
			Devoured(false, -1)
		{}
	protected:
		void loop(){}
	};

	class ManageDevoured final : public Devoured, public IoStateObserver<Connection> {
	private:
		EventPoll event_poll;
		Network network;
		uint16_t req_id;
		std::unique_ptr<Connection> connection;
		std::chrono::steady_clock::time_point next_update;
		std::chrono::steady_clock::time_point response_timeout;
		const std::string target;
		const std::string command;
	protected:
		void loop()override{
			while(isActive()){
				std::this_thread::sleep_until(next_update);
				next_update += sleep_interval;
				event_poll.poll();
				if(std::chrono::steady_clock::now() < response_timeout){
					break;
				}
			}
		}
	public:
		ManageDevoured(const Parameter& params):
			Devoured(true, 0),
			network{event_poll},
			req_id{0},
			connection{nullptr},
			next_update{std::chrono::steady_clock::now()},
			response_timeout{next_update + single_request_timeout},
			target{params.target.has_value()?(*params.target):""},
			command{params.manage.has_value()?(*params.manage):""}
		{
			connection = network.connect(std::string{"/tmp/devoured/default"}+user_id_string, *this);
			MessageRequest msg{
				0,
				static_cast<uint8_t>(Devoured::Mode::MANAGE),
				target,
				command
			};
			if(connection){
				if(!asyncWriteRequest(*connection, msg)){
					stop();
				}
			}else{
				stop();
			}
		}
		
		void notify(Connection& conn, IoState state) override {
			switch(state){
				case IoState::ReadReady:{
					auto opt_msg = asyncReadResponse(conn);
					if(opt_msg.has_value()){
						std::cout<<(*opt_msg)<<std::endl;
						stop();
					}
				}
				break;
				case IoState::Broken:{
					stop();
				}
				break;
				case IoState::WriteReady:{
				}
				break;
			}
		}
	};

	class StatusDevoured final : public Devoured, public IoStateObserver<Connection> {
	private:
		EventPoll event_poll;
		Network network;
		uint16_t req_id;
		std::unique_ptr<Connection> connection;
		std::chrono::steady_clock::time_point next_update;
		const std::string target;
		
	protected:
		void loop()override{
			while(isActive()){
				std::this_thread::sleep_until(next_update);
				next_update += sleep_interval;
				event_poll.poll();
			}
		}
	public:
		StatusDevoured(const Parameter& params):
			Devoured(true, 0),
			network{event_poll},
			req_id{0},
			connection{nullptr},
			next_update{std::chrono::steady_clock::now()},
			target{params.target.has_value()?(*params.target):""}
		{
			connection = network.connect(std::string{"/tmp/devoured/default"}+user_id_string, *this);
			MessageRequest msg{
				0,
				static_cast<uint8_t>(Devoured::Mode::STATUS),
				target,
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

		void notify(Connection& conn, IoState state) override {
			switch(state){
				case IoState::ReadReady:{
					auto opt_msg = asyncReadResponse(conn);
					if(opt_msg.has_value()){
						std::cout<<(*opt_msg)<<std::endl;
						stop();
					}
				}
				break;
				case IoState::Broken:{
					stop();
				}
				break;
				case IoState::WriteReady:{
				}
				break;
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
			case Devoured::Mode::INVALID:{
				context = std::make_unique<InvalidDevoured>();
				break;
			}
			case Devoured::Mode::DAEMON:{
				context = std::make_unique<DaemonDevoured>("config.toml");
				break;
			}
			case Devoured::Mode::STATUS: {
				context = std::make_unique<StatusDevoured>(parameter);
				break;
			}
			case Devoured::Mode::MANAGE: {
				context = std::make_unique<ManageDevoured>(parameter);
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
