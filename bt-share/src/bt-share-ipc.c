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

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <notification.h>
#include <sysman.h>
#include <aul.h>
#include <vconf.h>

#include "applog.h"
#include "bluetooth-api.h"
#include "bt-share-main.h"
#include "bt-share-ipc.h"
#include "bt-share-syspopup.h"
#include "bt-share-notification.h"
#include "bt-share-resource.h"
#include "obex-event-handler.h"
#include "bluetooth-share-api.h"
#include "bt-share-common.h"

GSList *bt_transfer_list = NULL;
DBusConnection *dbus_connection = NULL;

extern struct bt_appdata *app_state;

static void __bt_create_send_data(opc_transfer_info_t *node);
static void __bt_share_update_tr_info(int tr_uid, int tr_type);

static void __bt_tr_data_free(bt_tr_data_t *data)
{
	retm_if(data == NULL, "Invalid param");

	g_free(data->file_path);
	g_free(data->dev_name);
	g_free(data->addr);
	g_free(data->type);
	g_free(data->content);
	g_free(data);
}

static void __popup_res_cb(int res)
{
	DBG("+\n");
	struct bt_appdata *ad = app_state;
	if (ad->popups.syspopup_request == FALSE) {
			DBG("This event is not mine\n");
			return;
	}
	DBG(" res : %d\n", res);
	/* Inorder to support calling popup from callback, we have to make
	 * syspopup_request false here and also should not assign
	 * ad->popups.popup_cb = NULL */

	ad->popups.syspopup_request = FALSE;

	if (NULL != ad->popups.popup_cb) {
		if (res == 0)
			ad->popups.popup_cb(ad->popups.popup_cb_data,
				NULL,
				(void *)POPUP_RESPONSE_OK);
		else if (res == 1)
			ad->popups.popup_cb(ad->popups.popup_cb_data,
				NULL,
				(void *)POPUP_RESPONSE_CANCEL);
		else if (res == 2)
			ad->popups.popup_cb(ad->popups.popup_cb_data,
				NULL,
				(void *)POPUP_RESPONSE_TIMEOUT);
	}
	DBG("-\n");
}


static opc_transfer_info_t *__add_transfer_info(DBusMessage *msg)
{
	int reserved = 0;
	int cnt = 0;
	char *addr = NULL;
	int len;
	char *filepath = NULL;
	char *mode = NULL;
	char *name = NULL;
	char *type = NULL;
	opc_transfer_info_t *data;
	int i = 0;
	char *token = NULL;
	char *ptr = NULL;

	retv_if(msg == NULL, NULL);
	dbus_message_get_args(msg, NULL,
				DBUS_TYPE_INT32, &reserved,
				DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				&addr, &len,
				DBUS_TYPE_INT32, &cnt,
				DBUS_TYPE_STRING, &filepath,
				DBUS_TYPE_STRING, &mode,
				DBUS_TYPE_STRING, &name,
				DBUS_TYPE_STRING, &type,
				DBUS_TYPE_INVALID);

	retv_if(cnt <= 0, NULL);
	retv_if(addr == NULL, NULL);
	retv_if(filepath == NULL, NULL);
	retv_if(name == NULL, NULL);

	DBG("reserved ( %d )\n", reserved);
	DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", addr[0],
	     addr[1], addr[2], addr[3], addr[4], addr[5]);
	DBG(" cnt( %d )\n", cnt);
	DBG(" filepath( %s )\n", filepath);
	DBG(" mode ( %s )\n", mode);
	DBG(" name ( %s )\n", name);
	DBG(" type ( %s )\n", type);

	data = g_new0(opc_transfer_info_t, 1);

	data->content = g_new0(char *, cnt + 1);
	data->file_path = g_new0(char *, cnt + 1);
	data->file_cnt = cnt;
	memcpy(data->addr, addr, BLUETOOTH_ADDRESS_LENGTH);
	memcpy(data->name, name, BLUETOOTH_DEVICE_NAME_LENGTH_MAX);
	data->type = g_strdup(type);

	token = strtok_r(filepath, FILE_PATH_DELIM, &ptr);
	while ((token != NULL) && (i < cnt)) {
		if (g_strcmp0(type, "text") == 0) {
			data->file_path[i] = _bt_share_create_transfer_file(token);
		} else {
			data->file_path[i] = g_strdup(token);
		}

		DBG(" file path ( %s )\n", data->file_path[i]);

		data->content[i] = g_strdup(token);
		DBG("Content [%d] [%s]\n", i, data->content[i]);
		i++;

		token = strtok_r(NULL, FILE_PATH_DELIM, &ptr);
	}

	bt_transfer_list = g_slist_append(bt_transfer_list, data);

	return data;
}


