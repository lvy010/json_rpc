#include <iostream>
#include <thread>

#include "../../common/util.hpp"
#include "../../server/rpc_server.hpp"

using namespace JsonRpc;

void Add(Json::Value& req, Json::Value& rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main()
{
    // 构造并注册add方法
    auto desc_build = std::make_shared<Server::ServiceDescriberBuilder>();
    desc_build->setName("Add");
    desc_build->setParamsDesc("num1", Server::VType::INTERGAL);
    desc_build->setParamsDesc("num2", Server::VType::INTERGAL);
    desc_build->setReturnType(Server::VType::INTERGAL);
    desc_build->setCallback(Add);

    Server::RpcServer server({"127.0.0.1", 6666}, true, {"127.0.0.1", 7777});
    server.registerMethod(desc_build->build());
    server.start();
    return 0;
}