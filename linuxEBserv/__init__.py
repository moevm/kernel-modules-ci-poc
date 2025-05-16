# -*- coding: utf-8 -*-
r"""
server based on flask.
API:
--send submission
POST /submissions
{
    "task_id": int,
    "solution": "[base64-encoded-data]",
    "makefile": "[base64-encoded-data]" or ""
}

return:
{
    "id": "uuid"  # unique submission id
}
--check state and answer
GET /submissions/id
return:
{
    "status": "READY" if ready "IN_PROCESS" else,
    "result": float [0..1] or 0.0 if IN_PROCESS,
    "compilation_log": string or "",
    "execution_log": string or "",
}

"""

import base64
import json
import os
import subprocess
import uuid
from flask import Flask, jsonify, request
from random import randint
import requests
import sys

class WrongRequest(Exception):
    """exception, throw in incorrect requst. Has addition method to form error code"""
    def __init__(self, err_type, message):
        self.err_type = err_type
        self.message = message

    def __str__(self):
        return "error {}: '{}'".format(self.err_type, self.message)

    def send_as_response(self):
        return self.message, self.err_type

app = Flask(__name__)
PATH_TO_MOOC_LP = './'
TASK_DIR = 'task_folder/'
TESTS_DIR = 'tests/'
CURRENT_TASK_FILE = 'status/task'
GLOBAL_STATUS_FILE = 'status/state'
TASK_STATUS_FILE = 'solve.json'

BUSY_MESSAGE = "Busy!"
BUSY_ERROR_CODE = 429
INCORRECT_FORMAT_ERROR_CODE = 400
NOT_FOUND_CODE = 404
NOT_FOUND_ID_MESAGE = 'Submisson not found'

GLOBAL_STATUS_KEY = 'status'

# Xqueue parameters
PATH_TO_XQ_CONFIG = "/xqueue_watcher/conf.d/conf.d/config.json"
XQ_CONFIG = None
XQ_QUEUE = None
XQ_CALLBACK_ROUTE = "/callback"
XQ_REQUEST_TIMEOUT = 5

# Global statuses
VALIDATION_STATUS = 'VALIDATION'

DEBUG_INFO_FILE = "DEBUG_INFO"
DEFAULT_MAKEFILE = "default_makefile"

ENCONDING_ERROR = "One of the solution files is not in UTF-8 encoding. Both file content is replaced with this text. Please check and fix solution file encoding."


def setPathToMoocL(path):
    """
    path to python files
    """
    global PATH_TO_MOOC_LP
    PATH_TO_MOOC_LP = path+"/"


def getTaskPath(id):
    return PATH_TO_MOOC_LP + TASK_DIR + id + "/"

def parseSubmissionRequest(request):
    data = request.get_json()
    id = str(uuid.uuid4()) # generate unique id
    task_id = data.get('task_id', None)
    solution = data.get('solution', "")
    solution = base64.b64decode(solution)
    makefile = data.get('makefile', None)
    if makefile:
        # Attempt to decode Makefile
        makefile = base64.b64decode(makefile)
    else:
        makefile = open(getDefaultMakeFilePath(), 'r').read()

    try:
       solution = solution.decode("utf-8")
       makefile = makefile.decode("utf-8")
    except UnicodeDecodeError as e:
       solution = ENCONDING_ERROR
       makefile = "all:\n\t echo \"{}\"".format(ENCONDING_ERROR)

    return (id, task_id, solution, makefile)

def storeTask(id, task_id, solution, makefile):
    # write data to filesystem. In directories path/to/files/task_folder/id
    user_store_dir = getTaskPath(id)
    subprocess.call(["mkdir", "-p", user_store_dir])
    subprocess.call("echo '{task_id}' 1>{upath}/task_id.txt".format(task_id=task_id, upath=user_store_dir), shell=True)
    solution_name = 'solution.c'
    with open(user_store_dir + solution_name, 'wb') as f:
        f.write(solution)

    makefile_name = "Makefile"
    with open(user_store_dir + makefile_name, 'wb') as f:
        f.write(makefile)

def getCurrentTaskFilePath():
    return PATH_TO_MOOC_LP + CURRENT_TASK_FILE

def getStatusFilePath():
    return PATH_TO_MOOC_LP + GLOBAL_STATUS_FILE

def writeTaskId(id):
    with open(getCurrentTaskFilePath(), 'wb') as f:
        f.write(id)

def doesFileExist(filePath):
    return os.path.isfile(filePath)

def isBusy():
    return doesFileExist(getCurrentTaskFilePath()) 

def readGlobalStatusFile():
    with open(getStatusFilePath()) as statusFile:
        return statusFile.read().replace('\n','')

def readTaskStatusFile(id):
    taskStatusPath = getTaskPath(id) + TASK_STATUS_FILE
    taskStatus = {}
    print(taskStatusPath, file=sys.stderr)
    if doesFileExist(taskStatusPath):
        with open(taskStatusPath, 'r') as f:
            taskStatusString = f.read()
            taskStatus = json.loads(taskStatusString) 
    else:
        raise WrongRequest(NOT_FOUND_CODE, NOT_FOUND_ID_MESAGE)
    return taskStatus 

def formTaskStatusForResponse(id):
    # Try to read task status
    taskStatus = readTaskStatusFile(id)
    return taskStatus

