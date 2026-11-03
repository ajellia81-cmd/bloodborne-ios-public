#pragma once
#import <Metal/Metal.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <os/proc.h>
#endif
#include <atomic>
#include <deque>
#include <filesystem>
#include <fstream>
#include <condition_variable>
#include <thread>
#include <optional>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <type_traits>
#include <string>
#include <chrono>
#include <pthread.h>
#include <xxhash.h>
#include "bb_ios_pso_cache_policy.h"
namespace BbIosPso {
class Shards {
    struct Job{std::filesystem::path path;id descriptor=nil;};
    id<MTLDevice> device;
    std::filesystem::path root;
    std::mutex load_mutex,queue_mutex,report_mutex;
    std::condition_variable cv;
    std::deque<Job> queue;
    bool stop=false,on=false;
    unsigned building=0;
    std::thread writer;
    using Clock=std::chrono::steady_clock;
    Clock::time_point last_creation=Clock::now(),last_report=Clock::now();
    std::atomic<unsigned long long> hits{0},misses{0},writes{0},dropped{0},rejected{0};
    static bool Headroom(){
#if TARGET_OS_IPHONE
        return os_proc_available_memory()>=MinHeadroom;
#else
        return false;
#endif
    }
    void Report(){
        std::scoped_lock lock{report_mutex};auto now=Clock::now();
        if(now-last_report<std::chrono::seconds(5))return;
        last_report=now;
        std::printf("iOS PSO cache: hits=%llu misses=%llu writes=%llu dropped=%llu rejected=%llu (cumulative)\n",hits.load(),misses.load(),writes.load(),dropped.load(),rejected.load());
    }
    void Sweep(){
        struct Entry{std::filesystem::path path;std::uintmax_t size;std::filesystem::file_time_type time;};
        std::vector<Entry> entries;std::uintmax_t total=0;std::error_code ec;
        for(std::filesystem::directory_iterator i(root,ec),end;!ec&&i!=end;i.increment(ec)){
            if(i->path().extension()!=".bin")continue;
            auto n=i->file_size(ec);if(ec){ec.clear();continue;}
            auto t=i->last_write_time(ec);if(ec){ec.clear();continue;}
            entries.push_back({i->path(),n,t});total+=n;
        }
        std::sort(entries.begin(),entries.end(),[](auto&a,auto&b){return a.time<b.time;});auto count=entries.size();
        for(auto&e:entries){
            if(total<=MaxDisk&&count<=MaxFiles)break;
            if(std::filesystem::remove(e.path,ec)){total-=e.size;--count;}ec.clear();
        }
    }
    void Loop(){
        pthread_setname_np("bb:iOSPsoCache");pthread_set_qos_class_self_np(QOS_CLASS_UTILITY,0);
        for(;;){
            Job job;
            {
                std::unique_lock lock{queue_mutex};cv.wait(lock,[&]{return stop||!queue.empty();});
                // Cache writes start after a quiet interval to avoid competing with new PSOs.
                while(!stop&&(building||Clock::now()<last_creation+std::chrono::seconds(2))){
                    if(building)cv.wait(lock);else cv.wait_until(lock,last_creation+std::chrono::seconds(2));
                }
                if(stop)return;
                job=std::move(queue.front());queue.pop_front();
            }
            if(!Headroom()){++dropped;continue;}
            @autoreleasepool{
                NSError* error=nil;
                id<MTLBinaryArchive> archive=[device newBinaryArchiveWithDescriptor:[MTLBinaryArchiveDescriptor new] error:&error];
                bool ok=false;
                if(archive){
                    if([job.descriptor isKindOfClass:[MTLRenderPipelineDescriptor class]])ok=[archive addRenderPipelineFunctionsWithDescriptor:(MTLRenderPipelineDescriptor*)job.descriptor error:&error];
                    else ok=[archive addComputePipelineFunctionsWithDescriptor:(MTLComputePipelineDescriptor*)job.descriptor error:&error];
                }
                std::error_code ec;auto temp=job.path;temp+=".tmp";
                if(ok)ok=[archive serializeToURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:temp.c_str()]] error:&error];
                auto n=ok?std::filesystem::file_size(temp,ec):0;
                if(ok&&!ec&&Eligible(n)){
                    std::filesystem::rename(temp,job.path,ec);if(!ec)++writes;else ++dropped;
                }else ++rejected;
                std::filesystem::remove(temp,ec);
            }
            Sweep();Report();
        }
    }
    template<typename Descriptor>std::string Label(Descriptor* desc){
        std::string out=desc.label?desc.label.UTF8String:"unnamed";
        if constexpr(std::is_same_v<Descriptor,MTLComputePipelineDescriptor>){
            out+="/compute/";out+=desc.computeFunction.name?desc.computeFunction.name.UTF8String:"none";
            std::vector<std::uint64_t> values;
            auto add=[&](auto... v){(values.push_back(static_cast<std::uint64_t>(v)),...);};
            add(desc.maxTotalThreadsPerThreadgroup,desc.threadGroupSizeIsMultipleOfThreadExecutionWidth,desc.supportIndirectCommandBuffers);
            auto sid=desc.stageInputDescriptor;
            if(sid){
                values.push_back(sid.indexBufferIndex);values.push_back(sid.indexType);
                for(unsigned i=0;i<31;++i){
                    auto a=sid.attributes[i];auto l=sid.layouts[i];
                    add(a.format,a.offset,a.bufferIndex,l.stride,l.stepFunction,l.stepRate);
                }
            }
            out+=std::to_string(XXH3_64bits(values.data(),values.size()*sizeof(values[0])));
        }else out+="/render";
        return out;
    }
