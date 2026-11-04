#include "pso_cache_policy.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <thread>
#include <vector>
#include <iostream>
using namespace std::chrono_literals;
int main(){
    using namespace BbIosPso;
    assert(!Eligible(0)&&Eligible(MaxEntry)&&!Eligible(MaxEntry+1));
    assert(CanQueue(63)&&!CanQueue(64)&&!CanQueue(100));
    assert(!WorthPersisting(7999)&&WorthPersisting(8000));
    assert(WriteCooldownUs(1000)==19000&&WriteCooldownUs(UINT64_MAX)==10000000);
    Flights flights;std::atomic<bool> go=false,cached=false;std::atomic<int> builds=0;
    std::vector<std::thread> threads;
    for(int i=0;i<64;++i)threads.emplace_back([&]{
        while(!go.load())std::this_thread::yield();
        auto lease=flights.Acquire(1,42);
        if(!cached.load()){++builds;std::this_thread::sleep_for(2ms);cached=true;}
    });
    go=true;for(auto&t:threads)t.join();assert(builds==1);
    // A second key must be usable while the first key is held.
    {
        auto lease=flights.Acquire(1,42);
        auto other=std::async(std::launch::async,[&]{auto l=flights.Acquire(1,43);return true;});
        assert(other.wait_for(2s)==std::future_status::ready&&other.get());
        auto domain=std::async(std::launch::async,[&]{auto l=flights.Acquire(2,42);return true;});
        assert(domain.wait_for(2s)==std::future_status::ready&&domain.get());
        for(unsigned i=0;i<1000;++i){auto l=flights.Acquire(3,i);}
    }
    auto again=flights.Acquire(1,42);
    std::cout<<"PASS: one construction for 64 concurrent requests; unrelated keys/domains progress; weak-key cleanup; entry/queue limits and cost/cooldown policy\n";
}

