# louds-patricia

LOUDS patricia trie implementation example (C++)

## Results

```
$ export LC_ALL=C

$ ls -sh1 data/
total 344M
303M enwiki-20191001-all-titles-in-ns0
 41M jawiki-20191001-all-titles-in-ns0

$ sort data/jawiki-20191001-all-titles-in-ns0 > data/jawiki-20191001-all-titles-in-ns0.sorted
$ sort data/enwiki-20191001-all-titles-in-ns0 > data/enwiki-20191001-all-titles-in-ns0.sorted

$ make
g++ -O2 -Wall -Wextra -march=native *.cpp -o louds-patricia

$ ./louds-patricia < data/jawiki-20191001-all-titles-in-ns0.sorted
build = 421.433 ns/key
#keys = 1887667
#nodes = 2581968
size = 23277697 bytes
seq. lookup = 259.545 ns/key
rnd. lookup = 885.37 ns/key

$ ./louds-patricia < data/enwiki-20191001-all-titles-in-ns0.sorted
build = 383.461 ns/key
#keys = 14837096
#nodes = 20342232
size = 161724112 bytes
seq. lookup = 265.385 ns/key
rnd. lookup = 1526.46 ns/key
```

## See also

https://github.com/s-yata/louds-trie
