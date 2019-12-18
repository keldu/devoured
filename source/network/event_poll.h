#pragma once

#include <memory>

namespace dvr {
	class IFdObserver;
	class EventPoll {
	private:
		// PImpl Pattern, because of platform specific implementations
		class Impl;
		std::unique_ptr<Impl> impl;
	public:
		EventPoll();
		~EventPoll();

		bool poll();

		void subscribe(IFdObserver& obsv);
		void unsubscribe(IFdObserver& obsv);
	};
}
