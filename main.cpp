#include <memory>
#include <iostream>

#include "core/process_stream.h"
#include "core/devoured.h"

/*
	Even though I know that copy on write is implemented for linux when executing fork
	I still want to stay as low as possible in the stack. I will maintain a little bit
	C style language while handling fork requests.
*/

int main(int argc, char** argv){

	//createContext always creates a valid pointer
	std::unique_ptr<dvr::Devoured> devoured = dvr::createContext(argc,argv);
	int status_code = devoured->run();

	return status_code;
}
