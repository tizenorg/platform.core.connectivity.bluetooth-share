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

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <glib.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <notification.h>
#include <vconf.h>

#include "applog.h"
#include "bt-share-main.h"
#include "bt-share-ipc.h"
#include "bt-share-syspopup.h"
#include "bt-share-notification.h"
#include "bt-share-resource.h"
#include "obex-event-handler.h"
#include "bluetooth-share-api.h"
#include "bt-share-common.h"

#define FILEPATH_LEN_MAX 4096

GSList *bt_transfer_list = NULL;
DBusConnection *dbus_connection = NULL;

extern struct bt_appdata *app_state;

static void __bt_create_send_data(opc_transfer_info_t * node);
static void __bt_create_send_failed_data(char *filepath, char *dev_name,
						char *addr, char *type, unsigned int size);
static void __bt_share_update_tr_info(int tr_uid, int tr_type);

static void __bt_tr_data_free(bt_tr_data_t * data)
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
	DBG("+");
	struct bt_appdata *ad = app_state;
	if (ad->popups.syspopup_request == FALSE) {
		DBG("This event is not mine\n");
		return;
	}
	DBG(" res : %d", res);
	/* Inorder to support calling popup from callback, we have to make
	 * syspopup_request false here and also should not assign
	 * ad->popups.popup_cb = NULL */

	ad->popups.syspopup_request = FALSE;

	if (NULL != ad->popups.popup_cb) {
		if (res == 0)
			ad->popups.popup_cb(ad->popups.popup_cb_data,
					    NULL, (void *)POPUP_RESPONSE_OK);
		else if (res == 1)
			ad->popups.popup_cb(ad->popups.popup_cb_data,
					    NULL,
					    (void *)POPUP_RESPONSE_CANCEL);
		else if (res == 2)
			ad->popups.popup_cb(ad->popups.popup_cb_data,
					    NULL,
					    (void *)POPUP_RESPONSE_TIMEOUT);
	}
	DBG("-");
}

