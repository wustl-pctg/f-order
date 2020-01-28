docker run --rm -it \
			 -u=$(id -u):$(id -g) \
			 --security-opt seccomp=unconfined \
			 -v${PWD}:/mnt/futurerd -w=/mnt/futurerd futurerd
