%define ver    0.5.7 
%define  RELEASE 1
%define  rel     %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix  /usr

Summary: High-performance CORBA Object Request Broker.
Name: ORBit
Version: %ver
Release: %rel
Source: ftp://ftp.labs.redhat.com/pub/ORBit/ORBit-%{PACKAGE_VERSION}.tar.gz
Group: System Environment/Libraries
Copyright: LGPL/GPL
BuildRoot: /var/tmp/orbit-%{PACKAGE_VERSION}-root
Prefix: %{prefix}
Docdir: %{prefix}/doc
Prereq: /sbin/install-info
URL: http://www.labs.redhat.com/orbit/

%description
ORBit is a high-performance CORBA (Common Object Request Broker 
Architecture) ORB (object request broker). It allows programs to 
send requests and receive replies from other programs, regardless 
of the locations of the two programs. CORBA is an architecture that 
enables communication between program objects, regardless of the 
programming language they're written in or the operating system they
run on.

You will need to install this package if you want to run programs that use
the ORBit implementation of CORBA technology.

%package devel
Summary: Development libraries, header files and utilities for ORBit.
Group: Development/Libraries
Requires: indent
Requires: glib-devel
Requires: ORBit = %{ver}

%description devel
This package contains the header files, libraries and utilities 
necessary to write programs that use CORBA technology. If you want to
write such programs, you'll also need to install the ORBit package.

%prep
%setup

%build
# Needed for snapshot releases.
#MYCFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS $RPM_OPT_FLAGS"
MYCFLAGS="$RPM_OPT_FLAGS"
if [ ! -f configure ]; then
	CFLAGS="$MYCFLAGS" ./autogen.sh --prefix=%prefix
else
	CFLAGS="$MYCFLAGS" ./configure --prefix=%prefix
fi
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{prefix}
make prefix=$RPM_BUILD_ROOT%{prefix} install

/sbin/ldconfig -n $RPM_BUILD_ROOT%{prefix}/lib

strip $RPM_BUILD_ROOT%{prefix}/bin/* || :

gzip -9 $RPM_BUILD_ROOT%{prefix}/info/*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post devel
/sbin/install-info %{prefix}/info/libIDL.info.gz %{prefix}/info/dir

%preun devel
if [ $1 = 0 ]; then
   /sbin/install-info --delete %{prefix}/info/libIDL.info.gz %{prefix}/info/dir
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)

%doc AUTHORS COPYING ChangeLog NEWS README TODO
%doc -P libIDL/COPYING libIDL/ChangeLog libIDL/AUTHORS
%doc -P libIDL/README* libIDL/NEWS libIDL/BUGS libIDL/tstidl.c

%{prefix}/lib/lib*.so.*
%{prefix}/bin/orbit-event-server
%{prefix}/bin/orbit-name-server
#don't install old-name-server
#%{prefix}/bin/old-name-server
%{prefix}/bin/name-client
%{prefix}/bin/orbit-ird

%files devel
%{prefix}/bin/orbit-idl
%{prefix}/bin/orbit-config
%{prefix}/bin/libIDL-config
%{prefix}/include/*
%{prefix}/info/libIDL.info.gz
%{prefix}/lib/*.sh
%{prefix}/lib/lib*.a
%{prefix}/lib/lib*.so
%{prefix}/share/aclocal/*

%changelog
* Mon Aug 30 1999 Elliot Lee <sopwith@redhat.com> 0.4.94-1
- Spec file fixes from RHL 6.0.

* Wed Jun 2 1999  Jose Mercado <jmercado@mit.edu>
- Fixed configure.in so spec.in could be used.

* Mon Nov 23 1998 Pablo Saratxaga <srtxg@chanae.alphanet.ch>

- improved %files section, and added use of %{prefix} and install-info
  (well,... no. The info file has not dir info inside, commented out)
