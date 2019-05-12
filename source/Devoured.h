#pragma once

#include <memory>
#include <set>

#include "UnixSocket.h"

namespace dvr {
	class Devoured {
	public:
		Devoured(bool act, int sta);
		virtual ~Devoured() = default;

		int run();
	private:

		bool active;
		int status;
	protected:
		void stop();
		void setStatus(int state);
		bool isActive()const;
		int getStatus()const;

		virtual void loop() = 0;
	};

	class InvalidDevoured final : public Devoured {
	public:
		InvalidDevoured();
	protected:
		void loop();
	};

	class ServiceDevoured final : public Devoured {
	public:
		ServiceDevoured(const std::string& f, const std::string& t);
	protected:
		void loop();

		std::string config_path;
		std::string target;
	private:
		struct Config{
			std::string control_path;
		};

		Config readConf();
		void setupControl(Config& conf);

		std::unique_ptr<StreamAcceptor> control_acceptor;
		std::set<Stream> control_streams;
	};

	std::unique_ptr<Devoured> createContext(int argc, char** argv);
}