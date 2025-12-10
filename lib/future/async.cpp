#include <iostream>
#include <future>
#include <thread>

int add(int a, int b)
{
    std::cout << "calculate: " << a << " + " << b << std::endl;
    return a + b;
}

int main()
{
    // 进行一个异步的非阻塞调用
    std::future<int> res = std::async(std::launch::deferred, add, 11, 22);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "--------------" << std::endl;
    std::cout << "结果为: " << res.get() << std::endl; // 获取异步任务结果，如果还没有结果，就会阻塞
    
    return 0;
}