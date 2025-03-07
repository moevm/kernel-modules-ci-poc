#!/usr/bin/env python3

from xmlrpc.server import SimpleXMLRPCServer


def hello() -> str:
    return "Hello, World!"


server = SimpleXMLRPCServer(("0.0.0.0", 8000))
server.register_function(hello, "hello")
server.serve_forever()
