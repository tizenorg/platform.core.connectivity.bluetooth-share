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

#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <appcore-efl.h>
#include <vconf.h>
#include <errno.h>
#include <sys/stat.h>

/* For multi-user support */
#include <tzplatform_config.h>

#include "applog.h"
#include "bt-share-main.h"
#include "bluetooth-api.h"
#include "obex-event-handler.h"
#include "bt-share-ipc.h"
#include "bt-share-noti-handler.h"
#include "bt-share-resource.h"
#include "bt-share-notification.h"
#include "bt-share-common.h"
#include "bt-share-cynara.h"
#include "bt-share-db.h"


#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

#include "bluetooth-share-api.h"
#include "notification_internal.h"

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
	_bt_delete_notification(ad->send_noti);
	_bt_delete_notification(ad->receive_noti);
	_bt_delete_notification(ad->opc_noti);
	_bt_clear_receive_noti_list();
	ad->send_noti = NULL;
	ad->receive_noti = NULL;
	ad->opc_noti = NULL;

	bluetooth_opc_deinit();
	bluetooth_obex_server_deinit();
	_bt_unregister_notification_cb(ad);

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

static void __bt_update_notification_status_values()
{
	struct bt_appdata *ad = app_state;
	GSList *tr_data_list = NULL;
	GSList *list_iter = NULL;
	bt_tr_data_t *info = NULL;;
	char str[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	notification_h noti = NULL;
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
				ERR("Invalid status\n");
			}

			list_iter = g_slist_next(list_iter);
		}

		if ((ad->send_data.tr_success + ad->send_data.tr_fail) != 0) {
			snprintf(str, sizeof(str), "%s %d %d", BT_TR_STATUS,
			ad->send_data.tr_success, ad->send_data.tr_fail);

			noti = _bt_create_notification(BT_NOTI_T);
			//_bt_set_notification_app_launch(noti,
			//	CREATE_TR_LIST,
			//	NOTI_TR_TYPE_OUT, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE | QP_NO_TICKER);
//			_bt_insert_notification(noti,
//				BT_STR_SENT, str,
//				BT_ICON_QP_SEND);
			ad->send_noti = noti;
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

			snprintf(str, sizeof(str), "%s %d %d", BT_TR_STATUS,
				ad->recv_data.tr_success, ad->recv_data.tr_fail);
			DBG("str = [%s] \n", str);

			noti  = _bt_create_notification(BT_NOTI_T);
			//_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
			//	NOTI_TR_TYPE_IN, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE | QP_NO_TICKER);
//			_bt_insert_notification(noti,
//				BT_STR_RECEIVED, str,
//				BT_ICON_QP_RECEIVE);
			ad->receive_noti = noti;
		}

		bt_share_release_tr_data_list(tr_data_list);
		tr_data_list = NULL;
		list_iter = NULL;
	}

	bt_share_close_db(db);

	return;
}

static notification_h __bt_update_notification_adapter_status(void)
{
	notification_h noti;
	notification_error_e ret;

	noti  = _bt_create_notification(BT_NOTI_T);
	if (!noti)
		return NULL;

	ret = notification_set_property(noti, QP_NO_DELETE | QP_NO_TICKER);
	if (ret != NOTIFICATION_ERROR_NONE) {
		goto failed;
	}

	ret = notification_set_application(noti, "ug-bluetooth-efl");
	if (ret != NOTIFICATION_ERROR_NONE) {
		goto failed;
	}

	ret = notification_set_display_applist(noti,
			 NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY);
	if (ret != NOTIFICATION_ERROR_NONE) {
		goto failed;
	}

//	_bt_insert_notification(noti,
//			BT_STR_BLUETOOTH_ON, BT_STR_BLUETOOTH_AVAILABLE,
//			BT_ICON_QP_BT_ON);
	return noti;

failed:
	ERR("Fail to register notification");
	notification_free(noti);
	return NULL;

}

static int __bt_lang_changed_cb(void *data, void *user_data)
{
	if (appcore_set_i18n(BT_COMMON_PKG, BT_COMMON_RES) < 0)
		return -1;

	return 0;
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

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == ret_code) {
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

int _bt_init_obex_server(void)
{
	char storage[STORAGE_PATH_LEN_MAX];

	_bt_get_default_storage(storage);
	if (bluetooth_obex_server_init(storage) !=
					BLUETOOTH_ERROR_NONE) {
		DBG("Fail to init obex server");
		return BT_SHARE_FAIL;
	}

	bluetooth_obex_server_set_root(storage);

	return BT_SHARE_ERROR_NONE;
}

void _bt_terminate_app(void)
{
	if (main_loop) {
		g_main_loop_quit(main_loop);
	}
}

void __bt_create_transfer_db(void)
{
	struct stat sts;
	int ret;

	/* Check if the DB exists; if not, create it and initialize it */
	ret = stat(BT_TRANSFER_DB, &sts);
	if (ret == -1 && errno == ENOENT) {
		DBG("DB %s doesn't exist, it needs to be created and initialized", BT_TRANSFER_DB);
		DBG("script path: %s", SCRIPT_INIT_DB);
		ret = system(SCRIPT_INIT_DB);
		if (ret != EXIT_SUCCESS)
			ERR("Exit code of epp not clean: %i", ret);
	}
}

int main(void)
{
	int ret;
	struct bt_appdata ad;
	DBG("Starting bluetooth-share daemon");
	memset(&ad, 0, sizeof(struct bt_appdata));
	app_state = &ad;

	signal(SIGTERM, __bt_sigterm_handler);

	__bt_create_transfer_db();

	if (__bt_dbus_request_name() == FALSE) {
		DBG("Aleady dbus instance existed\n");
		exit(0);
	}

	ret = appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE, __bt_lang_changed_cb, NULL);
	if (ret < 0)
		DBG("Failed to excute the change of language");

	/* init internationalization */
	if (appcore_set_i18n(BT_COMMON_PKG, BT_COMMON_RES) < 0)
		return -1;

	uid_t network_user = 5001; /* uid of owner */
	gid_t network_group = 100; /* gid of users */

#if 0
	uid_t network_user = 551; /* uid of network_fw */
	gid_t network_group = 551; /* gid of network_fw */
	initgroups("network_fw", network_group);
#endif
	initgroups("users", network_group);
	ret = setgid(network_group);
	DBG("setgid return : %d", ret);
	ret = setuid(network_user);
	DBG("setuid return : %d", ret);

	if (_bt_share_cynara_init()) {
		ERR("Failed to initialize Cynara.\n");
		return -1;
	}

	bluetooth_register_callback(_bt_share_event_handler, NULL);
	ret = bluetooth_opc_init();
	if (ret != BLUETOOTH_ERROR_NONE) {
		ERR("bluetooth_opc_init failed!!\n");
		return -1;
	}

	_bt_init_dbus_signal();
	_bt_init_vconf_notification(&ad);
	__bt_update_notification_status_values();
	_bt_register_notification_cb(&ad);

	if (_bt_init_obex_server() == BT_SHARE_ERROR_NONE)
		ad.obex_server_init = TRUE;

	if (terminated == TRUE) {
		__bt_release_service(&ad);
		bluetooth_unregister_callback();
		return -1;
	}

	notification_h noti;
	noti = __bt_update_notification_adapter_status();

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	_bt_delete_notification(noti);
	__bt_release_service(&ad);
	bluetooth_unregister_callback();
	_bt_share_cynara_finish();

	return 0;
}
