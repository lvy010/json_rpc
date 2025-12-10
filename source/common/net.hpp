#pragma once

#include "util.hpp"
#include "fields.hpp"
#include "abstract.hpp"
#include "message.hpp"

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpClient.h>

#include <unordered_map>
#include <mutex>

namespace JsonRpc
{
    class MuduoBuffer : public BaseBuffer
    {
    public:
        using s_ptr = std::shared_ptr<MuduoBuffer>;

        MuduoBuffer(muduo::net::Buffer* buf)
            : _buf(buf)
        {}

        // 缓冲区可读的数据范围
        virtual size_t readableSize() override
        {
            return _buf->readableBytes();
        }

        // 尝试取出 4 byte, 但是不删除
        virtual int32_t peekInt32()override
        {
            // muduo在这一步自动完成 网络字节序转化主机字节序
            return _buf->peekInt32();
        }

        // 删除前 4 byte数据
        virtual void retrieveInt32()override
        {
            _buf->retrieveInt32();
        }

        // 取出并删除前 4 byte数据
        virtual int32_t readInt32()override
        {
            return _buf->readInt32();
        }

        // 取出指定长度的数据
        virtual std::string retrieveAsString(size_t len)override
        {
            return _buf->retrieveAsString(len);
        }

    private:
        muduo::net::Buffer* _buf;
    };

