#!/usr/bin/env python3
"""
unified_vm_tester.py
====================

Usage:
    ./unified_vm_tester.py path/to/module.c

Requirements:
    - libvirt-python (`pip install libvirt-python`)
    - KVM and libvirtd running on the host
    - checker_client module available on PYTHONPATH
    - A qcow2 image (nixos.qcow2) with a userspace
      process that listens on guest TCP port 8000
"""

import libvirt
import socket
import time
import os
import sys
import subprocess
import checker_client

# VM parameters
DOMAIN_NAME = "nixos-test"
DISK_PATH = "image/nixos.qcow2"
RAM_MIB = 2048
VCPUS = 2
HOSTFWD_PORTS = {8000: 8000}
BOOT_TIMEOUT = 120


# libvirt helpers
def connect_libvirt(uri: str = "qemu:///system") -> libvirt.virConnect:
    conn = libvirt.open(uri)
    if conn is None:
        raise RuntimeError("Unable to open libvirt connection")
    return conn


def _define_domain(conn: libvirt.virConnect) -> libvirt.virDomain:
    """
    Define a minimal KVM domain that uses user-mode networking with
    explicit port redirections so we keep the same semantics as the
    original `-nic user,hostfwd=tcp::HOST-:GUEST`.
    """
    # Build <redirect/> entries
    redirects = "\n".join(
        f"      <redirect protocol='tcp' hostport='{h}' guestport='{g}'/>"
        for h, g in HOSTFWD_PORTS.items())

    xml = f"""
<domain type='kvm'>
  <name>{DOMAIN_NAME}</name>
  <memory unit='MiB'>{RAM_MIB}</memory>
  <vcpu placement='static'>{VCPUS}</vcpu>

  <os>
    <type arch='x86_64'>hvm</type>
  </os>

  <cpu mode='host-model'/>
  <features><acpi/></features>

  <devices>
    <disk type='file' device='disk'>
      <driver name='qemu' cache='none'/>
      <source file='{DISK_PATH}'/>
      <target dev='vda' bus='virtio'/>
    </disk>

    <!-- user-mode networking -->
    <interface type='user'>
      <model type='virtio'/>
{redirects}
    </interface>

    <console type='pty'/>
    <graphics type='none'/>
  </devices>
</domain>
"""
    return conn.defineXML(xml)


def get_or_create_domain(conn: libvirt.virConnect) -> libvirt.virDomain:
    try:
        return conn.lookupByName(DOMAIN_NAME)
    except libvirt.libvirtError:
        return _define_domain(conn)


def ensure_domain_running(dom: libvirt.virDomain) -> None:
    state, _ = dom.state()
    if state == libvirt.VIR_DOMAIN_SHUTOFF:
        dom.create()
    elif state == libvirt.VIR_DOMAIN_PAUSED:
        dom.resume()


def wait_for_port(host: str, port: int, timeout: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=2):
                return
        except OSError:
            time.sleep(1)
    raise TimeoutError(
        f"Port {port} on {host} did not become ready in {timeout}s")


# Test logic
def run_checker(module_path: str) -> None:
    module_src = open(module_path, "rb").read()
    module_name = os.path.splitext(os.path.basename(module_path))[0]
    checker = checker_client.Checker()

    result = checker.compile_module(module_src, module_name)
    checker.load_module(module_name)
    checker.unload_module(module_name)

    print(result.stdout.decode().strip())
    kernel_log = "\n".join(checker.get_kernel_log().splitlines()[-9:])
    print(kernel_log)


def main() -> None:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <module.c>")
        sys.exit(1)

    module_path = sys.argv[1]

    conn = connect_libvirt()
    try:
        dom = get_or_create_domain(conn)
        ensure_domain_running(dom)
        wait_for_port("127.0.0.1", 8000, BOOT_TIMEOUT)

        run_checker(module_path)

    finally:
        conn.close()


if __name__ == "__main__":
    main()
