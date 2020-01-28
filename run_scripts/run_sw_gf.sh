
cd /mnt/future-rd/benchmarks/futurerd-bench/basic-baseline

echo -e "running sw-gf baseline"
echo -e "on 1 core:\n"
taskset -c 0 ./sw-gf -n 2048
echo -e "\non 2 cores:\n"
taskset -c 0-1 ./sw-gf -n 2048
echo -e "\non 4 cores:\n"
taskset -c 0-3 ./sw-gf -n 2048
echo -e "\non 8 cores:\n"
taskset -c 0-7 ./sw-gf -n 2048
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./sw-gf -n 2048
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./sw-gf -n 2048

cd -
cd /mnt/future-rd/benchmarks/futurerd-bench/basic

echo -e "\nrunning sw-gf reachability"
echo -e "on 1 core:\n"
taskset -c 0 ./sw-gf -n 2048
echo -e "\non 2 cores:\n"
taskset -c 0-1 ./sw-gf -n 2048
echo -e "\non 4 cores:\n"
taskset -c 0-3 ./sw-gf -n 2048
echo -e "\non 8 cores:\n"
taskset -c 0-7 ./sw-gf -n 2048
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./sw-gf -n 2048
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./sw-gf -n 2048

echo -e "\nrunning sw-gf full detection"
echo -e "on 1 core:\n"
taskset -c 0 ./sw-gf-full -n 2048
echo -e "\non 2 cores:\n"
taskset -c 0-1 ./sw-gf-full -n 2048
echo -e "\non 4 cores:\n"
taskset -c 0-3 ./sw-gf-full -n 2048
echo -e "\non 8 cores:\n"
taskset -c 0-7 ./sw-gf-full -n 2048
echo -e "\non 16 cores:\n"
taskset -c 0-15 ./sw-gf-full -n 2048
echo -e "\non 20 cores:\n"
taskset -c 0-19 ./sw-gf-full -n 2048


