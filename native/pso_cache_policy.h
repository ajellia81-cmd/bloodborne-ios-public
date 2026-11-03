#pragma once
#include <cstdint>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
namespace BbIosPso {
inline constexpr std::uintmax_t MaxEntry=1024*1024,MaxDisk=128*1024*1024;
inline constexpr std::size_t MaxFiles=2048,MaxQueued=8,MinHeadroom=512*1024*1024;
inline bool Eligible(std::uintmax_t n){return n>0&&n<=MaxEntry;}
inline bool CanQueue(std::size_t n){return n<MaxQueued;}
class Flights {
    std::mutex mutex;
    std::map<std::pair<int,std::uint64_t>,std::weak_ptr<std::mutex>> active;
public:
    struct Lease {
        std::shared_ptr<std::mutex> owner;
        std::unique_lock<std::mutex> lock;
        explicit Lease(std::shared_ptr<std::mutex> p):owner(std::move(p)),lock(*owner){}
    };
    Lease Acquire(int kind,std::uint64_t key){
        std::shared_ptr<std::mutex> p;
        {
            std::scoped_lock guard{mutex};
            if(active.size()>256) for(auto i=active.begin();i!=active.end();){
                if(i->second.expired()) i=active.erase(i);else ++i;
            }
            auto &entry=active[{kind,key}];p=entry.lock();
            if(!p){p=std::make_shared<std::mutex>();entry=p;}
        }
        return Lease{std::move(p)};
    }
};
}
