#pragma once
#include "winsock2.h"
#include <stdio.h>
void handleHttpRequest(SOCKET client_socket);
string parseHttpRequest(const string& request);
string createHttpResponse(const string& file_path);
