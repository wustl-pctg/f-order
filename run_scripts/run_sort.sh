cd /mnt/future-rd/benchmarks/future-bench-baseline/
echo "running sort baseline"

echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./sort-sf -n 10000000

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./sort-sf -n 10000000

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./sort-sf -n 10000000

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./sort-sf -n 10000000

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./sort-sf -n 10000000

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./sort-sf -n 10000000

cd -
cd /mnt/future-rd/benchmarks/future-bench/  
echo -e "\nrunning sort reachability"


echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./sort-sf -n 10000000

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./sort-sf -n 10000000

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./sort-sf -n 10000000

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./sort-sf -n 10000000

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./sort-sf -n 10000000

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./sort-sf -n 10000000

echo -e "\nrunning sort full detection"

echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./sort-sf-full -n 10000000

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./sort-sf-full -n 10000000

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./sort-sf-full -n 10000000

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./sort-sf-full -n 10000000

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./sort-sf-full -n 10000000

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./sort-sf-full -n 10000000

