#include "event_poll.h"

#include <cassert>
#include <map>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>

#include "fd_observer.h"

namespace dvr {
	const size_t max_events = 256;

	class EventPoll::Impl {
	private:
		std::map<int, IFdObserver*> observers;

		int epoll_fd;
		bool broken;

		::epoll_event events[max_events];
	public:
		Impl():
			broken{false}
		{
			epoll_fd = epoll_create1(0);
			if(epoll_fd < 0){
				broken = true;
			}
		}

		~Impl(){
			if(epoll_fd > 0){
				close(epoll_fd);
			}
		}

		bool poll(){
			if(broken){
				return true;
			}
			int nfds = ::epoll_wait(epoll_fd, events, max_events, -1);
			if(nfds < 0){
				return broken = true;
			}

			for(int n = 0; n < nfds; ++n){
				int fd_event = events[n].data.fd;
				auto finder = observers.find(fd_event);
				if(finder == observers.end()){
					return broken = true;
				}
				finder->second->notify(events[n].events);
			}

			return broken;
		}

		void subscribe(IFdObserver& obsv){
			if(!broken){
				int fd = obsv.fd();
				assert(fd >= 0);
				::epoll_event event;
				event.events = obsv.mask();
				event.data.fd = fd;
				
				if(::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)){
					broken = true;
					return;
				}

				observers.insert(std::make_pair(fd, &obsv));
			}
		}

		void unsubscribe(IFdObserver& obsv){
			if(!broken){
				int fd = obsv.fd();
				assert(fd >= 0);
				::epoll_event event;
				event.events = obsv.mask();
				event.data.fd = fd;

				if(::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event)){
					broken = true;
					return;
				}

				observers.erase(fd);
			}
		}
	};

	EventPoll::EventPoll():
		impl{std::make_unique<EventPoll::Impl>()}
	{
	}
	EventPoll::~EventPoll(){}

	bool EventPoll::poll(){
		return impl->poll();
	}

	void EventPoll::subscribe(IFdObserver& obv){
		impl->subscribe(obv);
	}

	void EventPoll::unsubscribe(IFdObserver& obv){
		impl->unsubscribe(obv);
	}

}
