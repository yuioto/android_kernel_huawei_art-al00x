# android_kernel_huawei_art-al00x

4.14.116 kernel source for Huawei Enjoy 10 on HarmonyOS 2.0.0

## README_kernel.txt

### How to Build

- get Toolchain
From android git server, codesourcery and etc ..
- aarch64-linux-android-4.9
- clang-r353983c(git clone <https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86>)

- edit Makefile
edit CROSS_COMPILE to right toolchain path(You downloaded).
Ex)   export PATH :=${PATH}:$(android platform directory you download)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin
Ex)   export CROSS_COMPILE=aarch64-linux-android-

- put the clang in the right path.
put the clang-r353983c in the $(kernel directory)/../../prebuilts/clang/host/linux-x86 path

No't need use root user.

```shell
mkdir ../out
make ARCH=arm64 O=../out merge_kirin710_defconfig
make ARCH=arm64 O=../out -j8
```

### Output files

- Kernel : out/arch/arm64/boot/Image.gz
- module : out/drivers/*/*.ko

### How to Clean

No't need use root user.

```shell
make ARCH=arm64 distclean
rm -rf out
```
