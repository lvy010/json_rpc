#pragma once

#include "../common/dispatcher.hpp"
#include "requestor.hpp"
#include "rpc_caller.hpp"
#include "rpc_registry.hpp"
#include "rpc_topic.hpp"

namespace JsonRpc
{
    namespace Client
    {
        // 服务注册客户端
        class RegistryClient
        {
        public:
            using s_ptr = std::shared_ptr<RegistryClient>;

            // 构造函数传入注册中心的地址，与注册中心建立连接
            RegistryClient(const std::string& ip, int port)
                : _requestor(std::make_shared<Requestor>())
                , _provider(std::make_shared<Provider>(_requestor))
                , _dispatcher(std::make_shared<Dispatcher>())
                , _client(ClientFactory::create(ip, port))
            {
                auto rsp_cb = std::bind(&Requestor::onResponse, _requestor.get(),
                                         std::placeholders::_1, std::placeholders::_2);
            
                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(),
                                            std::placeholders::_1, std::placeholders::_2);

                _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE, rsp_cb);
                _client->setMessageCallback(message_cb);
                _client->connect();
            }

            // 服务注册接口
            bool registryMethod(const std::string& method, Address& host)
            {
                return _provider->registeryMethod(_client->getConnection(), method, host);
            }

        private:
            Requestor::s_ptr _requestor;
            Provider::s_ptr _provider;
            Dispatcher::s_ptr _dispatcher;
            BaseClient::s_ptr _client;
        };

        // 服务发现客户端
        class DiscoverClient
        {
        public:
            using s_ptr = std::shared_ptr<DiscoverClient>;

            // 构造函数传入注册中心的地址，与注册中心建立连接
            DiscoverClient(const std::string& ip, int port, const Discover::OfflientCallback& offCb)
                : _requestor(std::make_shared<Requestor>())
                , _discover(std::make_shared<Discover>(_requestor, offCb))
                , _dispatcher(std::make_shared<Dispatcher>())
                , _client(ClientFactory::create(ip, port))
            {
                // 处理响应
                auto rsp_cb = std::bind(&Requestor::onResponse, _requestor.get(),
                                         std::placeholders::_1, std::placeholders::_2);
                                         
                _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE, rsp_cb);

                // 处理服务上下线请求
                auto req_cb = std::bind(&Discover::onServiceRequest, _discover.get(),
                                         std::placeholders::_1, std::placeholders::_2);
                
                _dispatcher->registerHandler<BaseMessage>(MType::REQ_SERVICE, rsp_cb);

                // 将dispatcher注册到客户端消息处理
                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(),
                                        std::placeholders::_1, std::placeholders::_2);

                _client->setMessageCallback(message_cb);
                _client->connect();
            }

            // 服务发现接口
            bool serviceDiscover(const std::string& method, Address& host)
            {
                return _discover->serviceDiscover(_client->getConnection(), method, host);
            }

        private:
            Requestor::s_ptr _requestor;
            Discover::s_ptr _discover;
            Dispatcher::s_ptr _dispatcher;
            BaseClient::s_ptr _client;
        };

        // rpc客户端
        class RpcClient
        {
        public:
            using s_ptr = std::shared_ptr<RpcClient>;

            // enableDiscover决定rpc调用模式
            // true: 传入的为注册中心的地址，向服务中心发现后，再进行调用
            // false: 传入的是服务提供方的地址，直接向该地址进行 rpc 请求
            RpcClient(bool enableDiscover, const std::string& ip, int port)
                : _enableDiscover(enableDiscover)
                , _requestor(std::make_shared<Requestor>())
                , _dispatcher(std::make_shared<Dispatcher>())
                , _caller(std::make_shared<RpcCaller>(_requestor))
            {
                // 处理rpc请求后的响应
                auto rsp_cb = std::bind(&Requestor::onResponse, _requestor.get(),
                                         std::placeholders::_1, std::placeholders::_2);
                                         
                _dispatcher->registerHandler<BaseMessage>(MType::RSP_RPC, rsp_cb);

                if (_enableDiscover) // 启用服务发现
                {
                    // 创建一个服务发现客户端，连接服务中心
                    auto offCb = std::bind(&RpcClient::delClient, this, std::placeholders::_1);
                    _discover_client = std::make_shared<DiscoverClient>(ip, port, offCb); 
                }
                else // 未启用服务发现
                {
                    // 将dispatcher注册到客户端消息处理
                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(),
                                            std::placeholders::_1, std::placeholders::_2);

                    _rpc_client = ClientFactory::create(ip, port);
                    _rpc_client->setMessageCallback(message_cb);
                    _rpc_client->connect();
                }
            }

            // 同步调用
            bool call(const std::string& method, const Json::Value& params, Json::Value& result)
            {
                BaseClient::s_ptr client = getClient(method);
                if (!client)
                    return false;

                I_LOG("%s 存在服务提供者!", method.c_str());

                return _caller->call(client->getConnection(), method, params, result);
            }
       
            // 异步调用
            bool call(const std::string& method, const Json::Value& params, RpcCaller::JsonAsyncResponse& result)
            {
                BaseClient::s_ptr client = getClient(method);

                if (!client)
                    return false;

                return _caller->call(client->getConnection(), method, params, result);
            }

            // 回调
            bool call(const std::string& method, const Json::Value& params, const RpcCaller::JsonResponseCallback& cb)
            {
                BaseClient::s_ptr client = getClient(method);

                if (!client)
                    return false;

                return _caller->call(client->getConnection(), method, params, cb);
            }


        private:
            BaseClient::s_ptr newClient(const Address& host)
            {
                // 将dispatcher注册到客户端消息处理
                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(),
                                        std::placeholders::_1, std::placeholders::_2);

                auto client = ClientFactory::create(host.first, host.second);
                client->setMessageCallback(message_cb);
                client->connect();
                // bug记录: 此处额外加锁，造成死锁，在putClient内已经加锁了
                // std::unique_lock<std::mutex> lock(_mtx);
                putClient(host, client);
                return client;
            }

            BaseClient::s_ptr getClient(const Address& host)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (_rpc_clients.count(host))
                    return _rpc_clients[host];

                return BaseClient::s_ptr();
            }

            BaseClient::s_ptr getClient(const std::string& method)
            {
                BaseClient::s_ptr client;
                if (_enableDiscover)
                {
                    Address host;
                    if (!_discover_client->serviceDiscover(method, host))
                    {
                        E_LOG("%s 没有服务提供者!", method.c_str());
                        return nullptr;
                    }

                    // 查看对应服务是否有已实例化客户端
                    client = getClient(host);
                    if (!client.get())
                        client = newClient(host);
                }
                else
                {
                    client = _rpc_client; // 向固定服务端请求
                }

                return client;
            }

            void putClient(const Address& host, BaseClient::s_ptr& client)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _rpc_clients[host] = client;
            }

            void delClient(const Address& host)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _rpc_clients.erase(host);
            }

            // 后面 Address 作为哈希表的key，需要自定义哈希函数
            struct AddressHash
            {
                size_t operator()(const Address& host) const // 需要标记为const函数
                {
                    std::string addr = host.first + std::to_string(host.second);
                    return std::hash<std::string>{}(addr); // C++11 内置基本数据类型哈希函数
                }
            };

        private:
            bool _enableDiscover;
            Requestor::s_ptr _requestor;
            DiscoverClient::s_ptr _discover_client; // 进行服务发现
            RpcCaller::s_ptr _caller; // 进行rpc调用
            Dispatcher::s_ptr _dispatcher;
            BaseClient::s_ptr _rpc_client; // 关闭服务发现，直接与rpc服务端连接
            std::mutex _mtx; // 对 _rpc_clients 加锁
            //                                              哈希函数
            std::unordered_map<Address, BaseClient::s_ptr, AddressHash> _rpc_clients; // 启用服务发现，客户端连接池
        };

        class TopicClient
        {
        public:
            // 中转服务器的地址
            TopicClient(const std::string& ip, int port)
                : _requestor(std::make_shared<Requestor>())
                , _dispatcher(std::make_shared<Dispatcher>())
                , _topic_manager(std::make_shared<TopicManager>(_requestor))
                , _rpc_client(ClientFactory::create(ip, port))
            {
                // 处理主题请求后的响应
                auto rsp_cb = std::bind(&Requestor::onResponse, _requestor.get(),
                                        std::placeholders::_1, std::placeholders::_2);
                                        
                _dispatcher->registerHandler<BaseMessage>(MType::RSP_TOPIC, rsp_cb);

                // 主题消息推送回调
                auto top_msg_cb = std::bind(&TopicManager::onPublish, _topic_manager.get(),
                                        std::placeholders::_1, std::placeholders::_2);

                _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC, top_msg_cb);

                // 将dispatcher注册到客户端消息处理
                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(),
                                        std::placeholders::_1, std::placeholders::_2);

                _rpc_client->setMessageCallback(message_cb);
                _rpc_client->connect();
            }
            
            // 主题创建
            bool createTopic(const std::string& key)
            {
                return _topic_manager->createTopic(_rpc_client->getConnection(), key);
            }

            // 主题删除
            bool removeTopic(const std::string& key)
            {
                return _topic_manager->removeTopic(_rpc_client->getConnection(), key);
            }

            // 主题订阅
            bool subscribeTopic(const std::string& key, const TopicManager::SubscribeCallback& cb)
            {
                return _topic_manager->subscribeTopic(_rpc_client->getConnection(), key, cb);
            }

            // 取消订阅
            bool cancelTopic(const std::string& key)
            {
                return _topic_manager->cancelTopic(_rpc_client->getConnection(), key);
            } 

            // 主题消息发布
            bool publishTopic(const std::string& key, const std::string& msg)
            {
                return _topic_manager->publishTopic(_rpc_client->getConnection(), key, msg);
            }

            void shutDown()
            {
                _rpc_client->shutdown();
            }

        private:
            Requestor::s_ptr _requestor;
            Dispatcher::s_ptr _dispatcher;
            BaseClient::s_ptr _rpc_client;
            TopicManager::s_ptr _topic_manager;
        };
    }
}