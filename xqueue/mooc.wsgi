#! /usr/bin/python3
# -*- coding: utf-8 -*-
"""
main file for Apache (httpd) running.
"""

import sys
import os
import logging

logging.basicConfig(stream=sys.stderr)
path_to_mooc = os.path.dirname(os.path.realpath(__file__))
sys.path.append(path_to_mooc) # добавляем питоновские наши файлы к путям поиска библиотек
from linuxEBserv import app, setPathToMoocL # после предыдущей строки можем загрузить модуль

setPathToMoocL(path_to_mooc) # перебрасываем в модуль путь к папке

application = app # для запуска
