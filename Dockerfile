FROM ubuntu:18.04 AS builder

# Install build tools and libraries
RUN dpkg --add-architecture i386 &&\
	apt-get -q update &&\
	DEBIAN_FRONTEND="noninteractive" apt-get -q install -y -o Dpkg::Options::="--force-confnew" --no-install-recommends build-essential gcc-multilib g++-multilib cmake libjpeg-dev libjpeg-dev:i386 libpng-dev libpng-dev:i386 zlib1g-dev zlib1g-dev:i386 &&\
	rm -rf /var/lib/apt/lists/*

# Copy sources
COPY . /usr/src/openjk

# Build i386 arch
RUN mkdir /usr/src/openjk/build.i386 &&\
	cd /usr/src/openjk/build.i386 &&\
	cmake -DCMAKE_TOOLCHAIN_FILE=/usr/src/openjk/cmake/Toolchains/linux-i686.cmake \
		-DCMAKE_INSTALL_PREFIX=/opt \
		-DBuildMPCGame=OFF -DBuildMPEngine=OFF -DBuildMPRdVanilla=OFF -DBuildMPUI=OFF \
		-DBuildSPEngine=OFF -DBuildSPGame=OFF -DBuildSPRdVanilla=OFF -DBuildMPRend2=OFF \
		.. && \
	make && \
	make install

FROM ubuntu:jammy

# Install utilities and libraries
RUN dpkg --add-architecture i386 &&\
	apt-get -q update &&\
	DEBIAN_FRONTEND="noninteractive" apt-get -q install -y -o Dpkg::Options::="--force-confnew" --no-install-recommends socat libstdc++6 libstdc++6:i386 zlib1g zlib1g:i386 &&\
	rm -rf /var/lib/apt/lists/*

# Copy binaries and scripts
COPY --from=builder /opt/JediAcademy/openjkded.* /ja/
COPY --from=builder /opt/JediAcademy/base/ /ja/base/
COPY --from=builder /opt/JediAcademy/OpenJK/ /ja/OpenJK/

WORKDIR /ja

COPY ./ci/assets/ base/

COPY ["./ci/start_server.sh", "start_server.sh"]

RUN chmod a+x openjkded.i386 \
    && chmod a+x start_server.sh

EXPOSE 29070/udp

CMD ["./start_server.sh"]