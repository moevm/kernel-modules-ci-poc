# mooc-linux-programming

MOOC: Linux Programming fundamentals

## Настройка среды

Установка зависимостей

<pre>
./scripts/install_dependencies_ubuntu.sh
./scripts/add_source_repos_ubuntu.sh
</pre>
Запускать на директорию выше

<pre>
./scripts/install_libvirt_dependencies_ubuntu.sh
</pre>

<pre>
./scripts/install_pip_dependencies.sh

 sudo python -m pip install -r xqueue_watcher/requirements/production.txt
</pre>

Установка последней версии библиотеки request

<pre>
pip install --upgrade requests
</pre>

Развертывание докер контейнера

Возможно понадобится создать папку и выдать права 777 на неё
<pre>
/var/www/mooc-linux-programming/local-lc
</pre>

<pre>
docker run -d --name xqueue \
  -p 8000:7777 \
  -v ./settings.py:/app/xqueue/xqueue/settings.py \
  -v /var/www/mooc-linux-programming/local-lc:/app/xqueue/local-lc \
  dvivanov/xqueue:18022025
</pre>


Скачивание образов

<pre>
./scripts/download_boxes.sh
</pre>


## Развертывание

Ubuntu 22.04

<pre>
sudo scripts/local_deploy.sh -s apache2
</pre>

## Тестирование


Как проверить, что все заработало (веб-сервер и демон): 
- проверить что http://mooc-linux-programming/hello отвечает "HELLO, WORLD!"

Как проверить, что чекеры заработали корректно (для удаленной проверки замените http://mooc-linux-programming/ на внешнее имя узла, который требуется проверить): 
- [Docker] **./tst/integration/heavy_test.sh --url http://mooc-linux-programming/ 1**

должны завершиться с exit_code=0 и следующим текстом в конце: 

<pre>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
////////////////////////////////////////////////////
[SUCCESS] All tests successfuly completed!
</pre>

## Устранение неполадок и логи

 - /var/www/mooc-linux-programming/pdaemon.log - логи демона проверки (отвечает за то, чтобы забрать студенческое решение из очереди, запустить проверку и отдать результат в очередь)
 - /var/www/mooc-linux-programming/error.log - логи веб-приложения (отвечает за прием тестовых решений)

## Взаимосвязь заданий

Задания 42, 72 и 162 это модифицированные версии заданий 4, 7 и 16. 
