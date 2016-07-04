#!/bin/sh
PATH=/bin:/usr/bin:/sbin:/usr/sbin

eval $(tzplatform-get TZ_SYS_DB)

if [ ! -f $TZ_SYS_DB/.bluetooth_transfer.db ]
then
	/usr/bin/sqlite3 $TZ_SYS_DB/.bluetooth_transfer.db 'PRAGMA journal_mode = PERSIST;
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

/usr/bin/chown 5001:100 $TZ_SYS_DB/.bluetooth_transfer.db
/usr/bin/chown 5001:100 $TZ_SYS_DB/.bluetooth_transfer.db-journal
/usr/bin/chmod 664 $TZ_SYS_DB/.bluetooth_transfer.db
/usr/bin/chmod 664 $TZ_SYS_DB/.bluetooth_transfer.db-journal

/usr/bin/chsmack -a 'User::Home' $TZ_SYS_DB/.bluetooth_transfer.db
/usr/bin/chsmack -a 'User::Home' $TZ_SYS_DB/.bluetooth_transfer.db-journal
