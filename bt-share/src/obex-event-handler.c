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

#include <sys/vfs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <vconf-keys.h>
#include <calendar2.h>
#include <contacts.h>
#include <vconf.h>
#include <Ecore_File.h>
#include <bluetooth-share-api.h>
#include <notification.h>
#include <media_content.h>

#include "applog.h"
#include "bluetooth-api.h"
#include "obex-event-handler.h"
#include "bt-share-main.h"
#include "bt-share-syspopup.h"
#include "bt-share-resource.h"
#include "bt-share-ipc.h"
#include "bt-share-notification.h"
#include "bt-share-common.h"
#include "bt-share-noti-handler.h"


extern struct bt_appdata *app_state;

typedef struct {
	void *noti_handle;
	int transfer_id;
	int noti_id;
} bt_noti_data_t;

bt_obex_server_authorize_into_t server_auth_info;
extern GSList *bt_transfer_list;
GSList *bt_receive_noti_list;

gboolean _bt_obex_writeclose(bt_obex_server_transfer_info_t *transfer_complete_info);

static void __bt_obex_file_push_auth(bt_obex_server_authorize_into_t *server_auth_info);

static bt_noti_data_t *__bt_get_noti_data_by_transfer_id(int transfer_id)
{
	GSList *l;
	bt_noti_data_t *data;

	for (l = bt_receive_noti_list; l != NULL; l = l->next) {
		data = l->data;

		if (data == NULL)
			continue;

		if (data->transfer_id == transfer_id)
			return data;
	}

	return NULL;
}

static char *__get_file_name(int cnt, char **path)
{
	char *pfilename;
	if (path == NULL) {
		DBG("Path is invalid");
		return NULL;
	}
	pfilename = strrchr(path[cnt], '/') + 1;
	DBG("File name[%d] : %s \n", cnt, pfilename);
	return pfilename;
}

static void __delete_notification(gpointer data, gpointer user_data)
{
	bt_noti_data_t *noti_data = data;

	if (noti_data == NULL)
		return;

	_bt_delete_notification(noti_data->noti_handle);
	g_free(noti_data);
}

int _bt_get_transfer_id_by_noti_id(int noti_id)
{
	GSList *l;
	bt_noti_data_t *data;

	for (l = bt_receive_noti_list; l != NULL; l = l->next) {
		data = l->data;

		if (data == NULL)
			continue;

		if (data->noti_id == noti_id)
			return data->transfer_id;
	}

	return -1;
}

void _bt_clear_receive_noti_list(void)
{
	if (bt_receive_noti_list) {
		g_slist_foreach(bt_receive_noti_list,
				(GFunc)__delete_notification,
				NULL);
		g_slist_free(bt_receive_noti_list);
		bt_receive_noti_list = NULL;
	}
}

