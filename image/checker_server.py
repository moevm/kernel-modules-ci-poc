#!/usr/bin/env python3

import xmlrpc.server
import subprocess
import os
import tempfile


class CheckerServer:

    def __init__(self, host="0.0.0.0", port=8000):
        self.server = xmlrpc.server.SimpleXMLRPCServer((host, port))
        self.register_functions()

    def register_functions(self):
        self.server.register_function(self.write_file)
        self.server.register_function(self.stat)
        self.server.register_function(self.run_command)

    def start(self):
        self.server.serve_forever()

    def write_file(self, path: os.path, content: bytes) -> int:
        with open(path, "wb") as f:
            return f.write(content)

    def read_file(self, path: os.path) -> bytes:
        with open(path, "rb") as f:
            return f.readall()

    def stat(self, path: os.path) -> os.stat_result:
        return os.stat(path)

    def run_command(self,
                    command: list[str]) -> subprocess.CompletedProcess[str]:
        return subprocess.run(command, capture_output=True)


if __name__ == "__main__":
    server = CheckerServer()
    server.start()
