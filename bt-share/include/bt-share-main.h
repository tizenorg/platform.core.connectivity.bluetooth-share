/*
 *  bluetooth-share
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved
 *
 * Contact:  Hocheol Seo <hocheol.seo@samsung.com>
 *           GirishAshok Joshi <girish.joshi@samsung.com>
 *           DoHyun Pyun <dh79.pyun@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __DEF_BLUETOOTH_SHARE_H_
#define __DEF_BLUETOOTH_SHARE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <dbus/dbus-glib.h>

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "bluetooth-share"
#endif

#define UI_PACKAGE "org.tizen.bluetooth-share-ui"
#define UI_PKG_PATH "/opt/apps/org.tizen.bluetooth-share-ui/bin/bluetooth-share-ui"

#define BT_VCONF_OPP_SERVER_INIT "memory/private/bluetooth-share/opp_server_init"

typedef void (*bt_app_cb) (void *, void *, void *);

typedef struct {
	bt_app_cb popup_cb;
	void *popup_cb_data;
	gboolean syspopup_request;
} bt_app_popups_t;

typedef struct {
	int tr_success;
	int tr_fail;
} bt_transfer_status_info_t;

struct bt_appdata {
	bt_app_popups_t popups;
	bt_transfer_status_info_t send_data;
	bt_transfer_status_info_t recv_data;
	GSList *tr_send_list;
	GSList *tr_next_data;
	int current_tr_uid;
	DBusGConnection *conn;
	GObject *object;
};


void _bt_terminate_app(void);

#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_H_ */