void _bt_share_event_handler(int event, bluetooth_event_param_t *param,
			       void *user_data)
{
	int noti_id;
	static int send_index = 0;
	char *name = NULL;
	char str[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	char opc_cnt[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	int percentage = 0;
	notification_h noti = NULL;
	bt_obex_server_authorize_into_t *auth_info;
	bt_obex_server_transfer_info_t *transfer_info;
	opc_transfer_info_t *node = NULL;
	struct bt_appdata *ad = app_state;
	bt_tr_data_t *info = NULL;
	bt_opc_transfer_info_t *client_info = NULL;
	bt_noti_data_t *data;
	int s_id = 0;

	if (bt_transfer_list)
		node = bt_transfer_list->data;
	DBG("OPC event : [0x%x] \n", event);

	switch (event) {
	case BLUETOOTH_EVENT_ENABLED:
		if (ad->obex_server_init == FALSE) {
			if (_bt_init_obex_server() == BT_SHARE_ERROR_NONE)
				ad->obex_server_init = TRUE;
		}
		break;

	case BLUETOOTH_EVENT_DISABLED:
		g_free(server_auth_info.filename);
		server_auth_info.filename = NULL;
		_bt_terminate_app();
		break;

	case BLUETOOTH_EVENT_OPC_CONNECTED:
		DBG("BLUETOOTH_EVENT_OPC_CONNECTED, result [%d] \n", param->result);
		if (param->result != BLUETOOTH_ERROR_NONE) {
			_bt_create_warning_popup(param->result);
			if (NULL != node && node->file_cnt > send_index) {
				info = (bt_tr_data_t *)(ad->tr_next_data)->data;
				if (info == NULL) {
					DBG("info is NULL");
					return;
				}

				s_id = info->sid;
				DBG("info->sid = %d info->id = %d\n", info->sid, info->id);
				while (NULL != ad->tr_next_data) {
					info = (bt_tr_data_t *)(ad->tr_next_data)->data;
					if (info == NULL)
						break;
					DBG("info->sid = %d info->id = %d\n", info->sid, info->id);
					if (info->sid != s_id) {
						DBG("SID did not match so break done.\n");
						break;
					}

					_bt_update_sent_data_status(info->id, BT_TR_FAIL);
					ad->send_data.tr_fail++;
					ad->tr_next_data = g_slist_next(ad->tr_next_data);
				}

				snprintf(str, sizeof(str), BT_TR_STATUS,
					ad->send_data.tr_success,
					ad->send_data.tr_fail);

				DBG("str = [%s] \n", str);

				if (ad->send_noti == NULL) {
					noti = _bt_create_notification(BT_NOTI_T);
					_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
							NOTI_TR_TYPE_OUT, NULL, NULL);
					_bt_set_notification_property(noti, QP_NO_DELETE);
					_bt_insert_notification(noti,
								BT_STR_SENT, str,
								BT_ICON_QP_SEND);
					ad->send_noti = noti;
				} else {
					_bt_update_notification(ad->send_noti,
								BT_STR_SENT, str,
								BT_ICON_QP_SEND);
				}
			}

			send_index = 0;
			_remove_transfer_info(node);

			if (!ad->tr_next_data) {
				bt_share_release_tr_data_list(ad->tr_send_list);
				ad->tr_send_list = NULL;
			}

			_bt_update_transfer_list_view(BT_OUTBOUND_TABLE);
		} else {
			_bt_share_block_sleep(TRUE);
			_bt_set_transfer_indicator(TRUE);
		}
		break;

	case BLUETOOTH_EVENT_OPC_TRANSFER_STARTED:
		DBG("BLUETOOTH_EVENT_OPC_TRANSFER_STARTED \n");

		ret_if(node == NULL);
		name = __get_file_name(send_index++, node->file_path);
		snprintf(opc_cnt, sizeof(opc_cnt), "%d/%d",
					send_index, node->file_cnt);

		if (ad->opc_noti) {
			_bt_delete_notification(ad->opc_noti);
			ad->opc_noti = NULL;
			ad->opc_noti_id = 0;
		}

		noti = _bt_create_notification(BT_ONGOING_T);
		_bt_set_notification_property(noti, QP_NO_TICKER);
		ad->opc_noti_id = _bt_insert_notification(noti,
					name, opc_cnt,
					BT_ICON_QP_SEND);
		_bt_set_notification_app_launch(noti, CREATE_PROGRESS,
					NOTI_TR_TYPE_OUT, name, opc_cnt);
		_bt_update_notification(noti, NULL, NULL, NULL);

		ad->opc_noti = noti;

		if (ad->tr_next_data == NULL) {
			ERR("ad->tr_next_data is NULL \n");
			break;
		}

		info = (bt_tr_data_t *)(ad->tr_next_data)->data;
		if (info == NULL)
			break;

		ad->current_tr_uid = info->id;
		DBG("ad->current_tr_uid = [%d] \n", ad->current_tr_uid);
		break;

	case BLUETOOTH_EVENT_OPC_TRANSFER_PROGRESS:
		client_info = (bt_opc_transfer_info_t *)param->param_data;
		ret_if(client_info == NULL);

		name =  strrchr(client_info->filename, '/');
		if (name)
			name++;
		else
			name = client_info->filename;

		percentage = client_info->percentage;
		_bt_update_notification_progress(NULL,
				ad->opc_noti_id, percentage);
		_bt_send_message_to_ui(ad->opc_noti_id, name, percentage,
					FALSE, BLUETOOTH_ERROR_NONE);
		break;

	case BLUETOOTH_EVENT_OPC_TRANSFER_COMPLETE:
		DBG("BLUETOOTH_EVENT_OPC_TRANSFER_COMPLETE \n");
		client_info = (bt_opc_transfer_info_t *)param->param_data;
		ret_if(client_info == NULL);

		DBG("client_info->filename = [%s] \n", client_info->filename);
		DBG("ad->current_tr_uid = [%d] \n", ad->current_tr_uid);

		name =  strrchr(client_info->filename, '/');
		if (name)
			name++;
		else
			name = client_info->filename;

		DBG("name address = [%x] \n", name);

		_bt_send_message_to_ui(ad->opc_noti_id, name, 100, TRUE, param->result);

		_bt_delete_notification(ad->opc_noti);
		ad->opc_noti = NULL;
		ad->opc_noti_id = 0;

		DBG("ad->send_data.tr_fail = %d, ad->send_data.tr_success= %d \n",
			ad->send_data.tr_fail, ad->send_data.tr_success);

		if (param->result != BLUETOOTH_ERROR_NONE)
			ad->send_data.tr_fail++;
		else
			ad->send_data.tr_success++;

		snprintf(str, sizeof(str), BT_TR_STATUS,
			ad->send_data.tr_success,
			ad->send_data.tr_fail);

		DBG("str = [%s] \n", str);

		if (ad->send_noti == NULL) {
			noti = _bt_create_notification(BT_NOTI_T);
			_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
					NOTI_TR_TYPE_OUT, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE);
			_bt_insert_notification(noti,
						BT_STR_SENT, str,
						BT_ICON_QP_SEND);
			ad->send_noti = noti;
		} else {
			_bt_update_notification(ad->send_noti,
						BT_STR_SENT, str,
						BT_ICON_QP_SEND);
		}

		_bt_remove_tmp_file(client_info->filename);

		if (param->result != BLUETOOTH_ERROR_NONE) {
			_bt_update_sent_data_status(ad->current_tr_uid,
							BT_TR_FAIL);
			_bt_create_warning_popup(param->result);
		} else {
			_bt_update_sent_data_status(ad->current_tr_uid,
							BT_TR_SUCCESS);
		}
		_bt_update_transfer_list_view(BT_OUTBOUND_TABLE);
		ad->tr_next_data = g_slist_next(ad->tr_next_data);
		break;

	case BLUETOOTH_EVENT_OPC_DISCONNECTED:
		DBG("BLUETOOTH_EVENT_OPC_DISCONNECTED \n");

		ret_if(node == NULL);

		if (node->file_cnt > send_index) {
			info = (bt_tr_data_t *)(ad->tr_next_data)->data;
			if (info == NULL)
				break;
			s_id = info->sid;
			DBG("info->sid = %d info->id = %d\n", info->sid, info->id);

			while (NULL != ad->tr_next_data) {
				info = (bt_tr_data_t *)(ad->tr_next_data)->data;
				if (info == NULL)
					break;
				DBG("info->sid = %d info->id = %d\n", info->sid, info->id);

				if (s_id != info->sid) {
					DBG("SID did not match so break done.\n");
					break;
				}

				_bt_update_sent_data_status(info->id, BT_TR_FAIL);
				ad->send_data.tr_fail++;
				ad->tr_next_data = g_slist_next(ad->tr_next_data);
			}

			snprintf(str, sizeof(str), BT_TR_STATUS,
				ad->send_data.tr_success,
				ad->send_data.tr_fail);

			DBG("str = [%s] \n", str);

			if (ad->send_noti == NULL) {
				noti = _bt_create_notification(BT_NOTI_T);
				_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
						NOTI_TR_TYPE_OUT, NULL, NULL);
				_bt_set_notification_property(noti, QP_NO_DELETE);
				_bt_insert_notification(noti,
							BT_STR_SENT, str,
							BT_ICON_QP_SEND);
				ad->send_noti = noti;
			} else {
				_bt_update_notification(ad->send_noti,
							BT_STR_SENT, str,
							BT_ICON_QP_SEND);
			}
		}

		send_index = 0;
		_bt_share_block_sleep(FALSE);
		_bt_set_transfer_indicator(FALSE);
		_remove_transfer_info(node);
		if (!ad->tr_next_data) {
			bt_share_release_tr_data_list(ad->tr_send_list);
			ad->tr_send_list = NULL;
		}
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_AUTHORIZE:
		DBG("BT_EVENT_OBEX_TRANSFER_AUTHORIZE \n");
		if (param->result == BLUETOOTH_ERROR_NONE) {
			g_free(server_auth_info.filename);
			server_auth_info.filename = NULL;

			auth_info = param->param_data;
			server_auth_info.filename = g_strdup(auth_info->filename);
			server_auth_info.length = auth_info->length;
			if (server_auth_info.filename)
				__bt_obex_file_push_auth(&server_auth_info);
		}
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_STARTED:
		DBG("BT_EVENT_OBEX_TRANSFER_STARTED \n");
		transfer_info = param->param_data;

		DBG(" %s \n", transfer_info->type);
		noti  = _bt_create_notification(BT_ONGOING_T);
		_bt_set_notification_property(noti, QP_NO_TICKER);
		if (0 == g_strcmp0(transfer_info->type, TRANSFER_GET)) {
			noti_id = _bt_insert_notification(noti,
					transfer_info->filename, NULL,
					BT_ICON_QP_SEND);
			_bt_set_notification_app_launch(noti, CREATE_PROGRESS,
					NOTI_TR_TYPE_IN, transfer_info->filename, NULL);
			_bt_update_notification_progress(NULL,
					noti_id, 0);
			_bt_update_notification(noti, NULL, NULL, NULL);
		} else {
			noti_id = _bt_insert_notification(noti,
					transfer_info->filename, NULL,
					BT_ICON_QP_RECEIVE);
			_bt_set_notification_app_launch(noti, CREATE_PROGRESS,
					NOTI_TR_TYPE_IN, transfer_info->filename, NULL);
			_bt_update_notification(noti, NULL, NULL, NULL);
		}

		data = g_new0(bt_noti_data_t, 1);
		data->noti_handle = noti;
		data->noti_id = noti_id;
		data->transfer_id = transfer_info->transfer_id;

		bt_receive_noti_list = g_slist_append(bt_receive_noti_list, data);

		_bt_set_transfer_indicator(TRUE);
		_bt_share_block_sleep(TRUE);
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_PROGRESS:
		DBG("BT_EVENT_OBEX_TRANSFER_PROGRESS \n");
		if (param->result == BLUETOOTH_ERROR_NONE) {
			transfer_info = param->param_data;

			data = __bt_get_noti_data_by_transfer_id(transfer_info->transfer_id);

			_bt_update_notification_progress(NULL,
					data->noti_id,
					transfer_info->percentage);

			_bt_send_message_to_ui(data->noti_id,
					transfer_info->filename,
					transfer_info->percentage,
					FALSE, BLUETOOTH_ERROR_NONE);
		}
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_COMPLETED:
		DBG("BT_EVENT_OBEX_TRANSFER_COMPLETED \n");
		   transfer_info = param->param_data;
		_bt_share_block_sleep(FALSE);
		_bt_set_transfer_indicator(FALSE);

		data = __bt_get_noti_data_by_transfer_id(transfer_info->transfer_id);

		if (data) {
			_bt_send_message_to_ui(data->noti_id,
			transfer_info->filename,
			transfer_info->percentage, TRUE, param->result);

			if (param->result == BLUETOOTH_ERROR_NONE) {
				_bt_obex_writeclose(param->param_data);
				_bt_update_notification_progress(NULL,
						data->noti_id, 100);
			} else {
				DBG("param->result = %d  \n", param->result);
				_bt_create_warning_popup(param->result);
			}


			_bt_delete_notification(data->noti_handle);
			bt_receive_noti_list = g_slist_remove(bt_receive_noti_list, data);
			g_free(data);
		}

		if (0 == g_strcmp0(transfer_info->type, TRANSFER_PUT)) {
			if (param->result != BLUETOOTH_ERROR_NONE) {
				ad->recv_data.tr_fail++;
				_bt_add_recv_transfer_status_data(
							transfer_info->device_name,
							transfer_info->filename,
							BT_TR_FAIL);
			} else {
				ad->recv_data.tr_success++;
				_bt_add_recv_transfer_status_data(
							transfer_info->device_name,
							transfer_info->filename,
							BT_TR_SUCCESS);
			}

			snprintf(str, sizeof(str), BT_TR_STATUS,
				ad->recv_data.tr_success, ad->recv_data.tr_fail);
			DBG("str = [%s] \n", str);

			if (ad->receive_noti == NULL) {
				noti  = _bt_create_notification(BT_NOTI_T);
				_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
					NOTI_TR_TYPE_IN, NULL, NULL);
				_bt_set_notification_property(noti, QP_NO_DELETE);
				_bt_insert_notification(noti,
						BT_STR_RECEIVED, str,
						BT_ICON_QP_RECEIVE);
				ad->receive_noti = noti;
			} else {
				_bt_update_notification(ad->receive_noti,
						BT_STR_RECEIVED, str,
						BT_ICON_QP_RECEIVE);
			}
		}

		_bt_update_transfer_list_view(BT_INBOUND_TABLE);
		break;

	default:
		DBG("Unhandled event %x", event);
		break;
	}

}