    class BufferFactory
    {
    public:
        template <typename ...Args>
        static BaseBuffer::s_ptr create(Args&& ...args)
        {
            return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...);
        }
    };

    class LVProtocol : public BaseProtocol
    {
        // | len | value |
        // | len | mtype | idlen | id | body |
    
    public:
        using s_ptr = std::shared_ptr<LVProtocol>;

        // 缓冲区是否可转化为消息
        virtual bool canProcessed(const BaseBuffer::s_ptr& buf) override
        {
            if (buf->readableSize() < totalLenfieldsize)
                return false;

            int32_t totalLen = buf->peekInt32();
            return buf->readableSize() >= totalLen + totalLenfieldsize;
        }

        // 将缓冲区转化为消息
        virtual bool onMessage(const BaseBuffer::s_ptr& buf, BaseMessage::s_ptr& msg) override
        {
            // | len | mtype | idlen | id | body |
            int32_t totalLen = buf->readInt32();
            MType mtype = (MType)buf->readInt32();
            int32_t idLen = buf->readInt32();
            std::string id = buf->retrieveAsString(idLen);

            int32_t bodyLen = totalLen - mtypefieldsize - idLenfieldsize - idLen;
            std::string body = buf->retrieveAsString(bodyLen);

            msg = MessageFactory::create(mtype);
            if (msg.get() == nullptr)
            {
                E_LOG("消息类型错误，构造消息对象失败!");
                return false;
            }

            if (!msg->unSerialize(body))
            {
                E_LOG("消息正文反序列化失败!");
                return false;
            }

            msg->setMtype(mtype);
            msg->setRid(id);
            return true;
        }

        // 序列化
        virtual std::string serialize(const BaseMessage::s_ptr& msg) override
        {
            // | len | mtype | idlen | id | body |
            int32_t mtype = htonl((int32_t)msg->mtype());
            std::string id = msg->rid();
            int32_t idLen = htonl(id.size());

            std::string body = msg->serialize();
            int32_t totalLen = mtypefieldsize + idLenfieldsize + id.size() + body.size();

            std::string result;
            result.reserve(totalLen + totalLenfieldsize);
            totalLen = htonl(totalLen);
            result.append((char*)&totalLen, totalLenfieldsize);
            result.append((char*)&mtype, mtypefieldsize);
            result.append((char*)&idLen, idLenfieldsize);
            result.append(id);
            result.append(body);

            return result;
        }
        
    private:
        static const int32_t totalLenfieldsize = 4;
        static const int32_t mtypefieldsize = 4;
        static const int32_t idLenfieldsize = 4;
    };

    class ProtocolFactory
    {
    public:
        template <typename ...Args>
        static BaseProtocol::s_ptr create(Args&& ...args)
        {
            return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
        }
    };

    class MuduoConnection : public BaseConnection
    {
    public:
        using s_ptr = std::shared_ptr<MuduoConnection>;

        MuduoConnection(muduo::net::TcpConnectionPtr conn, BaseProtocol::s_ptr proto)
            : _proto(proto)
            , _conn(conn)
        {}

        // 发送消息
        virtual void send(const BaseMessage::s_ptr& msg) override
        {
            _conn->send(_proto->serialize(msg));
        }

        // 关闭连接
        virtual void shutdown() override
        {
            _conn->shutdown();
        }

        // 检查连接
        virtual bool connected() override
        {
            return _conn->connected();
        }

    private:
        BaseProtocol::s_ptr _proto;
        muduo::net::TcpConnectionPtr _conn;
    };

    class ConnectionFactory
    {
    public:
        template <typename ...Args>
        static BaseConnection::s_ptr create(Args&& ...args)
        {
            return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
        }
    };

    // ------------------------------ 服务端 ------------------------------

    class MuduoServer : public BaseServer
    {
    public:
        using s_ptr = std::shared_ptr<MuduoServer>;

        MuduoServer(int32_t port)
            : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port), 
                    "MuduoServer", muduo::net::TcpServer::kReusePort)
            , _proto(ProtocolFactory::create())
        {
            // 触发连接回调
            _server.setConnectionCallback(std::bind(&MuduoServer::onConnection, this, std::placeholders::_1));
            // 触发消息回调
            _server.setMessageCallback(std::bind(&MuduoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        virtual void start() override
        {
            _server.start();
            _baseloop.loop();
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            if (conn->connected())
            {
                I_LOG("连接建立成功!");
                BaseConnection::s_ptr muduo_conn = ConnectionFactory::create(conn, _proto);
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _conns[conn] = muduo_conn;
                }
                if (_cb_connection) _cb_connection(muduo_conn);
            }
            else
            {
                I_LOG("连接断开!");
                BaseConnection::s_ptr muduo_conn;
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    auto it = _conns.find(conn);
                    if (it == _conns.end()) return;
                    muduo_conn = it->second;
                    _conns.erase(conn);
                }
                if (_cb_close) _cb_close(muduo_conn);
            }
        }

        void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp)
        {
            BaseBuffer::s_ptr base_buf = BufferFactory::create(buffer);
            BaseConnection::s_ptr base_conn;
            {
                std::unique_lock<std::mutex> lock(_mtx);
                auto it = _conns.find(conn);
                if (it == _conns.end())
                {
                    E_LOG("服务不存在!");
                    conn->shutdown();
                    return;
                }
                base_conn = it->second;
            }

            // 循环处理消息
            while (true)
            {
                if (!_proto->canProcessed(base_buf))
                {
                    if (base_buf->readableSize() > _maxBufferSize)
                    {
                        E_LOG("数据载荷过大!");
                        conn->shutdown();
                    }
                    break;
                }

                BaseMessage::s_ptr base_msg;
                bool ret = _proto->onMessage(base_buf, base_msg);
                if (!ret)
                {
                    conn->shutdown();
                    E_LOG("缓冲区数据错误!");
                    break;
                }

                if (_cb_message) _cb_message(base_conn, base_msg);
            }
        }

    private:
        static const int _maxBufferSize = (1 << 16);
        muduo::net::TcpServer _server;
        muduo::net::EventLoop _baseloop;
        BaseProtocol::s_ptr _proto;

        std::mutex _mtx;
        std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::s_ptr> _conns;
    };

    class ServerFactory
    {
    public:
        template <typename ...Args>
        static BaseServer::s_ptr create(Args&& ...args)
        {
            return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
        }
    };

    // ------------------------------ 客户端 ------------------------------
    class MuduoClient : public BaseClient
    {
    public:
        using s_ptr = std::shared_ptr<MuduoClient>;

        MuduoClient(const std::string& ip, int32_t port)
            : _proto(ProtocolFactory::create())
            , _loop(_loopthread.startLoop())
            , _downLatch(1)
            , _client(_loop, muduo::net::InetAddress(ip, port), "MuduoClient")
        {
            _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this, std::placeholders::_1));

            _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this, 
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        // 连接服务端
        virtual void connect() override
        {
            _client.connect();
            _downLatch.wait();
        }

        // 关闭连接
        virtual void shutdown() override
        {
            _client.disconnect();
        }

        // 发送消息
        virtual bool send(const BaseMessage::s_ptr& msg) override
        {
            if (!connected())
                return false;

            _conn->send(msg);

            return true;
        }

        // 获取连接对象
        virtual BaseConnection::s_ptr getConnection() override
        {
            return _conn;
        }

        // 判断连接是否正常
        virtual bool connected() override
        {
            return _conn && _conn->connected();
        }

    private:
        //连接处理函数  
        void onConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            if (conn->connected())
            {
                I_LOG("建立连接成功");
                _downLatch.countDown(); // 计数自减,此时变为0,不再阻塞
                _conn = ConnectionFactory::create(conn, _proto);
            }
            else
            {
                I_LOG("建立连接失败");
                _conn.reset();
            }
        }

        // 消息处理函数
        void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
        {
            BaseBuffer::s_ptr base_buf = BufferFactory::create(buf);
            while (true)
            {
                if (!_proto->canProcessed(base_buf))
                {
                    if (base_buf->readableSize() > _maxBufferSize)
                    {
                        E_LOG("数据载荷过大!");
                        conn->shutdown();
                    }
                    I_LOG("数据量不足,当前 %ld", base_buf->readableSize());
                    break;
                }

                BaseMessage::s_ptr msg;
                bool ret = _proto->onMessage(base_buf, msg);
                if (!ret)
                {
                    conn->shutdown();
                    E_LOG("缓冲区数据错误!");
                    break;
                }
                if (_cb_message) _cb_message(_conn, msg);
            }
        }

    private:
        static const int _maxBufferSize = (1 << 16);

        BaseProtocol::s_ptr _proto;
        BaseConnection::s_ptr _conn;
        muduo::net::EventLoopThread _loopthread;
        muduo::net::EventLoop* _loop;
        muduo::CountDownLatch _downLatch;
        muduo::net::TcpClient _client;
    };

    class ClientFactory
    {
    public:
        template <typename ...Args>
        static BaseClient::s_ptr create(Args&& ...args)
        {
            return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
        }
    };
}