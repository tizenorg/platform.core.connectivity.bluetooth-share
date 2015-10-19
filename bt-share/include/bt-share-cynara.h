/*
 * bluetooth-share
 *
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __DEF_BLUETOOTH_SHARE_CYNARA_H_
#define __DEF_BLUETOOTH_SHARE_CYNARA_H_

#include <unistd.h>
#include <dbus/dbus.h>

#define BT_SHARE_PRIVILEGE "http://tizen.org/privilege/bluetooth"

typedef struct
{
	pid_t pid;
	char *uid;
	char *smack;
} bt_share_cynara_creds;

int _bt_share_cynara_init(void);
int _bt_share_cynara_get_creds(DBusConnection *conn, const char *sender,
				   bt_share_cynara_creds *creds);
int _bt_share_cynara_check(const bt_share_cynara_creds *creds, const char *privilege);
void _bt_share_cynara_finish(void);

#endif				/* __DEF_BLUETOOTH_SHARE_CYNARA_H_ */
