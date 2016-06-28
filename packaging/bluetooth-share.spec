%define TZ_SYS_DATA /opt/data/

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
BuildRequires:  pkgconfig(aul)
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
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(deviced)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(capi-content-media-content)
BuildRequires:  pkgconfig(storage)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(cynara-client)
BuildRequires:  pkgconfig(cynara-session)
BuildRequires:  pkgconfig(cynara-creds-dbus)
BuildRequires:  pkgconfig(eventsystem)
BuildRequires:  pkgconfig(storage)

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
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

export CFLAGS+=" -fpie -fvisibility=hidden"
export LDFLAGS+=" -Wl,--rpath=/usr/lib -Wl,--as-needed -Wl,--unresolved-symbols=ignore-in-shared-libs -pie"

%cmake . \
	-DCMAKE_INSTALL_PREFIX=%{_prefix}
make

%install
%make_install

install -D -m 0755 %{SOURCE1004} %{buildroot}%{TZ_SYS_DATA}/%{name}/init_db.sh
install -D -m 0644 packaging/bluetooth-share.service %{buildroot}%{_libdir}/systemd/system/bluetooth-share.service

%post -n libbluetooth-share-devel -p /sbin/ldconfig

%post -n libbluetooth-share -p /sbin/ldconfig
ln -sf %{_libdir}/systemd/system/bluetooth-share.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

%postun -n libbluetooth-share-devel -p /sbin/ldconfig

%postun -n libbluetooth-share -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/bluetooth-share
#%{_datadir}/dbus-1/system-services/org.bluetooth.share.service
%{TZ_SYS_DATA}/%{name}/init_db.sh
%{_libdir}/systemd/system/bluetooth-share.service

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
