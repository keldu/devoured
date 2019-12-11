#include "signal_handler.h"

#include <signal.h>

volatile sig_atomic_t shutdown_status = 0;

bool shutdown_requested(){
	return shutdown_status != 0;
}

void signal_handler(int sig){
	switch(sig){
		case SIGINT: shutdown_status = 1; break;
		case SIGPIPE: break;
		case SIGCHLD: break;
	}
}

void signal_ignore_handler(){
}

void register_signal_handlers(){
	::signal(SIGINT, signal_handler);
	::signal(SIGPIPE, signal_handler);
	::signal(SIGCHLD, signal_handler);
}
