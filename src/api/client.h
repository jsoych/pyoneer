#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"

void client_handle_fd(Server* server, int client_fd, int listener_index);

#endif
