#include "server.h"
#include <iostream>

int main() {
	Server server;
	if (!server.initialize()) {
		std::cerr << "Server initialization failed!" << std::endl;
		return -1;
	}
	server.run();
	return 0;
}