def writeGlobalStatus(status):
    with open(getStatusFilePath(), 'wb') as statusFile:
        statusFile.write(status) 

def getDebugInfoPath():
    return PATH_TO_MOOC_LP + DEBUG_INFO_FILE


def readDebugInfo():
    debugInfoFile = open(getDebugInfoPath(), 'r')
    listFileData = debugInfoFile.readlines()
    return ''.join(listFileData)

def getDefaultMakeFilePath():
    return PATH_TO_MOOC_LP + TESTS_DIR + DEFAULT_MAKEFILE

def load_xq_config():
    global XQ_CONFIG, XQ_QUEUE
    if XQ_CONFIG and XQ_QUEUE:
        return
    with open(PATH_TO_MOOC_LP + PATH_TO_XQ_CONFIG, "r") as file:
        tmp = json.load(file)
        XQ_QUEUE = list(tmp.keys())[0]
        XQ_CONFIG = tmp[XQ_QUEUE]    

def login_to_xqueue():
    url = XQ_CONFIG['SERVER'] + "/xqueue/login/"
    data = {
        'username': XQ_CONFIG['AUTH'][0],
        'password': XQ_CONFIG['AUTH'][1],
    }
    response = requests.post(url=url, data=data, timeout=XQ_REQUEST_TIMEOUT, verify=False)
    check_xqueue_response(response)
    return response.cookies["sessionid"]

def submit_to_xqueue(submission_id, task_id, files):
    load_xq_config()
    url = XQ_CONFIG['SERVER'] + "/xqueue/submit/"
    xqueue_header = json.dumps({
        'lms_callback_url': "{}{}".format(XQ_CONFIG['LMC_CALLBACK_SERVER'], XQ_CALLBACK_ROUTE),
        'lms_key': XQ_CONFIG['LMS_KEY'],
        'queue_name': XQ_QUEUE
    })
    grader_payload = json.dumps({'task_id': task_id})
    xqueue_body = json.dumps({
        'student_response': "",
        'grader_payload': grader_payload,
        'mooc_request': True,
        'generated_sub_id': submission_id
    })
    data = {
        'xqueue_header': xqueue_header,
        'xqueue_body': xqueue_body,
    }
    cookies = {
        'sessionid': login_to_xqueue(),
    }
    response = requests.post(url=url, files=files, data=data, cookies=cookies, timeout=XQ_REQUEST_TIMEOUT, verify=False)
    check_xqueue_response(response)
    return response

def check_xqueue_response(response):
    if response.status_code != 200:
        raise Exception("check_xqueue_response. Response status code is {}, expected 200".format(response.status_code))
    try:
        response_content_as_json = json.loads(response.content.decode())
    except Exception:
        raise Exception("check_xqueue_response. Response content couldn't be decoded from a json to a dict")
    for key in ["return_code", "content"]:
        if key not in response_content_as_json:
            raise Exception("check_xqueue_response. '{}' key not found in decoded response content".format(key))
    return_code = response_content_as_json["return_code"]
    if return_code != 0:
        content = response_content_as_json["content"]
        raise Exception("check_xqueue_response. Return code is {}, expected 0. {}".format(return_code, content))

@app.route('/hello', methods=['GET'])
def hello_world():
    """
    function for testing connection
    """
    return "HELLO, WORLD! "

@app.route('/submissions', methods=['POST'])
def send_solution():
    """
    function on submiting solution via json.
    Requres task_id - id of problem
    Requires solution parametr - file of solution
    Optional parametr - makefile
    @return id of solution, which came into treatment
    """ 

    # If some task already exist then return 429 code
    if isBusy():
        return BUSY_MESSAGE, BUSY_ERROR_CODE

    # get data
    try:
        (id, task_id, solution, makefile) = parseSubmissionRequest(request)
    except Exception as e:
        return "Exception during solution parsing", INCORRECT_FORMAT_ERROR_CODE  # there's no name 'abort' in module and app

    # request to xqueue
    submit_to_xqueue(id, task_id, {'solution.c': solution, 'Makefile': makefile})

    # return generated id
    return jsonify(id=id)


@app.route('/submissions/<id>', methods=['GET'])
def check(id):
    """
    function, that told about current status of solution.
    Required id parametr - id, that was returned by @send_solution
    @return status ("READY" or "IN_PROCESS") and log and 
    result from 0 to 1
    """
    # read data
    try:
        data = formTaskStatusForResponse(id)
    except WrongRequest as e:
        return e.send_as_response()


    # send result
    return jsonify(data)

@app.route('{}'.format(XQ_CALLBACK_ROUTE), methods=['POST'])
def result_callback():
    """
    function for putting result from xqueue-server
    """
    # we can do nothing
    return "Result putted"

@app.route('/status', methods=['GET'])
def status():
    return readGlobalStatusFile()

@app.route('/debug_info', methods=['GET'])
def debug_info():
    return readDebugInfo()

@app.after_request
def after_request(response):
    response.headers.add('Access-Control-Allow-Origin', '*')
    response.headers.add('Access-Control-Allow-Headers',
                         'Content-Type, Authorization')
    response.headers.add('Access-Control-Allow-Methods',
                         'GET, POST, PUT, DELETE')
    return response
