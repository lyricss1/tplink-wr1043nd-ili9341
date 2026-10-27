FROM debian:bookworm-slim

ARG DEBIAN_FRONTEND=noninteractive

ARG SDK_URL=https://downloads.openwrt.org/releases/19.07.10/targets/ar71xx/generic/openwrt-sdk-19.07.10-ar71xx-generic_gcc-7.5.0_musl.Linux-x86_64.tar.xz
ARG SDK_SHA256=6c69f7672a3af1ad2791d0554f04a6dc05d58bb2a856efc1b8ce912b8c3ce428

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    xz-utils \
    file \
    binutils \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fL "$SDK_URL" -o /tmp/openwrt-sdk.tar.xz \
    && echo "$SDK_SHA256  /tmp/openwrt-sdk.tar.xz" | sha256sum -c - \
    && mkdir -p /opt/openwrt-sdk \
    && tar -xJf /tmp/openwrt-sdk.tar.xz \
       --strip-components=1 -C /opt/openwrt-sdk \
    && rm /tmp/openwrt-sdk.tar.xz

COPY build-mips.sh /usr/local/bin/build-mips
RUN chmod +x /usr/local/bin/build-mips

ENV SDK_DIR=/opt/openwrt-sdk
WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/build-mips"]
