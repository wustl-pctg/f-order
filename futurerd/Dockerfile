FROM ubuntu:16.04

# User setup
ARG UNAME=testuser
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME

# Install dependencies
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y \
			g++ gcc make automake libtool python groff libc-dev \
			git wget datamash binutils-dev bsdmainutils \
			zlib1g zlib1g-dev openssl libssl-dev
RUN update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.gold" 20 && \
		update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.bfd" 10

USER $UNAME
CMD bash
