#include "tess_inputs.h"
#include <cassert>
#include <iostream>
using V=std::vector<std::uint32_t>;
int main(){
 V module={0x07230203,0x10000,0,50,0,
  (4u<<16)|15,2,10,0,
  (3u<<16)|71,20,15,(3u<<16)|71,21,15,
  (4u<<16)|32,30,1,31,
  (4u<<16)|59,30,20,1,(4u<<16)|59,30,21,1,
  (5u<<16)|54,1,10,0,2,(2u<<16)|248,11,
  (4u<<16)|59,30,40,7,(1u<<16)|253,(1u<<16)|56};
 auto out=BbIosTess::TouchPatchInputs(module);assert(out.size()==module.size()+8&&out[3]==52);
 size_t loads=0;bool past_local=false;
 for(size_t i=5;i<out.size();){auto n=out[i]>>16,op=out[i]&0xffff;assert(n&&i+n<=out.size());
  if(op==59&&out[i+2]==40)past_local=true;
  if(op==61){assert(past_local&&n==4&&out[i+1]==31);assert(out[i+2]==50+loads&&out[i+3]==20+loads);++loads;}i+=n;
 }assert(loads==2);
 V plain={0x07230203,0x10000,0,1,0};assert(BbIosTess::TouchPatchInputs(plain)==plain);
 auto broken=module;broken[5]=0;assert(BbIosTess::TouchPatchInputs(broken)==broken);
 broken=module;broken[broken.size()-6]=59;assert(BbIosTess::TouchPatchInputs(broken)==broken);
 std::cout<<"PASS: all patch members retained, unique result IDs, loads follow locals, non-patch and malformed modules preserved\n";
}
