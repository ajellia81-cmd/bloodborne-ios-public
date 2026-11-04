// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from bloodborne-native/bloodborne-runtime 72148df82255df107fc88bdfec2e22928c324f9d.
// Keep all hull patch members in the Metal domain input layout, including unused ones.
#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <limits>
namespace BbIosTess {
inline std::vector<std::uint32_t> TouchPatchInputs(std::span<const std::uint32_t> spirv){
    using u32=std::uint32_t;
    std::vector<u32> words(spirv.begin(),spirv.end());if(words.size()<5)return words;
    constexpr u32 Entry=15,Pointer=32,Function=54,Variable=59,Load=61,Decorate=71,Label=248,Patch=15,Input=1;
    std::unordered_set<u32> patch;std::unordered_map<u32,u32> pointee;
    std::vector<std::pair<u32,u32>> inputs;u32 entry=0;size_t insert=0;bool in_entry=false;
    for(size_t at=5;at<words.size();){
        const u32 n=words[at]>>16,op=words[at]&0xffff;
        if(n==0||n>words.size()-at)return {spirv.begin(),spirv.end()};
        if(op==Entry&&n>=3)entry=words[at+2];
        else if(op==Decorate&&n>=3&&words[at+2]==Patch)patch.insert(words[at+1]);
        else if(op==Pointer&&n==4&&words[at+2]==Input)pointee[words[at+1]]=words[at+3];
        else if(op==Variable&&n>=4&&words[at+3]==Input&&patch.contains(words[at+2])){
            if(auto i=pointee.find(words[at+1]);i!=pointee.end())inputs.emplace_back(words[at+2],i->second);
        }else if(op==Function&&n>=3)in_entry=words[at+2]==entry;
        else if(op==Label&&in_entry&&insert==0){
            insert=at+n;
            while(insert<words.size()&&(words[insert]&0xffff)==Variable){
                const u32 count=words[insert]>>16;
                if(count==0||count>words.size()-insert)return {spirv.begin(),spirv.end()};
                insert+=count;
            }
        }
        at+=n;
    }
    if(!insert||inputs.empty()||inputs.size()>std::numeric_limits<u32>::max()-words[3])return words;
    std::vector<u32> loads;u32 bound=words[3];
    for(auto [var,type]:inputs)loads.insert(loads.end(),{(4u<<16)|Load,type,bound++,var});
    words[3]=bound;words.insert(words.begin()+std::ptrdiff_t(insert),loads.begin(),loads.end());return words;
}
}
