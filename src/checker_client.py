import xmlrpc.client
import os
import subprocess
import pickle

KERNEL_SOURCES_PATH = "/kernel-sources"


class CheckerClient:

    def __init__(self, server_host="localhost", server_port=8000):
        self.server_url = f"http://{server_host}:{server_port}"
        self.proxy = xmlrpc.client.ServerProxy(self.server_url)

    def write_file(self, path: os.path, content: bytes) -> int:
        return self.proxy.write_file(path, content)

    def read_file(self, path: os.path) -> bytes:
        return self.proxy.read_file(path)

    def stat(self, path: os.path) -> os.stat_result:
        return pickle.loads(self.proxy.stat(path).data)

    def run_command(self, command: list[str]) -> subprocess.CompletedProcess:
        return pickle.loads(self.proxy.run_command(command).data)


class Checker:

    def __init__(self, server_host="localhost", server_port=8000):
        self.client = CheckerClient(server_host, server_port)

    def compile_module(self, source_file: bytes,
                       module_name: str) -> subprocess.CompletedProcess:
        """Compile a kernel module on the remote server"""
        self.client.write_file(f"/tmp/{module_name}.c", source_file)
        self.client.write_file("/tmp/Makefile",
                               f"obj-m += {module_name}.o\n".encode())

        kernel_version = self.get_kernel_version()
        # Use the fixed path directly
        lib_modules_build_path = os.path.join(KERNEL_SOURCES_PATH,
                                              "lib/modules", kernel_version,
                                              "build")

        result = self.client.run_command(
            ["make", "-C", lib_modules_build_path, "M=/tmp", "modules"])
        result.check_returncode()

        return result

    def load_module(self, module_name: str):
        """Load a compiled module on the remote server"""
        result = self.client.run_command(
            ["insmod", os.path.join("/tmp", f"{module_name}.ko")])
        result.check_returncode()

    def unload_module(self, module_name: str):
        """Unload a module from the remote server"""
        result = self.client.run_command(["rmmod", module_name])
        result.check_returncode()

    def get_kernel_version(self) -> str:
        result = self.client.run_command(["uname", "-r"])
        result.check_returncode()
        return result.stdout.decode().strip()

    def get_kernel_log(self) -> str:
        result = self.client.run_command(["dmesg"])
        result.check_returncode()
        return result.stdout.decode().strip()

    def hello(self) -> str:
        result = self.client.run_command(["echo", "Hello, World!"])
        result.check_returncode()
        return result.stdout.decode().strip()


if __name__ == "__main__":
    checker = Checker()
    print(checker.hello())
    print("Kernel version:", checker.get_kernel_version())
