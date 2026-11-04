#include "tess_inputs.h"
#include <fstream>
#include <iterator>
#include <cstring>
#include <cassert>
#include <iostream>
int main(int argc,char**argv){
 assert(argc==3);std::ifstream in(argv[1],std::ios::binary);
 std::vector<char> bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 assert(bytes.size()>=20&&bytes.size()%4==0);std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
 auto out=BbIosTess::TouchPatchInputs(words);assert(out.size()>words.size());
 std::ofstream f(argv[2],std::ios::binary);f.write(reinterpret_cast<const char*>(out.data()),out.size()*4);assert(f.good());
 std::cout<<"PASS: SPIR-V domain module transformed; bound "<<words[3]<<" -> "<<out[3]<<"\n";
}
