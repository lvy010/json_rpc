#pragma once

#include "../common/net.hpp"
#include "../common/message.hpp"

namespace JsonRpc
{
    namespace Server
    {
        enum class VType
        {
            BOOL = 0,
            INTERGAL,
            NUMBERIC, // 浮点数
            STRING,
            ARRAY,
            OBJECT  
        };

        // 服务描述类
        class ServiceDescriber
        {
        public:
            using s_ptr = std::shared_ptr<ServiceDescriber>;
            using ServiceCallback = std::function<void(Json::Value& params, Json::Value& ret)>;
            using paramDescriber = std::pair<std::string, VType>;

            ServiceDescriber(const std::string&& name, std::vector<paramDescriber>&& params_desc, 
                VType return_type, ServiceCallback&& cb)
                : _name(std::move(name))
                , _params_desc(std::move(params_desc))
                , _return_type(return_type)
                , _cb(std::move(cb))
            {}

            const std::string& method()
            {
                return _name; 
            }

            // 检查参数是否合法
            bool checkParam(const Json::Value& params)
            {
                for (auto& desc : _params_desc)
                {
                    if (!params.isMember(desc.first))
                    {
                        E_LOG("%s 参数字段缺失!", desc.first.c_str());
                        return false;
                    }

                    if (!check(desc.second, params[desc.first]))
                    {
                        E_LOG("%s 参数字段类型错误!", desc.first.c_str());
                        return false;
                    }
                }

                return true;
            }

            bool checkReturnType(const Json::Value& val)
            {
                return check(_return_type, val);
            }

            bool call(Json::Value params, Json::Value& result)
            {
                _cb(params, result);
                if (!check(_return_type, result))
                {
                    E_LOG("返回值类型错误!");
                    return false;
                }
                return true;
            }

        private:
            bool check(VType vtype, const Json::Value& val)
            {
                switch (vtype)
                {
                    case VType::BOOL:
                        return val.isBool();
                    case VType::INTERGAL:
                        return val.isIntegral();
                    case VType::NUMBERIC:
                        return val.isNumeric();
                    case VType::STRING:
                        return val.isString();
                    case VType::ARRAY:
                        return val.isArray();
                    case VType::OBJECT:
                        return val.isObject();
                }

                return false;
            }

        private:
            std::string _name; // 方法名称
            ServiceCallback _cb; // 业务回调函数
            std::vector<paramDescriber> _params_desc; // 参数类型描述
            VType _return_type; // 返回值类型描述
        };

        // 建造者模式
        //     工厂模式	             建造者模式
        // 单方法直接返回对象	多步骤配置 + 最终组装
        class ServiceDescriberBuilder
        {
        public:
            ServiceDescriber::s_ptr build()
            {
                return std::make_shared<ServiceDescriber>(std::move(_name), std::move(_params_desc), std::move(_return_type), std::move(_cb));
            }

            void setName(const std::string name)
            {
                _name = name;
            }

            void setCallback(ServiceDescriber::ServiceCallback cb)
            {
                _cb = cb;
            }

            void setParamsDesc(const std::string& pname, VType vtype)
            {
                _params_desc.emplace_back(pname, vtype);
            }

            void setReturnType(VType return_type)
            {
                _return_type = return_type;
            }

        private:
            std::string _name; // 方法名称
            ServiceDescriber::ServiceCallback _cb; // 业务回调函数
            std::vector<ServiceDescriber::paramDescriber> _params_desc; // 参数类型描述
            VType _return_type; // 返回值类型描述
        };
        
        // 服务管理类 -> 将管理与使用区分开，在业务层面不考虑加锁问题
        class ServiceManager
        {
        public:
            using s_ptr = std::shared_ptr<ServiceManager>;

            bool insert(ServiceDescriber::s_ptr& desc)
            {
                std::unique_lock<std::mutex> lock(_mtx);

                std::string methodName = desc->method();
                if (_services.count(methodName))
                {
                    I_LOG("服务 %s 已存在!", methodName.c_str());
                    return false;
                }

                _services[desc->method()] = desc; 
                return true;
            }

            ServiceDescriber::s_ptr select(const std::string& methodName)
            {
                std::unique_lock<std::mutex> lock(_mtx);

                if (!_services.count(methodName))
                {
                    I_LOG("服务 %s 不存在!", methodName.c_str());
                    return ServiceDescriber::s_ptr();
                }

                return _services[methodName];
            }

            bool remove(const std::string& methodName)
            {
                std::unique_lock<std::mutex> lock(_mtx);

                if (!_services.count(methodName))
                {
                    I_LOG("服务 %s 不存在!", methodName.c_str());
                    return false;
                }

                _services.erase(methodName);
                return true;
            }

        private:
            std::mutex _mtx;
            std::unordered_map<std::string, ServiceDescriber::s_ptr> _services; // 管理所有的服务
        };

        class RpcRouter
        {
        public:
            using s_ptr = std::shared_ptr<RpcRouter>;

            RpcRouter()
                : _service_manager(std::make_shared<ServiceManager>())
            {}

            // 注册给dispatcher的回调
            void onRpcRequest(const BaseConnection::s_ptr& conn, RpcRequest::s_ptr& req)
            {
                I_LOG("收到rpc请求 %s!", req->method().c_str());

                // 检查是否可以提供服务
                ServiceDescriber::s_ptr service = _service_manager->select(req->method());
                if (!service)
                {
                    I_LOG("请求的服务 %s 不存在", req->method().c_str());
                    response(conn, req, Json::Value(), RetCode::RCODE_NOT_FOUND_SERVICE);
                    return;
                }

                // 检查参数类型
                if (!service->checkParam(req->params()))
                {
                    I_LOG("请求的服务 %s 参数错误", req->method().c_str());
                    response(conn, req, Json::Value(), RetCode::RCODE_INVALID_PARAM);
                    return;
                }

                // 调用
                Json::Value res;
                bool ret = service->call(req->params(), res);
                if (!ret)
                {
                    E_LOG("请求的服务 %s 内部错误", req->method().c_str());
                    response(conn, req, Json::Value(), RetCode::RCODE_INHTERNAL_ERROR);
                    return;
                }

                // 返回结果
                response(conn, req, res, RetCode::RCODE_OK);
            }
            
            // 注册服务
            void registerMethod(ServiceDescriber::s_ptr service)
            {
                _service_manager->insert(service);
            }
        
        private:
            void response(const BaseConnection::s_ptr& conn, const RpcRequest::s_ptr& req, 
                const Json::Value& res, RetCode rcode)
            {
                std::shared_ptr<JsonRpc::RpcResponse> response = MessageFactory::create<RpcResponse>();
                response->setRid(req->rid());
                response->setRcode(rcode);
                response->setMtype(MType::RSP_RPC);
                response->setResult(res);
                conn->send(response);
            }

        private:
            ServiceManager::s_ptr _service_manager;
        };
    }
}