cd /mnt/future-rd/benchmarks/future-bench-baseline/
echo "running smm baseline"

echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./smm-sf -n 2048

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./smm-sf -n 2048

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./smm-sf -n 2048

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./smm-sf -n 2048

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./smm-sf -n 2048

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./smm-sf -n 2048

cd /mnt/future-rd/benchmarks/future-bench/  
echo -e "\nrunning smm reachability"


echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./smm-sf -n 2048

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./smm-sf -n 2048

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./smm-sf -n 2048

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./smm-sf -n 2048

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./smm-sf -n 2048

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./smm-sf -n 2048

echo -e "\nrunning smm full detection"

echo -e "on 1 core:\n"
CILK_NWORKERS=1 ./smm-sf-full -n 2048

echo -e "\non 2 cores:\n"
CILK_NWORKERS=2 ./smm-sf-full -n 2048

echo -e "\non 4 cores:\n"
CILK_NWORKERS=4 ./smm-sf-full -n 2048

echo -e "\non 8 cores:\n"
CILK_NWORKERS=8 ./smm-sf-full -n 2048

echo -e "\non 16 cores:\n"
CILK_NWORKERS=16 ./smm-sf-full -n 2048

echo -e "\non 20 cores:\n"
CILK_NWORKERS=20 ./smm-sf-full -n 2048

