#include <fstream>
#include <arpa/inet.h>

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

class HTTP_server{
    public:
        HTTP_server();
        void headers(int, const char*);
        void not_found(int);
        void bad_request(int);
        void cannot_execute(int);
        void error_die(const char*);
        void cat(int, std::ifstream&);
        void execute_cgi(int, int, const char*, const char*, const char*);
        void server_file(int, int, const char*);
        int get_line(int, int, char*, const int);
        void unimplemented(int);
        void accept_request(int, int);
        int startup(short*);
        ~HTTP_server();
};

#endif