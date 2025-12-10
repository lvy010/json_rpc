#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"
#include "requestor.hpp"

#include <future>

namespace JsonRpc
{
    namespace Client
    {
        // 客户端调用rpc请求的类
        class RpcCaller
        {
        public:
            using s_ptr = std::shared_ptr<RpcCaller>;
            using JsonAsyncResponse = std::future<Json::Value>;
            using JsonResponseCallback = std::function<void(Json::Value)>;
            
            RpcCaller(Requestor::s_ptr& requestor)
                : _requestor(requestor)
            {}

            // 同步调用
            bool call(const BaseConnection::s_ptr& conn, const std::string& method,
                        const Json::Value& params, Json::Value& result)
            {
                // 构造请求对象
                BaseMessage::s_ptr req_base = MessageFactory::create(MType::REQ_RPC);
                RpcRequest::s_ptr req_msg = std::dynamic_pointer_cast<RpcRequest>(req_base);
                
                req_msg->setRid(UUID::uuid());
                req_msg->setMtype(MType::REQ_RPC);
                req_msg->setMethod(method);
                req_msg->setParams(params);

                // 构造响应对象
                BaseMessage::s_ptr rsp_base; // 此处为空智能指针，后续可能指向rpcmessage子类对象

                // 发送请求
                if (!_requestor->send(conn, req_base, rsp_base))
                {
                    E_LOG("发送rpc请求失败!");
                    return false; 
                }

                I_LOG("发送rpc请求成功!");

                // 获取响应结果
                RpcResponse::s_ptr rsp_rpc = std::dynamic_pointer_cast<RpcResponse>(rsp_base);
                if (!rsp_rpc)
                {
                    E_LOG("rpc响应结果向下转型失败!");
                    return false; 
                }

                if (rsp_rpc->rcode() != RetCode::RCODE_OK)
                {
                    E_LOG("rpc同步响应出错: %s", errReason(rsp_rpc->rcode()).c_str());
                    return false; 
                }

                result = rsp_rpc->result();
                return true;
            }

            // 异步调用
            bool call(const BaseConnection::s_ptr& conn, const std::string& method,
                const Json::Value& params, JsonAsyncResponse& result)
            {
                // 构造请求对象
                // RpcRequest::s_ptr req_msg = MessageFactory::create(MType::REQ_RPC);
                BaseMessage::s_ptr req_base = MessageFactory::create(MType::REQ_RPC);
                RpcRequest::s_ptr req_msg = std::dynamic_pointer_cast<RpcRequest>(req_base);
                req_msg->setRid(UUID::uuid());
                req_msg->setMtype(MType::REQ_RPC);
                req_msg->setMethod(method);
                req_msg->setParams(params);

                // 构造异步结果
                auto json_pms = std::make_shared<std::promise<Json::Value>>(); // 智能指针，防止被释放
                result = json_pms->get_future();

                Requestor::RequestCallback cb = std::bind(&RpcCaller::asyncCB, this, json_pms, std::placeholders::_1);

                // 发送请求
                if (!_requestor->send(conn, req_base, cb))
                {
                    E_LOG("发送rpc请求失败!");
                    return false; 
                }
                I_LOG("发送rpc请求成功!");
                
                return true;
            }
            
            // 回调
            bool call(const BaseConnection::s_ptr& conn, const std::string& method,
                const Json::Value& params, const JsonResponseCallback& user_cb)
            {
                // 构造请求对象
                // RpcRequest::s_ptr req_msg = MessageFactory::create(MType::REQ_RPC);
                BaseMessage::s_ptr req_base = MessageFactory::create(MType::REQ_RPC);
                RpcRequest::s_ptr req_msg = std::dynamic_pointer_cast<RpcRequest>(req_base);

                req_msg->setRid(UUID::uuid());
                req_msg->setMtype(MType::REQ_RPC);
                req_msg->setMethod(method);
                req_msg->setParams(params);

                Requestor::RequestCallback cb = std::bind(&RpcCaller::userCB, this, user_cb, std::placeholders::_1);

                // 发送请求
                if (!_requestor->send(conn, req_base, cb))
                {
                    E_LOG("发送rpc请求失败!");
                    return false; 
                }
                I_LOG("发送rpc请求成功!");
                return true;
            }

        private:
            // 异步回调
            // requestor 中 send 返回的是basemsg，而用户需要的是 json::value 正文
            // 收到响应时，触发该回调，设置promise<Json::Value>的值
            void asyncCB(std::shared_ptr<std::promise<Json::Value>> result, const BaseMessage::s_ptr& msg)
            {
                RpcResponse::s_ptr rsp_rpc = std::dynamic_pointer_cast<RpcResponse>(msg);
                if (!rsp_rpc)
                {
                    E_LOG("rpc响应结果向下转型失败!");
                    return; 
                }

                if (rsp_rpc->rcode() != RetCode::RCODE_OK)
                {
                    E_LOG("rpc异步响应出错: %s", errReason(rsp_rpc->rcode()).c_str());
                    return; 
                }

                result->set_value(rsp_rpc->result());
            }

            // 用户自定回调
            void userCB(const JsonResponseCallback& cb, const BaseMessage::s_ptr& msg)
            {
                RpcResponse::s_ptr rsp_rpc = std::dynamic_pointer_cast<RpcResponse>(msg);
                if (!rsp_rpc)
                {
                    E_LOG("rpc响应结果向下转型失败!");
                    return; 
                }

                if (rsp_rpc->rcode() != RetCode::RCODE_OK)
                {
                    E_LOG("rpc回调响应出错: %s", errReason(rsp_rpc->rcode()).c_str());
                    return; 
                }

                cb(rsp_rpc->result()); // 把 json::Value 传入用户自定义的回调中处理
            }

        private: 
            Requestor::s_ptr _requestor;
        };
    }
}