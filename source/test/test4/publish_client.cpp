#include "../../client/rpc_client.hpp"

int main()
{
    // 实例化客户端
    auto client = std::make_shared<JsonRpc::Client::TopicClient>("127.0.0.1", 6666);

    bool ret = client->createTopic("hello");
    if (ret == false)
        E_LOG("创建主题失败!");

    // 向主题发布消息
    for (int i = 0; i < 10; i++)
        client->publishTopic("hello", "hellowold!" + std::to_string(i));

    client->shutDown();
    return 0;
}