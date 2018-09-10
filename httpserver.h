#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__
#ifdef __cplusplus
extern "C" {
#endif
    #include "libevhtp/evhtp.h"
#ifdef __cplusplus
}
#endif

#include <string>
#include <functional>
#include<map>
#include<list>

class HttpContex
{
public:
    HttpContex(evhtp_request_t * req);
    ~HttpContex();

    void String(const std::string& resp, int respCode);
    void Json(const std::string& resp, int respCode);
    std::string Query(const std::string & param);
private:    
    evhtp_request_t * req_;
};

class Router
{
public:
    using call_back = std::function<void(HttpContex &ctx)>;
    Router() {};
    ~Router(){};
    void AddRouter(const std::string& method, const std::string& path, call_back);
    bool FindPath(const std::string& path);
    void ProcessReq(const std::string& method, const std::string& path, HttpContex& ctx);
    struct RouterNode
    {
        //evhtp_callback_t* evcb;
        std::string path_;
        std::map<std::string, call_back> callbackmap_;
    };

public:
    std::list<RouterNode> pathnodes_;
    //std::map<std::string, call_back> callbackmap_;
};

class HttpServer
{
public:

    HttpServer();
    ~HttpServer();

    //void AddRouter(const string & router, evhtp_callback_cb cb, void *arg);
    void AddRouter(const std::string &method, const std::string & path, std::function<void(HttpContex &ctx)> cb);
    
    void Run(const std::string & ip, int port, int backlog);

private:
    evbase_t *evbase;
    evhtp_t *htp;
    Router router_;
};

#endif