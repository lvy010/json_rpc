#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"

#include <set>

namespace JsonRpc
{
    namespace Server
    {
        // 管理服务注册
        class ProviderManager
        {
        public:
            using s_ptr = std::shared_ptr<ProviderManager>;

            struct Provider
            {
                using s_ptr = std::shared_ptr<Provider>;
                Address addr;
                BaseConnection::s_ptr conn;
                std::mutex mtx;
                std::vector<std::string> methods; // 提供的服务

                Provider(const BaseConnection::s_ptr& _conn, const Address& _addr)
                    : addr(_addr)
                    , conn(_conn)
                {}

                void appendMethod(const std::string& method)
                {
                    std::unique_lock<std::mutex> lock;
                    methods.push_back(method);
                }
            };
            
            void addProvider(const BaseConnection::s_ptr& conn, const Address& addr, const std::string& method)
            {
                Provider::s_ptr provider;
                {
                    std::unique_lock<std::mutex> lock;

                    // 获取/创建 provider
                    if (_conns.count(conn))
                    {
                        provider = _conns[conn];
                    }
                    else
                    {
                        provider = std::make_shared<Provider>(conn, addr);
                        _conns[conn] = provider;
                    }

                    // 添加服务
                    _providers[method].insert(provider);
                }

                provider->appendMethod(method);
            }

            Provider::s_ptr getProvider(const BaseConnection::s_ptr& conn)
            {
                std::unique_lock<std::mutex> lock;
                return _conns.count(conn) ? _conns[conn] : nullptr;
            }

            void delProvider(const BaseConnection::s_ptr& conn)
            {
                std::unique_lock<std::mutex> lock;
                auto con_it = _conns.find(conn);
                if (con_it == _conns.end())
                    return;

                for (auto& method : con_it->second->methods)
                {
                    auto pro_it = _providers.find(method);
                    pro_it->second.erase(con_it->second);

                    if (pro_it->second.empty())
                        _providers.erase(pro_it);
                }
                
                _conns.erase(con_it);
            }

            std::vector<Address> methodHosts(const std::string& method)
            {
                std::unique_lock<std::mutex> lock(_mtx);

                if (!_providers.count(method))
                    return std::vector<Address>();

                std::vector<Address> ret;
                for (auto& provider : _providers[method])
                    ret.push_back(provider->addr);

                return ret;
            }

        private:
            std::mutex _mtx;
            std::unordered_map<std::string, std::set<Provider::s_ptr>> _providers; // 服务被谁提供了
            std::unordered_map<BaseConnection::s_ptr, Provider::s_ptr> _conns;
        };

        // 管理服务发现
        class DiscoverManager
        {
        public:
            using s_ptr = std::shared_ptr<DiscoverManager>;

            struct Discover
            {
                using s_ptr = std::shared_ptr<Discover>;
                BaseConnection::s_ptr conn;
                std::mutex mtx;
                std::vector<std::string> methods; // 发现过的服务

                Discover(const BaseConnection::s_ptr& _conn)
                    : conn(_conn)
                {}

                void appendMethod(const std::string& method)
                {
                    std::unique_lock<std::mutex> lock;
                    methods.push_back(method);
                }
            };
            
            Discover::s_ptr addDiscover(const BaseConnection::s_ptr& conn, const std::string& method)
            {
                Discover::s_ptr discover;
                {
                    std::unique_lock<std::mutex> lock;

                    // 获取/创建 discover
                    if (_conns.count(conn))
                    {
                        discover = _conns[conn];
                    }
                    else
                    {
                        discover = std::make_shared<Discover>(conn);
                        _conns[conn] = discover;
                    }

                    // 添加服务
                    _discovers[method].insert(discover);
                }

                discover->appendMethod(method);
                return discover;
            }

            void delDiscover(const BaseConnection::s_ptr& conn)
            {
                std::unique_lock<std::mutex> lock;
                auto con_it = _conns.find(conn);
                if (con_it == _conns.end())
                    return;

                for (auto& method : con_it->second->methods)
                {
                    auto dis_it = _discovers.find(method);
                    dis_it->second.erase(con_it->second);

                    if (dis_it->second.empty())
                        _discovers.erase(dis_it);
                }
                
                _conns.erase(con_it);
            }

            // 服务上线通知
            void onlineNotify(const std::string& method, const Address& addr)
            {
                notify(method, addr, ServiceOpType::SERVICE_ONLINE);
            }

            // 服务下线通知
            void offlineNotify(const std::string& method, const Address& addr)
            {
                notify(method, addr, ServiceOpType::SERVICE_OUTLINE);
            }
        