void _bt_get_default_storage(char *storage)
{
	int val;
	int err;
	char *path;

	if (vconf_get_int(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
						(void *)&val)) {
		DBG("vconf error\n");
		val = BT_DEFAULT_MEM_PHONE;
	}

	if (val == BT_DEFAULT_MEM_MMC)
		path = BT_DOWNLOAD_MMC_FOLDER;
	else
		path = BT_DOWNLOAD_PHONE_FOLDER;

	if (access(path, W_OK) == 0) {
		g_strlcpy(storage, path, STORAGE_PATH_LEN_MAX);
		DBG("Storage path = [%s]\n", storage);
		return;
	}

	if (mkdir(BT_DOWNLOAD_PHONE_FOLDER, 0755) < 0) {
		err = -errno;
		DBG("mkdir: %s(%d)", strerror(-err), -err);
	}

	g_strlcpy(storage, BT_DOWNLOAD_PHONE_FOLDER, STORAGE_PATH_LEN_MAX);

	DBG("Default storage : %s\n", storage);
}


void _bt_app_obex_download_dup_file_cb(void *data, void *obj,
				       void *event_info)
{
	bt_obex_server_authorize_into_t *server_auth_info = data;
	char storage[STORAGE_PATH_LEN_MAX] = { 0, };
	char temp_filename[BT_FILE_PATH_LEN_MAX] = { 0, };

	DBG("response : %d\n", (int)event_info);

	if ((int)event_info == POPUP_RESPONSE_OK) {
		DBG("OK button pressed \n");

		_bt_get_default_storage(storage);
		snprintf(temp_filename, BT_FILE_PATH_LEN_MAX, "%s/%s",
				    storage, server_auth_info->filename);

		if (remove(temp_filename) != 0) {
			ERR("File remove failed\n");
			bluetooth_obex_server_reject_authorize();
			return;
		}

		DBG("temp_filename %s", temp_filename);
		bluetooth_obex_server_accept_authorize(server_auth_info->filename);
	} else if ((int)event_info == POPUP_RESPONSE_CANCEL) {
		bluetooth_obex_server_reject_authorize();
	}
	return;
}

