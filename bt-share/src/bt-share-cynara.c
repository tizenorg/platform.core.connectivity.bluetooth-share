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

#include "bt-share-cynara.h"

#include "applog.h"
#include "bt-share-common.h"

#include <cynara-client.h>
#include <cynara-session.h>
#include <cynara-creds-dbus.h>
#include <cynara-error.h>
#include <malloc.h>

static cynara *_cynara = NULL;
const unsigned int error_msg_size = 256;

// initialize cynara
int _bt_share_cynara_init(void)
{
	char error_msg[error_msg_size];
	int ret;

	ret = cynara_initialize(&_cynara, NULL);
	if (ret != CYNARA_API_SUCCESS) {
		cynara_strerror(ret, error_msg, error_msg_size);
		ERR("cynara_initialize failed: %s\n", error_msg);
		return BT_SHARE_FAIL;
	}

	return BT_SHARE_ERROR_NONE;
}

// fill creds structure with data needed to perform checks using cynara-creds lib
int _bt_share_cynara_get_creds(DBusConnection *conn, const char *sender,
				   bt_share_cynara_creds *creds)
{
	char error_msg[error_msg_size];
	int ret;

	ret = cynara_creds_dbus_get_pid(conn, sender, &(creds->pid));
	if (ret < 0) {
		cynara_strerror(ret, error_msg, error_msg_size);
		ERR("cynara_creds_dbus_get_pid failed: %s\n");
		return BT_SHARE_FAIL;
	}

	ret = cynara_creds_dbus_get_user(conn, sender, USER_METHOD_UID, &(creds->uid));
	if (ret < 0) {
		cynara_strerror(ret, error_msg, error_msg_size);
		ERR("cynara_creds_dbus_get_user failed\n");
		return BT_SHARE_FAIL;
	}

	ret = cynara_creds_dbus_get_client(conn, sender, CLIENT_METHOD_SMACK, &(creds->smack));
	if (ret < 0) {
		cynara_strerror(ret, error_msg, error_msg_size);
		ERR("cynara_creds_dbus_get_client failed\n");
		return BT_SHARE_FAIL;
	}

	return BT_SHARE_ERROR_NONE;
}

// check if client has required privilege
int _bt_share_cynara_check(const bt_share_cynara_creds *creds, const char *privilege)
{
	int ret;
	char *client_session;
	char error_msg[error_msg_size];

	if (!creds || !privilege)
		return BT_SHARE_FAIL;

	client_session = cynara_session_from_pid(creds->pid);
	if (!client_session) {
		ERR("cynara_session_from_pid failed\n");
		return BT_SHARE_FAIL;
	}

	ret = cynara_check(_cynara, creds->smack, client_session, creds->uid, privilege);
	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		cynara_strerror(ret, error_msg, error_msg_size);
		ERR("cynara_check error: %s\n", error_msg);
	}

	free(client_session);
	return ret == CYNARA_API_ACCESS_ALLOWED ? BT_SHARE_ERROR_NONE : BT_SHARE_FAIL;
}

// finish working with cynara
void _bt_share_cynara_finish(void)
{
	cynara_finish(_cynara);
	_cynara = NULL;
}