void _free_transfer_info(opc_transfer_info_t *node)
{
	int i;

	DBG("+\n");
	ret_if(node == NULL);

	for (i = 0; i < node->file_cnt; i++) {
		_bt_remove_tmp_file(node->file_path[i]);
		g_free(node->file_path[i]);
		g_free(node->content[i]);
	}
	g_free(node->file_path);
	g_free(node->content);
	g_free(node->type);
	g_free(node);

	DBG("-\n");
}

void _remove_transfer_info(opc_transfer_info_t *node)
{
	DBG("+\n");
	ret_if(node == NULL);

	bt_transfer_list = g_slist_remove(bt_transfer_list, node);
	_free_transfer_info(node);

	DBG("-\n");
}

int _request_file_send(opc_transfer_info_t *node)
{
	struct bt_appdata *ad = app_state;
	int ret;
	DBG("+\n");
	retv_if(ad == NULL, BLUETOOTH_ERROR_INVALID_PARAM);
	retv_if(node == NULL, BLUETOOTH_ERROR_INVALID_PARAM);

	ret = bluetooth_opc_push_files((bluetooth_device_address_t *)node->addr,
						node->file_path);
	if (ret != BLUETOOTH_ERROR_NONE) {
		DBG("bluetooth_opc_push_files failed : %d", ret);
		return ret;
	}

	__bt_create_send_data(node);

	DBG("-\n");
	return BLUETOOTH_ERROR_NONE;
}

