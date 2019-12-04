#pragma once

bool shutdown_requested();
void signal_handler(int sig);
void register_signal_handlers();