static gboolean __bt_get_available_memory(long *available_mem_size)
{
	struct statfs fs = {0, };
	int val = -1;
	char *default_memory = NULL;

	if (-1 == vconf_get_int(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
				(void *)&val)) {
		ERR("vconf error\n");
		return FALSE;
	}
	if (val == 0) /* Phone memory is 0, MMC is 1 */
		default_memory = BT_DOWNLOAD_MEDIA_FOLDER;
	else
		default_memory = BT_DOWNLOAD_MMC_FOLDER;
	if (statfs(default_memory, &fs) != 0) {
		*available_mem_size = 0;
		return FALSE;
	}
	*available_mem_size = fs.f_bavail * (fs.f_bsize/1024);
	return TRUE;
}

gboolean _bt_app_obex_openwrite_requested(bt_obex_server_authorize_into_t
					   *server_auth_info)
{
	char temp_filename[BT_FILE_PATH_LEN_MAX] = { 0, };
	bt_app_sys_popup_params_t popup_params = { NULL };
	char storage[STORAGE_PATH_LEN_MAX] = { 0, };

	/* Check if  file is already present */
	_bt_get_default_storage(storage);

	snprintf(temp_filename, BT_FILE_PATH_LEN_MAX, "%s/%s",
		    storage, server_auth_info->filename);

	if (access(temp_filename, F_OK) == 0) {
		DBG("opp_info->file_info.file_name  = %s is already exsist\n",
			     server_auth_info->filename);

		/*File exsists: Ask the user if he wants to download the file */
		popup_params.file = server_auth_info->filename;
		popup_params.type = "twobtn";
		_bt_launch_system_popup(BT_APP_EVENT_OVERWRITE_REQUEST,
					&popup_params,
					_bt_app_obex_download_dup_file_cb,
					server_auth_info);
	} else {
		bluetooth_obex_server_accept_authorize(server_auth_info->filename);
	}
	return FALSE;
}

