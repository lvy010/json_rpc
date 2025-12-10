#pragma once

#include <string>
#include <unordered_map>

namespace JsonRpc
{
    // 请求字段
    const static std::string KEY_METHOD = "method";       // 方法名称
    const static std::string KEY_PARAMS = "parameters";   // 方法参数
    const static std::string KEY_TOPIC_KEY = "topic_key"; // 主题名称
    const static std::string KEY_TOPIC_MSG = "topic_msg"; // 主题消息
    const static std::string KEY_OPTYPE = "optype";       // 操作类型
    const static std::string KEY_HOST = "host";           // 主机信息
    const static std::string KEY_HOST_IP = "ip";          // ip地址
    const static std::string KEY_HOST_PORT = "port";      // 端口号

    // 响应字段
    const static std::string KEY_RCODE = "retcode";  // 响应码
    const static std::string KEY_RESULT = "result"; // 响应结果
    
    // 消息类型定义
    enum class MType 
    {
        // PRC请求响应
        REQ_RPC = 0,
        RSP_RPC,
        // 主题操作请求响应
        REQ_TOPIC,
        RSP_TOPIC,
        // 服务操作请求响应
        REQ_SERVICE,
        RSP_SERVICE
    };

    // 响应码定义
    enum class RetCode
    {
        RCODE_OK = 0,
        RCODE_PARSE_FAILED,      // 解析失败
        RCODE_INVALID_MSG,       // 消息字段无效
        RCODE_DISCONNECTED,      // 连接断开
        RCODE_INVALID_PARAM,     // rpc参数无效
        RCODE_INVALID_OPTYPE,    // 操作类型无效
        RCODE_NOT_FOUND_SERVICE, // 服务不存在
        RCODE_NOT_FOUND_TOPIC,   // 主题不存在
        RCODE_INHTERNAL_ERROR    // 服务内部错误
    };

    static std::string errReason(RetCode code)
    {
        static std::unordered_map<RetCode, std::string> err_map = {
            {RetCode::RCODE_OK, "成功"}, 
            {RetCode::RCODE_PARSE_FAILED, "消息解析失败"}, 
            {RetCode::RCODE_INVALID_MSG, "无效消息"}, 
            {RetCode::RCODE_DISCONNECTED, "连接断开"}, 
            {RetCode::RCODE_INVALID_PARAM, "无效的RPC参数"}, 
            {RetCode::RCODE_INVALID_OPTYPE, "无效的操作类型"}, 
            {RetCode::RCODE_NOT_FOUND_SERVICE, "没有找到服务"}, 
            {RetCode::RCODE_NOT_FOUND_TOPIC, "没有找到主题"},
            {RetCode::RCODE_INHTERNAL_ERROR, "服务内部错误"}
        };

        return err_map.count(code) ? err_map[code] : "未知错误";
    }

    // 请求类型定义
    enum class ReqType
    {
        REQ_ASYNC = 0,    // 异步请求
        REQ_CALLBACK  // 回调请求
    };

    // 主题操作类型
    enum class TopicOpType
    {
        TOPIC_CREATE = 0, // 主题创建
        TOPIC_REMOVE,     // 主题删除
        TOPIC_SUBSCRIBE,  // 主题订阅
        TOPIC_CANCEL,     // 主题取消订阅
        TOPIC_PUBLISH     // 发布消息
    };

    // 服务操作类型
    enum class ServiceOpType
    {
        SERVICE_REGISTRY = 0, // 服务注册
        SERVICE_DISCOVER,     // 服务发现
        SERVICE_ONLINE,       // 主题上线
        SERVICE_OUTLINE,      // 主题下线
        SERVICE_UNKNOW        // 未知错误
    };
}