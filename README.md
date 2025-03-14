Host system requirements:
- QEMU
- python3
- Podman or Docker

Basic testcase:
```ShellSession
$ cd image
$ make container-build
 # or "make DOCKER=docker container-build" if you have Docker
 # or just "make" if you have NixOS on your host
$ qemu-system-x86_64 -drive file=nixos.qcow2 -nographic -nic user,hostfwd=tcp::8000-:8000 -m 2G -smp 2 -enable-kvm
 # another terminal:
$ ./src/test.py test/test_module.c
```