static void __bt_obex_file_push_auth(bt_obex_server_authorize_into_t *server_auth_info)
{
	 long available_mem_size = 0;

	DBG("+\n");

	 if (__bt_get_available_memory(&available_mem_size) == FALSE) {
		ERR("Unable to get available memory size\n");
		goto reject;
	 }
	 if (available_mem_size < server_auth_info->length / 1024) {
		g_timeout_add(BT_APP_POPUP_LAUNCH_TIMEOUT,
			      (GSourceFunc)_bt_app_popup_memoryfull,
			      NULL);
		goto reject;
	 }

	DBG("Check for overwrite. \n");
	/* Two popups back to back. So add a timeout */
	g_timeout_add(BT_APP_POPUP_LAUNCH_TIMEOUT,
		      (GSourceFunc)_bt_app_obex_openwrite_requested,
		      server_auth_info);

	return;

reject:
	bluetooth_obex_server_reject_authorize();

	DBG("-\n");
	return;
}

static bt_file_type_t __get_file_type(char *extn)
{
	DBG("exten : %s \n", extn);

	if (NULL != extn) {
		if (!strcmp(extn, "vcf"))
			return BT_FILE_VCARD;
		else if (!strcmp(extn, "vcs"))
			return BT_FILE_VCAL;
		else if (!strcmp(extn, "vbm"))
			return BT_FILE_VBOOKMARK;
	}
	return BT_FILE_OTHER;
}

