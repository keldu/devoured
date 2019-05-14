#pragma once

#include <signal.h>

volatile sig_atomic_t shutdown_requested = 0;

static void signal_handler(int sig){
	(void)sig;
	shutdown_requested = 1;
}