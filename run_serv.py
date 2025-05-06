#! /usr/bin/python3.4
# -*- coding: utf-8 -*-
"""
main for manually running
"""

import sys
import os

path_to_mooc = os.path.abspath(".") # путь к питоновским файлам.
sys.path.append(path_to_mooc)  # добавляем питоновские наши файлы к путям поиска библиотек. Для этого файла - не обязательно, но ради однообразия с mooc.wsgi
from linuxEBserv import app, setPathToMoocL # после предыдущей строки можем загрузить модуль

setPathToMoocL(path_to_mooc) # перебрасываем в модуль путь к папке

app.run() # для запуска
