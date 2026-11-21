#!/bin/sh
SDK="$HOME/DRIVER_NEW/sdk/openwrt-sdk-19.07.10-ar71xx-generic_gcc-7.5.0_musl.Linux-x86_64"
KERNEL="$SDK/build_dir/target-mips_24kc_musl/linux-ar71xx_generic/linux-4.14.275"
TOOLCHAIN="$SDK/staging_dir/toolchain-mips_24kc_gcc-7.5.0_musl"
export STAGING_DIR="$SDK/staging_dir"
export PATH="$TOOLCHAIN/bin:$PATH"
unset LD_PRELOAD
make -C "$KERNEL" M="$(pwd)" ARCH=mips CROSS_COMPILE="$TOOLCHAIN/bin/mips-openwrt-linux-" modules