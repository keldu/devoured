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

namespace fs = std::filesystem;

namespace dvr {
	class DaemonDevoured final : public Devoured, public IStreamStateObserver {
	private:
		std::map<Devoured::Mode, std::function<void(IoStream&,const MessageRequest&)>> request_handlers;
		/*
		 * Key - the file descriptor
		 * Value - the connection ptr
		 */
		std::map<int, std::unique_ptr<IoStream>> connection_map;

		/*
		 * Key - Target - String
		 * Value - Target Configuration and State
		 */
		std::map<std::string, int> targets;
		
		std::chrono::steady_clock::time_point next_update;

		std::list<std::unique_ptr<IoStream>> to_be_destroyed;

		void handleStatus(IoStream& connection, const MessageRequest& req){
			std::cout<<"Handling status messages"<<std::endl;
			auto t_find = targets.find(req.target);
			if(t_find != targets.end()){

			}else if(req.target.empty()){
				MessageResponse resp{
					req.request_id,
					static_cast<uint8_t>(ReturnCode::OK),
					req.target,
					"Currently no service registered"
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
	public:
		DaemonDevoured(Environment&& env):
			Devoured(true, 0, std::move(env)),
			request_handlers{
				{Devoured::Mode::STATUS,std::bind(&DaemonDevoured::handleStatus, this, std::placeholders::_1, std::placeholders::_2)}
			},
			next_update{std::chrono::steady_clock::now()}
    	{
		}
    
		void notify(IoStream& conn, IoStreamState state) override {
			switch(state){
				case IoStreamState::Broken:{
					// connection_map.erase(conn.id());
					auto find = connection_map.find(conn.id());
					if( find != connection_map.end() ){
						to_be_destroyed.push_back(std::move(find->second));
						connection_map.erase(find);
						std::cout<<"IoStream unregistered in DaemonDevoured"<<std::endl;
					}
				}
				break;
				case IoStreamState::ReadReady:{
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
		/*
		void notify(Server& server, ServerState state) override {
			if( state == ServerState::Accept ){
				auto connection = server.accept(*this);
				if(connection){
					int id = connection->id();
					connection_map.insert(std::make_pair(id, std::move(connection)));
					std::cout<<"IoStream registered in DaemonDevoured"<<std::endl;
				}
			}
		}
		*/
	protected:
		void loop() override {
			setup();
			while(isActive()){
				std::this_thread::sleep_until(next_update);
				next_update += std::chrono::milliseconds{1000};

				// Update all subsystems like network ...
				io_context.wait_scope.poll();
				to_be_destroyed.clear();
			}
		}
	private:
		void setup(){
			// std::filesystem
			config = environment.parseConfig();
			
			setupControlInterface();
		}

		void setupControlInterface(){
			std::string sock_file = std::string{"default-"}+std::to_string(environment.userId());
			auto sock_path = environment.tmpPath();
			sock_path = sock_path / sock_file;

			// unix_socket_address = std::make_unique<UnixSocketAddress>(poll, socket_path);
			// TODO: Check correct file path somewhere

			auto address = io_context.provider->parseUnixAddress(sock_path.native());
			if(!address){
				stop();
				return;
			}
			control_server = address->listen(StreamErrorOrValueCallback<Server, Void>{
				std::function<void(Server&, const StreamError&)>{[this](Server& source, const StreamError& error){
					(void)source;
					if(error.isCritical()){
						control_server.reset();
						stop();
					}
				}},
				std::function<void(Server&, Void&&)>{[this](Server& source, Void&& val){
					(void)val;
					auto connection = source.accept(*this);
					if( connection ){
						int id = connection->id();
						connection_map.insert(std::make_pair(id, std::move(connection)));
						std::cout<<"IoStream registered in DaemonDevoured"<<std::endl;
					}
				}}
			});

			if(!control_server){
				stop();
			}
		}

		Config config;

		std::unique_ptr<Server> control_server;
		std::list<std::unique_ptr<IoStream>> control_streams;
	};
	
	class InvalidDevoured final : public Devoured {
	public:
		InvalidDevoured(Environment&& env):
			Devoured(false, -1,std::move(env))
		{}
	protected:
		void loop(){}
	};

	class SpawnDevoured final : public Devoured {
	public:
		SpawnDevoured(Environment&& env):
			Devoured(true, 0, std::move(env))
		{}
	protected:
		void loop(){
			while(isActive()){
			}
		}
	};

	class StatusDevoured final : public Devoured, public IStreamStateObserver {
	private:
		uint16_t req_id;

		std::unique_ptr<IoStream> connection;
		
		std::chrono::steady_clock::time_point next_update;
		
		const std::string target;
	public:
		StatusDevoured(Environment&& env, const Parameter& params):
			Devoured(true, 0, std::move(env)),
			req_id{0},
			connection{nullptr},
			next_update{std::chrono::steady_clock::now()},
			target{params.target.has_value()?(*params.target):""}
		{}

		void notify(IoStream& conn, IoStreamState state) override {
			switch(state){
				case IoStreamState::ReadReady:{
					auto opt_msg = asyncReadResponse(conn);
					if(opt_msg.has_value()){
						std::cout<<(*opt_msg)<<std::endl;
						stop();
					}
				}
				break;
				case IoStreamState::Broken:{
					stop();
				}
				break;
				case IoStreamState::WriteReady:{
				}
				break;
			}
		}
	protected:
		void loop()override{
			setup();
			while(isActive()){
				std::this_thread::sleep_until(next_update);
				next_update += std::chrono::milliseconds{1000};
				io_context.wait_scope.poll();
			}
		}
	private:
		void setup(){
			std::string unix_port_path = std::string{"default-"}+std::to_string(environment.userId());
			auto tmp_path = environment.tmpPath();
			tmp_path = tmp_path/unix_port_path;
			if(!fs::exists(tmp_path)){
				std::cerr<<tmp_path.native()<<" doesn't exist"<<std::endl;
				stop();
				return;
			}
			if(!fs::is_socket(tmp_path)){
				std::cerr<<tmp_path.native()<<" is not a unix socket"<<std::endl;
				stop();
				return;
			}
			auto address = io_context.provider->parseUnixAddress(std::string{tmp_path.native()});
			if(!address){
				stop();
				return;
			}
			connection = address->connect(*this);
			MessageRequest msg{
				0,
				static_cast<uint8_t>(Parameter::Mode::STATUS),
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
	};

	Devoured::Devoured(bool act, int sta, Environment&& env):
		active{act},
		status{sta},
		environment{std::move(env)},
		io_context{setupAsyncIo()}
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
		std::optional<Environment> env = setupEnvironment();
		if(!env.has_value()){
			std::cerr<<"Couldn't setup environment"<<std::endl;
			Environment env_dummy;
			context = std::make_unique<InvalidDevoured>(std::move(env_dummy));
			return context;
		}
		const Parameter parameter = parseParams(argc, argv);
		switch(parameter.mode){
			case Parameter::Mode::INVALID:{
				context = std::make_unique<InvalidDevoured>(std::move(env.value()));
				break;
			}
			case Parameter::Mode::DAEMON:{
				context = std::make_unique<DaemonDevoured>(std::move(env.value()));
				break;
			}
			case Parameter::Mode::CREATE:{
				context = std::make_unique<SpawnDevoured>(std::move(env.value()));
				break;
			}
			case Parameter::Mode::STATUS: {
				context = std::make_unique<StatusDevoured>(std::move(env.value()), parameter);
				break;
			}
			default:{
				std::cerr<<"Unimplemented case"<<std::endl;
				context = std::make_unique<InvalidDevoured>(std::move(env.value()));
				break;
			}
		}
		return context;
	}
}
