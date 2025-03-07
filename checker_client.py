import xmlrpc.client

proxy = xmlrpc.client.ServerProxy("http://localhost:8000/")

if __name__ == "__main__":
    print(proxy.hello())