public:
    Flights flights;
    Shards(id<MTLDevice> d,const std::filesystem::path& cache):device(d){
#if TARGET_OS_IPHONE
        const char *env=std::getenv("BB_IOS_PSO_SHARDS"),*validation=std::getenv("MTL_SHADER_VALIDATION");
        if(!env||env[0]!='1'||(validation&&validation[0]=='1'))return;
        std::string identity=std::string(d.name.UTF8String)+NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String;
        char version[96];std::snprintf(version,sizeof(version),"pso-shards-14db-ios-v1-%llx",(unsigned long long)XXH3_64bits(identity.data(),identity.size()));
        root=cache/version;std::error_code ec;std::filesystem::create_directories(root,ec);if(ec)return;
        for(std::filesystem::directory_iterator i(root,ec),end;!ec&&i!=end;i.increment(ec)){
            auto path=i->path();
            if(path.extension()==".loading"){
                path.replace_extension(".bin");std::filesystem::remove(path,ec);ec.clear();std::filesystem::remove(i->path(),ec);ec.clear();
            }else if(path.extension()==".tmp"){std::filesystem::remove(path,ec);ec.clear();}
        }
        Sweep();on=true;writer=std::thread{[this]{Loop();}};
        std::puts("iOS PSO cache: bounded shards enabled; entry <=1 MiB, disk budget 128 MiB/2048 entries, queue <=64; no bulk archive loaded");
#else
        (void)cache;
#endif
    }
    ~Shards(){
        {std::scoped_lock lock{queue_mutex};stop=true;}cv.notify_all();if(writer.joinable())writer.join();
    }
    bool Enabled()const{return on;}
    template<typename Descriptor,typename Create>id CreatePipeline(Descriptor* desc,Create create,bool& hit,std::string& error){
        hit=false;id state=nil;NSError* ns_error=nil;auto label=Label(desc);
        char name[32];std::snprintf(name,sizeof(name),"%016llx.bin",(unsigned long long)XXH3_64bits(label.data(),label.size()));const auto path=root/name;
        if(Headroom()){
            std::scoped_lock one_loaded_archive{load_mutex};std::error_code ec;auto size=std::filesystem::file_size(path,ec);
            if(!ec&&Eligible(size)){
                @autoreleasepool{
                    auto marker=path;marker.replace_extension(".loading");std::ofstream record(marker);bool marked=bool(record);record.close();
                    if(marked){
                        MTLBinaryArchiveDescriptor* ad=[MTLBinaryArchiveDescriptor new];ad.url=[NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
                        id<MTLBinaryArchive> archive=[device newBinaryArchiveWithDescriptor:ad error:&ns_error];
                        if(archive){desc.binaryArchives=@[archive];state=create(MTLPipelineOptionFailOnBinaryArchiveMiss,&ns_error);}
                        desc.binaryArchives=nil;std::filesystem::remove(marker,ec);
                    }
                }
                if(!state){std::filesystem::remove(path,ec);++rejected;}else{hit=true;++hits;std::filesystem::last_write_time(path,std::filesystem::file_time_type::clock::now(),ec);}
            }else if(!ec){std::filesystem::remove(path,ec);++rejected;}
        }
        if(!state){
            {std::scoped_lock lock{queue_mutex};++building;last_creation=Clock::now();cv.notify_one();}
            ++misses;ns_error=nil;desc.binaryArchives=nil;state=create(MTLPipelineOptionNone,&ns_error);
            std::scoped_lock lock{queue_mutex};--building;last_creation=Clock::now();
            if(state&&Headroom()&&CanQueue(queue.size()))queue.push_back({path,[desc copy]});else ++dropped;
            cv.notify_one();
        }
        if(!state)error=ns_error?ns_error.localizedDescription.UTF8String:"Metal pipeline creation failed";
        Report();return state;
    }
};
}
