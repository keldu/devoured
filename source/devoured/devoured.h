#pragma once

#include <memory>
#include <set>

#include "network/async.h"
#include "config/config.h"

namespace dvr {
	class Devoured {
	private:
		bool active;
		int status;
	protected:
		AsyncIoContext io_context;
		
		void stop();
		void setStatus(int state);
		bool isActive()const;
		int getStatus()const;

		virtual void loop() = 0;
	public:
		enum class Mode: uint8_t {
			INVALID,
			DAEMON,
			STATUS,
			MANAGE
		};
		Devoured(bool act, int sta);
		virtual ~Devoured() = default;

		int run();
	};
	std::unique_ptr<Devoured> createContext(int argc, char** argv);
}
