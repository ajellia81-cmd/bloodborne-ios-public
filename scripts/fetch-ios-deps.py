"""Fetch only upstream's pinned iOS dependencies, checking its archive SHA256s."""
import hashlib, io, pathlib, re, subprocess, tarfile, urllib.request
runtime=pathlib.Path("upstream-runtime").resolve()
wanted={"sdl3","zydis","vulkan-headers","spirv-cross","fmt","magic-enum","robin-map","xbyak","vma","miniz","xxhash","boost","ffmpeg"}
manifest=(runtime/"deps/packages.cmake").read_text()
dest=runtime/"deps/build/src"
dest.mkdir(parents=True,exist_ok=True)
found=set()
for kind,body in re.findall(r"bb_(dep|pin|git)\(([^)]+)\)",manifest):
    words=body.split()
    if not words or words[0] not in wanted:
        continue
    name,version=words[:2]
    if name in found:
        continue
    found.add(name)
    target=dest/name
    if target.exists():
        continue
    if kind=="git":
        subprocess.run(["git","clone","--depth","1","--branch",version,"--recursive","--shallow-submodules",words[2],str(target)],check=True)
    else:
        checksum,url=words[2:4]
        url=url.replace("@VERSION@",version)
        print(f"Fetch {name} {version}",flush=True)
        with urllib.request.urlopen(url) as response:
            data=response.read()
        if hashlib.sha256(data).hexdigest()!=checksum:
            raise ValueError(f"Checksum mismatch: {name}")
        target.mkdir()
        with tarfile.open(fileobj=io.BytesIO(data)) as archive:
            members=archive.getmembers()
            roots={m.name.split("/")[0] for m in members}
            if len(roots)!=1:
                raise ValueError(f"Archive has multiple roots: {name}")
            for member in members:
                parts=pathlib.PurePosixPath(member.name).parts
                if len(parts)<=1:
                    continue
                member.name=str(pathlib.PurePosixPath(*parts[1:]))
                archive.extract(member,target,filter="data")
if found!=wanted:
    raise ValueError(f"Missing packages: {wanted-found}")
