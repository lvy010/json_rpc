#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"
#include "requestor.hpp" 

namespace JsonRpc
{
    namespace Client
    {
        class TopicManager
        {
        public:
            using SubscribeCallback = std::function<void(const std::string& key, const std::string& msg)>;
            using s_ptr = std::shared_ptr<TopicManager>;

            TopicManager(Requestor::s_ptr requestor)
                : _requestor(requestor)
            {}

            // 主题创建
            bool createTopic(const BaseConnection::s_ptr& conn, const std::string& key)
            {
                return commonRequest(conn, key, TopicOpType::TOPIC_CREATE);
            }

            // 主题删除
            bool removeTopic(const BaseConnection::s_ptr& conn, const std::string& key)
            {
                return commonRequest(conn, key, TopicOpType::TOPIC_REMOVE);
            }

            // 主题订阅
            bool subscribeTopic(const BaseConnection::s_ptr& conn, const std::string& key, const SubscribeCallback& cb)
            {
                addSubscribe(key, cb); // 先设置回调函数，防止响应报文携带了要处理的数据

                bool ret = commonRequest(conn, key, TopicOpType::TOPIC_SUBSCRIBE);
                if (!ret)
                    delSubscribe(key);

                return ret;
            }

            // 取消订阅
            bool cancelTopic(const BaseConnection::s_ptr& conn, const std::string& key)
            {
                delSubscribe(key);
                return commonRequest(conn, key, TopicOpType::TOPIC_CANCEL);
            } 

            // 主题消息发布
            bool publishTopic(const BaseConnection::s_ptr& conn, const std::string& key, const std::string& msg)
            {
                return commonRequest(conn, key, TopicOpType::TOPIC_PUBLISH, msg);
            }

            // 收到推送消息处理
            void onPublish(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                auto op = msg->topicOpType();
                if (op != TopicOpType::TOPIC_PUBLISH)
                {
                    E_LOG("收到无法处理的消息类型!");
                    return;
                }

                auto cb = getSubscribe(msg->topicKey());
                if (!cb)
                {
                    E_LOG("无法处理主题 %s!", msg->topicKey().c_str());
                    return;
                }

                return cb(msg->topicKey(), msg->topicMsg());
            }
            
        private:
            bool commonRequest(const BaseConnection::s_ptr& conn, const std::string& key, TopicOpType op, const std::string& msg = "")
            {
                I_LOG("发送 %d 类型主题请求", (int)op);
                // 构造请求对象
                auto msg_req = MessageFactory::create<TopicRequest>();
                msg_req->setMtype(MType::REQ_TOPIC);
                msg_req->setRid(UUID::uuid());
                msg_req->setTopicKey(key);
                msg_req->setTopicOpType(op);
                if (op == TopicOpType::TOPIC_PUBLISH)
                    msg_req->setTopicMsg(msg);
                
                // 发送请求
                BaseMessage::s_ptr rsp;
                if (!_requestor->send(conn, msg_req, rsp))
                {
                    E_LOG("%s 主题操作失败!", key.c_str());
                    return false;
                }

                // 等待响应
                auto top_rsp = std::dynamic_pointer_cast<TopicResponse>(rsp);
                if (!top_rsp)       
                {
                    E_LOG("%s 主题响应向下转型失败!", key.c_str());
                    return false;
                }

                if (top_rsp->rcode() != RetCode::RCODE_OK)       
                {
                    E_LOG("%s 主题请求出错: %s!", key.c_str(), errReason(top_rsp->rcode()).c_str());
                    return false;
                }

                return true;
            }

            void addSubscribe(const std::string& key, const SubscribeCallback& cb)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _topic_cbs[key] = cb;
            }

            void delSubscribe(const std::string& key)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _topic_cbs.erase(key);
            }

            const SubscribeCallback getSubscribe(const std::string& key)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                return _topic_cbs.count(key) ? _topic_cbs[key] : SubscribeCallback();
            }

        private:
            std::mutex _mtx;
            std::unordered_map<std::string, SubscribeCallback> _topic_cbs;
            Requestor::s_ptr _requestor;
        };
    }
}
