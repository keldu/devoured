#include <memory>
#include <iostream>

#include "ProcessStream.h"

/*
	Even though I know that copy on write is implemented for linux when executing fork
	I still want to stay as low as possible in the stack. I will maintain a little bit
	C style language while handling fork requests.
*/

int main(int argc, char** argv){

	std::unique_ptr<dvr::ProcessStream> process;
	int rv = dvr::createProcessStream(process);

	return 0;
}
