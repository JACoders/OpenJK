# Builder image
FROM ubuntu:18.04 as builder

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
		-DBuildSPEngine=OFF -DBuildSPGame=OFF -DBuildSPRdVanilla=OFF \
		.. &&\
	make &&\
	make install

# Build x86_64 arch
RUN mkdir /usr/src/openjk/build.x86_64 &&\
	cd /usr/src/openjk/build.x86_64 &&\
	cmake -DCMAKE_INSTALL_PREFIX=/opt \
		-DBuildMPCGame=OFF -DBuildMPEngine=OFF -DBuildMPRdVanilla=OFF -DBuildMPUI=OFF \
		-DBuildSPEngine=OFF -DBuildSPGame=OFF -DBuildSPRdVanilla=OFF \
		.. &&\
	make &&\
	make install


# Server image
FROM ubuntu:18.04

# Install utilities and libraries
RUN dpkg --add-architecture i386 &&\
	apt-get -q update &&\
	DEBIAN_FRONTEND="noninteractive" apt-get -q install -y -o Dpkg::Options::="--force-confnew" --no-install-recommends socat libstdc++6 libstdc++6:i386 zlib1g zlib1g:i386 &&\
	rm -rf /var/lib/apt/lists/*

# Copy binaries and scripts
RUN mkdir -p /opt/openjk/cdpath/base /opt/openjk/basepath /opt/openjk/homepath
COPY --from=builder /opt/JediAcademy/openjkded.* /opt/openjk/
COPY --from=builder /opt/JediAcademy/base/ /opt/openjk/cdpath/base/
COPY --from=builder /opt/JediAcademy/OpenJK/ /opt/openjk/cdpath/OpenJK/
COPY scripts/docker/*.sh /opt/openjk/
COPY scripts/docker/server.cfg /opt/openjk/cdpath/base/
COPY scripts/docker/server.cfg /opt/openjk/cdpath/OpenJK/
RUN chmod +x /opt/openjk/openjkded.* /opt/openjk/*.sh

# Execution
ENV OJK_OPTS="+exec server.cfg"
EXPOSE 29070/udp
HEALTHCHECK --interval=10s --timeout=9s --retries=6 CMD ["/opt/openjk/healthcheck.sh"]
CMD ["/opt/openjk/run.sh"]
