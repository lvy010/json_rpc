#include <iostream>
#include <thread>

#include "../../common/util.hpp"
#include "../../client/rpc_client.hpp"

using namespace JsonRpc;

void callback(const Json::Value& result)
{
    std::cout << "call result: " << result.asInt() << std::endl;
}

int main()
{
    Client::RpcClient client(false, "127.0.0.1", 6666);

    {
        Json::Value params, result;
        params["num1"] = 11;
        params["num2"] = 22;
        bool ret = client.call("Add", params, result);
        if (ret)
            std::cout << "result: " << result.asInt() << std::endl;
    }

    {
        Json::Value params, result;
        params["num1"] = 33;
        params["num2"] = 44;

        Client::RpcCaller::JsonAsyncResponse res_future;
        bool ret = client.call("Add", params, res_future);
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
        bool ret = client.call("Add", params, callback);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}