static opc_transfer_info_t *__add_transfer_info(DBusMessage *msg)
{
	DBG("+");
	int i = 0;
	int cnt = 0;
	char addr[BLUETOOTH_ADDRESS_LENGTH] = { 0, };
	char *name = NULL;
	char *type = NULL;
	char byte;
	char *file_path = NULL;
	char mime_type[BT_MIME_TYPE_MAX_LEN] = { 0 };
	unsigned int file_size=0;
	struct stat file_attr;
	int len;
	opc_transfer_info_t *data;
	struct bt_appdata *ad = app_state;
	DBusMessageIter iter;
	DBusMessageIter iter_addr;
	DBusMessageIter iter_file;
	DBusMessageIter iter_filepath;
	GSList *list = NULL;

	retv_if(msg == NULL, NULL);

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_recurse(&iter, &iter_addr);

	while (dbus_message_iter_get_arg_type(&iter_addr) == DBUS_TYPE_BYTE) {
		dbus_message_iter_get_basic(&iter_addr, &byte);
		addr[i] = byte;
		i++;
		dbus_message_iter_next(&iter_addr);
	}

	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &name);
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &type);

	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &iter_file);

	while (dbus_message_iter_get_arg_type(&iter_file) == DBUS_TYPE_ARRAY) {
		i = 0;
		dbus_message_iter_recurse(&iter_file, &iter_filepath);
		len = dbus_message_iter_get_array_len(&iter_filepath);
		if (len <= 0)
			continue;

		file_path = g_malloc0(len + 1);
		while (dbus_message_iter_get_arg_type(&iter_filepath) ==
		       DBUS_TYPE_BYTE) {
			dbus_message_iter_get_basic(&iter_filepath, &byte);
			file_path[i] = byte;
			i++;
			dbus_message_iter_next(&iter_filepath);
		}
#ifdef _TEMP_
		if (aul_get_mime_from_file(file_path, mime_type,
				BT_MIME_TYPE_MAX_LEN) == AUL_R_OK)
			INFO("mime type = %s", mime_type);
#endif
		if (g_utf8_validate(file_path, -1, NULL)) {
			if (stat(file_path, &file_attr) == 0)
				file_size = file_attr.st_size;
			else
				file_size = 0;
			INFO("%d", file_size);

			list = g_slist_append(list, file_path);
		} else {
			DBG("Invalid filepath");
			ad->send_data.tr_fail++;
			__bt_create_send_failed_data(file_path, name, addr,
								mime_type, file_size);

			if (ad->send_noti == NULL) {
				ad->send_noti = _bt_insert_notification(ad, BT_SENT_NOTI);
			} else {
				_bt_update_notification(ad, ad->send_noti, NULL, NULL, NULL);
			}

			g_free(file_path);
		}

		dbus_message_iter_next(&iter_file);
	}

	cnt = g_slist_length(list);
	INFO("cnt = %d", cnt);

	if (cnt == 0) {
		/* Show unable to send popup */
		_bt_create_warning_popup(BLUETOOTH_ERROR_INTERNAL,
			BT_STR_UNABLE_TO_SEND);
		return NULL;
	}

	INFO("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", addr[0],
	    addr[1], addr[2], addr[3], addr[4], addr[5]);
	INFO(" cnt( %d )", cnt);
	DBG_SECURE(" name ( %s )", name);

	data = g_new0(opc_transfer_info_t, 1);
	data->content = g_new0(char *, cnt + 1);
	data->file_path = g_new0(char *, cnt + 1);
	data->file_cnt = cnt;
	data->type = g_new0(char *, cnt + 1);
	data->size = g_new0(unsigned int *, cnt + 1);
	memcpy(data->addr, addr, BLUETOOTH_ADDRESS_LENGTH);
	memcpy(data->name, name, BLUETOOTH_DEVICE_NAME_LENGTH_MAX);

	for (i = 0; i < cnt; i++) {
		char *ptr = g_slist_nth_data(list, i);
		DBG_SECURE("%s", ptr);
		if (g_strcmp0(type, "text") == 0)
			data->file_path[i] = _bt_share_create_transfer_file(ptr);
		else
			data->file_path[i] = g_strdup(ptr);
#ifdef _TEMP_
		if(aul_get_mime_from_file(data->file_path[i], mime_type,
			BT_MIME_TYPE_MAX_LEN) == AUL_R_OK)
			g_free(data->type);
			data->type[i] = g_strdup(mime_type);
#endif
		if (stat(data->file_path[i], &file_attr) == 0)
			file_size = file_attr.st_size;
		else
			file_size = 0;

		data->size[i] = file_size;
		data->content[i] = g_strdup(ptr);
		INFO(" type ( %s ), size (%d)", data->type[i], data->size[i]);
	}

	bt_transfer_list = g_slist_append(bt_transfer_list, data);

	g_slist_free_full(list, g_free);

	return data;
}

void _free_transfer_info(opc_transfer_info_t * node)
{
	int i;

	DBG("+");
	ret_if(node == NULL);

	for (i = 0; i < node->file_cnt; i++) {
		//_bt_remove_tmp_file(node->file_path[i]);
		g_free(node->file_path[i]);
		g_free(node->content[i]);
		g_free(node->type[i]);
	}
	g_free(node->file_path);
	g_free(node->content);
	g_free(node->type);
	g_free(node->size);
	g_free(node);

	DBG("-");
}

void _remove_transfer_info(opc_transfer_info_t * node)
{
	DBG("+");
	ret_if(node == NULL);

	bt_transfer_list = g_slist_remove(bt_transfer_list, node);
	_free_transfer_info(node);

	DBG("-");
}

