
cd /mnt/future-rd/benchmarks/ferret

echo -e "running ferret baseline"
echo -e "on 1 core:\n"
taskset -c 0 ./run.sh cilk-future-base simlarge cilk-future-base-1.out 1 4
echo -e "\non 2 cores:\n"
taskset -c 0-1 ./run.sh cilk-future-base simlarge cilk-future-base-2.out 2 8
echo -e "\non 4 cores:\n"
taskset -c 0-3 ./run.sh cilk-future-base simlarge cilk-future-base-4.out 4 16
echo -e "\non 8 cores:\n"
taskset -c 0-7 ./run.sh cilk-future-base simlarge cilk-future-base-8.out 8 32
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./run.sh cilk-future-base simlarge cilk-future-base-16.out 16 64
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./run.sh cilk-future-base simlarge cilk-future-base-20.out 20 80

echo -e "\nrunning ferret reachability"
echo -e "on 1 core:\n"
taskset -c 0 ./run.sh cilk-future simlarge cilk-future-base-1.out 1 4
echo -e "\non 2 cores:\n"
taskset -c 0-1 ./run.sh cilk-future simlarge cilk-future-base-2.out 2 8
echo -e "\non 4 cores:\n"
taskset -c 0-3 ./run.sh cilk-future simlarge cilk-future-base-4.out 4 16
echo -e "\non 8 cores:\n"
taskset -c 0-7 ./run.sh cilk-future simlarge cilk-future-base-8.out 8 32
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./run.sh cilk-future simlarge cilk-future-base-16.out 16 64
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./run.sh cilk-future simlarge cilk-future-base-20.out 20 80

echo -e "\nrunning ferret full detection"
echo -e "on 1 core:\n"
taskset -c 0 ./run.sh cilk-future-full simlarge cilk-future-base-1.out 1 4
echo -e "\non 2 cores:\n"
taskset -c 0-1 ./run.sh cilk-future-full simlarge cilk-future-base-2.out 2 8
echo -e "\non 4 cores:\n"
taskset -c 0-3 ./run.sh cilk-future-full simlarge cilk-future-base-4.out 4 16
echo -e "\non 8 cores:\n"
taskset -c 0-7 ./run.sh cilk-future-full simlarge cilk-future-base-8.out 8 32
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./run.sh cilk-future-full simlarge cilk-future-base-16.out 16 64
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./run.sh cilk-future-full simlarge cilk-future-base-20.out 20 80
