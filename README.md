# Gemtext to HTML static site generator 

## Сборка

Написано и протестировано на Debian 11.
Требуется поддержка pthread, наличие cmake и подходящей для него build системы (make, ninja), в остальном ничего platform-specific не используется.

```
$ cmake <project dir>
$ cmake --build .
```
Будет создан исполняемый файл `site_generator`.
```
$ ./site_generator 
usage: ./site_generator <input_dir> <output_dir> [-v]  
```
параметр `-v` (verbose) является опциональным; если указан, утилита будет записывать в stdout журнал выполненных действий:
```
created directory /tmp/output_dir/layer1dir_A/layer2dir_B
copied /tmp/input_dir/bird.jpg -> /tmp/output_dir/bird.jpg
parsed /tmp/input_dir/layer1dir_B/layer2dir_A/code.gmi -> /tmp/output_dir/layer1dir_B/layer2dir_A/code.html
```

* Входная директория `input_dir` должна существовать. 
* Если `output_dir` не существует, эта директория будет создана; `.gmi` файлы из `input_dir` будут преобразованы в `.html`; остальные скопированы. Если существует, директория не будет пересоздана.
* Если в `output_dir` существует файл по такому же относительному пути, этот файл будет пропущен (кроме файлов `.html`, которые будут транслированы из `.gmi` файлов и перезаписаны):
    * Например, если существует `input_dir/file.pdf` и `output_dir/file.pdf` (относительные пути), при выполнении утилиты этот файл будет проигнорирован. 
    * С другой стороны, существующий `output_dir/index.html` будет перезаписан, если существует `input_dir/index.gmi`.

Поддерево входной директории обходится в ширину (BFS реализован неявно: каждая `direntry` добавляется в очередь задач пула потоков и выполняется при появлении свободного потока). Таким образом, при рассмотрении каждой вершины дерева, гарантируется, что ее предки уже существуют в выходной директории. В зависимости от расширения (`.gmi`/ не `.gmi`) и типа файла (`regular file` / `directory`) каждая задача обрабатывается сооветствующим образом.  
Утилита работает в N потоков, где N — число доступных аппаратных потоков, включая hyperthreading (узнается с помощью `std::thread::hardware_concurrency()`).    
Для пула потоков используется header-only библиотека `BS_thread_pool.hpp`  
  

## Пример работы утилиты
````
$ tree in_dir
in_dir
└── test.gmi

0 directories, 1 file
$ cat in_dir/test.gmi
# Заголовок первого уровня
## Заголовок второго уровня
### Заголовок третьего уровня

* Элемент списка

> Цитата

=> http://some-address.com Ссылка

```
Преформатированный текст
```
$ ./site_generator in_dir out_dir
$ tree out_dir
out_dir
└── test.html

0 directories, 1 file
$ cat out_dir/test.html 
<h1>Заголовок первого уровня</h1>
<h2>Заголовок второго уровня</h2>
<h3>Заголовок третьего уровня</h3>

<ul>
<li>Элемент списка</li>
</ul>

<blockquote>Цитата</blockquote>

<a href="http://some-address.com">Ссылка</a>

<pre>
Преформатированный текст
</pre>

````
