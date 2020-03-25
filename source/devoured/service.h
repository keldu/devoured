#pragma once

#include <map>
#include <string>
#include <memory>

#include "process_stream.h"
#include "network/network.h"
#include "arguments/parameter.h"

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace dvr {
	class Service : public IStreamStateObserver{
	private:
		enum class State : uint8_t {
			ON,
			OFF,
			BROKEN
		};
		const ServiceConfig config;
		AsyncIoProvider& provider;

		std::unique_ptr<ProcessStream> process;
		State state;

		std::deque<std::string> output_buffer;
	public:
		Service(const ServiceConfig& config, AsyncIoProvider& provider);
		Service(Service&&);
		Service& operator=(Service&&);

		Service(const Service&) = delete;
		Service& operator=(const Service&) = delete;

		void start();
		void stop();

		void notify(IoStream& conn, IoStreamState state) override {
			std::cout<<"Program doing sth"<<std::endl;
		}
	};
}
