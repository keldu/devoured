#include "fd_observer.h"

#include "event_poll.h"

namespace dvr {
	IFdObserver::IFdObserver(EventPoll& p, int file_d, uint32_t msk):
		poll{p},
		file_desc{file_d},
		event_mask{msk}
	{
		poll.subscribe(*this);
	}

	IFdObserver::~IFdObserver(){
		poll.unsubscribe(*this);
	}

	int IFdObserver::fd()const{
		return file_desc;
	}

	uint32_t IFdObserver::mask()const{
		return event_mask;
	}
}
