#include "async.h"

#include <cassert>

namespace dvr {
	/*
	 * This is okay since the enter/leave scope functionality is introduced
	 * through WaitScope. I had this in another internal project and it
	 * was incredibly annoying to work with that functionality directly
	 * which meant i couldn't create eventLoops and send them off to other
	 * threads. It was annoying to work around that.
	 */
	thread_local EventLoop* thread_local_event_loop = nullptr;

	void EventLoop::enterScope(){
		assert(!thread_local_event_loop);
		thread_local_event_loop = this;
	}

	void EventLoop::leaveScope(){
		assert(thread_local_event_loop);
		thread_local_event_loop = nullptr;
	}

	EventLoop::EventLoop():
		ev_poll{nullptr},
		running{false}
	{}
	
	EventLoop::EventLoop(EventPoll& p):
		ev_poll{&p},
		running{false}
	{}

	void EventLoop::turn(){

	}

	void EventLoop::poll(){
		
	}

	bool EventLoop::isRunnable() const {
		return running;
	}

	WaitScope::WaitScope(EventLoop& l):
		loop{l}
	{
		loop.enterScope();
	}

	WaitScope::~WaitScope(){
		loop.leaveScope();
	}

	void WaitScope::poll(){
		assert(thread_local_event_loop == &loop);
		assert(!loop.running);

		while(true){
			if(!loop.turn()){
				loop.poll();

				if(!loop.isRunnable()){
					return;
				}
			}
		}
	}

	void WaitScope::wait(){

	}
}
