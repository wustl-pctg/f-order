#!/bin/sh
set -e

docker build \
			 --build-arg UID=$(id -u) --build-arg GID=$(id -g) --build-arg UNAME=${USER}-docker \
			 -t futurerd:latest .


