// export LD_LIBRARY_PATH=$HOME/local/lib:$LD_LIBRARY_PATH
// g++ -Wall -pedantic -std=c++1y -I$HOME/local/include -L$HOME/local/lib folly_test.cpp -lfolly -lpthread -lglog

#include <folly/futures/Future.h>
#include <folly/wangle/concurrent/CPUThreadPoolExecutor.h>

#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
    folly::wangle::CPUThreadPoolExecutor gExecutor(std::thread::hardware_concurrency());
    auto gTid = []() -> auto { return std::this_thread::get_id(); };
}

void foo(int x)
{
    // do something with x
    std::cout << gTid() << " foo(" << x << ")" << std::endl;

    throw std::runtime_error("panic-panic!");
}

int main()
{
    // ...
    std::cout << gTid() << " making Promise" << std::endl;

    folly::Promise<int> p;
    auto f = p.getFuture().via(&gExecutor).then(
        [](int x)
        {
            std::cout << gTid() << " bar(" << x << ")" << std::endl;
            return x * 2;
        }
     )
     .then(foo)
     .onError([](const std::exception & e)
        {
            std::cerr << gTid() << " Caught an exception: " << e.what() << std::endl;
        });

    std::cout << gTid() << " Future chain made" << std::endl;

    // ... now perhaps in another event callback
    std::cout << gTid() << " fulfilling Promise" << std::endl;
    p.setValue(42);
    std::cout << gTid() << " Promise fulfilled" << std::endl;

    while (!f.isReady())
        std::this_thread::yield();

    return 0;
}
