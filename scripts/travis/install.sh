#!/bin/sh

set -e
set -x

host="$1"
shift 1

sudo apt-get update -yq

# This is what Travis does using the apt-addon. Didn't want to duplicate a load
# of packages in the .travis.yml file though so we have this script instead.
APT_INSTALL='sudo apt-get -yq --no-install-suggests --no-install-recommends --force-yes install'

${APT_INSTALL} cmake

case "${host}" in
	(native)
		${APT_INSTALL} \
			libsdl2-dev \
			libjpeg-turbo8-dev \
			zlib1g-dev \
			libpng-dev
		;;

	(i686-w64-mingw32)
		${APT_INSTALL} g++-mingw-w64-i686
		;;

	(x86_64-w64-mingw32)
		${APT_INSTALL} g++-mingw-w64-x86-64
		;;

	(i?86-linux-gnu)
		${APT_INSTALL} \
			libglib2.0-dev:i386 \
			libgl1-mesa-dev:i386 \
			libpulse-dev:i386 \
			libglu1-mesa-dev:i386 \
			libsdl2-dev:i386 \
			libjpeg-turbo8-dev:i386 \
			zlib1g-dev:i386 \
			libc6-dev:i386 \
			libpng-dev:i386 \
			cpp \
			g++ \
			g++-multilib \
			gcc \
			gcc-multilib \
			${NULL+}
		;;
esac
