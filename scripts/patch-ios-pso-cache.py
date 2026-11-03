#!/usr/bin/env python3
from pathlib import Path
import shutil
root=Path('upstream-runtime/gpu/shadps4/video_core/renderer_metal')
for name in ('pso_shards','pso_cache_policy'):
    shutil.copyfile(Path('native')/(name+'.h'),root/('bb_ios_'+name+'.h'))
p=root/'mtl_pso_cache.mm';s=p.read_text()
s='#include "bb_ios_pso_shards.h"\n#include <optional>\n'+s
needle='    std::string last_error;       ///< guarded by flush_mutex: logged once per distinct error'
assert s.count(needle)==1
s=s.replace(needle,needle+'\n    std::unique_ptr<BbIosPso::Shards> ios_shards; ///< destroyed first; joins its writer')
needle='    archive_path = cache_dir / "pipelines.binaryarchive";'
assert s.count(needle)==1
s=s.replace(needle,needle+'\n    impl->ios_shards=std::make_unique<BbIosPso::Shards>(impl->mtl_device,cache_dir);')
needle='''    from_archive = false;
    NSError* ns_error = nil;'''
assert s.count(needle)==1
s=s.replace(needle,'''    if(impl.ios_shards->Enabled())
        return impl.ios_shards->CreatePipeline(descriptor,create,from_archive,error);
'''+needle)
for kind,cache,hash_,member in [(1,'render','RenderMapHash','state'),(2,'compute','ComputeMapHash','state'),(3,'tessellation','TessMapHash','render'),(4,'geometry','GeometryMapHash','render')]:
    block=f'''    {{
        std::shared_lock lock{{impl->mutex}};
        const auto it = impl->{cache}.find(map_key);
        if (it != impl->{cache}.end()) {{
            return it->second->{member} ? it->second.get() : nullptr;
        }}
    }}'''
    assert s.count(block)==1,cache
    extra=f'''\n    std::optional<BbIosPso::Flights::Lease> ios_flight;
    if(impl->ios_shards->Enabled()) {{
        ios_flight.emplace(impl->ios_shards->flights.Acquire({kind},{hash_}{{}}(map_key)));
        // The first caller may have finished while this caller waited.
{block}
    }}'''
    s=s.replace(block,block+extra)
needle='            desc.label = [NSString stringWithUTF8String:StageHashes(key).c_str()];'
assert s.count(needle)==1
s=s.replace(needle,needle+'''
            desc.label=[desc.label stringByAppendingFormat:@" cache%llx attrs%llx flags%u%u%u",
                (unsigned long long)std::hash<Vulkan::GraphicsPipelineKey>{}(key),
                (unsigned long long)map_key.attributes_hash,
                unsigned(input.rect_list_triangles),unsigned(input.tessellation_eval),unsigned(input.geometry)];''')
p.write_text(s)
print('iOS PSO: bounded per-pipeline archives, quiet-period writes, exact-map recheck after same-key wait')
