#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"

#include <future>

namespace JsonRpc
{
    namespace Client
    {
        // 请求管理类
        // 由于muduo是异步网络库，并发地发送多个请求，接收多个响应
        // 导致接收到响应时，无法确定是哪一个请求的响应
        // 因此使用rid做一个映射，收到响应时，查询哈希表的rid，获取对应的请求
        class Requestor
        {
        public:
            using s_ptr = std::shared_ptr<Requestor>;
            using RequestCallback = std::function<void(BaseMessage::s_ptr&)>;
            using AsyncResponse = std::future<BaseMessage::s_ptr>;

            struct RequestDescriber
            {
                using s_ptr = std::shared_ptr<RequestDescriber>;
                BaseMessage::s_ptr request; // 请求消息
                ReqType rtype; // 请求类型
                std::promise<BaseMessage::s_ptr> response; // 响应结果
                RequestCallback callback; // 请求
            };

            // 收到响应的回调，注册到dispatcher
            void onResponse(const BaseConnection::s_ptr& conn, BaseMessage::s_ptr& msg)
            {
                std::string rid = msg->rid();
                if (_req_descs.count(rid) == 0)
                {
                    E_LOG("%s 请求不存在", rid.c_str());
                    return;
                }

                RequestDescriber::s_ptr reqDesc = _req_descs[rid];
                if (reqDesc->rtype == ReqType::REQ_ASYNC)
                {
                    // 异步请求，响应到达后，设置异步future的值
                    reqDesc->response.set_value(msg);
                }
                else if (reqDesc->rtype == ReqType::REQ_CALLBACK)
                {
                    // 回调请求，响应到达后，触发回调函数
                    if (reqDesc->callback) 
                        reqDesc->callback(msg);
                }
                else
                {
                    E_LOG("不存在的请求类型!");
                }

                delDescriber(rid); // 得到响应后删除
            }
            
            // 发送异步请求
            bool send(const BaseConnection::s_ptr& conn, const BaseMessage::s_ptr& req, AsyncResponse& async_rsp)
            {
                if (!conn) return false;
                
                RequestDescriber::s_ptr reqDesc = newDescriber(req, ReqType::REQ_ASYNC);
                if (reqDesc == nullptr)
                {
                    E_LOG("请求描述对象构造失败!");
                    return false;
                }
                
                conn->send(req);
                async_rsp = reqDesc->response.get_future();
                return true;
            }

            // 发送同步请求
            bool send(const BaseConnection::s_ptr& conn, const BaseMessage::s_ptr& req, BaseMessage::s_ptr& sync_rsp)
            {
                if (!conn) return false;

                AsyncResponse async_rsp;
                if (!send(conn, req, async_rsp))
                    return false;

                sync_rsp = async_rsp.get();
                return true;
            }

            // 发送回调请求
            bool send(const BaseConnection::s_ptr& conn, const BaseMessage::s_ptr& req, RequestCallback& cb)
            {
                if (!conn) return false;

                RequestDescriber::s_ptr reqDesc = newDescriber(req, ReqType::REQ_CALLBACK, cb);
                if (reqDesc == nullptr)
                {
                    E_LOG("请求描述对象构造失败!");
                    return false;
                }

                conn->send(req);
                return true;
            }

        private:
            RequestDescriber::s_ptr newDescriber(const BaseMessage::s_ptr& req, ReqType rtype, const RequestCallback& cb = RequestCallback())
            {
                std::unique_lock<std::mutex> lock(_mtx);
                
                RequestDescriber::s_ptr reqDes = std::make_shared<RequestDescriber>();
                reqDes->rtype = rtype;
                reqDes->request = req;

                // if (rtype == ReqType::REQ_ASYNC && cb)
                if (rtype == ReqType::REQ_CALLBACK && cb)
                    reqDes->callback = cb;

                _req_descs[req->rid()] = reqDes;
                return reqDes;
            }

            RequestDescriber::s_ptr getDescriber(const std::string& rid)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (_req_descs.count(rid) == 0)
                {
                    I_LOG("id = %s 的请求不存在", rid.c_str());
                    return std::make_shared<RequestDescriber>();
                }

                return _req_descs[rid];
            }

            void delDescriber(const std::string& rid)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (_req_descs.count(rid) == 0)
                {
                    I_LOG("id = %s 的请求不存在", rid.c_str());
                    return;
                }
                
                _req_descs.erase(rid);
            }

        private:
            std::mutex _mtx;
            std::unordered_map<std::string, RequestDescriber::s_ptr> _req_descs;
            // rid -> 请求描述 
        };
    }
}
