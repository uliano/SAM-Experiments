import sys
from pathlib import Path

Import("env")

if sys.platform == "win32":
    tc = Path("C:/Users/uliano/stuff/toolchains/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi/bin")
    ext = ".exe"
else:
    tc = Path("/opt/arm-gcc-15/bin")
    ext = ""


def _t(name):
    return str(tc / (name + ext))


print(f"toolchain_select: using {tc}")

env.PrependENVPath("PATH", str(tc))
env.Replace(
    AR=_t("arm-none-eabi-ar"),
    AS=_t("arm-none-eabi-gcc"),
    CC=_t("arm-none-eabi-gcc"),
    CXX=_t("arm-none-eabi-g++"),
    GDB=_t("arm-none-eabi-gdb"),
    LINK=_t("arm-none-eabi-g++"),
    RANLIB=_t("arm-none-eabi-ranlib"),
    OBJCOPY=_t("arm-none-eabi-objcopy"),
    OBJDUMP=_t("arm-none-eabi-objdump"),
    NM=_t("arm-none-eabi-nm"),
    SIZE=_t("arm-none-eabi-size"),
)
