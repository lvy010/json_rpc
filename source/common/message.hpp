#pragma once

#include "util.hpp"
#include "fields.hpp"
#include "abstract.hpp"

namespace JsonRpc
{
    // json消息类
    class JsonMessgae : public BaseMessage
    {
    public:
        using s_ptr = std::shared_ptr<JsonMessgae>;

        // json序列化            
        virtual std::string serialize() override
        {
            std::string body;
            bool ret = JsonUtil::serialize(_body, body);

            return ret ? body : "";
        }

        // json反序列化
        virtual bool unSerialize(const std::string& msg) override
        {
            return JsonUtil::unSerialize(msg, _body);
        }
    
    protected:

        enum class JsonType
        {
            OBJECT = 0,
            ARRAY,
            INT,
            STRING
        };

        // 检查_body
        bool checkField(const std::string& val, JsonType type)
        {
            return checkField(_body, val, type);
        }

        // 检查指定对象
        bool checkField(Json::Value value, const std::string& val, JsonType type)
        {
            if (value[val].isNull())
            {
                E_LOG("%s 字段为空!", val.c_str());
                return false;
            }

            bool ret = true;
            std::string typeName;
            switch (type)
            {
                case JsonType::OBJECT:
                    ret = value[val].isObject();
                    typeName = "object";
                    break;
                case JsonType::ARRAY:
                    ret = value[val].isArray();
                    typeName = "array";
                    break;
                case JsonType::INT:
                    ret = value[val].isInt();
                    typeName = "int";
                    break;
                case JsonType::STRING:
                    ret = value[val].isString();
                    typeName = "string";
                    break;
            }

            if (!ret)
                E_LOG("%s 字段类型应该为 %s!", val.c_str(), typeName.c_str());

            return ret;
        }

    protected:
        Json::Value _body; // json对象
    };

    // ------------------------------ 请求 ------------------------------

    // json请求基类
    class JsonRequest : public JsonMessgae
    {
    public:
        using s_ptr = std::shared_ptr<JsonRequest>;
    };

    // 请求格式:
    // | length | mtype | idlength | id | body |

    // rpc请求
    // {
    //      method: "xxx",
    //      parameters: {
    //          a: xxx,
    //          b: xxx
    //      }
    // }
    class RpcRequest : public JsonRequest
    {
    public:
        using s_ptr = std::shared_ptr<RpcRequest>;

        // 检查字段合法
        virtual bool check() override
        {
            return checkField(KEY_METHOD, JsonType::STRING) 
                && checkField(KEY_PARAMS, JsonType::OBJECT);
        }

        // 返回请求方法
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        // 设置请求方法
        void setMethod(const std::string& method)
        {
            _body[KEY_METHOD] = method;
        }

        // 返回请求参数
        Json::Value params()
        {
            return _body[KEY_PARAMS];
        }

        // 设置请求参数
        void setParams(const Json::Value& params)
        {
            _body[KEY_PARAMS] = params;
        }
    };

    // topic请求
    // {
    //      key: xxx,
    //      msg: xxx,
    //      optype: xxx
    // }
    class TopicRequest : public JsonRequest
    {
    public:
        using s_ptr = std::shared_ptr<TopicRequest>;

        virtual bool check() override
        {
            // 检查主题名称、主题操作类型字段
            if (!checkField(KEY_TOPIC_KEY, JsonType::STRING) 
                || !checkField(KEY_OPTYPE, JsonType::INT))
                return false;

            // 如果操作类型为 publish 还需要 msg 字段
            if (_body[KEY_OPTYPE].asInt() == (int)TopicOpType::TOPIC_PUBLISH
                && !checkField(KEY_TOPIC_MSG, JsonType::STRING))
                return false;

            return true;
        }

        // 返回主题名称
        std::string topicKey()
        {
            return _body[KEY_TOPIC_KEY].asString();
        }

        // 设置主题名称
        void setTopicKey(const std::string& topicKey)
        {
            _body[KEY_TOPIC_KEY] = topicKey;
        }

        // 返回主题类型
        TopicOpType topicOpType()
        {
            return (TopicOpType)_body[KEY_OPTYPE].asInt();
        }

        // 设置主题类型
        void setTopicOpType(const TopicOpType topicOpType)
        {
            _body[KEY_OPTYPE] = (int)topicOpType;
        }

        // 返回主题信息
        std::string topicMsg()
        {
            return _body[KEY_TOPIC_MSG].asString();
        }

        // 设置主题信息
        void setTopicMsg(const std::string& topicMsg)
        {
            _body[KEY_TOPIC_MSG] = topicMsg;
        }
    };

    // service请求
    // {
    //      method: xxx,
    //      optype: xxx,
    //      host: {
    //          ip: xxx,
    //          port: xxx      
    //      }   
    // }
    using Address = std::pair<std::string, int>; 
    class ServiceRequest : public JsonRequest
    {
    public:
        using s_ptr = std::shared_ptr<ServiceRequest>;

        virtual bool check() override
        {
            if (!checkField(KEY_METHOD, JsonType::STRING)
                || !checkField(KEY_OPTYPE, JsonType::INT))
                return false;

            // 除了服务发现，其它的请求有host字段
            if (_body[KEY_OPTYPE].asInt() != (int)ServiceOpType::SERVICE_DISCOVER
                && (!checkField(KEY_HOST, JsonType::OBJECT)
                    || !checkField(_body[KEY_HOST], KEY_HOST_IP, JsonType::STRING)
                    || !checkField(_body[KEY_HOST], KEY_HOST_PORT, JsonType::STRING)))
                return false;

            return true;   
        }

