Name:       bluetooth-share
Summary:    Bluetooth file share Agent
Version:    0.0.47
Release:    0
Group:      Network & Connectivity/Bluetooth
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: %{name}.manifest
Source1002: libbluetooth-share.manifest
Source1003: libbluetooth-share-devel.manifest
Source1004: init_db.sh
Requires(post): coreutils
Requires(post): smack
BuildRequires:  cmake
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(bluetooth-api)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(ecore)
# Same check as in tizen-extensions-crosswalk. A per-package or global
# bcond would be better, but for now let's follow Crosswalk's approach.
#%if "%{profile}" == "mobile"
#BuildRequires:  pkgconfig(calendar-service2)
#BuildRequires:  pkgconfig(contacts-service2)
#%endif
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(pmapi)
BuildRequires:  pkgconfig(sysman)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(capi-content-media-content)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(cynara-client)
BuildRequires:  pkgconfig(cynara-session)
BuildRequires:  pkgconfig(cynara-creds-dbus)

%description
Bluetooth File Share Agent

%package -n libbluetooth-share
Summary:    Bluetooth share library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libbluetooth-share
Bluetooth share library

%package -n libbluetooth-share-devel
Summary:    Development package for libbluetooth-share
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libbluetooth-share-devel
Development package for libbluetooth-share

%prep
%setup -q
cp %{SOURCE1001} %{SOURCE1002} %{SOURCE1003} .

%build
export CFLAGS+=" -fpie -fvisibility=hidden"
export LDFLAGS+=" -Wl,--rpath=/usr/%{_libdir} -Wl,--as-needed -Wl,--unresolved-symbols=ignore-in-shared-libs -pie"

%cmake . \
	-DTZ_SYS_ETC=%{TZ_SYS_ETC}
make

%install
%make_install
mkdir -p  %{buildroot}%{TZ_SYS_SHARE}/bt-ftp
install -D -m 0755 %{SOURCE1004} %{buildroot}%{TZ_SYS_SHARE}/%{name}/ressources/init_db.sh

%post -n libbluetooth-share-devel -p /sbin/ldconfig

%post -n libbluetooth-share -p /sbin/ldconfig

%postun -n libbluetooth-share-devel -p /sbin/ldconfig

%postun -n libbluetooth-share -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/bluetooth-share
%{_datadir}/dbus-1/system-services/org.bluetooth.share.service
%{TZ_SYS_SHARE}/bt-ftp
%{TZ_SYS_SHARE}/%{name}

%files -n libbluetooth-share
%manifest libbluetooth-share.manifest
%defattr(-, root, root)
%{_libdir}/libbluetooth-share-api.so.0.*

%files -n libbluetooth-share-devel
%manifest libbluetooth-share-devel.manifest
%defattr(-, root, root)
%{_includedir}/bluetooth-share-api/bluetooth-share-api.h
%{_libdir}/libbluetooth-share-api.so
%{_libdir}/pkgconfig/bluetooth-share-api.pc
