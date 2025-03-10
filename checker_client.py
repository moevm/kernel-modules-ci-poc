import xmlrpc.client
import os
import subprocess
import pickle


class CommandFailedException(Exception):
    pass


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

    def compile_module(self, source_file: bytes, module_name: str):
        """Compile a kernel module on the remote server"""
        pass

    def load_module(self, module_name: str):
        """Load a compiled module on the remote server"""
        pass

    def unload_module(self, module_name: str):
        """Unload a module from the remote server"""
        return self.client.run_command(["rmmod", module_name])

    def get_kernel_version(self) -> str:
        result = self.client.run_command(["uname", "-r"])
        if result.returncode != 0:
            raise CommandFailedException("dmesg: " +
                                         str(result.stderr).strip())
        return result.stdout.decode().strip()

    def hello(self) -> str:
        result = self.client.run_command(["echo", "Hello, World!"])
        if result.returncode != 0:
            raise CommandFailedException()
        return result.stdout.decode().strip()


if __name__ == "__main__":
    checker = Checker()
    print(checker.hello())
    print("Kernel version:", checker.get_kernel_version())
