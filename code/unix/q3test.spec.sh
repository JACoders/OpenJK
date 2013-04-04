#!/bin/sh
# Generate Quake3 test
# $1 is version
# $2 is release
# $3 is arch
# $4 is install dir (assumed to be in /var/tmp)
cat <<EOF
%define name q3test
%define version ${1}
%define release ${2}
%define arch ${3}
%define builddir \$RPM_BUILD_DIR/%{name}-%{version}
Name:		%{name}
Version:	%{version}
Release:	%{release}
Vendor:		id Software
Packager:	Dave "Zoid" Kirsch <zoid@idsoftware.com>
URL:		http://www.idsoftware.com/
Source:		q3test-%{version}.tar.gz
Group:		Games
Copyright:	Restricted
Icon:		quake3.gif
BuildRoot:	/var/tmp/%{name}-%{version}
Summary:	Q3Test for Linux

%description

%install

%files

%attr(644,root,root) $4/README.EULA
%attr(644,root,root) $4/README.Q3Test
%attr(755,root,root) $4/linuxquake3
%attr(755,root,root) $4/cgamei386.so
%attr(755,root,root) $4/qagamei386.so
%attr(755,root,root) $4/libMesaVoodooGL.so.3.1
%attr(644,root,root) $4/demoq3/pak0.pk3

EOF

