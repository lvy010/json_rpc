#include <iostream>
#include <thread>

#include "../../common/dispatcher.hpp"

#include "../../client/rpc_caller.hpp"
#include "../../client/requestor.hpp"

using namespace JsonRpc;

void callback(const Json::Value& result)
{
    std::cout << "call result: " << result.asInt() << std::endl;
}

int main()
{
    auto requestor = std::make_shared<Client::Requestor>();
    auto caller = std::make_shared<Client::RpcCaller>(requestor);

    auto dispatcher = std::make_shared<Dispatcher>();
    auto rsp_cb = std::bind(&Client::Requestor::onResponse, requestor.get(),
                             std::placeholders::_1, std::placeholders::_2);

    dispatcher->registerHandler<BaseMessage>(MType::RSP_RPC, rsp_cb);

    auto client = ClientFactory::create("127.0.0.1", 6666);
    auto message_cb = std::bind(&Dispatcher::onMessage, dispatcher.get(),
        std::placeholders::_1, std::placeholders::_2);

    client->setMessageCallback(message_cb);
    client->connect();

    auto conn = client->getConnection();

    {
        Json::Value params, result;
        params["num1"] = 11;
        params["num2"] = 22;
        bool ret = caller->call(conn, "Add", params, result);
        if (ret)
            std::cout << "result: " << result.asInt() << std::endl;
    }

    {
        Json::Value params, result;
        params["num1"] = 33;
        params["num2"] = 44;

        Client::RpcCaller::JsonAsyncResponse res_future;
        bool ret = caller->call(conn, "Add", params, res_future);
        if (ret)
        {
            result = res_future.get();
            std::cout << "result: " << result.asInt() << std::endl;
        }
    }

    {
        Json::Value params, result;
        params["num1"] = 55;
        params["num2"] = 66;
        bool ret = caller->call(conn, "Add", params, callback);
        if (ret)
            std::cout << "result: " << result.asInt() << std::endl;
    }

    client->shutdown();

    return 0;
}