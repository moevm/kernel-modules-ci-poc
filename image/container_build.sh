#!/usr/bin/env sh

podman run -it --rm -v .:/build --privileged -v /dev/kvm:/dev/kvm:rw localhost/nix-image-builder:latest bash -c "nix build .#qcow -o/tmp/result && cp /tmp/result/nixos.qcow2 ."
