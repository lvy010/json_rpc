#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>

#include <iostream>
#include <string>
#include <unordered_map>

class DictServer
{
public:
    DictServer(int port)
        //        事件循环                             监听地址、端口   
        : _server(&_baseLoop, muduo::net::InetAddress("0.0.0.0", port), 
        "DictServer", muduo::net::TcpServer::kReusePort)
        // 服务名                            地址复用(可选)
    {
        // 设置连接处理函数
        _server.setConnectionCallback(std::bind(&DictServer::onConnection, this, std::placeholders::_1));
        // 设置事件处理函数
        _server.setMessageCallback(std::bind(&DictServer::onMessage, this,
         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void start()
    {
        _server.start();  // 先启动服务
        _baseLoop.loop(); // 后开启事件循环
    }

private:
    // 连接处理函数                                        连接
    void onConnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if (conn->connected())
            std::cout << "建立连接成功" << std::endl;
        else
            std::cout << "建立连接失败" << std::endl;
    }

    // 消息处理函数                                   连接类                  缓冲区          时间 
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
    {
        static std::unordered_map<std::string, std::string> dict_map = {
            {"hello", "你好"},
            {"world", "世界"},
            {"C++", "一款面向对象语言"},
            {"Linux", "开源操作系统"},
            {"AI", "人工智能"}
        };

        std::string msg = buf->retrieveAllAsString(); // 获取所有数据
        std::string ret;
        auto it = dict_map.find(msg);
        if (it != dict_map.end())
            ret = it->second;
        else
            ret = "未知单词";

        conn->send(ret); // 发送数据
    }

private:
    muduo::net::EventLoop _baseLoop;
    muduo::net::TcpServer _server;
};

int main()
{
    DictServer server(8888);
    server.start();

    return 0;
}