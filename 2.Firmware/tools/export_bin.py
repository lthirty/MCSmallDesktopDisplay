import os
import re
import shutil

Import("env")


def _read_version(project_dir: str) -> str:
    candidates = [
        os.path.join(project_dir, "SmallDesktopDisplay", "SmallDesktopDisplay.ino"),
        os.path.join(project_dir, "SmallDesktopDisplay.ino"),
    ]
    pattern = re.compile(r'#define\s+Version\s+"([^"]+)"')

    for path in candidates:
        if not os.path.exists(path):
            continue
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
        m = pattern.search(text)
        if m:
            return m.group(1).strip()
    return "firmware"


def _copy_bin(target, source, env):
    project_dir = env.subst("$PROJECT_DIR")
    version = _read_version(project_dir)
    out_dir = os.path.join(project_dir, "02.bin文件-封装好的代码")
    os.makedirs(out_dir, exist_ok=True)

    src_bin = str(target[0])
    dst = os.path.join(out_dir, f"{version}.bin")
    shutil.copy2(src_bin, dst)
    print(f"[export_bin] {src_bin} -> {dst}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _copy_bin)