static DBusHandlerResult __event_filter(DBusConnection *sys_conn,
							DBusMessage *msg, void *data)
{
	int ret;
	char *member;
	const char *path = dbus_message_get_path(msg);

	if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (path == NULL || strcmp(path, "/") == 0)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	member = (char *)dbus_message_get_member(msg);
	DBG("member (%s)\n", member);

	if (dbus_message_is_signal(msg, BT_SYSPOPUP_INTERFACE, BT_SYSPOPUP_METHOD_RESPONSE)) {
		int res = 0;
		dbus_message_get_args(msg, NULL,
					DBUS_TYPE_INT32, &res,
					DBUS_TYPE_INVALID);
		__popup_res_cb(res);
	} else if (dbus_message_is_signal(msg, BT_UG_IPC_INTERFACE, BT_UG_IPC_METHOD_SEND)) {
		opc_transfer_info_t *node;
		node = __add_transfer_info(msg);
		if (node == NULL)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		ret = _request_file_send(node);
		if (ret == BLUETOOTH_ERROR_IN_PROGRESS) {
			DBG("Aleady OPC progressing. Once completed previous job, will be started\n");
		} else if ( ret != BLUETOOTH_ERROR_NONE) {
			_bt_create_warning_popup(BLUETOOTH_ERROR_INTERNAL);
			g_slist_free_full(bt_transfer_list,
						(GDestroyNotify)_free_transfer_info);
			bt_transfer_list = NULL;
		}
	} else if (dbus_message_is_signal(msg, BT_SHARE_UI_INTERFACE, BT_SHARE_UI_SIGNAL_OPPABORT)) {
			const char *transfer_type = NULL;
			int transfer_id = 0;
			int noti_id = 0;
			if (!dbus_message_get_args(msg, NULL,
					DBUS_TYPE_STRING, &transfer_type,
					DBUS_TYPE_INT32, &noti_id,
					DBUS_TYPE_INVALID)) {
				ERR("OPP abort handling failed");
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
			}
			DBG("transfer_type = %s\n", transfer_type);
			if (!g_strcmp0(transfer_type, NOTI_TR_TYPE_OUT)) {
				bluetooth_opc_cancel_push();
			} else {
				transfer_id = _bt_get_transfer_id_by_noti_id(noti_id);
				bluetooth_obex_server_cancel_transfer(transfer_id);
			}
	} else if (dbus_message_is_signal(msg, BT_SHARE_UI_INTERFACE,
				BT_SHARE_UI_SIGNAL_SEND_FILE)) {
		opc_transfer_info_t *node;
		node = __add_transfer_info(msg);
		if (node == NULL)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		ret = _request_file_send(node);
		if (ret == BLUETOOTH_ERROR_IN_PROGRESS) {
			DBG("Aleady OPC progressing. Once completed previous job, will be started\n");
		} else if ( ret != BLUETOOTH_ERROR_NONE) {
			_bt_create_warning_popup(BLUETOOTH_ERROR_INTERNAL);
			g_slist_free_full(bt_transfer_list,
							(GDestroyNotify)_free_transfer_info);
			bt_transfer_list = NULL;
		}
	} else if (dbus_message_is_signal(msg, BT_SHARE_UI_INTERFACE,
				BT_SHARE_UI_SIGNAL_INFO_UPDATE)) {
		int tr_uid = 0;
		int tr_type = 0;
		dbus_message_get_args(msg, NULL,
					DBUS_TYPE_INT32, &tr_uid,
					DBUS_TYPE_INT32, &tr_type,
					DBUS_TYPE_INVALID);

		DBG("tr_uid = %d, tr_type = %d \n", tr_uid, tr_type);
		__bt_share_update_tr_info(tr_uid, tr_type);
	} else if (dbus_message_is_signal(msg, BT_SHARE_FRWK_INTERFACE,
				BT_SHARE_FRWK_SIGNAL_DEINIT)) {
		/* Deinitialize the obex server */
		if (bluetooth_obex_server_deinit() == BLUETOOTH_ERROR_NONE) {
			if (vconf_set_bool(BT_VCONF_OPP_SERVER_INIT, FALSE)< 0)
				ERR("Fail to set the vconf");
		}
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

gboolean _bt_init_dbus_signal(void)
{
	DBG("+\n");
	DBusGConnection *conn;
	GError *err = NULL;
	DBusError dbus_error;

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &err);
	if (!conn) {
		DBG(" DBUS get failed\n");
		g_error_free(err);
		return FALSE;
	}
	dbus_connection = dbus_g_connection_get_connection(conn);

	/* Add the filter for network client functions */
	dbus_error_init(&dbus_error);
	dbus_connection_add_filter(dbus_connection, __event_filter, NULL, NULL);
	dbus_bus_add_match(dbus_connection,
			   "type=signal,interface=" BT_SYSPOPUP_INTERFACE
			   ",member=Response", &dbus_error);
	dbus_bus_add_match(dbus_connection,
			   "type=signal,interface=" BT_UG_IPC_INTERFACE
			   ",member=Send", &dbus_error);
	dbus_bus_add_match(dbus_connection,
			   "type=signal,interface=" BT_SHARE_UI_INTERFACE
			   ",member=opp_abort", &dbus_error);
	dbus_bus_add_match(dbus_connection,
			   "type=signal,interface=" BT_SHARE_UI_INTERFACE
			   ",member=send_file", &dbus_error);
	dbus_bus_add_match(dbus_connection,
			   "type=signal,interface=" BT_SHARE_UI_INTERFACE
			   ",member=info_update", &dbus_error);
	dbus_bus_add_match(dbus_connection,
			   "type=signal,interface=" BT_SHARE_FRWK_INTERFACE
			   ",member=deinit", &dbus_error);
	if (dbus_error_is_set(&dbus_error)) {
		ERR("Fail to add dbus filter signal\n");
		dbus_error_free(&dbus_error);
	}

	DBG("-\n");
	return TRUE;
}

void _bt_send_message_to_ui(int transfer_id, char *name, int percentage, gboolean completed, int error_type)
{
	DBusMessage *msg = NULL;
	DBG("+\n");
	ret_if(dbus_connection == NULL);
	ret_if(name == NULL);

	msg = dbus_message_new_signal(BT_SHARE_ENG_OBJECT,
				      BT_SHARE_ENG_INTERFACE,
				      BT_SHARE_ENG_SIGNAL_PROGRESS);
	if (!msg) {
		ERR("Unable to allocate memory\n");
		return;
	}
	if (!dbus_message_append_args(msg,
			DBUS_TYPE_INT32, &transfer_id,
			DBUS_TYPE_STRING, &name,
			DBUS_TYPE_INT32, &percentage,
			DBUS_TYPE_BOOLEAN, &completed,
			DBUS_TYPE_INT32, &error_type,
			DBUS_TYPE_INVALID)) {
		ERR("Event sending failed\n");
		dbus_message_unref(msg);
		return;
	}
	DBG("Dbus send message append request is done.\n");

	dbus_message_set_destination(msg, BT_SHARE_UI_INTERFACE);
	dbus_connection_send(dbus_connection, msg, NULL);
	dbus_message_unref(msg);
	DBG("-\n");
	return;
}


void _bt_create_warning_popup(int error_type)
{
	/* If bluetooth-share-ui process is existed, send dbus signal. */
	/* Otherwise create the process and terminate it after popup shown */
	if (sysman_get_pid(UI_PKG_PATH) == -1) {
		char str[BT_TEXT_LEN_MAX] = {0,};
		bundle *b;

		switch(error_type) {
		case BLUETOOTH_ERROR_SERVICE_NOT_FOUND:
		case BLUETOOTH_ERROR_NOT_CONNECTED:
		case BLUETOOTH_ERROR_ACCESS_DENIED:
		case BLUETOOTH_ERROR_OUT_OF_MEMORY:
		case BLUETOOTH_ERROR_INTERNAL:
			snprintf(str, BT_TEXT_LEN_MAX, "%s", BT_STR_UNABLE_TO_SEND);
			break;
		default:
			return;
		}

		b = bundle_create();
		ret_if(b == NULL);

		bundle_add(b, "launch-type", "warning_popup");
		bundle_add(b, "message", str);

		aul_launch_app(UI_PACKAGE, b);

		bundle_free(b);
	}else {
		DBusMessage *msg = NULL;
		ret_if(dbus_connection == NULL);

		msg = dbus_message_new_signal(BT_SHARE_ENG_OBJECT,
					      BT_SHARE_ENG_INTERFACE,
					      BT_SHARE_ENG_SIGNAL_ERROR);
		if (!msg) {
			ERR("Unable to allocate memory\n");
			return;
		}

		if (!dbus_message_append_args(msg,
				DBUS_TYPE_INT32, &error_type,
				DBUS_TYPE_INVALID)) {
			DBG("Event sending failed\n");
			dbus_message_unref(msg);
			return;
		}

		dbus_message_set_destination(msg, BT_SHARE_UI_INTERFACE);
		dbus_connection_send(dbus_connection, msg, NULL);
		dbus_message_unref(msg);
	}

	return;
}

void _bt_update_transfer_list_view(const char *table)
{
	DBusMessage *msg = NULL;
	ret_if(dbus_connection == NULL);

	msg = dbus_message_new_signal(BT_SHARE_ENG_OBJECT,
				      BT_SHARE_ENG_INTERFACE,
				      BT_SHARE_ENG_SIGNAL_UPDATE_VIEW);
	if (!msg) {
		ERR("Unable to allocate memory\n");
		return;
	}

	if (!dbus_message_append_args(msg,
			DBUS_TYPE_STRING, &table,
			DBUS_TYPE_INVALID)) {
		DBG("Event sending failed\n");
		dbus_message_unref(msg);
		return;
	}

	dbus_message_set_destination(msg, BT_SHARE_UI_INTERFACE);
	dbus_connection_send(dbus_connection, msg, NULL);
	dbus_message_unref(msg);
}

static time_t __bt_get_current_timedata(void)
{
	time_t t;
	time(&t);
	return ((int) t);
}

static char *__bt_conv_addr_type_to_addr_string(char *addr)
{
	char address[BT_ADDR_STR_LEN_MAX] = {0,};
	retv_if(addr == NULL, NULL);

	snprintf(address, BT_ADDR_STR_LEN_MAX,
		"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	return g_strdup(address);
}

static void __bt_create_send_data(opc_transfer_info_t *node)
{
	DBG("__bt_create_send_data  \n");
	ret_if(node == NULL);
	struct bt_appdata *ad = app_state;

	int count = 0;
	int session_id;

	sqlite3 *db = NULL;

	DBG("Name [%s]\n", node->name);

	db = bt_share_open_db();
	if (!db)
		return;

	session_id = bt_share_get_last_session_id(db, BT_DB_OUTBOUND);
	DBG("Last session id = %d\n", session_id);
	for (count = 0; count < node->file_cnt; count++) {
		bt_tr_data_t *tmp;
		tmp = g_malloc0(sizeof(bt_tr_data_t));

		tmp->tr_status = BT_TR_ONGOING;
		tmp->sid = session_id + 1;
		tmp->file_path = g_strdup(node->file_path[count]);
		tmp->content = g_strdup(node->content[count]);
		tmp->dev_name =  g_strdup(node->name);
		tmp->type =  g_strdup(node->type);
		tmp->timestamp = __bt_get_current_timedata();
		tmp->addr = __bt_conv_addr_type_to_addr_string(node->addr);
		bt_share_add_tr_data(db, BT_DB_OUTBOUND, tmp);
		__bt_tr_data_free(tmp);

	}

	if (ad->tr_send_list) {
		bt_share_release_tr_data_list(ad->tr_send_list);
		ad->tr_send_list = NULL;
	}

	ad->tr_send_list = bt_share_get_tr_data_list_by_status(db,
						BT_DB_OUTBOUND, BT_TR_ONGOING);
	bt_share_close_db(db);

	if (ad->tr_send_list == NULL)
		return;

	ad->tr_next_data = ad->tr_send_list;

	return;
}

gboolean _bt_update_sent_data_status(int uid, bt_app_tr_status_t status)
{
	DBG("_bt_update_sent_data_status  \n");

	DBG("uid = %d  \n", uid);
	sqlite3 *db = NULL;
	bt_tr_data_t *tmp;

	db = bt_share_open_db();
	if (!db)
		return FALSE;

	tmp = g_malloc0(sizeof(bt_tr_data_t));

	tmp->tr_status = status;
	tmp->timestamp = __bt_get_current_timedata();
	bt_share_update_tr_data(db, BT_DB_OUTBOUND, uid, tmp);
	bt_share_close_db(db);
	__bt_tr_data_free(tmp);

	return TRUE;
}

gboolean _bt_add_recv_transfer_status_data(char *device_name,
				char *filepath, int status)
{
	if (device_name == NULL || filepath == NULL)
		return FALSE;

	sqlite3 *db = NULL;
	bt_tr_data_t *tmp;

	DBG("Name [%s]\n", device_name);

	db = bt_share_open_db();
	if (!db)
		return FALSE;

	tmp = g_malloc0(sizeof(bt_tr_data_t));

	tmp->tr_status = status;
	tmp->file_path = g_strdup(filepath);
	tmp->dev_name =  g_strdup(device_name);
	tmp->timestamp = __bt_get_current_timedata();
	bt_share_add_tr_data(db, BT_DB_INBOUND, tmp);
	bt_share_close_db(db);
	__bt_tr_data_free(tmp);

	return TRUE;
}

static void __bt_share_update_tr_info(int tr_uid, int tr_type)
{
	DBG("__bt_share_update_tr_info tr_uid = %d \n", tr_uid);
	int status = -1;
	struct bt_appdata *ad = app_state;
	char str[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	bt_tr_data_t *info = NULL;
	sqlite3 *db = NULL;

	db = bt_share_open_db();
	if (!db)
		return;

	if (tr_type == BT_TR_OUTBOUND) {
		if (tr_uid == -1) {
			/* Click the "clear list" button in bluetooth-share-ui */
			/* Delete all outbound db / notification info */
			_bt_delete_notification(ad->send_noti);
			ad->send_noti = NULL;
			ad->send_data.tr_success = 0;
			ad->send_data.tr_fail = 0;
			bt_share_remove_tr_data_by_notification(db, BT_DB_OUTBOUND);
			bt_share_close_db(db);
		} else {
			/* Delete selected outbound db / notification info */

			info = bt_share_get_tr_data(db, BT_DB_OUTBOUND, tr_uid);
			if (info != NULL) {
				status = info->tr_status;
				g_free(info);
			}

			bt_share_remove_tr_data_by_id(db, BT_DB_OUTBOUND, tr_uid);
			bt_share_close_db(db);

			if (status == BT_TR_SUCCESS && ad->send_data.tr_success > 0)
				ad->send_data.tr_success--;
			else if (status == BT_TR_FAIL && ad->send_data.tr_fail > 0)
				ad->send_data.tr_fail--;
			else
				return;

			if ((ad->send_data.tr_success + ad->send_data.tr_fail) != 0) {
				snprintf(str, sizeof(str), BT_TR_STATUS,
					ad->send_data.tr_success, ad->send_data.tr_fail);

				_bt_update_notification(ad->send_noti,
						BT_STR_SENT, str,
						BT_ICON_QP_SEND);
			} else {
				_bt_delete_notification(ad->send_noti);
			}
		}
	} else if (tr_type == BT_TR_INBOUND) {
		if (tr_uid == -1) {
			/* Click the "clear list" button in bluetooth-share-ui */
			/* Delete all inbound db / notification info */

			_bt_delete_notification(ad->receive_noti);
			ad->receive_noti = NULL;
			ad->recv_data.tr_success = 0;
			ad->recv_data.tr_fail = 0;
			bt_share_remove_all_tr_data(db, BT_DB_INBOUND);
			bt_share_close_db(db);
		} else {
			/* Delete selected outbound db / notification info */

			info = bt_share_get_tr_data(db, BT_DB_INBOUND, tr_uid);
			if (info != NULL) {
				status = info->tr_status;
				g_free(info);
			}

			bt_share_remove_tr_data_by_id(db, BT_DB_INBOUND, tr_uid);
			bt_share_close_db(db);

			if (status == BT_TR_SUCCESS && ad->recv_data.tr_success > 0)
				ad->recv_data.tr_success--;
			else if (status == BT_TR_FAIL && ad->recv_data.tr_fail > 0)
				ad->recv_data.tr_fail--;
			else
				return;

			if ((ad->recv_data.tr_success + ad->recv_data.tr_fail) != 0) {
				snprintf(str, sizeof(str), BT_TR_STATUS,
					ad->recv_data.tr_success, ad->recv_data.tr_fail);

				_bt_update_notification(ad->receive_noti,
						BT_STR_RECEIVED, str,
						BT_ICON_QP_RECEIVE);
			} else {
				_bt_delete_notification(ad->receive_noti);
			}
		}
	} else {
		ERR("Invalid transfer type  \n");
	}
	return;
}

