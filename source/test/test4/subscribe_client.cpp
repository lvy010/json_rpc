#include "../../client/rpc_client.hpp"

#include <thread>

void callback(const std::string& key, const std::string& msg)
{
    I_LOG("%s 收到推送消息: %s", key.c_str(), msg.c_str());
}

int main()
{
    // 实例化客户端
    auto client = std::make_shared<JsonRpc::Client::TopicClient>("127.0.0.1", 6666);

    // 创建主题
    bool ret = client->createTopic("hello");
    if (ret == false)
        E_LOG("创建主题失败!");

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 订阅主题
    ret = client->subscribeTopic("hello", callback);
    if (ret == false)
        E_LOG("主题订阅失败!");

    // 等待
    std::this_thread::sleep_for(std::chrono::seconds(10));

    client->shutDown();

    return 0;
}