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
build = 421.753 ns/key
#keys = 1887667
#nodes = 2581968
size = 23277697 bytes
seq. lookup = 306.363 ns/key
rnd. lookup = 915.683 ns/key

$ ./louds-patricia < data/enwiki-20191001-all-titles-in-ns0.sorted
build = 383.389 ns/key
#keys = 14837096
#nodes = 20342232
size = 161724112 bytes
seq. lookup = 328.714 ns/key
rnd. lookup = 1683.18 ns/key
```

## See also

https://github.com/s-yata/louds-trie
