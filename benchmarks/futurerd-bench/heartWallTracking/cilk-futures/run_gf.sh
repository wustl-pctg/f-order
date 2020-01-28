taskset -c 0    ./hw-gf ../data/test.avi 10 1
taskset -c 0-1  ./hw-gf ../data/test.avi 10 2
taskset -c 0-3  ./hw-gf ../data/test.avi 10 4
taskset -c 0-7  ./hw-gf ../data/test.avi 10 8
taskset -c 0-15 ./hw-gf ../data/test.avi 10 16
taskset -c 0-19 ./hw-gf ../data/test.avi 10 20
