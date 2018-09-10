#include "httpserver.h"
#include <iostream>
 
int main(int argc, char **argv)
{
    HttpServer server;
    
    std::function<void(HttpContex &ctx)> f = [](HttpContex &ctx){   std::cout<<"test function"; ctx.Json(ctx.Query("id"), 200); };
    std::function<void(HttpContex &ctx)> f1 = [](HttpContex &ctx){   std::cout<<"something function"; ctx.Json("something ok", 200); };

    std::cout<<"function "<<&f<<std::endl;
    server.AddRouter("GET", "/v1/api/camera/*", f);
     server.AddRouter("GET", "/v1/api/something", f1);

    server.Run("192.168.117.129",8080, 1024);
    return 0;
}
