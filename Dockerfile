# Builder image
FROM centos:7 as builder

# Install EPEL repository for cmake3
RUN yum -y install epel-release &&\
	yum -y clean all

# Install build tools and libraries
RUN yum -y install gcc gcc-c++ make cmake3 \
        glibc-devel libstdc++-devel libjpeg-turbo-devel libpng-devel zlib-devel \
        glibc-devel.i686 libstdc++-devel.i686 libjpeg-turbo-devel.i686 libpng-devel.i686 zlib-devel.i686 &&\
	yum -y clean all

# Copy sources
COPY . /usr/src/openjk

# Build i386 arch
RUN mkdir /usr/src/openjk/build.i386 &&\
	cd /usr/src/openjk/build.i386 &&\
	cmake3 -DCMAKE_TOOLCHAIN_FILE=CMakeModules/Toolchains/linux-i686.cmake \
		-DBuildMPCGame=OFF -DBuildMPEngine=OFF -DBuildMPRdVanilla=OFF -DBuildMPUI=OFF \
		-DBuildSPEngine=OFF -DBuildSPGame=OFF -DBuildSPRdVanilla=OFF \
		.. &&\
    make

# Build x86_64 arch
RUN mkdir /usr/src/openjk/build.x86_64 &&\
	cd /usr/src/openjk/build.x86_64 &&\
	cmake3 -DBuildMPCGame=OFF -DBuildMPEngine=OFF -DBuildMPRdVanilla=OFF -DBuildMPUI=OFF \
		-DBuildSPEngine=OFF -DBuildSPGame=OFF -DBuildSPRdVanilla=OFF \
		.. &&\
    make


# Server image
FROM centos:7

# Install utilities and libraries
RUN yum -y install file iproute less socat wget which \
        libstdc++ zlib \
        libstdc++.i686 zlib.i686 &&\
	yum -y clean all

# Copy binaries and scripts
RUN mkdir -p /opt/openjk/cdpath/base /opt/openjk/basepath /opt/openjk/homepath
COPY --from=builder /usr/src/openjk/build.i386/openjkded.i386 /opt/openjk/
COPY --from=builder /usr/src/openjk/build.i386/codemp/game/jampgamei386.so /opt/openjk/cdpath/base/
COPY --from=builder /usr/src/openjk/build.x86_64/openjkded.x86_64 /opt/openjk/
COPY --from=builder /usr/src/openjk/build.x86_64/codemp/game/jampgamex86_64.so /opt/openjk/cdpath/base/
COPY scripts/docker/*.sh /opt/openjk/
COPY scripts/docker/server.cfg /opt/openjk/cdpath/base/
RUN chmod +x /opt/openjk/openjkded.* /opt/openjk/*.sh

# Execution
ENV OJK_OPTS="+exec server.cfg"
EXPOSE 29070/udp
HEALTHCHECK --interval=10s --timeout=9s --retries=6 CMD ["/opt/openjk/healthcheck.sh"]
CMD ["/opt/openjk/run.sh"]