        // 返回服务方法
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        // 设置服务方法
        void setMethod(const std::string& method)
        {
            _body[KEY_METHOD] = method;
        }

        // 返回服务类型
        ServiceOpType serviceOpType()
        {
            return (ServiceOpType)_body[KEY_OPTYPE].asInt();
        }

        // 设置服务类型
        void setServiceOpType(const ServiceOpType serviceOpType)
        {
            _body[KEY_OPTYPE] = (int)serviceOpType;
        }

        // 返回服务信息
        Address serviceHost()
        {
            Address addr;
            addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
            addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
            return addr;
        }

        // 设置服务信息
        void setServiceHost(const Address& host)
        {
            Json::Value val;
            val[KEY_HOST_IP] = host.first;
            val[KEY_HOST_PORT] = host.second;
            _body[KEY_HOST] = val;
        }
    };

    // ------------------------------ 响应 ------------------------------

    // json响应基类
    class JsonResponse : public JsonMessgae
    {
    public:
        using s_ptr = std::shared_ptr<JsonResponse>;

        // 返回响应code
        RetCode rcode()
        {
            return (RetCode)_body[KEY_RCODE].asInt();
        }

        // 设置响应code
        void setRcode(RetCode rcode)
        {
            _body[KEY_RCODE] = (int)rcode;
        }
    };

    // rpc响应类
    // {
    //      rcode: xxx,
    //      result: xxx   
    // }
    class RpcResponse : public JsonResponse
    {
    public:
        using s_ptr = std::shared_ptr<RpcResponse>;

        virtual bool check() override
        {
            return checkField(KEY_RCODE, JsonType::INT);
        }

        // 返回响应结果
        Json::Value result()
        {
            return _body[KEY_RESULT];
        }

        // 设置响应结果
        void setResult(const Json::Value& result)
        {
            _body[KEY_RESULT] = result;
        }
    };

    // topic 响应类
    // {
    //      rcode: xxx,
    // }
    class TopicResponse : public JsonResponse
    {
    public:
        using s_ptr = std::shared_ptr<TopicResponse>;

        virtual bool check() override
        {
            return checkField(KEY_RCODE, JsonType::INT);
        }
    };

    // service 响应类
    // 服务发现:
    // {
    //      rcode: xxx,
    //      method: xxx,
    //      optype: xxx,
    //      host: [ 
    //          { ip: xxx, port: xxx },
    //          { ip: xxx, port: xxx }
    //      ]
    // }
    // 服务注册\上线\下线: 
    // {
    //      rcode: xxx,
    //      optype: xxx
    // }
    class ServiceResponse : public JsonResponse
    {
    public:
        using s_ptr = std::shared_ptr<ServiceResponse>;

        virtual bool check() override
        {
           if (!checkField(KEY_RCODE, JsonType::INT)
            || !checkField(KEY_OPTYPE, JsonType::INT)) 
                return false;
            
            // 如果是服务发现，判断是否有方法名称和host数组
            if (_body[KEY_OPTYPE].asInt() == (int)ServiceOpType::SERVICE_DISCOVER
                && (!checkField(KEY_METHOD, JsonType::STRING)
                    || !checkField(KEY_HOST, JsonType::ARRAY)))
                return false;

            return true;
        }

        ServiceOpType optype() {
            return (ServiceOpType)_body[KEY_OPTYPE].asInt();
        }

        void setOptype(ServiceOpType optype) {
            _body[KEY_OPTYPE] = (int)optype;
        }

        // 返回服务方法
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        // 设置服务方法
        void setMethod(const std::string& method)
        {
            _body[KEY_METHOD] = method;
        }

        // 返回服务端信息
        std::vector<Address> Hosts()
        {
            std::vector<Address> addrs;
            for (int i = 0; i < _body[KEY_HOST].size(); i++)
            {
                Address addr;
                addr.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();
                addr.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();
                addrs.push_back(addr);
            }
            return addrs;
        }

        // 设置服务端信息
        void setHosts(std::vector<Address> addrs)
        {
            for (auto& addr : addrs)
            {
                Json::Value val;
                val[KEY_HOST_IP] = addr.first;
                val[KEY_HOST_PORT] = addr.second;
                _body[KEY_HOST].append(val);
            }
        }
    };

    // ------------------------------ 消息对象工厂 ------------------------------
    class MessageFactory
    {
    public:
        // 直接构造
        static BaseMessage::s_ptr create(MType mtype)
        {
            switch(mtype)
            {
                case MType::REQ_RPC:
                    return std::make_shared<RpcRequest>();
                case MType::RSP_RPC:
                    return std::make_shared<RpcResponse>();
                case MType::REQ_TOPIC:
                    return std::make_shared<TopicRequest>();
                case MType::RSP_TOPIC:
                    return std::make_shared<TopicResponse>();
                case MType::REQ_SERVICE:
                    return std::make_shared<ServiceRequest>();
                case MType::RSP_SERVICE:
                    return std::make_shared<ServiceResponse>();
            }

            return std::shared_ptr<BaseMessage>();
        }

        // 含参构造
        template <typename T, typename... Args>
        static std::shared_ptr<T> create(Args&&... args)
        {
            return std::make_shared<T>(std::forward(args)...);
        }
    };
}