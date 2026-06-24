#!/bin/bash

docker run --interactive --rm --tty --ulimit 'nofile=1024:262144' \
--volume "$(pwd)/openwrt-openwifi:/workdir" \
--workdir '/workdir' openwrt:debian_12 /bin/bash