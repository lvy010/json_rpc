#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace JsonRpc
{
    namespace Server
    {
        // 主题管理类
        class TopicManager
        {
        public:
            using s_ptr = std::shared_ptr<TopicManager>;

            TopicManager() = default;

            void onTopicRequest(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                TopicOpType op = msg->topicOpType();
                I_LOG("收到主题消息，类型 %d!", (int)op);

                bool ret = true;
                switch(op)
                {
                case TopicOpType::TOPIC_CREATE:    // 主题创建
                    topicCreate(conn, msg);
                    break;
                case TopicOpType::TOPIC_REMOVE:    // 主题删除
                    topicRemove(conn, msg);
                    break;
                case TopicOpType::TOPIC_SUBSCRIBE: // 主题订阅
                    ret = topicSubscribe(conn, msg);
                    break;
                case TopicOpType::TOPIC_CANCEL:    // 主题取消订阅
                    topicCancel(conn, msg);
                    break;
                case TopicOpType::TOPIC_PUBLISH:   // 主题消息发布
                    ret = topicPublish(conn, msg);
                    break;
                default:
                    E_LOG("不存在的主题类型: %d", (int)op);
                    return errorResponse(conn, msg, RetCode::RCODE_INVALID_OPTYPE);
                }

                return ret ? topicResponse(conn, msg) : errorResponse(conn, msg, RetCode::RCODE_NOT_FOUND_TOPIC);
            }

            void onShutdown(const BaseConnection::s_ptr& conn)
            {
                // 消息发布者退出: 不处理
                // 消息订阅者退出: 删除相关数据
                std::vector<Topic::s_ptr> topics;
                Subscriber::s_ptr sub;
                
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (_subscribes.count(conn) == 0)
                        return; // 消息发布者，直接退出

                    sub = _subscribes[conn];
                    for (auto& name : sub->topics())
                        topics.push_back(_topics[name]);

                    _subscribes.erase(conn);
                }

                for (auto& topic : topics)
                    topic->removeSubscriber(sub);
            }

        private:
            // 主题创建
            void topicCreate(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                auto topic = std::make_shared<Topic>(msg->topicKey());
                _topics.emplace(msg->topicKey(), topic);
            }

            // 主题删除
            void topicRemove(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                const std::string& topicName = msg->topicKey();
                std::unordered_set<Subscriber::s_ptr> subs;

                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (_topics.count(topicName) == 0)
                        return;

                    // 删获取该主题的所有订阅者
                    subs = _topics[topicName]->subscribers();

                    // 删除主题
                    _topics.erase(topicName);
                }

                for (auto& sub : subs) // 每个 sub 本身删除，不需要manager的mutex保护，减少锁的占用时间
                    sub->removeTopic(topicName); // 取消订阅
            }

            // 主题订阅
            bool topicSubscribe(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                const std::string& topicName = msg->topicKey();
                // 获取主题对象和订阅者对象
                Topic::s_ptr topic;
                Subscriber::s_ptr subscriber;

                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (_topics.count(topicName) == 0)
                        return false;

                    topic = _topics[topicName];

                    if (_subscribes.count(conn) == 0) // 第一次进行订阅
                        _subscribes.emplace(conn, std::make_shared<Subscriber>(conn));

                    subscriber = _subscribes[conn];
                }

                // 订阅添加主题， 主题添加订阅
                topic->appendSubscriber(subscriber);
                subscriber->appendTopic(topicName);
                return true;
            }

            // 取消订阅
            void topicCancel(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                const std::string& topicName = msg->topicKey();
                // 获取主题对象和订阅者对象
                Topic::s_ptr topic;
                Subscriber::s_ptr subscriber;

                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (_topics.count(topicName) == 0        // 没有主题
                            || _subscribes.count(conn) == 0) // 没有订阅
                        return;

                    topic = _topics[topicName];
                    subscriber = _subscribes[conn];
                }

                // 订阅删除主题， 主题删除订阅
                topic->removeSubscriber(subscriber);
                subscriber->removeTopic(topicName);
            }

            // 主题消息发布
            bool topicPublish(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                const std::string& topicName = msg->topicKey();
                Topic::s_ptr topic;
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (_topics.count(topicName) == 0)
                        return false;

                    topic = _topics[topicName];
                }

                topic->pushMessage(msg);
                return true;
            }

            // 发生错误的响应
            void errorResponse(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg, RetCode rcode)
            {
                TopicResponse::s_ptr msg_rsp = MessageFactory::create<TopicResponse>();
                msg_rsp->setRid(msg->rid());
                msg_rsp->setMtype(MType::RSP_TOPIC);
                msg_rsp->setRcode(rcode);
                conn->send(msg_rsp);
            }

            // 正常响应
            void topicResponse(const BaseConnection::s_ptr& conn, const TopicRequest::s_ptr& msg)
            {
                TopicResponse::s_ptr msg_rsp = MessageFactory::create<TopicResponse>();
                msg_rsp->setRid(msg->rid());
                msg_rsp->setMtype(MType::RSP_TOPIC);
                msg_rsp->setRcode(RetCode::RCODE_OK);
                conn->send(msg_rsp);
            }

        private:
            // 订阅者描述类
            class Subscriber 
            {
            public:
                using s_ptr = std::shared_ptr<Subscriber>;

                Subscriber(const BaseConnection::s_ptr& conn)
                    : _conn(conn)
                {}

                BaseConnection::s_ptr& conn()
                {
                    return _conn;
                }

                std::unordered_set<std::string>& topics()
                {
                    return _topics;
                }

                // 订阅主题
                void appendTopic(const std::string& topic)
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _topics.insert(topic);
                }
                
                // 取消订阅
                void removeTopic(const std::string& topic)
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _topics.erase(topic);
                }

            private:
                std::mutex _mtx;
                BaseConnection::s_ptr _conn;
                std::unordered_set<std::string> _topics; // 订阅者订阅的主题
            };  
            
            // 主题类
            class Topic
            {
            public:
                using s_ptr = std::shared_ptr<Topic>;

                Topic(const std::string& name)
                    : _name(name)
                {}

                std::string& topicName()
                {
                    return _name;
                }

                std::unordered_set<Subscriber::s_ptr>& subscribers()
                {
                    return _subscribers;
                }

                // 订阅主题
                void appendSubscriber(const Subscriber::s_ptr& sub)
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _subscribers.insert(sub);
                }

                // 取消订阅
                void removeSubscriber(const Subscriber::s_ptr& sub)
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _subscribers.erase(sub);
                }

                // 推送消息
                void pushMessage(const BaseMessage::s_ptr& msg)
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    for (auto sub : _subscribers) // 遍历所有订阅者
                        sub->conn()->send(msg);
                }
            
            private:
                std::string _name;
                std::mutex _mtx;
                std::unordered_set<Subscriber::s_ptr> _subscribers; // 该主题的订阅者连接
            };

        private:
            std::mutex _mtx;
            std::unordered_map<std::string, Topic::s_ptr> _topics;
            std::unordered_map<BaseConnection::s_ptr, Subscriber::s_ptr> _subscribes;
        };
    }
}