import sys
from os import path
import platform

# 信息属性 class
class Info:
    machine = platform.machine().lower()
    system = platform.system().lower()

env = Environment()
info = Info()

# 平台和编译器配置
if info.system == "darwin":
    env["CC"] = "clang"
    if info.machine == "arm64":
        env.Append(CCFLAGS=["-target", "arm64-apple-macos11"])
    else:
        env.Append(CCFLAGS=["-target", "x86_64-apple-macos10.15"])
elif info.system == "windows":
    env["CC"] = "cl"
    if "arm" in info.machine:
        env.Append(CCFLAGS=["/arch:ARM64"])
    else:
        env.Append(CCFLAGS=["/arch:x64"])
# elif info.system == "linux":
#     env["CC"] = "gcc"
#     if "arm" in info.machine or "aarch64" in info.machine:
#         env.Append(CCFLAGS=["-march=armv8-a"])
#     else:
#         env.Append(CCFLAGS=["-m64"])

#
#
#

# 从命令行参数获取编译阶段，默认 all
stage = ARGUMENTS.get("stage", "all").lower()

# 编译 OpenSSL
if stage in ("openssl", "all"):
    if info.system == "darwin":
        SConscript("third_party/openssl/mac/SConscript", exports=["env", "info"])
    elif info.system == "windows":
        SConscript("third_party/openssl/win/SConscript", exports=["env", "info"])

# 编译 SQLCipher（依赖 OpenSSL）
if stage in ("sqlcipher", "all"):
    if info.system == "darwin":
        SConscript("third_party/sqlcipher/mac/SConscript", exports=["env", "info"])
    elif info.system == "windows":
        SConscript("third_party/sqlcipher/win/SConscript", exports=["env", "info"])

# 编译 GDExtension（依赖 SQLCipher + OpenSSL）
if stage in ("gdextension", "all"):
    SConscript("src_cpp/SConscript", exports=["env", "info"])


# on mac m1 `scons stage=gdextension platform=macos arch=arm64 target=template_release use_lto=yes`