#!/bin/sh

eval $(tzplatform-get TZ_SYS_DB)

if [ ! -f $TZ_SYS_DB/.bluetooth_transfer.db ]
then
	sqlite3 $TZ_SYS_DB/.bluetooth_transfer.db 'PRAGMA journal_mode = PERSIST;
        create table if not exists inbound (
		id INTEGER PRIMARY KEY autoincrement,
		sid INTEGER,
		tr_status INTEGER,
		file_path TEXT,
		dev_name TEXT,
		timestamp INTEGER default 0,
		addr TEXT,
		type TEXT,
		content TEXT
	);
	create table if not exists outbound (
		id INTEGER PRIMARY KEY autoincrement,
		sid INTEGER,
		tr_status INTEGER,
		file_path TEXT,
		dev_name TEXT,
		timestamp INTEGER default 0,
		addr TEXT,
		type TEXT,
		content TEXT
	);
	'
fi

chown network_fw:network_fw $TZ_SYS_DB/.bluetooth_transfer.db
chown network_fw:network_fw $TZ_SYS_DB/.bluetooth_transfer.db-journal
chmod 664 $TZ_SYS_DB/.bluetooth_transfer.db
chmod 664 $TZ_SYS_DB/.bluetooth_transfer.db-journal

if [ -f /usr/lib/rpm-plugins/msm.so ]
then
	chsmack -a '*' $TZ_SYS_DB/.bluetooth_transfer.db
	chsmack -a '*' $TZ_SYS_DB/.bluetooth_transfer.db-journal
fi
