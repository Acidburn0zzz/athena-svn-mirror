# Note that this is NOT a relocatable package
%define name		nautilus
%define ver		0.8
%define RELEASE		0_cvs_0
%define rel		%{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix		/usr
%define sysconfdir	/etc

Name:		%name
Summary:	Nautilus is a free file manager and graphical shell.
Vendor:		GNOME
Distribution:	CVS
Version: 	%ver
Release: 	%rel
Copyright: 	GPL
Group:		User Interface/Desktop
Source: 	%{name}-%{ver}.tar.gz
URL: 		http://nautilus.eazel.com/
BuildRoot:	/var/tmp/%{name}-%{ver}-root
Docdir: 	%{prefix}/doc
Requires:	ORBit = 0.5.6
Requires:	glib >= 1.2.8
Requires:	gtk+ >= 1.2.8
Requires:	imlib >= 1.9.8
Requires:	libxml >= 1.8.10
Requires:	gnome-libs >= 1.2.8
Requires:	GConf >= 0.12
Requires:	oaf >= 0.6.2
Requires:	gnome-vfs >= 0.4.2
Requires:	gdk-pixbuf >= 0.9.0
Requires:	bonobo >= 0.33
Requires:	popt >= 1.5
Requires:	freetype2 >= 2.0
Requires:	medusa >= 0.2.2
Requires:	esound >= 0.2
Requires:	libghttp >= 1.0.9

%description
Nautilus is an open-source file manager and graphical shell being
developed by Eazel, Inc. and others. It is part of the GNOME project, and its
source code can be found in the GNOME CVS repository. Nautilus is still in
the early stages of development. It will become an integral part of the
GNOME desktop environment when it is finished.

%package devel
Summary:	Libraries and include files for developing Nautilus components
Group:		Development/Libraries
Requires:	%name = %{PACKAGE_VERSION}

%package mozilla
Summary:        Nautilus component for use with Mozilla
Group:          User Interface/Desktop
Requires:       %name = %{PACKAGE_VERSION}
Requires:	mozilla = 0.7
Requires:	mozilla-mail = 0.7
Requires:	mozilla-psm = 0.7

%package trilobite
Summary:        Nautilus component framework for services
Group:          User Interface/Desktop
Requires:       %name = %{PACKAGE_VERSION}
Requires:	ammonite >= 0.1
Requires:	rpm >= 3.0.5
Requires:	usermode >= 1.35

%package extras
Summary:	Extra goodies to use with Nautilus
Group:          User Interface/Desktop
Requires:	xpdf >= 0.90

%package suggested
Summary:	Nautilus and a suggested set of components
Group:          User Interface/Desktop
Requires:       %name = %{PACKAGE_VERSION}
Requires:	%name-mozilla = %{PACKAGE_VERSION}
Requires:	%name-trilobite = %{PACKAGE_VERSION}
Requires:	%name-extras = %{PACKAGE_VERSION}

%description devel
This package provides the necessary development libraries and include
files to allow you to develop Nautilus components.

%description mozilla
This enables the use of embedded Mozilla as a Nautilus component.

%description trilobite
This is a framework library for service components in Nautilus.  It is
required by all Eazel services, including the package installer, and
can be used to develop new services.

%description suggested
This is a meta-package that requires packages useful for running
Nautilus, and getting multimedia to work, such as eog and mpg123.

%description extras
This is a meta-package that requires useful add-ons for Nautilus.

%changelog
* Tue Oct 10 2000 Robin Slomkowski <rslomkow@eazel.com>
- removed obsoletes from sub packages and added mozilla and trilobite
subpackages

* Wed Apr 26 2000 Ramiro Estrugo <ramiro@eazel.com>
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

## Warning!  Make sure there are no spaces or tabs after the \ 
## continuation character, or else the rpm demons will eat you.
CFLAGS="$RPM_OPT_FLAGS" ./configure $MYARCH_FLAGS --prefix=%{prefix} \
	--enable-eazel-services \
	--enable-more-warnings \
	--sysconfdir=%{sysconfdir}

make -k check

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT
make -k prefix=$RPM_BUILD_ROOT%{prefix} sysconfdir=$RPM_BUILD_ROOT%{sysconfdir} install
for FILE in "$RPM_BUILD_ROOT/bin/*"; do
	file "$FILE" | grep -q not\ stripped && strip $FILE
done

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%post
if ! grep %{prefix}/lib /etc/ld.so.conf > /dev/null ; then
	echo "%{prefix}/lib" >> /etc/ld.so.conf
fi
/sbin/ldconfig

%postun -p /sbin/ldconfig

%files

%defattr(0555, bin, bin)
%doc AUTHORS COPYING COPYING.LIB ChangeLog NEWS README
%{prefix}/bin/*.sh
%{prefix}/bin/eazel-helper
%{prefix}/bin/gnome-db2html2
%{prefix}/bin/gnome-info2html2
%{prefix}/bin/gnome-man2html2
%{prefix}/bin/hyperbola
%{prefix}/bin/nautilus
%{prefix}/bin/nautilus-adapter
%{prefix}/bin/nautilus-authenticate
%{prefix}/bin/nautilus-content-loser
%{prefix}/bin/nautilus-error-dialog
%{prefix}/bin/nautilus-hardware-view
%{prefix}/bin/nautilus-history-view
%{prefix}/bin/nautilus-image-view
# %{prefix}/bin/nautilus-mpg123
%{prefix}/bin/nautilus-music-view
%{prefix}/bin/nautilus-notes
%{prefix}/bin/nautilus-sample-content-view
%{prefix}/bin/nautilus-sidebar-loser
%{prefix}/bin/nautilus-text-view
%{prefix}/bin/nautilus-throbber
%{prefix}/bin/run-nautilus
%{prefix}/bin/nautilus-launcher-applet
%{prefix}/idl/*.idl
%{prefix}/lib/libnautilus-adapter.so.0
%{prefix}/lib/libnautilus-adapter.so.0.0.0
%{prefix}/lib/libnautilus-extensions.so.0
%{prefix}/lib/libnautilus-extensions.so.0.0.0
%{prefix}/lib/libnautilus-tree-view.so.0
%{prefix}/lib/libnautilus-tree-view.so.0.0.0
%{prefix}/lib/libnautilus.so.0
%{prefix}/lib/libnautilus.so.0.0.0
%{prefix}/lib/libnautilus-adapter.so
%{prefix}/lib/libnautilus-extensions.so
%{prefix}/lib/libnautilus-tree-view.so
%{prefix}/lib/libnautilus.so



%{prefix}/lib/vfs/modules/*.so


%defattr (0444, bin, bin)
%config %{sysconfdir}/vfs/modules/*.conf
%config %{sysconfdir}/CORBA/servers/nautilus-launcher-applet.gnorba
%{prefix}/share/gnome/apps/Applications/*.desktop
%{prefix}/share/gnome/ui/*.xml
%{prefix}/share/hyperbola/maps/*.map
%{prefix}/share/locale/*/LC_MESSAGES/*.mo
%{prefix}/share/nautilus/*.xml
%{prefix}/share/nautilus/emblems/*.png
%{prefix}/share/nautilus/fonts/urw/*.dir
%{prefix}/share/nautilus/fonts/urw/*.pfb
%{prefix}/share/nautilus/fonts/urw/*.afm
%{prefix}/share/nautilus/fonts/urw/*.pfm
%{prefix}/share/nautilus/linksets/*.xml
%{prefix}/share/nautilus/patterns/*.jpg
%{prefix}/share/nautilus/patterns/*.png
%{prefix}/share/nautilus/patterns/.*.png
%{prefix}/share/nautilus/services/text/*.xml
%{prefix}/share/nautilus/top/.*.xml
%{prefix}/share/nautilus/top/Computer
%{prefix}/share/nautilus/top/Services
%{prefix}/share/pixmaps/*.png
%{prefix}/share/pixmaps/nautilus/*.gif
%{prefix}/share/pixmaps/nautilus/*.png
%{prefix}/share/pixmaps/nautilus/*.svg
%{prefix}/share/pixmaps/nautilus/*.xml
%{prefix}/share/pixmaps/nautilus/ardmore/*.png
%{prefix}/share/pixmaps/nautilus/ardmore/*.xml
%{prefix}/share/pixmaps/nautilus/arlo/*.png
%{prefix}/share/pixmaps/nautilus/arlo/*.xml
%{prefix}/share/pixmaps/nautilus/arlo/throbber/*.png
%{prefix}/share/pixmaps/nautilus/arlo/backgrounds/*.png
%{prefix}/share/pixmaps/nautilus/arlo/sidebar_tab_pieces/*.png
%{prefix}/share/pixmaps/nautilus/crux_eggplant/*.png
%{prefix}/share/pixmaps/nautilus/crux_eggplant/*.xml
%{prefix}/share/pixmaps/nautilus/crux_eggplant/throbber/*.png
%{prefix}/share/pixmaps/nautilus/crux_eggplant/backgrounds/*.png
%{prefix}/share/pixmaps/nautilus/crux_eggplant/sidebar_tab_pieces/*.png
%{prefix}/share/pixmaps/nautilus/gnome/*.png
%{prefix}/share/pixmaps/nautilus/gnome/*.xml
%{prefix}/share/pixmaps/nautilus/gnome/throbber/*.png
%{prefix}/share/pixmaps/nautilus/sidebar_tab_pieces/*.png
%{prefix}/share/pixmaps/nautilus/throbber/*.png
%{prefix}/share/pixmaps/nautilus/gray_tab_pieces/*.png
%{prefix}/share/pixmaps/nautilus/villanova/*.xml
%{prefix}/share/pixmaps/nautilus/villanova/*.png
%{prefix}/share/oaf/Nautilus_View_help.oaf
%{prefix}/share/oaf/Nautilus_ComponentAdapterFactory_std.oaf
%{prefix}/share/oaf/Nautilus_View_content-loser.oaf
%{prefix}/share/oaf/Nautilus_View_hardware.oaf
%{prefix}/share/oaf/Nautilus_View_history.oaf
%{prefix}/share/oaf/Nautilus_View_image.oaf
%{prefix}/share/oaf/Nautilus_View_music.oaf
%{prefix}/share/oaf/Nautilus_View_notes.oaf
%{prefix}/share/oaf/Nautilus_View_sample.oaf
%{prefix}/share/oaf/Nautilus_View_sidebar-loser.oaf
%{prefix}/share/oaf/Nautilus_View_text.oaf
%{prefix}/share/oaf/Nautilus_View_tree.oaf
%{prefix}/share/oaf/Nautilus_shell.oaf
%{prefix}/share/oaf/Nautilus_Control_throbber.oaf
%{prefix}/share/gnome/help/nautilus/C/*
%{prefix}/share/gnome/help/gnufdl/C/*

%files devel

%defattr(0555, bin, bin)
%{prefix}/lib/*.la
%{prefix}/lib/vfs/modules/*.la

%defattr(0444, bin, bin)
%{prefix}/include/libtrilobite/eazel/*/*.h
%{prefix}/include/libnautilus/*.h
%{prefix}/include/libtrilobite/*.h

%files mozilla

%defattr(0555, bin, bin)
%{prefix}/bin/nautilus-mozilla-content-view

%defattr(0444, bin, bin)
%{prefix}/share/oaf/Nautilus_View_mozilla.oaf

%files trilobite

%defattr(0555, bin, bin)
%{prefix}/bin/eazel-install
%{prefix}/bin/nautilus-service-install-view
%{prefix}/bin/trilobite-eazel-install-service
%{prefix}/bin/nautilus-summary-view
%{prefix}/bin/nautilus-change-password-view
%{prefix}/bin/nautilus-rpm-view
%{prefix}/bin/eazel-gen-xml
%{prefix}/bin/eazel-vault
%{prefix}/lib/libeazelinstall.so.0
%{prefix}/lib/libeazelinstall.so.0.0.0
%{prefix}/lib/libeazelpackagesystem.so
%{prefix}/lib/libeazelpackagesystem.so.0
%{prefix}/lib/libeazelpackagesystem.so.0.0.0
%{prefix}/lib/libeazelpackagesystem-rpm*.so
%{prefix}/lib/libeazelpackagesystem-rpm*.so.0
%{prefix}/lib/libeazelpackagesystem-rpm*.so.0.0.0
%{prefix}/lib/libtrilobite-service.so.0
%{prefix}/lib/libtrilobite-service.so.0.0.0
%{prefix}/lib/libtrilobite.so.0
%{prefix}/lib/libtrilobite.so.0.0.0
%{prefix}/lib/libeazelinstall.so
%{prefix}/lib/libtrilobite-service.so
%{prefix}/lib/libtrilobite.so

%defattr(0444, bin, bin)
%config %{sysconfdir}/pam.d/eazel-helper
%config %{sysconfdir}/security/console.apps/eazel-helper
%{prefix}/share/oaf/Nautilus_View_install.oaf
%{prefix}/share/oaf/Trilobite_Service_install.oaf
%{prefix}/share/oaf/Nautilus_View_change-password.oaf
%{prefix}/share/oaf/Nautilus_View_services-summary.oaf
%{prefix}/share/oaf/Nautilus_View_rpm.oaf

%files extras

%defattr(0444, bin, bin)
%{prefix}/share/nautilus/nautilus-extras.placeholder

%files suggested

%defattr(0444, bin, bin)
%{prefix}/share/nautilus/nautilus-suggested.placeholder
