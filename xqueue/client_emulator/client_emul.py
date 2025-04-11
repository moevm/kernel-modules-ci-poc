#! /usr/bin/python3
# -*- coding: utf-8 -*-
r"""
emulate client behavior by sending files
"""
import argparse
import base64
import json
import sys
import time

import requests

SUBMIT_ACTION = 'submit'
CHECK_ACTION = 'check'
DECODE_ACTION = 'decode'

RETRY_TIMEOUT = 120
DELAY = 1


def retry_on_failure(request_function):
    def wrapper(*args, **kwargs):
        elapsed = 0

        while elapsed < RETRY_TIMEOUT:
            try:
                r = request_function(*args, **kwargs)
                if elapsed > 0:
                    print('\nserver is ready', file=sys.stderr)
                return r
            except BaseException:
                if elapsed == 0:
                    print('Server is not ready . . .', end=' ', file=sys.stderr, flush=True)
                else:
                    print('.', end=' ', file=sys.stderr, flush=True)
                time.sleep(DELAY)
            elapsed += DELAY

    return wrapper


def perform_client_emulation(namespace):
    """
    Infinite cycle for testing REST protocol.
    Press "s" to emulate sending and "c" to emulate getting info
    """
    if namespace.action == SUBMIT_ACTION:
        submit_send(namespace)
    else:
        check_send(namespace)


def decode_json():
    try:
        filename = sys.argv[2]
        key = sys.argv[3]
        with open(filename) as f:
            print(json.load(f)[key])
    except BaseException:
        return 1
    return 0


def print_results(request, namespace):
    if namespace.http_code_only:
        print(request.status_code)
    else:
        print(json.dumps(request.json(), sort_keys=True))


@retry_on_failure
def submit_send(namespace):
    """
    emulate sending solution
    """
    headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
    jdata = {"task_id": namespace.task}
    with open(namespace.solution) as f:
        solution = f.read()
    solution = solution.encode('utf-8')
    solution = base64.b64encode(solution).decode('utf-8')
    jdata['solution'] = solution
    if namespace.makefile:
        with open(namespace.makefile) as f:
            makefile = f.read()
        makefile = makefile.encode('utf-8')
        makefile = base64.b64encode(makefile).decode('utf-8')
        jdata['makefile'] = makefile
    request = requests.post(namespace.url.format("submissions"),
                            data=json.dumps(jdata),
                            headers=headers)
    print_results(request, namespace)


@retry_on_failure
def check_send(namespace):
    """
    emulate checking id
    """
    url = namespace.url.format("submissions/{}".format(namespace.id))
    request = requests.get(url)
    print_results(request, namespace)


def parse():
    # client_emul.py decode "{"a": "b"}" a
    if len(sys.argv) > 1 and sys.argv[1] == DECODE_ACTION:
        exit(decode_json())

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--action', default=SUBMIT_ACTION,
                        choices=[SUBMIT_ACTION, CHECK_ACTION])
    parser.add_argument('-c', '--solution',
                        default='./client_emulator/solution.c')
    parser.add_argument('-m', '--makefile', default=None)
    parser.add_argument('-n', '--task', default="1", )
    parser.add_argument('-u', '--url',
                        default='http://ec2-user@ec2-54-74-15-56'
                                '.eu-west-1.compute.amazonaws.com/')
    parser.add_argument('-i', '--id', default='')
    parser.add_argument('--http_code_only', default=False, type=bool)
    parsed = parser.parse_args(sys.argv[1:])
    parsed.url += "{}"
    return parsed


if __name__ == '__main__':
    perform_client_emulation(parse())
