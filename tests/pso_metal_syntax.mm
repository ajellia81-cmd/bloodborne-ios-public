// Compile-only fixture: instantiate both descriptor templates with the real iOS Metal SDK.
#include "pso_shards.h"
void instantiate_pso_cache(BbIosPso::Shards& cache,MTLRenderPipelineDescriptor* r,MTLComputePipelineDescriptor* c){
    bool hit=false;std::string error;
    auto create=[](MTLPipelineOption,NSError**)->id{return nil;};
    (void)cache.CreatePipeline(r,create,hit,error);
    (void)cache.CreatePipeline(c,create,hit,error);
}
