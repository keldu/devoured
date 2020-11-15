#include <kelgin/io.h>

int main(int argc, char** argv){
	auto aio = gin::setupAsyncIo();

	bool running = true;

	auto terminate_signal = aio.event_port.onSignal(gin::Signal::Terminate).then([&running](){
		running = false;
	}).sink([](const gin::Error& error){return error;});

	while(running){
		aio.wait_scope.wait(std::chrono::seconds{1});
	}
	return 0;
}
