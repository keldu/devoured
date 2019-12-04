#include "signal_handler.h"

#include <signal.h>

volatile sig_atomic_t shutdown_status = 0;

bool shutdown_requested(){
	return shutdown_status != 0;
}

void signal_handler(int sig){
	(void)sig;
	shutdown_status = 1;
}

void register_signal_handlers(){
	::signal(SIGINT, signal_handler);
}
