%define _optdir /opt

Name:       bluetooth-share
Summary:    Bluetooth file share Agent
Version:    0.0.26
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(bluetooth-api)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(calendar)
BuildRequires:  pkgconfig(contacts-service)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(pmapi)
BuildRequires:  pkgconfig(sysman)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(libprivilege-control)

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

%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
make

%install
rm -rf %{buildroot}
%make_install
mkdir -p  %{buildroot}%{_optdir}/share/bt-ftp

%post
# For the FTP server folder
if  [ ! -e /opt/share/bt-ftp/Media ]
then
	ln -s /opt/media /opt/share/bt-ftp/Media
fi

if  [ ! -e /opt/share/bt-ftp/SD_External ]
then
	ln -s /opt/storage/sdcard /opt/share/bt-ftp/SD_External
fi

vconftool set -t bool memory/private/bluetooth-share/quickpanel_clear_btn_status FALSE -g 6520 -i
vconftool set -t bool memory/private/bluetooth-share/opp_server_init FALSE -g 6520 -i

if [ ! -f /opt/dbspace/.bluetooth_trasnfer.db ]
then
	sqlite3 /opt/dbspace/.bluetooth_trasnfer.db 'PRAGMA journal_mode = PERSIST;
        create table if not exists inbound (
		id INTEGER PRIMARY KEY autoincrement,
		tr_status INTEGER,
		file_path TEXT,
		dev_name TEXT,
		timestamp INTEGER default 0,
		addr TEXT
	);
	create table if not exists outbound (
		id INTEGER PRIMARY KEY autoincrement,
		tr_status INTEGER,
		file_path TEXT,
		dev_name TEXT,
		timestamp INTEGER default 0,
		addr TEXT
	);
	'
fi

chown :5000 /opt/dbspace/.bluetooth_trasnfer.db
chown :5000 /opt/dbspace/.bluetooth_trasnfer.db-journal
chmod 660 /opt/dbspace/.bluetooth_trasnfer.db
chmod 660 /opt/dbspace/.bluetooth_trasnfer.db-journal


%files
%defattr(-,root,root,-)
%{_bindir}/bluetooth-share
%{_datadir}/dbus-1/services/org.bluetooth.share.service

%files -n libbluetooth-share
%defattr(-, root, root)
%{_libdir}/libbluetooth-share-api.so.0.*

%files -n libbluetooth-share-devel
%defattr(-, root, root)
/usr/include/bluetooth-share-api/bluetooth-share-api.h
%{_libdir}/libbluetooth-share-api.so
%{_libdir}/pkgconfig/bluetooth-share-api.pc
