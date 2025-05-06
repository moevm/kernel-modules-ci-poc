## Сборка и запуск докер-образов
```
cd image
docker build -t localhost/nix-image-builder:latest . 
make DOCKER=docker container-build
```

```
cd ..
docker compose build
docker compose up -d
```

## Тестирование

```
docker-compose exec web bash
./tst/integration/heavy_test.sh --url http://mooc-linux-programming/ kernel_params
```

Тесты должны завершиться с exit_code=0 и следующим текстом в конце: 

<pre>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
////////////////////////////////////////////////////
[SUCCESS] All tests successfuly completed!
</pre>


## Отладка
Для просмотра логов можно написать
`docker-compose logs -f web xqueue client`
