#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"
#include "requestor.hpp" 

namespace JsonRpc
{
    namespace Client
    {
        // 服务提供方
        class Provider
        {
        public:
            using s_ptr = std::shared_ptr<Provider>;

            Provider(const Requestor::s_ptr& requestor)
                : _requestor(requestor)
            {}

            // 注册服务
            bool registeryMethod(const BaseConnection::s_ptr& conn, const std::string& method, const Address& addr)
            {
                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->setRid(UUID::uuid());
                msg_req->setMtype(MType::REQ_SERVICE);
                msg_req->setMethod(method);
                msg_req->setServiceHost(addr);
                msg_req->setServiceOpType(ServiceOpType::SERVICE_REGISTRY);
                
                BaseMessage::s_ptr msg_rsp;
                bool ret = _requestor->send(conn, msg_req, msg_rsp);
                if (!ret)
                {
                    E_LOG("%s 服务注册失败!", method.c_str());
                    return false;
                }

                auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                if (!service_rsp)
                {
                    E_LOG("%s 服务注册失败!", method.c_str());
                    return false;
                }

                if (service_rsp->rcode() != RetCode::RCODE_OK)
                {             
                    E_LOG("服务注册失败: %s", errReason(service_rsp->rcode()).c_str());
                    return false;                
                }

                I_LOG("%s 服务注册成功!", method.c_str());
                return true;
            }

        private:
            Requestor::s_ptr _requestor;
        };

        // 管理服务的多个提供方
        class MethodHost
        {
        public:
            using s_ptr = std::shared_ptr<MethodHost>;

            MethodHost()
                : _index(0)
            {}
            
            MethodHost(const std::vector<Address>& hosts)
                : _hosts(hosts)
                , _index(0)
            {}

            void appendHost(const Address& host)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _hosts.push_back(host);
            }

            void removeHost(const Address& host)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                for (auto it = _hosts.begin(); it != _hosts.end(); it++)
                {
                    if (*it == host)
                    {
                        _hosts.erase(it);
                        break;
                    }
                }
            }

            Address chooseHost()
            {
                std::unique_lock<std::mutex> lock(_mtx);
                return _hosts[_index++ % _hosts.size()];
            }

            bool empty()
            {
                return _hosts.empty();
            }

        private:
            std::mutex _mtx;
            size_t _index; // 当前选中的主机
            std::vector<Address> _hosts;
        };

        // 服务发现
        class Discover
        {
            public:
            using OfflientCallback = std::function<void(const Address&)>;
            using s_ptr = std::shared_ptr<Discover>;

            Discover(const Requestor::s_ptr& requestor, const OfflientCallback& offCb)
                : _requestor(requestor)
                , _offCb(offCb)
            {}

            bool serviceDiscover(const BaseConnection::s_ptr& conn, const std::string& method, Address& host)
            {
                // 在_methodHosts查找
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (_methodHosts.count(method) && !_methodHosts[method]->empty())
                    {
                        host = _methodHosts[method]->chooseHost();
                        return true;
                    }
                }

                // 向服务端请求
                ServiceRequest::s_ptr msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->setRid(UUID::uuid());
                msg_req->setMethod(method);
                msg_req->setMtype(MType::REQ_SERVICE);
                msg_req->setServiceOpType(ServiceOpType::SERVICE_DISCOVER);

                BaseMessage::s_ptr msg_rsp;
                if (!_requestor->send(conn, msg_req, msg_rsp))
                {
                    E_LOG("%s 服务发现请求失败!", method.c_str());
                    return false;
                }

                auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                if (!service_rsp)
                {
                    E_LOG("%s 服务发现结果错误: 类型转换失败", method.c_str());
                    return false;
                }
                
                if (service_rsp->rcode() != RetCode::RCODE_OK)
                {
                    E_LOG("%s 服务发现结果错误: %s", method.c_str(), errReason(service_rsp->rcode()).c_str());
                    return false;
                }

                std::unique_lock<std::mutex> lock(_mtx);
                auto method_hosts = std::make_shared<MethodHost>(service_rsp->Hosts());
                if (method_hosts->empty())
                {
                    E_LOG("%s 没有对应的服务!", method.c_str());
                    return false;
                }

                _methodHosts[method] = method_hosts;
                host = _methodHosts[method]->chooseHost();
                return true;
            }

            // 注册到dispatcher，对服务上下线回调处理
            void onServiceRequest(const BaseConnection::s_ptr& conn, const ServiceRequest::s_ptr& msg)
            {

                if (msg->serviceOpType() == ServiceOpType::SERVICE_ONLINE)
                { // 服务上线通知
                    std::unique_lock<std::mutex> lock(_mtx);
                    if (!_methodHosts.count(msg->method())) // method不存在
                        _methodHosts[msg->method()] = std::make_shared<MethodHost>();

                    _methodHosts[msg->method()]->appendHost(msg->serviceHost());
                    I_LOG("%s 服务上线!", msg->method().c_str());
                }
                else if (msg->serviceOpType() == ServiceOpType::SERVICE_OUTLINE)
                { // 服务下线通知
                    std::unique_lock<std::mutex> lock(_mtx);
                    _methodHosts[msg->method()]->removeHost(msg->serviceHost());
                    _offCb(msg->serviceHost());
                    I_LOG("%s 服务下线!", msg->method().c_str());
                }
                else
                { // 未知通知
                    E_LOG("%d 收到未知请求!", (int)msg->serviceOpType())
                }
            }

        private:
            OfflientCallback _offCb;
            std::mutex _mtx;
            std::unordered_map<std::string, MethodHost::s_ptr> _methodHosts;
            Requestor::s_ptr _requestor;
        };
    }
}