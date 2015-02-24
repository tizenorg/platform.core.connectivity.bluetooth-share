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

#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <appcore-efl.h>
#include <privilege-control.h>
#include <vconf.h>
#include "applog.h"
#include "bt-share-main.h"
#include "bluetooth-api.h"
#include "obex-event-handler.h"
#include "bt-share-ipc.h"
#include "bt-share-noti-handler.h"
#include "bt-share-resource.h"
#include "bt-share-notification.h"
#include "bt-share-common.h"

#include "bluetooth-share-api.h"

#define BLUETOOTH_SHARE_BUS		"org.projectx.bluetooth.share"

static gboolean terminated;

GMainLoop *main_loop = NULL;
struct bt_appdata *app_state = NULL;

void _bt_terminate_bluetooth_share(void)
{
	DBG("+");

	if (main_loop) {
		g_main_loop_quit(main_loop);
	} else {
		terminated = TRUE;
	}
	DBG("-");
}

static void __bt_release_service(struct bt_appdata *ad)
{
	if (ad == NULL)
		return;

	_bt_deinit_vconf_notification();
	_bt_delete_notification(ad->opc_noti);
	_bt_clear_receive_noti_list();
	ad->send_noti = NULL;
	ad->receive_noti = NULL;
	ad->opc_noti = NULL;

	bluetooth_opc_deinit();
	bluetooth_obex_server_deinit();
	_bt_unregister_notification_cb(ad);

	g_free(server_auth_info.filename);
	server_auth_info.filename = NULL;

	DBG("Terminating bluetooth-share daemon");
}

static void __bt_sigterm_handler(int signo)
{
	DBG("+");

	if (main_loop) {
		g_main_loop_quit(main_loop);
	} else {
		terminated = TRUE;
	}

	DBG("-");
}

static void __bt_update_transfer_status_values(void)
{
	struct bt_appdata *ad = app_state;
	GSList *tr_data_list = NULL;
	GSList *list_iter = NULL;
	bt_tr_data_t *info = NULL;;
	sqlite3 *db = NULL;

	/* Update notification status durning BT off */
	if (_bt_update_notification_status(ad) == FALSE) {
		DBG("Notification item is not existed.");
		return;
	}

	DBG("Initialize transfer information");

	db = bt_share_open_db();
	if (!db)
		return;

	tr_data_list = bt_share_get_all_tr_data_list(db, BT_DB_OUTBOUND);
	if (NULL != tr_data_list) {
		list_iter = tr_data_list;

		while (NULL != list_iter) {
			info = list_iter->data;
			if (NULL == info)
				break;

			if (info->tr_status == BT_TR_SUCCESS) {
				ad->send_data.tr_success++;
			} else if (info->tr_status == BT_TR_FAIL) {
				ad->send_data.tr_fail++;
			} else if (info->tr_status == BT_TR_ONGOING) {
			/* In case of ongoing file transfer if bluetooth is switched off
			we need to update the status to fail for these transaction */
				ad->send_data.tr_fail++;
				info->tr_status = BT_TR_FAIL;
				bt_share_update_tr_data(db, BT_DB_OUTBOUND, info->id, info);
			} else {
				ERR("Invalid status");
			}

			list_iter = g_slist_next(list_iter);
		}

		bt_share_release_tr_data_list(tr_data_list);
		tr_data_list = NULL;
		list_iter = NULL;
	}

	tr_data_list = bt_share_get_all_tr_data_list(db, BT_DB_INBOUND);
	if (NULL != tr_data_list) {
		list_iter = tr_data_list;

		while (NULL != list_iter) {
			info = list_iter->data;
			if (NULL == info)
				break;

			if (info->tr_status == BT_TR_SUCCESS)
				ad->recv_data.tr_success++;
			else
				ad->recv_data.tr_fail++;

			list_iter = g_slist_next(list_iter);
		}
		bt_share_release_tr_data_list(tr_data_list);
		tr_data_list = NULL;
		list_iter = NULL;
	}

	bt_share_close_db(db);

	DBG("[Send] success %d, fail %d", ad->send_data.tr_success, ad->send_data.tr_fail);
	DBG("[Receive] success %d, fail %d", ad->recv_data.tr_success, ad->recv_data.tr_fail);

	return;
}

static gboolean __bt_dbus_request_name(void)
{
	int ret_code = 0;
	DBusConnection *conn;
	DBusError err;

	dbus_error_init(&err);

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err))
		goto failed;

	ret_code = dbus_bus_request_name(conn,
					BLUETOOTH_SHARE_BUS,
					DBUS_NAME_FLAG_DO_NOT_QUEUE,
					&err);
	if (dbus_error_is_set(&err))
		goto failed;

	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == ret_code) {
		dbus_connection_unref(conn);
		return TRUE;
	}

failed:
	if (dbus_error_is_set(&err)) {
		ERR("D-Bus Error: %s", err.message);
		dbus_error_free(&err);
	}

	if (!conn)
		dbus_connection_unref(conn);


	return FALSE;
}

int _bt_init_obex_server(void)
{
	char storage[STORAGE_PATH_LEN_MAX];
#ifdef _TEMP_
	_bt_get_default_storage(storage);
	if (bluetooth_obex_server_init(storage) !=
					BLUETOOTH_ERROR_NONE) {
		ERR("Fail to init obex server");
		return BT_SHARE_FAIL;
	}

	bluetooth_obex_server_set_root(BT_FTP_FOLDER);
#endif

	return BT_SHARE_ERROR_NONE;
}

int main(void)
{
	int ret;
	struct bt_appdata ad;
	INFO("Starting bluetooth-share daemon");
	memset(&ad, 0, sizeof(struct bt_appdata));
	app_state = &ad;

	signal(SIGTERM, __bt_sigterm_handler);

	g_type_init();

	if (__bt_dbus_request_name() == FALSE) {
		INFO("Aleady dbus instance existed");
		exit(0);
	}
#ifdef _TEMP_
	/* init internationalization */
	if (appcore_set_i18n(BT_COMMON_PKG, BT_COMMON_RES) < 0)
		return -1;

	/* Set the uid / gid to 5000 */
	if (perm_app_set_privilege("org.tizen.bluetooth-share", NULL, NULL) !=
							PC_OPERATION_SUCCESS)
		ERR("Failed to set app privilege");

	bluetooth_register_callback(_bt_share_event_handler, NULL);
#endif
	ret = bluetooth_opc_init();
	if (ret != BLUETOOTH_ERROR_NONE) {
		ERR("bluetooth_opc_init failed");
		return -1;
	}

	_bt_init_dbus_signal();
	_bt_init_vconf_notification(&ad);
	__bt_update_transfer_status_values();
	_bt_register_notification_cb(&ad);

	if (_bt_init_obex_server() == BT_SHARE_ERROR_NONE)
		ad.obex_server_init = TRUE;

	if (terminated == TRUE) {
		__bt_release_service(&ad);
		bluetooth_unregister_callback();
		return -1;
	}

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	__bt_release_service(&ad);
	bluetooth_unregister_callback();

	return 0;
}
