#pragma once

#include <cstdint>

namespace dvr {
	class EventPoll;
	class IFdObserver {
	private:
		EventPoll& poll;
		const int file_desc;
		uint32_t event_mask;

		friend class EventPoll;
	public:
		IFdObserver(EventPoll& poll, int fd, uint32_t mask);
		virtual ~IFdObserver();

		virtual void notify(uint32_t mask) = 0;

		int fd() const;
		uint32_t mask() const;
	};
}