static bool __bt_vcard_handler(contacts_record_h record, void *user_data)
{
	int ret;

	ret = contacts_db_insert_record(record, NULL);
	if (ret != CONTACTS_ERROR_NONE) {
		ERR("contacts_svc_insert_vcard error : %d\n", ret);
	}

	return true;
}

static bool __bt_vcalendar_handler(calendar_record_h record, void *user_data)
{
	int ret;

	ret = calendar_db_insert_record(record, NULL);
	if (ret != CALENDAR_ERROR_NONE) {
		ERR("calendar_db_insert_record error : %d\n", ret);
	}

	return true;
}

static gboolean __bt_save_v_object(char *file_path,
					    bt_file_type_t file_type)
{
	retv_if(NULL == file_path, FALSE);
	int ret;

	DBG("file_path = %s, file_type = %d\n", file_path, file_type);

	switch (file_type) {
	case BT_FILE_VCARD:
		ret = contacts_connect2();
		if (ret != CONTACTS_ERROR_NONE) {
			ERR("contacts_connect2 error = %d \n", ret);
			return FALSE;
		}

		ret = contacts_vcard_parse_to_contact_foreach(file_path,
					__bt_vcard_handler, NULL);
		if (ret != CONTACTS_ERROR_NONE) {
			ERR("[error] = %d \n", ret);
			ret = contacts_disconnect2();
			if (ret != CONTACTS_ERROR_NONE) {
				ERR("contacts_disconnect2 error = %d \n", ret);
			}
			return FALSE;
		}

		ret = contacts_disconnect2();
		if (ret != CONTACTS_ERROR_NONE) {
			ERR("contacts_disconnect2 error = %d \n", ret);
			return FALSE;
		}
		break;

	case BT_FILE_VCAL:
		ret = calendar_connect();
		if (ret != CALENDAR_ERROR_NONE) {
			ERR("calendar_connect error = %d \n", ret);
			return FALSE;
		}

		ret = calendar_vcalendar_parse_to_calendar_foreach(file_path,
					__bt_vcalendar_handler, NULL);
		if (ret != CALENDAR_ERROR_NONE) {
			ERR("[error] = %d \n", ret);
			ret = calendar_disconnect();
			if ( ret != CALENDAR_ERROR_NONE) {
				ERR("calendar_disconnect error = %d \n", ret);
			}
			return FALSE;
		}
		ret = calendar_disconnect();
		if (ret != CALENDAR_ERROR_NONE) {
			ERR("calendar_disconnect error = %d \n", ret);
			return FALSE;
		}
		break;

	default:
		break;
	}

	return TRUE;
}

