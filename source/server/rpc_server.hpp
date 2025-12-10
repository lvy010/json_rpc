#pragma once

#include "../common/dispatcher.hpp"
#include "../client/rpc_client.hpp"
#include "../server/rpc_topic.hpp"
#include "rpc_router.hpp"
#include "rpc_registry.hpp"

namespace JsonRpc
{
    namespace Server
    {
        // 注册中心服务端
        class RegistryServer
        {
        public:
            using s_ptr = std::shared_ptr<RegistryServer>;
            
            RegistryServer(int port)
                : _pd_manager(std::make_shared<PDManager>())
                , _dispatcher(std::make_shared<Dispatcher>())
                , _server(ServerFactory::create(port))
            {
                auto service_cb = std::bind(&PDManager::onServiceRequest, _pd_manager.get(),
                                    std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE, service_cb);

                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                                    std::placeholders::_1, std::placeholders::_2);

                auto shutdown_cb = std::bind(&RegistryServer::onShutDown, this, std::placeholders::_1);

                _server->setMessageCallback(message_cb);
                _server->setCloseCallback(shutdown_cb);
            }

            void start()
            {
                _server->start();
            }
        
        private:
            void onShutDown(const BaseConnection::s_ptr& conn)
            {
                _pd_manager->onConnShutdown(conn);
            }

        private:
            PDManager::s_ptr _pd_manager;
            Dispatcher::s_ptr _dispatcher;
            BaseServer::s_ptr _server;
        };

        // rpc提供服务端
        class RpcServer
        {
        public:
            using s_ptr = std::shared_ptr<RpcServer>;

            //rpc——server端有两套地址信息：
            //  1. rpc服务提供端地址信息--必须是rpc服务器对外访问地址（云服务器---监听地址和访问地址不同）
            //  2. 注册中心服务端地址信息 -- 启用服务注册后，连接注册中心进行服务注册用的
            RpcServer(const Address &access_addr, bool enableRegistry = false, const Address &registry_server_addr = Address())
                : _enableRegistry(enableRegistry)
                , _access_addr(access_addr)
                , _router(std::make_shared<RpcRouter>())
                , _dispatcher(std::make_shared<Dispatcher>()) 
                , _server(ServerFactory::create(access_addr.second))
            {
                if (enableRegistry) // 如果启动了服务注册，则实例化注册客户端
                {
                    _reg_client = std::make_shared<Client::RegistryClient>(
                        registry_server_addr.first, registry_server_addr.second);
                }

                //当前成员server是一个rpcserver，用于提供rpc服务
                auto rpc_cb = std::bind(&RpcRouter::onRpcRequest, _router.get(), 
                    std::placeholders::_1, std::placeholders::_2);

                _dispatcher->registerHandler<RpcRequest>(MType::REQ_RPC, rpc_cb);

                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                    std::placeholders::_1, std::placeholders::_2);

                _server->setMessageCallback(message_cb);
            }

            void start()
            {
                _server->start();
            }

            void registerMethod(const ServiceDescriber::s_ptr& service)
            {
                if (_enableRegistry)
                    _reg_client->registryMethod(service->method(), _access_addr);

                _router->registerMethod(service);
            }

        private:
            bool _enableRegistry;
            Address _access_addr;
            Client::RegistryClient::s_ptr _reg_client;
            RpcRouter::s_ptr _router;
            Dispatcher::s_ptr _dispatcher;
            BaseServer::s_ptr _server;
        };

        // 主题订阅服务端
        class TopicServer
        {
        public:
            using s_ptr = std::shared_ptr<TopicServer>;
            
            TopicServer(int port)
                : _topic_manager(std::make_shared<TopicManager>())
                , _dispatcher(std::make_shared<Dispatcher>())
                , _server(ServerFactory::create(port))
            {
                auto topic_cb = std::bind(&TopicManager::onTopicRequest, _topic_manager.get(),
                                    std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC, topic_cb);

                auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                                    std::placeholders::_1, std::placeholders::_2);

                auto shutdown_cb = std::bind(&TopicServer::onShutDown, this, std::placeholders::_1);

                _server->setMessageCallback(message_cb);
                _server->setCloseCallback(shutdown_cb);
            }

            void start()
            {
                _server->start();
            }
        
        private:
            void onShutDown(const BaseConnection::s_ptr& conn)
            {
                _topic_manager->onShutdown(conn);
            }

        private:
            TopicManager::s_ptr _topic_manager;
            Dispatcher::s_ptr _dispatcher;
            BaseServer::s_ptr _server;
        };
    }
}