"""Generate constructor tracing wrappers and instrument only their initializer pointers."""
import sys,struct,json
from pathlib import Path
N=256
if sys.argv[1]=="generate":
    out=Path(sys.argv[2])
    src='#include <stdint.h>\n#include <stdio.h>\n#include <dlfcn.h>\nextern void bb_diagnostic_mark(const char*);\n'
    src+='__attribute__((used)) uintptr_t bb_init_offsets[256]={1};\n'
    src+='typedef void (*initfn)(int,char**,char**,char**,void*);\n'
    src+='static void invoke(unsigned i,int a,char**b,char**c,char**d,void*e) { Dl_info image; char line[160]; if(!dladdr((void*)&invoke,&image)) { bb_diagnostic_mark("initializer dladdr failed"); __builtin_trap(); } snprintf(line,sizeof(line),"initializer %u begin image+0x%llx",i,(unsigned long long)bb_init_offsets[i]); bb_diagnostic_mark(line); ((initfn)((uintptr_t)image.dli_fbase+bb_init_offsets[i]))(a,b,c,d,e); snprintf(line,sizeof(line),"initializer %u end",i); bb_diagnostic_mark(line); }\n'
    for i in range(N):
        src+=f'__attribute__((used,noinline)) void bb_trace_{i}(int a,char**b,char**c,char**d,void*e) {{ invoke({i},a,b,c,d,e); }}\n'
    out.write_text(src)
    sys.exit()
path=Path(sys.argv[2]);b=bytearray(path.read_bytes());p=32;segments=[];table=None;syms=None
for _ in range(struct.unpack_from('<I',b,16)[0]):
    cmd,size=struct.unpack_from('<II',b,p)
    if cmd==2:syms=struct.unpack_from('<4I',b,p+8)
    if cmd==0x19:
        name=b[p+8:p+24].split(b'\0')[0]
        vm,vs,off,fs=struct.unpack_from('<4Q',b,p+24);segments.append((vm,fs,off))
        if name==b'__TEXT':base=vm
        for j in range(struct.unpack_from('<I',b,p+64)[0]):
            q=p+72+j*80
            if b[q:q+16].split(b'\0')[0]==b'__mod_init_func':
                assert table is None
                table=(struct.unpack_from('<I',b,q+48)[0],struct.unpack_from('<Q',b,q+40)[0]//8)
    p+=size
so,n,st,ss=syms;symbols={}
for i in range(n):
    si,typ,se,desc,val=struct.unpack_from('<IBBHQ',b,so+16*i)
    if typ & 0xe == 0xe:
        start=st+si;end=b.find(0,start,st+ss)
        symbols[bytes(b[start:end]).decode('utf8','replace')]=val
def fileoff(addr):
    for vm,fs,off in segments:
        if vm<=addr<vm+fs:return off+addr-vm
    raise ValueError('unmapped symbol')
off,count=table;assert 1<count<=N
originals=list(struct.unpack_from('<'+'Q'*count,b,off))
assert originals[0]==symbols['_bb_diagnostic_init'],'diagnostic constructor must remain first'
array=fileoff(symbols['_bb_init_offsets']);report=[]
for i,original in enumerate(originals):
    assert any(vm<=original<vm+fs for vm,fs,fo in segments)
    struct.pack_into('<Q',b,array+i*8,original-base)
    if i:
        wrapper=symbols[f'_bb_trace_{i}']
        struct.pack_into('<Q',b,off+i*8,wrapper)
    report.append({'index':i,'offset':hex(original-base),'symbols':[k for k,v in symbols.items() if v==original]})
path.write_bytes(b)
Path(sys.argv[3]).write_text(json.dumps(report,indent=2))
print('Preserved constructor order; traced',count-1,'constructors; diagnostic remains first')
