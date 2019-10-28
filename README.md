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
TODO

$ ./louds-patricia < data/enwiki-20191001-all-titles-in-ns0.sorted
TODO
```
