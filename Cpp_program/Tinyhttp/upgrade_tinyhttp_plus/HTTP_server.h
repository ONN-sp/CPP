#include <fstream>
#include <arpa/inet.h>

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

typedef struct socketinfo
{
    int fd;
    int epfd;
}SocketInfo;

class HTTP_server{
    public:
        HTTP_server();
        void acceptConn(SocketInfo*, sockaddr_in, int);
        void headers(int, const char*);
        void not_found(int);
        void bad_request(int);
        void cannot_execute(int);
        void error_die(const char*);
        void cat(int, std::ifstream&);
        void execute_cgi(SocketInfo*, const char*, const char*, const char*);
        void server_file(SocketInfo*, const char*);
        int get_line(SocketInfo*, char*, const int);
        void unimplemented(int);
        void accept_request(SocketInfo*);
        int startup(short*);
        ~HTTP_server();
};

#endif