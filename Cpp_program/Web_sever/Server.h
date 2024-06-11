#include "EventLoop.h"

#ifndef SERVER_H
#define SERVER_H

class Server{
    public:
        Server(EventLoop* loop, int thread_number, int port);
        ~Server();
        void startup(int* port);       
};

#endif
