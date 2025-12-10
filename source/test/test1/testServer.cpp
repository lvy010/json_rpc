#include <iostream>
#include <thread>

#include "../../common/message.hpp"
#include "../../common/net.hpp"
#include "../../common/dispatcher.hpp"

#include "../../server/rpc_router.hpp"

using namespace JsonRpc;

void Add(Json::Value& req, Json::Value& rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main()
{
    auto dispatcher = std::make_shared<Dispatcher>();
    auto router = std::make_shared<Server::RpcRouter>();

    // 构造并注册add方法
    auto desc_build = std::make_shared<Server::ServiceDescriberBuilder>();
    desc_build->setName("Add");
    desc_build->setParamsDesc("num1", Server::VType::INTERGAL);
    desc_build->setParamsDesc("num2", Server::VType::INTERGAL);
    desc_build->setReturnType(Server::VType::INTERGAL);
    desc_build->setCallback(Add);
    router->registerMethod(desc_build->build());

    // 注册rpc回调到dispatcher
    auto rpc_cb = std::bind(&Server::RpcRouter::onRpcRequest, router.get(), std::placeholders::_1, std::placeholders::_2);
    dispatcher->registerHandler<RpcRequest>(MType::REQ_RPC, rpc_cb); // 收到rpc请求后，调用rpc_cb接口

    auto server = ServerFactory::create(6666);

    // 注册dispatcher到服务的MessageCallback
    auto message_cb = std::bind(&Dispatcher::onMessage, dispatcher.get(),
        std::placeholders::_1, std::placeholders::_2);

    server->setMessageCallback(message_cb);
    server->start();
    return 0;
}