import os
import re
import shutil
import zipfile
from datetime import datetime

Import("env")

# --------------- 需要排除的目录/文件（压缩项目时跳过） ---------------
_EXCLUDE_DIRS = {".pio", ".vscode", "01.Backup", "02.bin文件-封装好的代码", "__pycache__"}
_EXCLUDE_EXTS = {".bin"}


def _read_version(project_dir: str) -> str:
    candidates = [
        os.path.join(project_dir, "MCSmallDesktopDisplay.ino"),
        os.path.join(project_dir, "SmallDesktopDisplay.ino"),
        os.path.join(project_dir, "src", "main.cpp"),
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


def _timestamp() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def _zip_project(project_dir: str, zip_path: str):
    """将项目目录压缩为 zip，排除构建产物和备份目录。"""
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(project_dir):
            # 就地修改 dirs 以跳过排除目录
            dirs[:] = [d for d in dirs if d not in _EXCLUDE_DIRS]
            for fname in files:
                if os.path.splitext(fname)[1].lower() in _EXCLUDE_EXTS:
                    continue
                full = os.path.join(root, fname)
                arcname = os.path.relpath(full, os.path.dirname(project_dir))
                zf.write(full, arcname)


def _copy_bin(target, source, env):
    project_dir = env.subst("$PROJECT_DIR")
    version = _read_version(project_dir)
    ts = _timestamp()
    base_name = f"MCSmallDesktopDisplay-{version}-{ts}"

    # ---------- 1. 导出 bin 到 02.bin文件 目录（保持原有行为） ----------
    out_dir = os.environ.get("BIN_EXPORT_DIR", "").strip()
    if not out_dir:
        out_dir = os.path.normpath(os.path.join(project_dir, "..", "02.bin文件-封装好的代码"))
    os.makedirs(out_dir, exist_ok=True)

    src_bin = str(target[0])
    dst_bin_release = os.path.join(out_dir, f"{version}.bin")
    shutil.copy2(src_bin, dst_bin_release)
    print(f"[export_bin] {src_bin} -> {dst_bin_release}")

    # ---------- 2. 复制 bin 到 01.Backup ----------
    backup_dir = os.path.join(project_dir, "01.Backup")
    os.makedirs(backup_dir, exist_ok=True)

    dst_bin_backup = os.path.join(backup_dir, f"{base_name}.bin")
    shutil.copy2(src_bin, dst_bin_backup)
    print(f"[backup_bin] {src_bin} -> {dst_bin_backup}")

    # ---------- 3. 压缩项目到 01.Backup ----------
    dst_zip = os.path.join(backup_dir, f"{base_name}.zip")
    _zip_project(project_dir, dst_zip)
    print(f"[backup_zip] project -> {dst_zip}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _copy_bin)