        private:
            void notify(const std::string& method, const Address& addr, ServiceOpType op)
            {
                std::unique_lock<std::mutex> lock;
                if (!_discovers.count(method))
                    return;

                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->setRid(UUID::uuid());
                msg_req->setMtype(MType::REQ_SERVICE);
                msg_req->setMethod(method);
                msg_req->setServiceHost(addr);
                msg_req->setServiceOpType(op);               

                for (auto& discover : _discovers[method])
                    discover->conn->send(msg_req); // 通知服务上下线
            }
            
        private:
            std::mutex _mtx;
            std::unordered_map<std::string, std::set<Discover::s_ptr>> _discovers; // 服务被谁发现了
            std::unordered_map<BaseConnection::s_ptr, Discover::s_ptr> _conns;
        };

        // 统一管理 provider 和 discover
        class PDManager
        {
        public:
            using s_ptr = std::shared_ptr<PDManager>;
            PDManager()
                : _proManager(std::make_shared<ProviderManager>())
                , _disManager(std::make_shared<DiscoverManager>())
            {}

            // 收到服务消息的回调，注册到dispatcher
            void onServiceRequest(const BaseConnection::s_ptr& conn, const ServiceRequest::s_ptr& msg)
            {
                ServiceOpType op = msg->serviceOpType();
                if (op == ServiceOpType::SERVICE_REGISTRY)
                {
                    // 服务注册 
                    _proManager->addProvider(conn, msg->serviceHost(), msg->method());
                    _disManager->onlineNotify(msg->method(), msg->serviceHost()); // 通知所有发现者，服务上线
                    registryResponse(conn, msg);
                    I_LOG("%s 服务上线!", msg->method().c_str());
                }
                else if (op == ServiceOpType::SERVICE_DISCOVER)
                {
                    // 服务发现
                    _disManager->addDiscover(conn, msg->method());
                    I_LOG("%s 服务发现中...", msg->method().c_str());
                    discoverResponse(conn, msg);
                }
                else
                {
                    E_LOG("不存在的操作类型%d!", (int)op);
                    errorResponse(conn, msg);
                }
            }

            // 收到连接断开的回调
            void onConnShutdown(const BaseConnection::s_ptr& conn)
            {
                auto provider = _proManager->getProvider(conn);
                if (provider)
                {
                    for (auto& method : provider->methods) // 通知所有发现者，服务下线
                        _disManager->offlineNotify(method, provider->addr);

                    _proManager->delProvider(conn);
                    return;
                }

                // 发现者下线，不做任何通知，如conn不是发现者，del内部啥也不做
                _disManager->delDiscover(conn); 
            }

        private:
            // 服务注册响应
            void registryResponse(const BaseConnection::s_ptr& conn, const ServiceRequest::s_ptr& msg)
            {
                ServiceResponse::s_ptr msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->setRid(msg->rid());
                msg_rsp->setMtype(MType::RSP_SERVICE);
                msg_rsp->setRcode(RetCode::RCODE_OK);
                msg_rsp->setOptype(ServiceOpType::SERVICE_REGISTRY);
                conn->send(msg_rsp);
            }
               
            // 服务发现响应
            void discoverResponse(const BaseConnection::s_ptr& conn, const ServiceRequest::s_ptr& msg)
            {
                ServiceResponse::s_ptr msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->setRid(msg->rid());
                msg_rsp->setMtype(MType::RSP_SERVICE);
                msg_rsp->setOptype(ServiceOpType::SERVICE_DISCOVER);

                std::vector<Address> hosts = _proManager->methodHosts(msg->method());
                if (hosts.empty()) // 未发现服务
                {
                    msg_rsp->setRcode(RetCode::RCODE_NOT_FOUND_SERVICE);
                    I_LOG("%s 未发现服务!", msg->method().c_str());
                }
                else // 发现服务
                {
                    msg_rsp->setRcode(RetCode::RCODE_OK);
                    msg_rsp->setMethod(msg->method());
                    msg_rsp->setHosts(hosts);
                    I_LOG("%s 成功发现服务!", msg->method().c_str());
                }

                conn->send(msg_rsp);
            }

            // 发生错误的响应
            void errorResponse(const BaseConnection::s_ptr& conn, const ServiceRequest::s_ptr& msg)
            {
                ServiceResponse::s_ptr msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->setRid(msg->rid());
                msg_rsp->setMtype(MType::RSP_SERVICE);
                msg_rsp->setRcode(RetCode::RCODE_OK);
                msg_rsp->setOptype(ServiceOpType::SERVICE_UNKNOW);
                conn->send(msg_rsp);
            }
               
        private:
            ProviderManager::s_ptr _proManager;
            DiscoverManager::s_ptr _disManager;
        };
    }
}