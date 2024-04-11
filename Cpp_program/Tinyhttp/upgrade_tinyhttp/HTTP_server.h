#include <fstream>

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
        void execute_cgi(int, const char*, const char*, const char*);
        void server_file(int, const char*);
        int get_line(const int, char*, const int);
        void unimplemented(int);
        void accept_request(const int);
        int startup(short*);
        ~HTTP_server();
};

#endif