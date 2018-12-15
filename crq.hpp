#pragma once


#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <unordered_map>

#include <cstring>

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>


namespace crq
{

    class Response
    {
    public:

        int           status_code;
        ::std::string reason;
        ::std::string body;


        Response() : status_code(400) {}

        Response(::std::string &data)
        {
            ::std::stringstream ss(data);
            ss >> reason;
            ss >> status_code;
            ss >> reason;

            auto pos = data.find("<html>");
            body = pos == data.npos ? data.substr(data.find("\r\n\r\n") + (::std::size_t)4) : data.substr(pos);
        }

        Response(Response &&res) noexcept = default;
        ~Response() = default;


        Response& operator=(Response &rhs)
        {
            status_code = rhs.status_code;
            reason      = rhs.reason;
            body        = rhs.body;
            
            return *this;
        }

        Response& operator=(Response &&rhs) noexcept
        {
            ::std::swap(status_code, rhs.status_code);
            ::std::swap(reason,      rhs.reason);
            ::std::swap(body,        rhs.body);

            return *this;
        }
    
    };


    class Request
    {
    private:

        static ::std::string gen_host(::std::string url)
        {
            auto pos = url.find("/");
            return pos == url.npos ? url : url.substr(0, pos);
        }

        static ::std::string gen_req(::std::string url)
        {
            auto pos = url.find('/');
            return pos == url.npos ? "/" : url.substr(pos);
        }

        static Response request(const ::std::string url, const ::std::string method)
        {
            ::std::string response_msg;


            // init server_addr
            addrinfo hints, *result;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            getaddrinfo(gen_host(url).c_str(), "80", &hints, &result);


            // connect to server
            auto socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            for (auto addr = result; addr != NULL; addr = addr->ai_next)
            {
                if (connect(socket_fd, addr->ai_addr, addr->ai_addrlen) == 0)
                {
                    break;
                }
            }


            // send request message to server
            ::std::string request_msg = method + gen_req(url) + " HTTP/1.1\r\n"
                "Host: " + gen_host(url) + "\r\n"
                "Content-type: text/html\r\n"
                "Connection: close\r\n"
                "Accept-Language: zh-CN,zh,en-US\r\n"
                "User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows NT)\r\n\r\n";
            send(socket_fd, request_msg.c_str(), request_msg.length(), 0);


            // receive response message from server
            char tmp[4096] = { 0 };

            while (recv(socket_fd, tmp, 4096, 0) > 0)
            {
                response_msg += tmp;
                memset(tmp, 0, sizeof(tmp));
            }

            close(socket_fd);
            freeaddrinfo(result);

            return Response(response_msg);
        }

    public:

        static Response get(::std::string url)
        {
            return request(url, "GET ");
        }
        
    };

}