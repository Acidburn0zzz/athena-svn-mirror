Summary: The base GNOME icon theme
Name: gnome-icon-theme
Version: 2.8.0
Release: 1
License: GPL
Group: User Interface/Desktop
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildArch: noarch

%description
The base GNOME icon theme

%prep
%setup

%build
%configure
make

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-, root, root)
%{_datadir}/icons/*
%doc ChangeLog COPYING NEWS

%changelog
* Sun Jan 26 2003 Yanko Kaneti <yaneti@declera.com>
- First spec.in

