FROM debian:trixie

RUN \
    apt update && \
    apt upgrade -y --no-install-recommends && \
    apt install -y --no-install-recommends debconf debhelper dpkg-dev devscripts equivs debhelper-compat build-essential dh-dkms linux-headers-generic && \
    apt purge && \
    apt clean

WORKDIR /tmp/build