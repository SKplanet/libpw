%global _enable_debug_package 1
%global with_jsoncpp 0@HAVE_JSONCPP@
%global with_uriparser 0@HAVE_URIPARSER@

Name: @pw_NAME@
Version: @pw_VERSION@
Release: @pw_VERSION_RELEASE@%{?dist}
Summary: @pw_DESCRIPTION_SUMMARY@
Group: @pw_GROUP@
License: @pw_LICENSE@
URL: @pw_URL@
Requires: openssl, zlib
%if 0%{with_jsoncpp}
Requires: jsoncpp
%endif
%if 0%{with_uriparser}
Requires: uriparser
%endif
BuildRequires: cmake >= 3.0
Source: %{name}-%{version}.tgz

%package devel
Summary: PW library headers
Requires: @pw_NAME@ = @pw_VERSION@
Requires: openssl-devel, zlib-devel
%if 0%{with_jsoncpp}
Requires: jsoncpp-devel
%endif
%if 0%{with_uriparser}
Requires: uriparser-devel
%endif


%package static
Summary: PW static library
Requires: @pw_NAME@ = @pw_VERSION@, @pw_NAME@-devel = @pw_VERSION@

%description
@pw_DESCRIPTION_DATA@

%description devel
PW library headers

%description static
PW static library

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%build

%install
rm -rf $RPM_BUILD_ROOT
cd @pw_BINARY_DIR@
make %{?_smp_mflags} install DESTDIR=$RPM_BUILD_ROOT

%files
%{_libdir}/@pw_NAME@.so
%{_libdir}/@pw_NAME@.so.*
%{_defaultdocdir}/libpw/*.md
%{_defaultdocdir}/libpw/LICENSE
%{_defaultdocdir}/libpw/ChangeLog
%{_defaultdocdir}/libpw/DESCRIPTION

%files devel
%{_includedir}/pw/*.h
%{_libdir}/pkgconfig/pw.pc

%files static
%{_libdir}/@pw_NAME@.a

#%changelog
