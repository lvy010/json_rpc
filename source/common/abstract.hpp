#pragma once

#include <string>
#include <memory>
#include <functional>
#include "fields.hpp"

namespace JsonRpc
{
    // 消息基类
    // {
    //      id: xxx,
    //      mtype: xxx   
    // }
    class BaseMessage
    {
    public:
        using s_ptr = std::shared_ptr<BaseMessage>;

        virtual ~BaseMessage() = default;

        virtual std::string rid() { return _rid; };
        virtual void setRid(std::string rid) { _rid = rid; }

        virtual MType mtype() { return _mtype; };
        virtual void setMtype(MType mtype) { _mtype = mtype; }

        // 序列化
        virtual std::string serialize() = 0;
        // 反序列化
        virtual bool unSerialize(const std::string& msg) = 0;
        // 检查 BaseMessage 字段
        virtual bool check() = 0;

    private:
        MType _mtype; // 消息类型
        std::string _rid; // 消息uuid
    };

    // 缓冲区基类
    class BaseBuffer 
    {
    public:
        using s_ptr = std::shared_ptr<BaseBuffer>;

        // 缓冲区可读的数据范围
        virtual size_t readableSize() = 0; 
        // 尝试取出 4 byte, 但是不删除
        virtual int32_t peekInt32() = 0; 
        // 删除前 4 byte数据
        virtual void retrieveInt32() = 0;
        // 取出并删除前 4 byte数据
        virtual int32_t readInt32() = 0;
        // 取出指定长度的数据
        virtual std::string retrieveAsString(size_t len) = 0;
    };

    // 协议基类
    class BaseProtocol
    {
    public:
        using s_ptr = std::shared_ptr<BaseProtocol>;

        // 缓冲区是否可转化为消息
        virtual bool canProcessed(const BaseBuffer::s_ptr& buf) = 0;
        // 将缓冲区转化为消息
        virtual bool onMessage(const BaseBuffer::s_ptr& buf, BaseMessage::s_ptr& msg) = 0;
        // 序列化
        virtual std::string serialize(const BaseMessage::s_ptr& msg) = 0;
    };

    // 连接基类
    class BaseConnection
    {
    public:
        using s_ptr = std::shared_ptr<BaseConnection>;
        // 发送消息
        virtual void send(const BaseMessage::s_ptr& msg) = 0;
        // 关闭连接
        virtual void shutdown() = 0;
        // 检查连接
        virtual bool connected() = 0;
    };

    // 回调函数
    // 连接建立成功
    using ConnectionCallback = std::function<void(const BaseConnection::s_ptr&)>;
    // 连接关闭
    using CloseCallback = std::function<void(const BaseConnection::s_ptr&)>;
    // 消息到达
    using MessageCallback = std::function<void(const BaseConnection::s_ptr&, BaseMessage::s_ptr&)>;

    // 服务基类
    class BaseServer
    {
    public:
        using s_ptr = std::shared_ptr<BaseServer>;

        virtual void setConnectionCallback(const ConnectionCallback& cb_connection) 
        {
            _cb_connection = cb_connection;
        }

        virtual void setCloseCallback(const CloseCallback& cb_close) 
        {
            _cb_close = cb_close;
        }

        virtual void setMessageCallback(const MessageCallback& cb_message) 
        {
            _cb_message = cb_message;
        }

        virtual void start() = 0;

    protected:
        ConnectionCallback _cb_connection;
        CloseCallback _cb_close;
        MessageCallback _cb_message;
    };

    // 客户基类
    class BaseClient
    {
    public:
        using s_ptr = std::shared_ptr<BaseClient>;

        virtual void setConnectionCallback(const ConnectionCallback& cb_connection) 
        {
            _cb_connection = cb_connection;
        }

        virtual void setCloseCallback(const CloseCallback& cb_close) 
        {
            _cb_close = cb_close;
        }

        virtual void setMessageCallback(const MessageCallback& cb_message) 
        {
            _cb_message = cb_message;
        }

        // 连接服务端
        virtual void connect() = 0;
        // 关闭连接
        virtual void shutdown() = 0;
        // 发送消息
        virtual bool send(const BaseMessage::s_ptr&) = 0;
        // 获取连接对象
        virtual BaseConnection::s_ptr getConnection() = 0;
        // 判断连接是否正常
        virtual bool connected() = 0;

    protected:
        ConnectionCallback _cb_connection;
        CloseCallback _cb_close;
        MessageCallback _cb_message;
    };
}
