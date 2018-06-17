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
			"$@"
		;;

	(i?86-linux-gnu)
		set -- \
			-D CMAKE_TOOLCHAIN_FILE=$(pwd)/CMakeModules/Toolchains/linux-i686.cmake \
			"$@"
		;;
	(macosx-universal-clang)
		set -- \
			-D UseInternalJPEG=ON \
			-D UseInternalPNG=ON \
			-D UseInternalOpenAL=ON \
			-D UseInternalSDL2=OFF \
			-D UseInternalZlib=OFF \
			-D CMAKE_OSX_SYSROOT="" \
			-D OPENGL_INCLUDE_DIR=/System/Library/Frameworks/OpenGL.framework \
			-D OPENGL_gl_LIBRARY=/System/Library/Frameworks/OpenGL.framework \
			-D OPENGL_glu_LIBRARY=/System/Library/Frameworks/OpenGL.framework \
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
		if [ -n "${deploy}" ]; then
		( cd $(pwd)/build/DESTDIR/prefix/JediAcademy/ && \
			tar czvf eternaljk-macos-"${arch}".tar.gz * && \
			mv eternaljk-macos-"${arch}".tar.gz /Users/travis/build/eternalcodes/EternalJK/ && \
			cd ../../ && \
			find . -ls )
		else
		( cd $(pwd)/build/DESTDIR && find . -ls )
		fi
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
	(i686-w64-mingw32)
		if [ -n "${deploy}" ]; then
			( cd $(pwd)/build/DESTDIR/prefix/JediAcademy/ && \
				zip -r eternaljk-win32-portable.zip * && \
				mv eternaljk-win32-portable.zip /home/travis/build/eternalcodes/EternalJK/ && \
				cd eternaljk/ && \
				zip -r ejk-japro-pk3only.zip * && \
				mv ejk-japro-pk3only.zip /home/travis/build/eternalcodes/EternalJK/ && \
				cd ../../../ && \
				find . -ls )
		else
		( cd $(pwd)/build/DESTDIR && find . -ls )
		fi
		;;
	(*)
		( cd $(pwd)/build/DESTDIR && find . -ls )
		;;
esac
