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
    std::promise<int> pro;
    std::future<int> ret = pro.get_future();

    std::thread thr([&pro](){
        int sum = add(11, 22);
        pro.set_value(sum);
    });

    std::cout << ret.get() << std::endl;
    thr.join();
    return 0;
}