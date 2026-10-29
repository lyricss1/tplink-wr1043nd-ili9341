#SETUP

(ill update this later)

import this stuff
```sh
SDK="$HOME/owrt-sdk/openwrt-sdk-19.07.10-ar71xx-generic_gcc-7.5.0_musl.Linux-x86_64"
TC="$SDK/staging_dir/toolchain-mips_24kc_gcc-7.5.0_musl"
K="$SDK/build_dir/target-mips_24kc_musl/linux-ar71xx_generic/linux-4.14.275"
export STAGING_DIR="$SDK/staging_dir"
```

n then compile
```sh
"$CC" -O3 -static -Wall -Wextra badapple5.c -o badapple
```

n then send it to router via scp

+ copy archive
