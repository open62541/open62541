Name:     open62541
Version:  0.2
Release:  1%{?dist}
Summary:  Open 62541
License:  Mozilla 2.0
URL:      http://open62541.org
Source0:  https://github.com/open62541/open62541/archive/v%{version}.tar.gz

BuildRequires: cmake, python2

%description
Open 62541

%package  devel
Summary:  Development files for %{name}
Requires: %{name} = %{version}-%{release}

%description devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -q

%build
%cmake .
make %{?_smp_mflags}

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_libdir}/libopen62541.so
%{_libdir}/libopen62541.so.*

%files devel
%{_includedir}/open62541.h

%changelog
* Thu Aug 31 2017 Jens Reimann <jreimann@redhat.com> - 0.2-1
- Initial version of the package
