#!/usr/bin/env python

__author__ = "Gordeev Stanislav"
__email__ = "gordeev.so@yandex.com"

import time
import json
import logging
import os
import re
import glob
import requests
import subprocess
import uuid
import errno
from requests.auth import HTTPBasicAuth

from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


import datetime as dt


#logging.basicConfig(filename='xqueue_watcher.log', level="DEBUG")
# 2018-04-30 13:18:48.921313083+03:00]
logging.basicConfig(filename='../pdaemon.log', level="DEBUG", format="[%(asctime)s][{}]: (GRADER) %(levelname)s %(message)s".format(os.getppid()), datefmt="%Y-%m-%d %H:%M:%S.000000000%z")
log = logging.getLogger(__name__)


SUBMISSION_BACKUP_DIR_PATH = 'submissions_backup'
SUBMISSION_DIR_PATH = 'task_folder'
STATUS_FILE = 'status/state'
TASK_FILE = 'status/task'
RESULT_FILE = 'solve.json'
DEFAULT_MAKEFILE_PATH = "tests/default_makefile"
XQUEUE_PUT_TIMEOUT = 10

VALIDATION_STATUS = 'VALIDATION'
IDDLE_STATUS = 'IDDLE'
SOLVED_STATUS = 'VERDICT'
LOCAL_LC_DIR = "/var/www/mooc-linux-programming/"

def delete_submission(file_path):
    try:
        os.remove(file_path)
        log.info('successfully deleted ' + file_path)
    except OSError as e:
        log.error('error deleting ' + file_path)
        log.error('error code' + e.errno)


def parse_submission(content):
    body = json.loads(content.get('xqueue_body'))
    payload = json.loads(body.get('grader_payload'))
    content['xqueue_files'] = json.loads(content['xqueue_files'])
    dict_files = content.get('xqueue_files')

    log.info("get json data with submission")
    log.info("task_id = {}".format(payload.get('task_id')))
    solution_filename = dict_files.get('solution.c', 'Empty solution.c').lstrip('/')
    solution_path = os.path.join(LOCAL_LC_DIR, solution_filename)
    with open(solution_path, 'r') as f:
        solution_file = f.read()
    log.info("solution.c text = {}".format(solution_file))
    if 'Makefile' in dict_files:
        makefile_filename = dict_files.get('Makefile').lstrip('/')
        makefile_path = os.path.join(LOCAL_LC_DIR, makefile_filename)
        with open(makefile_path, 'r') as f:
            makefile_file = f.read()
        log.info("Makefile text = {}".format(makefile_file))
        return (payload.get('task_id'), solution_file, makefile_file)
    else:
        log.info("Makefile text = None")
        return (payload.get('task_id'), solution_file, None)


def parse_result(result_path):
    content = None
    MAX_SIZE = 524288
    with open(result_path) as json_data:
        content = json.load(json_data)

    log.info("trying to parse result")
    log.info("result {}".format(content.get('solve_status')))
    is_correct = content.get('solve_status')

    comp_log = content.get('comp_log')[0:MAX_SIZE]
    comp_exec = content.get('comp_exec')[0:MAX_SIZE]
    error_message = content.get('error_message')
    submission_id = content.get('submission_id')

    wrong_result = {
        'score': content.get('solve_status', '0'),
        'msg': u"""Compilation log: {}\n
Execution log: {}\n
{}\n
Unique solution id: {}""".format(comp_log, comp_exec, error_message, submission_id),
    }

    correct_result = {
        'score': content.get('solve_status', '1'),
        'msg': u"""Compilation log: {}\n
Execution log: {}\n
Unique solution id: {}""".format(comp_log, comp_exec, error_message, submission_id),
    }

    if is_correct:
        return correct_result
    return wrong_result


def check_system_responsed(solve_json_file):
    log.info("reading result file {}".format(solve_json_file))
    with open(solve_json_file, 'rb') as solve_json:
        content = json.load(solve_json)

    log.info("current status {}".format(content.get('status')))
    if content.get('status') == SOLVED_STATUS:
        return True
    else:
        return False

