#pragma once

#include "net.hpp"
#include "message.hpp"

#include <mutex>
#include <memory>
#include <unordered_map>

namespace JsonRpc
{ 
    // 模板化回调处理
    
    // 基类
    class Callback
    {
    public:
        using s_ptr = std::shared_ptr<Callback>;

        virtual void onMessage(const BaseConnection::s_ptr& conn, BaseMessage::s_ptr& msg) = 0;
    };

    // 多个模板派生类
    template <typename T>
    class CallbackT : public Callback
    {
    public:
        using s_ptr = std::shared_ptr<CallbackT<T>>;
        // 回调函数类型， 模板参数 T 定义为接收的 message 派生类
        using MessageCallback = std::function<void (const BaseConnection::s_ptr& conn, std::shared_ptr<T>& msg)>;

        CallbackT(const MessageCallback& handler)
            : _handler(handler)
        {}
 
        void onMessage(const BaseConnection::s_ptr& conn, BaseMessage::s_ptr& msg) override
        {
            auto type_msg = std::dynamic_pointer_cast<T>(msg); // 从基类转化为派生类
            if (!type_msg) {
                E_LOG("消息类型转换失败!");
                conn->shutdown();
                return;
            }
            
            _handler(conn, type_msg); // 使用派生的msg调用回调函数
        }

    private:
        MessageCallback _handler;
    };

    class Dispatcher
    {
    public:
        using s_ptr = std::shared_ptr<Dispatcher>;

        Dispatcher() = default;

        template <typename T>
        void registerHandler(MType mtype, const typename CallbackT<T>::MessageCallback& handler)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            auto cb = std::make_shared<CallbackT<T>>(handler);
            _handlers[mtype] = cb;
        }

        void onMessage(const BaseConnection::s_ptr& conn, BaseMessage::s_ptr& msg)
        {
            I_LOG("dispatcher 收到消息, MType: %d!", (int)msg->mtype());

            std::unique_lock<std::mutex> lock(_mtx);

            if (_handlers.count(msg->mtype()) == 0)
            {
                E_LOG("消息类型 %d 不存在!", (int)msg->mtype());
                conn->shutdown();
                return;
            }
            
            //             cb对象    回调函数
            _handlers[msg->mtype()]->onMessage(conn, msg);
        }

    private:
        std::mutex _mtx;
        std::unordered_map<MType, Callback::s_ptr> _handlers;
        // msg的派生类无法统一使用模板处理，进而无法放到一个模板容器中
        // 为此，额外封装一个 callbackT<T> 类，给 dispatcher 传参时传入 basemessage 基类
        // 在 callbackT<T> 中把 basemessage 向下转型为派生类
        // 为了进一步统一模板 callbackT<T>，将其继承于 callback，哈希表中存储 callback 的指针
        // 父类指针指向子类对象，形成多态，可以进一步调用子类方法
    };
}