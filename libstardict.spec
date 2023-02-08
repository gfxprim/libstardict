#
# LIBSTARDICT specfile
#
# (C) Cyril Hrubis metan{at}ucw.cz 2013-2023
#

Summary: A stardict dictionary parser library
Name: libstardict
Version: git
Release: 1
License: LGPL-2.1-or-later
Group: System/Libraries
Url: https://github.com/gfxprim/libstardict
Source: libstardict-%{version}.tar.bz2
BuildRequires: zlib-devel

BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%package -n libstardict1
Group: System/Libraries
Summary: A stardict dictionary parser library

%package devel
Group: Development/Libraries/C and C++
Summary: Development files for libstardict library
Requires: libstardict1

%package -n sd-cmd
Group: Applications/Text
Summary: Stardict dictionary lookup command

%description
A stardict dictionary parser library

%description -n libstardict1
A stardict dictionary parser library

%description devel
Devel package for libstardict library

%description -n sd-cmd
Stardict dictionary lookup command

%prep
%setup -n libstardict-%{version}

%build
./configure --prefix='/usr' --bindir=%{_bindir} --libdir=%{_libdir} --includedir=%{_includedir} --mandir=%{_mandir}
make %{?jobs:-j%jobs}

%install
DESTDIR="$RPM_BUILD_ROOT" make install

%files -n libstardict1
%defattr(-,root,root)
%{_libdir}/libstardict.so.1
%{_libdir}/libstardict.so.1.0.0

%files devel
%defattr(-,root,root)
%{_libdir}/libstardict.so
%{_libdir}/libstardict.a
%{_includedir}/*.h

%files -n sd-cmd
%defattr(-,root,root)
%{_bindir}/sd-cmd

%changelog
* Sun Jan 29 2023 Cyril Hrubis <metan@ucw.cz>

  Initial version.