class XQueueClient(object):
    def __init__(self,
                 queue_name,
                 xqueue_server='http://localhost:18040',
                 xqueue_auth=('user', 'pass'),
                 sync_course='..',
                 http_basic_auth=None):
        super(XQueueClient, self).__init__()
        self.session = requests.session()
        self.xqueue_server = xqueue_server
        self.queue_name = queue_name
        self.sync_course = sync_course
        self.handlers = []
        self.daemon = True
        self.username, self.password = xqueue_auth

        if http_basic_auth is not None:
            self.http_basic_auth = HTTPBasicAuth(*http_basic_auth)
        else:
            self.http_basic_auth = None

        self.running = True
        self.processing = False

    def get_queue_name(self):
        return self.queue_name

    def __repr__(self):
        return '{}({})'.format(self.__class__.__name__, self.queue_name)

    def _get_status_file_path(self):
        return "{}/{}".format(self.sync_course, STATUS_FILE)

    def _get_current_task_file_path(self):
        return "{}/{}".format(self.sync_course, TASK_FILE)

    def _get_submission_backup_dir_path(self):
        return "{}/{}".format(self.sync_course, SUBMISSION_BACKUP_DIR_PATH)

    def _get_task_dir_path(self):
        return "{}/{}".format(self.sync_course, SUBMISSION_DIR_PATH)

    def _get_default_makefile_path(self):
        return "{}/{}".format(self.sync_course, DEFAULT_MAKEFILE_PATH)

    def _parse_response(self, response, is_reply=True):
        if response.status_code not in [200]:
            error_message = "Server %s returned status_code=%d" % (response.url, response.status_code)
            log.error(error_message)
            return False, error_message

        try:
            xreply = response.json()
        except ValueError:
            error_message = "Could not parse xreply."
            log.error(error_message)
            return False, error_message

        if 'return_code' in xreply:
            return_code = xreply['return_code'] == 0
            content = xreply['content']
        elif 'success' in xreply:
            return_code = xreply['success']
            content = xreply
        else:
            return False, "Cannot find a valid success or return code."

        if return_code not in [True, False]:
            return False, 'Invalid return code.'

        return return_code, content

    def _request(self, method, uri, timeout=0.5, **kwargs):
        url = self.xqueue_server + uri
        r = None
        while not r:
            try:
                r = self.session.request(
                    method,
                    url,
                    auth=self.http_basic_auth,
                    timeout=timeout,
                    allow_redirects=False,
                    verify=False,
                    **kwargs
                )
            except requests.exceptions.ConnectionError as e:
                log.error('Could not connect to server at %s in timeout=%r', url, timeout)
                return (False, e.message)
            if r.status_code == 200:
                return self._parse_response(r)
            # Django can issue both a 302 to the login page and a
            # 301 if the original URL did not have a trailing / and
            # APPEND_SLASH is true in XQueue deployment, which is the default.
            elif r.status_code in (301, 302):
                if self._login():
                    r = None
                else:
                    return (False, "Could not log in")
            else:
                message = "Received un expected response status code, {0}, calling {1}.".format(
                    r.status_code,url)
                log.error(message)
                return (False, message)

    def _login(self):
        if self.username is None:
            return True
        url = self.xqueue_server + '/xqueue/login/'
        log.debug("Trying to login to {0} with user: {1}".format(url, self.username))
        response = self.session.request('post', url, auth=self.http_basic_auth, data={
            'username': self.username,
            'password': self.password,
            },
            verify=False
        )
        if response.status_code != 200:
            log.error('Log in error %s %s', response.status_code, response.content)
            return False
        msg = response.json()
        log.debug("login response from %r: %r", url, msg)
        return msg['return_code'] == 0

    def shutdown(self):
        """
        Close connection and shutdown
        """
        self.running = False
        self.session.close()

    def add_handler(self, handler):
        """
        Add handler function to be called for every item in the queue
        """
        self.handlers.append(handler)

    def remove_handler(self, handler):
        """
        Remove handler function
        """
        self.handlers.remove(handler)

    def _backup_submission(self, content, submission_id):
        submission_backup_file_path = "{}/{}.json".format(self._get_submission_backup_dir_path(), submission_id)
        if not os.path.isfile(submission_backup_file_path):
            subprocess.call(["mkdir", "-p", self._get_submission_backup_dir_path()])
            log.info("serializing " + submission_backup_file_path)
            with open(submission_backup_file_path, 'w') as file:
                json.dump(content, file)
        return submission_backup_file_path

    def _download_submission(self, solution_url, makefile_url):
        log.info("downloading {}".format(solution_url))
        solution_file = requests.get(solution_url)
        makefile_file = None
        if makefile_url is not None:
            log.info("downloading {}".format(makefile_url))
            makefile_file = requests.get(makefile_url)
        return (solution_file, makefile_file)

    def _store_submission(self, submission_id, task_id, solution_file, makefile_file):
        solution_name = 'solution.c'
        makefile_name = "Makefile"
        task_id_name = 'task_id.txt'
        submission_store_dir_path = "{}/{}".format(self._get_task_dir_path(), submission_id)

        # TODO: Deal with os.chmod in script start params?!
        if not os.path.isdir("{}".format(submission_store_dir_path)):
            log.info("creating submission store dir {}".format(submission_store_dir_path))
            subprocess.call(["mkdir", "-p", submission_store_dir_path])
            # os.chmod(submission_store_dir_path, 0o777)

        if not os.path.isfile("{}/{}".format(submission_store_dir_path, task_id_name)):
            log.info("creating task_id.txt with {}".format(task_id))
            subprocess.call("echo '{}' 1>{}/{}".format(task_id, submission_store_dir_path, task_id_name),
                            shell=True)

        solution_file_path = "{}/{}".format(submission_store_dir_path, solution_name)
        if not os.path.isfile(solution_file_path):
            log.info("storing {}".format(solution_file_path))
            with open(solution_file_path, 'w') as file:
                file.writelines(solution_file)

        makefile_file_path = "{}/{}".format(submission_store_dir_path, makefile_name)
        if not os.path.isfile(makefile_file_path):
            log.info("storing {}".format(makefile_file_path))
            if makefile_file is None:
                log.info("makefile_file is None")
                subprocess.call(["cp",self._get_default_makefile_path(), makefile_file_path])
            else:
                with open(makefile_file_path, 'w') as file:
                    file.writelines(makefile_file)
         
        queue_file_path = "{}/{}".format(submission_store_dir_path, 'queue')
        if not os.path.isfile(queue_file_path):
            with open(queue_file_path, 'w') as file:
                file.write(self.queue_name)

    def _set_check_system_status_waiting(self, submission_id):
        status_file_path = self._get_status_file_path()

        log.info("setting global status {} {}".format(status_file_path, VALIDATION_STATUS))
        with open(status_file_path, 'w') as statusFile:
            statusFile.write(VALIDATION_STATUS)

        with open(self._get_current_task_file_path(), 'w') as taskFile:
            taskFile.write(submission_id)
        return True


    def _check_system_response(self, submission_id):
        log.info("waiting for solution ...")

        submission_dir_path = "{}/{}".format(self._get_task_dir_path(), submission_id)
        result_file_path = "{}/{}".format(submission_dir_path, RESULT_FILE)
        log.info("result file path = {}".format(result_file_path))

        with open(result_file_path, 'rb') as solve_json:
            content = json.load(solve_json)

        log.info("current status {}".format(content.get('status')))
        return result_file_path


    def _handle_submission(self, content, from_backup=False):

        content = json.loads(content)

        if not from_backup:
            if 'mooc_request' not in content['xqueue_body']:
                submission_id = str(uuid.uuid4())
                log.info("Handling direct submission from xqueue /submissions/. Generated sub_id: {}".format(submission_id))
            else:
                submission_id = json.loads(content['xqueue_body'])['generated_sub_id']  # handling submission from POST /submissions/
                log.info("Handling submission from POST /submissions/. Sub_id: {}".format(submission_id))
            content['generated_sub_id'] = submission_id
            self._backup_submission(content, submission_id)
        else:
            submission_id = content['generated_sub_id']

        (task_id, solution_file, makefile_file) = parse_submission(content)
        self._store_submission(submission_id, task_id, solution_file, makefile_file)

        status_setted = self._set_check_system_status_waiting(submission_id)

        if not status_setted:
            log.info("can't set status file, checker is busy, waiting for next task ...")

        return status_setted

    def process_one(self):
        try:
            self.processing = False
            get_params = {'queue_name': self.queue_name, 'block': 'true'}
            success, content = self._request('get', '/xqueue/get_submission/', params=get_params)
            if success:
                self.processing = True
                success = self._handle_submission(content)
            return success
        except requests.exceptions.Timeout:
            return True
        except Exception as e:
            log.exception(e.message)
            return True


    def process_saved_submissions(self):
        log.info('processing saved submissions')
        try:
            list_submissions = glob.glob("{}/{}".format(self._get_submission_backup_dir_path(), '*.json'))
            for submission in list_submissions:
                with open(submission, "r", encoding="utf-8") as json_data:
                    content = json_data.read()
                log.info('deserialized ' + submission)
                log.info(content)
                try:
                    json.loads(content)
                except Exception as e:
                    log.exception('Bad submission backup. Submission: {}\nError message: {}'.format(submission, e.message))
                    log.info('delete bad submission backup {}'.format(submission))
                    delete_submission(submission)
                    submission_id = submission.split('/')[-1][:-5] # submission='../submissions_backup/<submission_id>.json'
                    submission_dir_path = "{}/{}".format(self._get_task_dir_path(), submission_id)
                    os.mkdir(submission_dir_path) if not os.path.isdir(submission_dir_path) else None  # create submission dir
                    result_file_path = "{}/{}".format(submission_dir_path, RESULT_FILE)
                    with open(result_file_path, 'w') as file:
                        json.dump(
                            {
                                'submission_id': submission_id,
                                'fail_reason':"BUILD",
                                'solve_status': 0.0,
                                'comp_log': "\n",
                                'comp_exec': "\n",
                                'error_message': "Bad submussion's json",
                                'status': "VERDICT"
                            }, file) # create solve.json
                    continue
                self._handle_submission(content, from_backup=True)
                return True
        except IOError:
            return False
        return False

    def _check_busy(self):
        status_file_path = self._get_status_file_path()

        task_path = self._get_current_task_file_path()
        if os.path.isfile(task_path):
            return True

        with open(status_file_path, 'r') as status_file:
            status = status_file.read()
            if not status.startswith(IDDLE_STATUS):
                return True
        return False



    def check_queue(self):
        """
        Processing exactly one item from the queue or from backup_submissions
        """
        if self._check_busy():
            return False

        log.info("Collecting submissions from {}".format(self.queue_name))
        if not self._login():
            log.error("Could not log in to Xqueue %s. Retrying every 5 seconds..." % self.queue_name)
            num_tries = 1
            while self.running:
                num_tries += 1
                time.sleep(5)
                if not self._login():
                    log.error("Still could not log in to %s (%s:%s) tries: %d",
                        self.queue_name,
                        self.username,
                        self.password,
                        num_tries)
                else:
                    break
        else:
            if self.process_saved_submissions():
                return True
            else:
                return self.process_one()

    def _get_submission_content(self, submission_id):
        """
        Deserializing backup_submission
        """
        log.info('processing backup submissions')
        submission_backup_file_path = "{}/{}".format(self._get_submission_backup_dir_path(), "{}.json".format(submission_id))
        with open(submission_backup_file_path, "rb") as json_data:
            content = json_data.read()
            content = json.loads(content)
        log.info('deserialized ' + submission_backup_file_path)
        log.info(content)
        return (content, submission_backup_file_path)

    def send_result(self, submission_id):
        (content, submission_backup_file_path) = self._get_submission_content(submission_id)

        if content:
            result = None
            result_file = self._check_system_response(submission_id)
            if result_file is None:
                log.info("Result is not acquired, waiting for next task ...")
            else:
                result = parse_result(result_file)
                log.info("result = {}".format(result))
            log.info("Going to check result")
            if result:
                reply = {'xqueue_body': json.dumps(result),
                         'xqueue_header': content.get('xqueue_header')}
                try:
                    status, message = self._request('post', '/xqueue/put_result/', data=reply,  timeout=XQUEUE_PUT_TIMEOUT)
                    log.info("self._request('post', '/xqueue/put_result/', data=reply, verify=False, timeout=XQUEUE_PUT_TIMEOUT) {} {}".format(status, message))
                    if not status:
                        log.error('Failure for %r -> %r', reply, message)
                except Exception as e:
                    status = False 
                    log.error(e)
                delete_submission(submission_backup_file_path)
                return status
            return False
        return False
