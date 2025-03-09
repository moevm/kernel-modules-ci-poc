import xmlrpc.client
import os


class CommandFailedException(Exception):
    pass


class CheckerClient:

    def __init__(self, server_host="localhost", server_port=8000):
        self.server_url = f"http://{server_host}:{server_port}"
        self.proxy = xmlrpc.client.ServerProxy(self.server_url)

    def compile_module(self, source_file: bytes, module_name: str):
        """Compile a kernel module on the remote server"""
        pass

    def load_module(self, module_name: str):
        """Load a compiled module on the remote server"""
        pass

    def unload_module(self, module_name: str):
        """Unload a module from the remote server"""
        return self.proxy.run_command(["rmmod", module_name])

    def get_kernel_version(self) -> str:
        result = self.proxy.run_command(["uname", "-r"])
        if result["returncode"] != 0:
            raise CommandFailedException("dmesg: " +
                                         str(result["stderr"]).strip())
        return str(result["stdout"]).strip()

    def hello(self) -> str:
        result = self.proxy.run_command(["echo", "Hello, World!"])
        if result["returncode"] != 0:
            raise CommandFailedException()
        return str(result["stdout"]).strip()


if __name__ == "__main__":
    client = CheckerClient()
    print(client.hello())
    print("Kernel version:", client.get_kernel_version())
