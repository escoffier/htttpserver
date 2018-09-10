#include "httpserver.h"
#include <ctype.h>
#include <iostream>

//#ifdef __cplusplus
extern "C"
{
//#endif
#include "cjson/cJSON.h"
}

static const char *method_strmap[] = {
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "MKCOL",
    "COPY",
    "MOVE",
    "OPTIONS",
    "PROPFIND",
    "PROPATCH",
    "LOCK",
    "UNLOCK",
    "TRACE",
    "CONNECT",
    "PATCH",
    "UNKNOWN",
};

HttpServer::HttpServer() : evbase(nullptr), htp(nullptr)
{
    evbase = event_base_new();
    htp = evhtp_new(evbase, NULL);
}

HttpServer::~HttpServer()
{
    evhtp_unbind_socket(htp);
    event_base_free(evbase);
}

void HttpServer::Run(const std::string &ip, int port, int backlog)
{

#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, init_thread, settings.num_threads, NULL);
#endif
    //evhtp_set_max_keepalive_requests(htp, settings.max_keepalives);
    std::cout << "Start server " << ip << ":" << port << std::endl;
    int ret = evhtp_bind_socket(htp, ip.c_str(), port, backlog);
    std::cout << "ret " << ret << std::endl;
    event_base_loop(evbase, 0);
}

static void ev_callback(evhtp_request_t *req, void *arg)
{
    std::cout << "ev_callback " << arg << std::endl;
    int req_method = evhtp_request_get_method(req);
    if (req_method >= 16)
        req_method = 16;

    std::cout << "Method: " << req_method << "---" << method_strmap[req_method] << std::endl;
    Router *router = (Router *)arg;

    std::string path = req->uri->path->full;
    auto const pos=path.find_last_of('/');
    auto leaf=path.substr(pos+1);
    std::cout << "Path: " << path << std::endl;
    HttpContex evctx(req);
    router->ProcessReq(method_strmap[req_method], path, evctx);
    // for(auto p : router->pathnodes_)
    // {
    //     auto search = p.callbackmap_.find(method_strmap[req_method]);
    //     if(search != p.callbackmap_.end() )
    //     {
    //         std::cout<<"call function"<<std::endl;
    //         HttpContex evctx(req);
    //         (search->second)(evctx);
    //         break;
    //     }

    // }
    //if(strcmp(method_strmap[req_method], method) == 0)
    //{

    //}
    //std::function<void(HttpContex &ctx)> func = *(std::function<void(HttpContex &ctx)>*)arg;
    //HttpContex evctx(req);
    //func(evctx);
}

//void HttpServer::AddRouter(const string & router, evhtp_callback_cb cb, void *arg)
void HttpServer::AddRouter(const std::string &method, const std::string &path, std::function<void(HttpContex &ctx)> cb)
{

    std::cout << " HttpServer AddRouter " << &cb << std::endl;
    if (!router_.FindPath(path))
    {
        std::cout << " HttpServer AddRouter set cb" << std::endl;
        //evhtp_set_glob_cb(htp, path.c_str(), ev_callback, (void *)&router_);
        evhtp_set_cb(htp, path.c_str(), ev_callback, (void *)&router_);
    }

    router_.AddRouter(method, path, cb);
}

HttpContex::HttpContex(evhtp_request_t *req) : req_(req)
{
}

HttpContex::~HttpContex()
{
}

void HttpContex::Json(const std::string &resp, int respCode)
{
    cJSON *j_ret = cJSON_CreateObject();
    cJSON_AddStringToObject(j_ret, "message", resp.c_str());

    char *ret_str_unformat = cJSON_PrintUnformatted(j_ret);
    //LOG_PRINT(LOG_DEBUG, "ret_str_unformat: %s", ret_str_unformat);
    evbuffer_add_printf(req_->buffer_out, "%s", ret_str_unformat);
    evhtp_headers_add_header(req_->headers_out, evhtp_header_new("Content-Type", "application/json", 0, 0));
    evhtp_headers_add_header(req_->headers_out, evhtp_header_new("Access-Control-Allow-Origin", "*", 0, 0));
    cJSON_Delete(j_ret);
    free(ret_str_unformat);

    //evhtp_headers_add_header(req_->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_send_reply(req_, EVHTP_RES_OK);
}

void Router::AddRouter(const std::string &method, const std::string &path, call_back cb)
{
    if (pathnodes_.empty())
    {
        std::cout << " empty Router AddRouter new path: " << path << std::endl;
        std::map<std::string, call_back> cbs;
        cbs.emplace(method, cb);
        RouterNode node{path, cbs};
        pathnodes_.push_back(node);
    }
    else
    {
        for (auto c : pathnodes_)
        {
            if (c.path_ == path)
            {
                std::cout << " Router AddRouter path: " << c.path_ << std::endl;
                c.callbackmap_.emplace(method, cb);
                break;
            }
            else
            {
                std::cout << " Router AddRouter new path: " << path << std::endl;
                std::map<std::string, call_back> cbs;
                cbs.emplace(method, cb);
                RouterNode node{path, cbs};
                pathnodes_.push_back(node);
                break;
            }
        }
    }
}

bool Router::FindPath(const std::string &path)
{
    for (auto c : pathnodes_)
    {
        if (c.path_ == path)
        {
            return true;
        }
    }

    return false;
}

void Router::ProcessReq(const std::string &method, const std::string &path, HttpContex &ctx)
{
    if (pathnodes_.empty())
    {
        ctx.Json("Invalid request", 401);
        return;
    }

    for (auto p : pathnodes_)
    {
        if (p.path_ == path)
        {
            auto search = p.callbackmap_.find(method);
            if (search != p.callbackmap_.end())
            {
                std::cout << "call function: " << path << std::endl;

                (search->second)(ctx);
                break;
            }
            ctx.Json("Invalid request", 401);
            break;
        }
    }
}

std::string HttpContex::Query(const std::string &param)
{
    evhtp_kvs_t *params;
    params = req_->uri->query;
    std::string value;
    if (params == NULL)
    {
        std::cout << " Requset Get Info Failed! No params" << std::endl;
        return value;
    }

    const char *strparam;
    strparam = evhtp_kv_find(params, param.c_str());
    if (strparam == NULL)
    {
        std::cout << " id() = NULL return" << std::endl;
        return value;
    }

    value = strparam;
    return value;
}
