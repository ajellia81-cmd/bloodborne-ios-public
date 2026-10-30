"""Prepare a personal dump outside the repository; never run with game data in public CI."""
import argparse, os, pathlib, subprocess, sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--runtime",type=pathlib.Path,required=True)
p.add_argument("--game",type=pathlib.Path,required=True)
p.add_argument("--out",type=pathlib.Path,required=True)
a=p.parse_args()
if os.environ.get("GITHUB_ACTIONS")=="true":
    p.error("Personal game preparation is disabled on GitHub Actions. Use a private cloud workspace.")
runtime=a.runtime.resolve()
game=a.game.resolve()
out=a.out.resolve()
for parent in [out,*out.parents]:
    if (parent/".git").exists():
        p.error("Output must be outside a Git checkout: generated game code is personal.")
for required in ["eboot.bin","sce_sys/param.sfo","dvdroot_ps4"]:
    if not (game/required).exists():
        p.error(f"Game folder is missing {required}")
out.mkdir(parents=True,exist_ok=True)
for name in ["prepare.py","link_libc.py","link_modules.py","content_profile.py"]:
    subprocess.run([sys.executable,str(runtime/"scripts"/name),str(game),"--out",str(out)],cwd=runtime,check=True)
subprocess.run([sys.executable,str(runtime/"scripts/patches.py"),"--out",str(out),"--fps","30","--game-dir",str(game),"--render-res","960x540","--output-res","960x540"],cwd=runtime,check=True)
for name in ["boot-linked.bin","content.bin","patches.bin"]:
    path=out/name
    if not path.is_file() or not path.stat().st_size:
        raise RuntimeError(f"Preparation did not produce {name}")
print("Personal boot files contain derived game code: keep them private and outside Git. Preparation does not validate game loading, DLC or performance.")
