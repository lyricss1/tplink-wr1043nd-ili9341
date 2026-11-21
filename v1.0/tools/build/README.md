# MIPS build environment

This directory contains a Docker-based build environment for userspace programs targeting the TP-Link WR1043ND v2 running OpenWrt 19.07.10.

The container downloads the matching OpenWrt SDK, verifies its SHA-256 checksum and uses its cross-compiler to build the C programs in `src/`.

## Requirements

- Docker
- The project source files in `src/`
- Internet access for the initial image build

## Build the image

Run from the repository root:

```sh
docker build -t wr1043nd-build -f tools/build/Dockerfile tools/build
```

The first build downloads and extracts the OpenWrt SDK. Later builds can reuse the cached Docker image.

## Compile the programs

Run from the repository root:

```sh
docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$PWD:/workspace" \
  wr1043nd-build
```

The output files are written to `build/`:

- `build/badapple2`
- `build/spibench`, if `src/spibench.c` is present and compiles successfully

The output directory is generated locally and should not be committed.

## Check the output

```sh
file build/badapple2
```

The binary should be a MIPS executable, not an x86-64 Linux executable.

## Kernel modules

This build environment does not build `.ko` files.

Kernel modules need a build tree and configuration matching the target kernel. The normal userspace SDK is not enough to guarantee a compatible module.

The requirements for `spibench_k.c` will be documented separately after its source and intended use have been checked.

## Notes

This SDK targets the legacy OpenWrt 19.07.10 `ar71xx/generic` platform. It is intended for the firmware used in this project, not for arbitrary OpenWrt versions or router revisions.