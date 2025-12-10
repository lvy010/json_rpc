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
    std::packaged_task<int(int, int)> task(add);
    std::future<int> ret = task.get_future();
    
    std::thread thr(std::move(task), 11, 22);
    std::cout << "结果为: " << ret.get() << std::endl;

    thr.join();
    return 0;
}