#!/bin/sh

eval $(tzplatform-get TZ_USER_DB)

if [ ! -f $TZ_USER_DB/.bluetooth_transfer.db ]
then
	sqlite3 $TZ_USER_DB/.bluetooth_transfer.db 'PRAGMA journal_mode = PERSIST;
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

chown :$UID $TZ_USER_DB/.bluetooth_transfer.db
chown :$UID $TZ_USER_DB/.bluetooth_transfer.db-journal
chmod 660 $TZ_USER_DB/.bluetooth_transfer.db
chmod 660 $TZ_USER_DB/.bluetooth_transfer.db-journal

if [ -f /usr/lib/rpm-plugins/msm.so ]
then
	chsmack -a 'bt_share::db' $TZ_USER_DB/.bluetooth_transfer.db
	chsmack -a 'bt_share::db' $TZ_USER_DB/.bluetooth_transfer.db-journal
fi
