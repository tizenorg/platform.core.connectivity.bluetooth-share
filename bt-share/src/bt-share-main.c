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

#include "bluetooth-share-api.h"

#define BLUETOOTH_SHARE_BUS		"org.projectx.bluetooth.share"

static gboolean terminated;

GMainLoop *main_loop = NULL;
struct bt_appdata *app_state = NULL;

static void __bt_release_service(struct bt_appdata *ad)
{
	_bt_delete_all_notification();

	bluetooth_opc_deinit();
	bluetooth_obex_server_deinit();
	_bt_unregister_notification_cb(ad);

	if (vconf_set_bool(BT_VCONF_OPP_SERVER_INIT, FALSE) < 0)
		ERR("Fail to set the vconf");
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

static void __bt_update_notification_status_values()
{
	struct bt_appdata *ad = app_state;
	GSList *tr_data_list = NULL;
	GSList *list_iter = NULL;
	bt_tr_data_t *info = NULL;;
	char str[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	notification_h noti = NULL;
	sqlite3 *db = NULL;

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
				ERR("Invalid status\n");
			}

			list_iter = g_slist_next(list_iter);
		}

		if ((ad->send_data.tr_success + ad->send_data.tr_fail) != 0) {
			snprintf(str, sizeof(str), BT_TR_STATUS,
			ad->send_data.tr_success, ad->send_data.tr_fail);

			noti = _bt_create_notification(BT_NOTI_T,
				SEND_NOTI_PRIV_ID);
			_bt_set_notification_app_launch(noti,
				CREATE_TR_LIST,
				NOTI_TR_TYPE_OUT, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE | QP_NO_TICKER);
			_bt_insert_notification(noti,
				BT_STR_SEND_NOTI, str,
				BT_ICON_QP_SEND);
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

		if ((ad->recv_data.tr_success + ad->recv_data.tr_fail) != 0) {

			snprintf(str, sizeof(str), BT_TR_STATUS,
				ad->recv_data.tr_success, ad->recv_data.tr_fail);
			DBG("str = [%s] \n", str);

			_bt_delete_notification(BT_NOTI_T,
				RECV_NOTI_PRIV_ID);

			noti  = _bt_create_notification(BT_NOTI_T,
				RECV_NOTI_PRIV_ID);
			_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
				NOTI_TR_TYPE_IN, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE | QP_NO_TICKER);
			_bt_insert_notification(noti,
				BT_STR_RECEIVED_NOTI, str,
				BT_ICON_QP_RECEIVE);
		}

		bt_share_release_tr_data_list(tr_data_list);
		tr_data_list = NULL;
		list_iter = NULL;
	}

	bt_share_close_db(db);

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
		ERR("D-Bus Error: %s\n", err.message);
		dbus_error_free(&err);
	}

	if (!conn)
		dbus_connection_unref(conn);


	return FALSE;
}

void _bt_terminate_app(void)
{
	if (main_loop) {
		g_main_loop_quit(main_loop);
	}
}

int main(int argc, char *argv[])
{
	DBG("+");
	int ret;
	struct bt_appdata ad;
	memset(&ad, 0, sizeof(struct bt_appdata));
	app_state = &ad;

	signal(SIGTERM, __bt_sigterm_handler);

	g_type_init();

	if (__bt_dbus_request_name() == FALSE) {
		DBG("Aleady dbus instance existed\n");
		exit(0);
	}

	/* init internationalization */
	if (appcore_set_i18n(BT_COMMON_PKG, BT_COMMON_RES) < 0)
		return -1;

	/* Set the uid / gid to 5000 */
	control_privilege();

	bluetooth_register_callback(_bt_share_event_handler, NULL);
	ret = bluetooth_opc_init();
	if (ret != BLUETOOTH_ERROR_NONE) {
		ERR("bluetooth_opc_init failed!!\n");
		return -1;
	}

	char storage[STORAGE_PATH_LEN_MAX];
	if (TRUE == _bt_get_default_storage(storage)) {
		if (bluetooth_obex_server_init(storage) ==
						BLUETOOTH_ERROR_NONE) {
			if (vconf_set_bool(BT_VCONF_OPP_SERVER_INIT, TRUE) < 0)
				ERR("Fail to set the vconf");

			bluetooth_obex_server_set_root(BT_FTP_FOLDER);
		}
	}

	_bt_init_dbus_signal();
	_bt_init_vconf_notification();
	__bt_update_notification_status_values();
	_bt_register_notification_cb(&ad);

	if (terminated == TRUE) {
		__bt_release_service(&ad);
		return -1;
	}

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	__bt_release_service(&ad);

	DBG("-");
	return 0;
}
