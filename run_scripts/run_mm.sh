cd /mnt/future-rd/benchmarks/future-bench-baseline/
echo "running mm baseline"

echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./mm-sf -n 2048

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./mm-sf -n 2048

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./mm-sf -n 2048

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./mm-sf -n 2048

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./mm-sf -n 2048

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./mm-sf -n 2048

cd /mnt/future-rd/benchmarks/future-bench/  
echo -e "\nrunning mm reachability"


echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./mm-sf -n 2048

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./mm-sf -n 2048

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./mm-sf -n 2048

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./mm-sf -n 2048

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./mm-sf -n 2048

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./mm-sf -n 2048

echo -e "\nrunning mm full detection"

echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./mm-sf-full -n 2048

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./mm-sf-full -n 2048

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./mm-sf-full -n 2048

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./mm-sf-full -n 2048

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./mm-sf-full -n 2048

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./mm-sf-full -n 2048

