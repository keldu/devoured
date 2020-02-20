#include "async.h"

#if !_WIN32

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <unistd.h>
#include <cassert>

#include <iostream>
#include <system_error>
#include <functional>
#include <list>
			
#define EPOLL_EVENTS_PER_POLL 256

namespace dvr {
	class UnixAutoCloseFd {
	private:
		int fd_;
	public:
		UnixAutoCloseFd(int fd):
			fd_{fd}{}
		UnixAutoCloseFd():fd_{-1}{}
		UnixAutoCloseFd(const UnixAutoCloseFd&) = delete;
		UnixAutoCloseFd(UnixAutoCloseFd&& rhs):fd_{rhs.fd_}{rhs.fd_ = -1;}

		UnixAutoCloseFd& operator=(const UnixAutoCloseFd&) = delete;
		UnixAutoCloseFd& operator=(UnixAutoCloseFd&& rhs){
			fd_ = rhs.fd_;
			rhs.fd_ = -1;
			return *this;
		}
		~UnixAutoCloseFd(){
			if(::close(fd_) < 0 ){
				std::cerr<<"Closing of socket failed"<<std::endl;
			}
		}

		int fd() const{
			return fd_;
		}
	};

	class UnixEventPoll final : public EventPoll {
		friend class FdObserver;
	public:
		class FdObserver {
		public:
			enum Flags {
				OBSERVE_READ = 1<<0,
				OBSERVE_WRITE = 1<<1,
				OBSERVE_READWRITE = OBSERVE_READ | OBSERVE_WRITE
			};
		private:
			UnixEventPoll& poll;
			const int fd_;
			const Flags flags_;
			std::function<void(uint32_t)> callback;
		public:
			FdObserver(UnixEventPoll& p, int fd, Flags flags):
				poll{p},
				fd_{fd},
				flags_{flags}
			{
				struct epoll_event event;
				memset(&event, 0, sizeof(event));
				if( flags & OBSERVE_READ ){
					event.events |= EPOLLIN;
				}
				if( flags & OBSERVE_WRITE ){
					event.events |= EPOLLOUT;
				}
				event.events |= EPOLLET;
				event.data.ptr = this;

				::epoll_ctl(poll.epoll_fd.fd(), EPOLL_CTL_ADD, fd, &event);
				++(poll.registered_fds);
			}
			virtual ~FdObserver(){
				epoll_ctl(poll.epoll_fd.fd(), EPOLL_CTL_DEL, fd(), nullptr);
				--(poll.registered_fds);
			}
			FdObserver(const FdObserver&) = delete;
			FdObserver& operator=(const FdObserver&) = delete;

			int fd() const {
				return fd_;
			}

			Flags flags() const {
				return flags_;
			}

			virtual void notify(uint32_t mask);
		};
	private:
		UnixAutoCloseFd epoll_fd;
		size_t registered_fds;

		void epollWait(int timeout){
			struct epoll_event events[EPOLL_EVENTS_PER_POLL];
			int n = epoll_wait(epoll_fd.fd(), events, sizeof(events), timeout);
			if ( n < 0 ){
				// TODO Error handling with errno or sth else like std::system_error
				return;
			}
			for( int i = 0; i < n; ++i ){
				FdObserver* observer = reinterpret_cast<FdObserver*>(events[i].data.ptr);
				observer->notify(events[i].events);
			}
		}
	public:
		UnixEventPoll():
			epoll_fd{-1},
			registered_fds{0}
		{
			{
				int fd = epoll_create1(EPOLL_CLOEXEC);
				epoll_fd = fd;
			}
		}
		~UnixEventPoll(){
			assert(registered_fds == 0);
		}

		void wait() override {
			epollWait(-1);
		}

		void poll() override {
			epollWait(0);
		}
	};

	class UnixAsyncIoProvider final : public AsyncIoProvider {
	private:
		UnixEventPoll event_poll;
	public:
		EventPoll& eventPoll() override {
			return event_poll;
		}

		void parseAddress(ErrorOr<std::unique_ptr<AsyncIoStream>> async, const std::string& addr, int32_t port_hint = 0) override {
		}
	};

	AsyncIoContext setupAsyncIo(){
		std::unique_ptr<UnixAsyncIoProvider> provider = std::make_unique<UnixAsyncIoProvider>();
		EventPoll& poll = provider->eventPoll();
		return {std::move(provider), poll};
	}
}
#endif
