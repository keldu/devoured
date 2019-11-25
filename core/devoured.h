#pragma once

#include <memory>
#include <set>

#include "network.h"
#include "config/config.h"

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


	std::unique_ptr<Devoured> createContext(int argc, char** argv);
}
