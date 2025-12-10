#include <iostream>

#include "../message.hpp"

int main()
{
    // 直接返回 shared_ptr
    JsonRpc::RpcRequset::s_ptr rrp = JsonRpc::MessageFactory::create<JsonRpc::RpcRequset>();

    // 返回父类指针，指向子类
    // JsonRpc::BaseMessage::s_ptr brp = JsonRpc::MessageFactory::create(JsonRpc::MType::REQ_RPC);
    // JsonRpc::RpcRequset::s_ptr rrp_2 = std::dynamic_pointer_cast<JsonRpc::RpcRequset>(brp);

    Json::Value param;
    param["num1"] = 11;
    param["num2"] = 22;
    rrp->setMethod("Add");
    rrp->setParams(param);

    std::string str = rrp->serialize();
    std::cout << str << std::endl;

    JsonRpc::BaseMessage::s_ptr bmp = JsonRpc::MessageFactory::create(JsonRpc::MType::REQ_RPC);
    JsonRpc::RpcRequset::s_ptr rrp_2 = std::dynamic_pointer_cast<JsonRpc::RpcRequset>(bmp);
    bool ret = rrp_2->unSerialize(str);
    if (ret && rrp_2->check())
    {
        std::cout << rrp_2->method() << std::endl;
        std::cout << rrp_2->params()["num1"].asInt() << std::endl;
        std::cout << rrp_2->params()["num2"].asInt() << std::endl;
    }

    std::cout << "-------------------" << std::endl;

    JsonRpc::TopicRequset::s_ptr trp = JsonRpc::MessageFactory::create<JsonRpc::TopicRequset>();
    trp->setTopicOpType(JsonRpc::TopicOpType::TOPIC_PUBLISH);
    trp->setTopicKey("news");
    trp->setTopicMsg("hello world!");

    std::string str2 = trp->serialize();
    std::cout << str2 << std::endl;

    JsonRpc::BaseMessage::s_ptr bmp2 = JsonRpc::MessageFactory::create(JsonRpc::MType::REQ_TOPIC);
    JsonRpc::TopicRequset::s_ptr trp_2 = std::dynamic_pointer_cast<JsonRpc::TopicRequset>(bmp2);
    bool ret2 = trp_2->unSerialize(str2);
    if (ret2 && trp_2->check())
    {
        std::cout << (int)trp_2->topicOpType() << std::endl;
        std::cout << trp_2->topicKey() << std::endl;
        std::cout << trp_2->topicMsg() << std::endl;
    }

    return 0;
}