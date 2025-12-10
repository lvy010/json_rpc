#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h> // 让一个线程去跑事件循环
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>

#include <iostream>
#include <string>

class DictClient
{
public:
    DictClient(const std::string& ip, int port)
        : _baseLoop(_loopThread.startLoop()) // 在线程内部开启事件循环，并返回指向事件循环对象的指针
        //         事件循环                    服务端地址、端口    客户端名称
        , _downLatch(1) // 计数初始化为1,发生阻塞
        , _client(_baseLoop, muduo::net::InetAddress(ip, port), "DictClient")
    {
        // 设置连接处理函数
        _client.setConnectionCallback(std::bind(&DictClient::onConnection, this, std::placeholders::_1));
        // 设置事件处理函数
        _client.setMessageCallback(std::bind(&DictClient::onMessage, this,
         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        _client.connect(); // 连接服务器 -> 非阻塞接口!!!
        _downLatch.wait(); // 阻塞，直到连接建立完毕

        // _baseLoop.loop(); 客户端不能直接使用 EventLoop事件循环 因为其会让整个进程陷入阻塞
        //                   这就导致无法调用send这样的接口发送数据，只能靠事件循环收到消息时
        //                   定义的回调函数，来处理业务
        //                   因此要把事件循环交给一个线程来跑，不阻塞主线程，主线程可以处理其它业务
        //                   muduo 提供了线程版本的事件循环 EventLoopThead
        //                   开启循环后，创建一个线程跑 EventLoop, 并返回指向EventLoop的指针
    }

    bool send(const std::string& msg)
    {
        if (_conn->connected() == false)
            return false; // 连接断开

        _conn->send(msg);
        return true;
    }

private:
    //连接处理函数
    void onConnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            std::cout << "建立连接成功" << std::endl;
            _downLatch.countDown(); // 计数自减,此时变为0,不再阻塞
            _conn = conn; // 设置TCP连接类（内含send接口，进行数据发送）
        }
        else
        {
            std::cout << "建立连接失败" << std::endl;
            _conn.reset();
        }
    }

    // 消息处理函数
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
    {
        std::string ret = buf->retrieveAllAsString();
        std::cout << "翻译结果为: " << ret << std::endl;
    }

private:
    muduo::net::TcpConnectionPtr _conn;
    muduo::CountDownLatch _downLatch; // 这个类直接在muduo命名空间
    muduo::net::EventLoopThread _loopThread;
    muduo::net::EventLoop* _baseLoop;
    muduo::net::TcpClient _client;
};

int main()
{
    DictClient client("127.0.0.1", 8888);

    while (true)
    {
        std::string str;
        std::cout << "enter:";
        std::cin >> str;
        client.send(str);
    }

    return 0;
}
