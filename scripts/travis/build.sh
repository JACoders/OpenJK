#!/bin/sh

set -e
set -x

host="$1"
flavour="$2"
shift 2

mkdir deps build

case "${host}" in
	(*-w64-mingw32)
		export CC=${host}-gcc
		export CXX=${host}-g++
		set -- \
			-D CMAKE_TOOLCHAIN_FILE=$(pwd)/CMakeModules/Toolchains/${host}.cmake \
			-D BuildMPCGame=OFF \
			"$@"
		;;

	(i?86-linux-gnu)
		set -- \
			-D CMAKE_TOOLCHAIN_FILE=$(pwd)/CMakeModules/Toolchains/linux-i686.cmake \
			"$@"
		;;
	(macosx-universal-clang)
		set -- \
			"$@"
		;;
	(native)
		if [ -n "${deploy}" ]; then
			set -- \
				"$@"
		fi
		;;
	(*)
		set +x
		echo "Error: don't know how to cross-compile for ${host} host"
		exit 1
		;;
esac

set -- -D CMAKE_BUILD_TYPE="$flavour" "$@"

( cd build && cmake \
	-D CMAKE_INSTALL_PREFIX=/prefix \
	"$@" .. )
make -C build
make -C build install DESTDIR=$(pwd)/build/DESTDIR

if [ x"${host}" = xi686-linux-gnu ]; then
	arch="i686"
else
	arch="x86_64"
fi

case "${host}" in
	(macosx-universal-clang)
		( cd $(pwd)/build/DESTDIR/prefix/JediAcademy/eternaljk.x86_64.app/ && \
			tar czvf eternaljk-macos-"${arch}".tar.gz * && \
			mv eternaljk-macos-"${arch}".tar.gz /Users/travis/build/eternalcodes/EternalJK/ && \
			cd ../../../ && \
			find . -ls )
		;;
	(i?86-linux-gnu|native)
		if [ -n "${deploy}" ]; then
			( cd $(pwd)/build/DESTDIR/prefix/JediAcademy/ && \
				tar czvf eternaljk-linux-"${arch}".tar.gz * && \
				mv eternaljk-linux-"${arch}".tar.gz /home/travis/build/eternalcodes/EternalJK/ && \
				cd ../../ && \
				find . -ls )
		else
			( cd $(pwd)/build/DESTDIR && find . -ls )
		fi
		;;
	(*)
		( cd $(pwd)/build/DESTDIR && find . -ls )
		;;
esac