int _request_file_send(opc_transfer_info_t * node)
{
	struct bt_appdata *ad = app_state;
	int ret;
	DBG("+");
	retv_if(ad == NULL, BLUETOOTH_ERROR_INVALID_PARAM);
	retv_if(node == NULL, BLUETOOTH_ERROR_INVALID_PARAM);

	ret =
	    bluetooth_opc_push_files((bluetooth_device_address_t *) node->addr,
				     node->file_path);
	if (ret != BLUETOOTH_ERROR_NONE) {
		ERR("bluetooth_opc_push_files failed : %d", ret);
		return ret;
	}

	__bt_create_send_data(node);

	DBG("-");
	return BLUETOOTH_ERROR_NONE;
}
static int __bt_get_owner_info(DBusMessage *msg, char **name,
				char **previous, char **current)
{
	DBusMessageIter item_iter;

	dbus_message_iter_init(msg, &item_iter);

	if (dbus_message_iter_get_arg_type(&item_iter)
					!= DBUS_TYPE_STRING) {
		ERR("This is bad format dbus\n");
		return BLUETOOTH_ERROR_INTERNAL;
	}

	dbus_message_iter_get_basic(&item_iter, name);

	retv_if(*name == NULL, BLUETOOTH_ERROR_INTERNAL);

	dbus_message_iter_next(&item_iter);

	if (dbus_message_iter_get_arg_type(&item_iter)
					!= DBUS_TYPE_STRING) {
		ERR("This is bad format dbus\n");
		return BLUETOOTH_ERROR_INTERNAL;
	}

	dbus_message_iter_get_basic(&item_iter, previous);

	retv_if(*previous == NULL, BLUETOOTH_ERROR_INTERNAL);

	dbus_message_iter_next(&item_iter);

	if (dbus_message_iter_get_arg_type(&item_iter)
					!= DBUS_TYPE_STRING) {
		ERR("This is bad format dbus\n");
		return BLUETOOTH_ERROR_INTERNAL;
	}

	dbus_message_iter_get_basic(&item_iter, current);

	retv_if(*current == NULL, BLUETOOTH_ERROR_INTERNAL);

	return BLUETOOTH_ERROR_NONE;
}
static DBusHandlerResult __event_filter(DBusConnection * sys_conn,
					DBusMessage * msg, void *data)
{
	int ret;
	char *member;
	struct bt_appdata *ad = app_state;
	const char *path = dbus_message_get_path(msg);

	if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (path == NULL || strcmp(path, "/") == 0)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	member = (char *)dbus_message_get_member(msg);
	INFO("member (%s)", member);

	if (dbus_message_is_signal
	    (msg, BT_SYSPOPUP_INTERFACE, BT_SYSPOPUP_METHOD_RESPONSE)) {
		int res = 0;
		dbus_message_get_args(msg, NULL,
				      DBUS_TYPE_INT32, &res, DBUS_TYPE_INVALID);
		__popup_res_cb(res);
	} else
	    if (dbus_message_is_signal
		(msg, BT_UG_IPC_INTERFACE, BT_UG_IPC_METHOD_SEND)) {
		opc_transfer_info_t *node;

		node = __add_transfer_info(msg);
		if (node == NULL)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		ret = _request_file_send(node);
		if (ret == BLUETOOTH_ERROR_IN_PROGRESS) {
			INFO("Aleady OPC progressing. Once completed previous job, will be started");
		} else if (ret != BLUETOOTH_ERROR_NONE) {
			_bt_create_warning_popup(BLUETOOTH_ERROR_INTERNAL,
				BT_STR_UNABLE_TO_SEND);
			g_slist_free_full(bt_transfer_list,
					  (GDestroyNotify) _free_transfer_info);
			bt_transfer_list = NULL;
		}
	} else
	    if (dbus_message_is_signal
		(msg, BT_SHARE_UI_INTERFACE, BT_SHARE_UI_SIGNAL_OPPABORT)) {
		const char *transfer_type = NULL;
		int noti_id = 0;

		if (!dbus_message_get_args(msg, NULL,
					   DBUS_TYPE_STRING, &transfer_type,
					   DBUS_TYPE_INT32, &noti_id,
					   DBUS_TYPE_INVALID)) {
			ERR("OPP abort handling failed");
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}
		ad->opp_transfer_abort = TRUE; /* Transfer aborted by user*/
		INFO("transfer_type = %s", transfer_type);
		if (!g_strcmp0(transfer_type, NOTI_TR_TYPE_OUT)) {
			bluetooth_opc_cancel_push();
			if (ad->opc_noti) {
				ret = _bt_delete_notification(ad->opc_noti);
				if (ret == NOTIFICATION_ERROR_NONE) {
					ad->opc_noti = NULL;
					ad->opc_noti_id = 0;
				}
			}
		} else {
			bluetooth_obex_server_cancel_transfer(noti_id);
		}
	} else if (dbus_message_is_signal(msg, BT_SHARE_UI_INTERFACE,
					  BT_SHARE_UI_SIGNAL_SEND_FILE)) {
		opc_transfer_info_t *node;

		node = __add_transfer_info(msg);
		if (node == NULL)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		ret = _request_file_send(node);
		if (ret == BLUETOOTH_ERROR_IN_PROGRESS) {
			INFO("Aleady OPC progressing. Once completed previous job, will be started");
		} else if (ret != BLUETOOTH_ERROR_NONE) {
			_bt_create_warning_popup(BLUETOOTH_ERROR_INTERNAL,
				BT_STR_UNABLE_TO_SEND);
			g_slist_free_full(bt_transfer_list,
					  (GDestroyNotify) _free_transfer_info);
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

		INFO("tr_uid = %d, tr_type = %d ", tr_uid, tr_type);
		__bt_share_update_tr_info(tr_uid, tr_type);
	} else if (dbus_message_is_signal(msg, BT_SHARE_FRWK_INTERFACE,
					  BT_SHARE_FRWK_SIGNAL_DEINIT)) {
		/* Deinitialize the obex server */
		if (bluetooth_obex_server_deinit() == BLUETOOTH_ERROR_NONE) {
			DBG("Obex Server deinit");
		}
	} else if (strcasecmp(member, "NameOwnerChanged") == 0) {
		char *name = NULL;
		char *previous = NULL;
		char *current = NULL;

		if (__bt_get_owner_info(msg, &name, &previous, &current)) {
			ERR("Fail to get the owner info");
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		if (*current != '\0')
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		if (strcasecmp(name, "org.bluez") == 0) {
			INFO("Bluetoothd is terminated");
			_bt_terminate_bluetooth_share();
		}
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

gboolean _bt_init_dbus_signal(void)
{
	DBG("+");
	DBusGConnection *conn;
	GError *err = NULL;
	DBusError dbus_error;

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &err);
	if (!conn) {
		ERR(" DBUS get failed");
		g_error_free(err);
		return FALSE;
	}
	dbus_connection = dbus_g_connection_get_connection(conn);

	/* Add the filter for network client functions */
	dbus_error_init(&dbus_error);
	dbus_connection_add_filter(dbus_connection, __event_filter, NULL, NULL);
	dbus_bus_add_match(dbus_connection,
				"type=signal,interface=" BT_BLUEZ_INTERFACE
				",member=NameOwnerChanged", &dbus_error);
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
		ERR("Fail to add dbus filter signal");
		dbus_error_free(&dbus_error);
	}

	DBG("-");
	return TRUE;
}

void _bt_update_transfer_list_view(char *db)
{
	DBusMessage *msg = NULL;
	ret_if(dbus_connection == NULL);

	msg = dbus_message_new_signal(BT_SHARE_ENG_OBJECT,
				BT_SHARE_ENG_INTERFACE,
				BT_SHARE_ENG_SIGNAL_UPDATE_VIEW);
	if (!msg) {
		ERR("Unable to allocate memory");
		return;
	}

	if (!dbus_message_append_args(msg,
				DBUS_TYPE_STRING, &db,
				DBUS_TYPE_INVALID)) {
		ERR("Event sending failed");
		dbus_message_unref(msg);
		return;
	}

	dbus_message_set_destination(msg, BT_SHARE_UI_INTERFACE);
	dbus_connection_send(dbus_connection, msg, NULL);
	dbus_message_unref(msg);
}

void _bt_create_warning_popup(int error_type, char *msg)
{
	/* If bluetooth-share-ui process is  not running */
	/* Then create the process and terminate it after popup shown */
	if (aul_app_is_running(UI_PACKAGE) == 0) {
		DBG("Creating new process for Warning Popup");
		char str[BT_TEXT_LEN_MAX] = { 0, };

		DBG("error_type: %d",error_type);
		switch (error_type) {
		case BLUETOOTH_ERROR_SERVICE_NOT_FOUND:
		case BLUETOOTH_ERROR_NOT_CONNECTED:
		case BLUETOOTH_ERROR_ACCESS_DENIED:
		case BLUETOOTH_ERROR_OUT_OF_MEMORY:
		case BLUETOOTH_ERROR_INTERNAL:
		case BLUETOOTH_ERROR_CANCEL:
			snprintf(str, BT_TEXT_LEN_MAX, "%s",
				 msg);
			break;
		default:
			return;
		}
		INFO("bt create warning popup notification");
	}
	return;
}

static time_t __bt_get_current_timedata(void)
{
	time_t t;
	time(&t);
	return ((int)t);
}

static char *__bt_conv_addr_type_to_addr_string(char *addr)
{
	char address[BT_ADDR_STR_LEN_MAX] = { 0, };
	retv_if(addr == NULL, NULL);

	snprintf(address, BT_ADDR_STR_LEN_MAX,
		 "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
		 addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	return g_strdup(address);
}

static void __bt_create_send_failed_data(char *filepath, char *dev_name,
					 char *addr, char *type, unsigned int size)
{
	int session_id;
	sqlite3 *db = NULL;

	db = bt_share_open_db();
	if (!db)
		return;

	session_id = bt_share_get_last_session_id(db, BT_DB_OUTBOUND);

	INFO("Last session id = %d", session_id);

	bt_tr_data_t *tmp;
	tmp = g_malloc0(sizeof(bt_tr_data_t));

	tmp->tr_status = BT_TR_FAIL;
	tmp->sid = session_id + 1;
	tmp->file_path = g_strdup(filepath);
	tmp->content = g_strdup(filepath);
	tmp->dev_name = g_strdup(dev_name);
	tmp->type = g_strdup(type);
	tmp->timestamp = __bt_get_current_timedata();
	tmp->addr = __bt_conv_addr_type_to_addr_string(addr);
	tmp->size = size;
	bt_share_add_tr_data(db, BT_DB_OUTBOUND, tmp);
	__bt_tr_data_free(tmp);

	bt_share_close_db(db);

	return;
}

static void __bt_create_send_data(opc_transfer_info_t * node)
{
	ret_if(node == NULL);
	struct bt_appdata *ad = app_state;

	int count = 0;
	int session_id;

	sqlite3 *db = NULL;

	DBG_SECURE("Name [%s]", node->name);

	db = bt_share_open_db();
	if (!db)
		return;

	session_id = bt_share_get_last_session_id(db, BT_DB_OUTBOUND);

	INFO("Last session id = %d", session_id);
	for (count = 0; count < node->file_cnt; count++) {
		bt_tr_data_t *tmp;
		tmp = g_malloc0(sizeof(bt_tr_data_t));

		tmp->tr_status = BT_TR_ONGOING;
		tmp->sid = session_id + 1;
		tmp->file_path = g_strdup(node->file_path[count]);
		DBG("tmp->file_path : %s", tmp->file_path);

		tmp->content = g_strdup(node->content[count]);
		tmp->dev_name = g_strdup(node->name);
		tmp->type = g_strdup(node->type[count]);
		tmp->timestamp = __bt_get_current_timedata();
		tmp->addr = __bt_conv_addr_type_to_addr_string(node->addr);
		tmp->size = node->size[count];
		bt_share_add_tr_data(db, BT_DB_OUTBOUND, tmp);
		__bt_tr_data_free(tmp);
	}

	if (ad->tr_send_list) {
		bt_share_release_tr_data_list(ad->tr_send_list);
		ad->tr_send_list = NULL;
	}

	ad->tr_send_list = bt_share_get_tr_data_list_by_status(db,
							       BT_DB_OUTBOUND,
							       BT_TR_ONGOING);
	bt_share_close_db(db);

	if (ad->tr_send_list == NULL)
		return;

	ad->tr_next_data = ad->tr_send_list;

	return;
}

gboolean _bt_update_sent_data_status(int uid, bt_app_tr_status_t status)
{
	INFO("uid = %d", uid);
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
			char *filepath, char *type,
			unsigned int size, int status)
{
	if (device_name == NULL || filepath == NULL)
		return FALSE;

	sqlite3 *db = NULL;
	bt_tr_data_t *tmp;

	DBG_SECURE("Name [%s]", device_name);

	db = bt_share_open_db();
	if (!db)
		return FALSE;

	tmp = g_malloc0(sizeof(bt_tr_data_t));

	tmp->tr_status = status;
	tmp->file_path = g_strdup(filepath);
	tmp->dev_name = g_strdup(device_name);
	tmp->timestamp = __bt_get_current_timedata();
	tmp->type = g_strdup(type);
	tmp->size = size;
	bt_share_add_tr_data(db, BT_DB_INBOUND, tmp);
	bt_share_close_db(db);
	__bt_tr_data_free(tmp);

	return TRUE;
}

static void __bt_share_update_tr_info(int tr_uid, int tr_type)
{
	DBG("tr_uid = %d", tr_uid);

	int status = -1;
	struct bt_appdata *ad = app_state;
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
			DBG("clear all");
			bt_share_remove_tr_data_by_notification(db,
								BT_DB_OUTBOUND);
			bt_share_close_db(db);
		} else {
			/* Delete selected outbound db / notification info */

			info = bt_share_get_tr_data(db, BT_DB_OUTBOUND, tr_uid);
			if (info != NULL) {
				status = info->tr_status;
				__bt_tr_data_free(info);
			}

			bt_share_remove_tr_data_by_id(db, BT_DB_OUTBOUND,
						      tr_uid);
			bt_share_close_db(db);

			if (status == BT_TR_SUCCESS
			    && ad->send_data.tr_success > 0)
				ad->send_data.tr_success--;
			else if (status == BT_TR_FAIL
				 && ad->send_data.tr_fail > 0)
				ad->send_data.tr_fail--;
			else
				return;

			if ((ad->send_data.tr_success +
			     ad->send_data.tr_fail) != 0) {
				_bt_update_notification(ad, ad->send_noti, NULL, NULL, NULL);
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
				__bt_tr_data_free(info);
			}

			bt_share_remove_tr_data_by_id(db, BT_DB_INBOUND,
						      tr_uid);
			bt_share_close_db(db);

			if (status == BT_TR_SUCCESS
			    && ad->recv_data.tr_success > 0)
				ad->recv_data.tr_success--;
			else if (status == BT_TR_FAIL
				 && ad->recv_data.tr_fail > 0)
				ad->recv_data.tr_fail--;
			else
				return;

			if ((ad->recv_data.tr_success +
			     ad->recv_data.tr_fail) != 0) {
				_bt_update_notification(ad, ad->receive_noti, NULL, NULL, NULL);
			} else {
				_bt_delete_notification(ad->receive_noti);
			}
		}
	} else {
		ERR("Invalid transfer type");
		bt_share_close_db(db);
	}
	return;
}