gboolean __bt_scan_media_file(char *file_path)
{
	int ret;

	ret = media_content_connect();
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		DBG("Fail to connect the media content: %d", ret);
		return FALSE;
	}

	ret = media_content_scan_file(file_path);
	if (ret != MEDIA_CONTENT_ERROR_NONE)
		DBG("Fail to scan file: %d", ret);

	ret = media_content_disconnect();
	if (ret != MEDIA_CONTENT_ERROR_NONE)
		DBG("Fail to disconnect the media content: %d", ret);

	return TRUE;
}

gboolean _bt_obex_writeclose(bt_obex_server_transfer_info_t *transfer_complete_info)
{
	char file_path[BT_TEMP_FILE_PATH_LEN_MAX] = { 0, };
	bt_file_type_t file_type = 0;
	char *extn = NULL;
	char storage[STORAGE_PATH_LEN_MAX] ={0, };

	retv_if(transfer_complete_info->filename == NULL, FALSE);


	DBG("File name[%s], File name length = [%d]\n",
			 transfer_complete_info->filename,
			 strlen(transfer_complete_info->filename));

	extn = strrchr(transfer_complete_info->filename, '.');
	if (NULL != extn)
		extn++;
	DBG("====== : %s\n", extn);
	file_type = __get_file_type(extn);
	DBG("file type : %d\n", file_type);

	_bt_get_default_storage(storage);
	snprintf(file_path, sizeof(file_path), "%s/%s", storage,
		    transfer_complete_info->filename);

	DBG("BT_OPP_FILE_SAVE : file_path = %s\n", file_path);

	switch (file_type) {
	case BT_FILE_VCARD:
	case BT_FILE_VCAL:
	case BT_FILE_VBOOKMARK:
		if (__bt_save_v_object(file_path, file_type) == FALSE) {
			ecore_file_remove(file_path);
		} else {
			__bt_scan_media_file(file_path);
		}
		break;
	default:
		__bt_scan_media_file(file_path);
		break;
	}
	return TRUE;
}

