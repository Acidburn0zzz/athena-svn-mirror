# Note this is NOT a relocatable thing :)
%define name		xml-i18n-tools
%define ver		0.6
%define RELEASE		0
%define rel		%{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix		/usr
%define sysconfdir	/etc

Name:		%name
Summary:	This module contains some utility scripts and assorted auto* magic for internationalizing various kinds of XML files.
Version: 	%ver
Release: 	%rel
Copyright: 	GPL
Group:		Development/Tools
Source: 	%{name}-%{ver}.tar.gz
URL: 		http://nautilus.eazel.com/
BuildRoot:	/var/tmp/%{name}-%{ver}-root
Docdir: 	%{prefix}/doc

%description
** Automatically extracts translatable strings from oaf, glade, bonobo
  ui, nautilus theme and other XML files into the po files.

** Automatically merges translations from po files back into .oaf files
  (encoding to be 7-bit clean). I can also extend this merging
  mechanism to support other types of XML files.

%package devel
Summary:	Libraries and include files for developing Ammonite clients
Group:		Development/Libraries
Requires:	%name = %{PACKAGE_VERSION}
Obsoletes:	%{name}-devel

%description devel
This package provides the necessary development libraries and include
files to allow you to develop components that make use of the Ammonite authentication
services.  You will need to install this package if you intend to build Nautilus
from source code.

%changelog
* Thu Jan 04 2000 Robin * Slomkowski <rslomkow@eazel.com>
- created this thing

%prep
%setup

%build
%ifarch alpha
  MYARCH_FLAGS="--host=alpha-redhat-linux"
%endif

LC_ALL=""
LINGUAS=""
LANG=""
export LC_ALL LINGUAS LANG

CFLAGS="$RPM_OPT_FLAGS" ./configure $MYARCH_FLAGS --prefix=%{prefix} \
	--sysconfdir=%{sysconfdir}

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT%{prefix} sysconfdir=$RPM_BUILD_ROOT%{sysconfdir} install

for FILE in "$RPM_BUILD_ROOT/bin/*"; do
	file "$FILE" | grep -q not\ stripped && strip $FILE
done

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%post
  
%postun 

%files

%defattr(0555, bin, bin)
%{prefix}/bin/xml-i18n-extract
%{prefix}/bin/xml-i18n-toolize
%{prefix}/bin/xml-i18n-update
%{prefix}/share/xml-i18n-tools/*

%defattr (0444, bin, bin)
%doc AUTHORS COPYING ChangeLog NEWS README
%{prefix}/share/aclocal/*.m4
