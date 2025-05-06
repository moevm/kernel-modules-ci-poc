#!/usr/bin/env python
from __future__ import print_function

__author__ = "Gordeev Stanislav"
__email__ = "gordeev.so@yandex.com"


import json
import logging
import logging.config
from pathlib import Path

from .client import XQueueClient

class Manager(object):
    """
    Manages XQueue clients.
    """
    def __init__(self):
        self.clients = []
        self.poll_time = 10
        self.log = logging
        self.http_basic_auth = None

    def client_from_config(self, queue_name, config):
        """
        Return an XQueueClient from the configuration object.
        """
        watcher = XQueueClient(queue_name,
                        xqueue_server=config.get('SERVER', 'http://localhost:18040'),
                        xqueue_auth=config.get('AUTH', (None, None)),
                        sync_course=config.get('SYNC_COURSE', '..'),
                        http_basic_auth=self.http_basic_auth)

        return watcher

    def configure_from_directory(self, directory):
        """
        Load configuration files from the config_root
        and one or more queue configurations from a conf.d
        directory relative to the config_root
        """
        directory = Path(directory)

        logging.basicConfig(filename='xqueue_watcher.log', level="DEBUG")
        self.log = logging.getLogger(__name__)

        app_config = directory / 'xqwatcher.json'

        if app_config.exists():
            with open(app_config) as config:
                config_tokens = json.load(config)
                self.http_basic_auth = config_tokens.get('HTTP_BASIC_AUTH',None)
                self.poll_time = config_tokens.get("POLL_TIME",10)

        confd = directory
        self.log.info(directory)
        for watcher in confd.glob('*.json'):
            self.log.info(directory)
            with open(watcher) as queue_config:
                self.log.info(confd)
                configuration = json.load(queue_config)
                for queue_name, config in configuration.items():
                    watcher = self.client_from_config(queue_name, config)
                    self.clients.append(watcher)

    def check_queues(self):
        """
        Start checking all queues, after first success returning True.
        """
        for c in self.clients:
            self.log.info("Checking client = {}".format(c.get_queue_name()))
            if c.check_queue():
                self.log.info("True")
                return True
        return False

    def send_submission(self, submission_id, queue):
        """
        Clients can send each other's submissions, queue_name doesn't matter
        """
        self.log.info("Sending id = {}".format(submission_id))
        client = None#self.clients.pop()
        for c in self.clients:
            if c.queue_name == queue:
                client = c
                break
        if client and client.send_result(submission_id):
            return True
        return False

def main(args=None):
    import argparse
    parser = argparse.ArgumentParser(prog="xqueue_watcher", description="Run grader from settings")
    parser.add_argument('-d', '--config_root', required=True,
                        help='Configuration root from which to load general '
                             'watcher configuration. Queue configuration '
                             'is loaded from a conf.d directory relative to '
                             'the root')

    group = parser.add_mutually_exclusive_group()
    group.add_argument("-g", "--get", action="store_true")
    group.add_argument("-s", "--send", action="store_true")

    parser.add_argument("-id", "--submission_id", type=str, default=False)
    parser.add_argument("-q", "--queue", type=str, default=False)

    args = parser.parse_args(args)
    manager = Manager()
    manager.configure_from_directory(args.config_root)

    result=False
    if args.get:
        manager.log.info("Starting manager with arguments -d {} -g {}".format(args.config_root, args.get))
        result = manager.check_queues()
    elif args.send and args.submission_id != "":
        manager.log.info("Starting manager with arguments -d {} -s {} -id {} -q {}".format(args.config_root, args.send, args.submission_id, args.queue))
        result = manager.send_submission(args.submission_id, args.queue)

    if result:
        manager.log.info("Manager: result true")
        return 0
    manager.log.info("Manager: result false")
    return 1

