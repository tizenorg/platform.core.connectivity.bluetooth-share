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

#include <sys/vfs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <vconf-keys.h>

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

#include "bluetooth-share-api.h"

#include <contacts-svc.h>
#include <calendar-svc-provider.h>
#include <vconf.h>
#include <Ecore_File.h>

extern struct bt_appdata *app_state;

bt_obex_server_authorize_into_t server_auth_info;
extern GSList *bt_transfer_list;

gboolean _bt_obex_writeclose(bt_obex_server_transfer_info_t *transfer_complete_info);

static void __bt_obex_file_push_auth(bt_obex_server_authorize_into_t *server_auth_info);

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


void _bt_share_event_handler(int event, bluetooth_event_param_t *param,
			       void *user_data)
{
	static int send_cnt = 0;
	static char *name = NULL;
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

	if (bt_transfer_list)
		node = bt_transfer_list->data;
	DBG("OPC event : [0x%x] \n", event);

	switch (event) {
	case BLUETOOTH_EVENT_DISABLED:
		_bt_terminate_app();
		break;

	case BLUETOOTH_EVENT_OPC_CONNECTED:
		DBG("BLUETOOTH_EVENT_OPC_CONNECTED, result [%d] \n", param->result);
		if (param->result != BLUETOOTH_ERROR_NONE) {
			_bt_create_warning_popup(param->result);

			if (NULL != node &&  node->file_cnt > send_cnt) {
				while (NULL != ad->tr_next_data) {
					info = (bt_tr_data_t *)(ad->tr_next_data)->data;
					if (info == NULL)
						break;

					_bt_update_sent_data_status(info->id, BT_TR_FAIL);
					ad->send_data.tr_fail++;
					ad->tr_next_data = g_slist_next(ad->tr_next_data);
				}

				_bt_delete_notification(BT_NOTI_T, SEND_NOTI_PRIV_ID);
				noti = _bt_create_notification(BT_NOTI_T,
						SEND_NOTI_PRIV_ID);

				snprintf(str, sizeof(str), BT_TR_STATUS,
					ad->send_data.tr_success,
					ad->send_data.tr_fail);

				DBG("str = [%s] \n", str);
				_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
						NOTI_TR_TYPE_OUT, NULL, NULL);
				_bt_set_notification_property(noti, QP_NO_DELETE);
				_bt_insert_notification(noti,
							BT_STR_SEND_NOTI, str,
							BT_ICON_QP_SEND);
			}

			send_cnt = 0;
			_remove_transfer_info(node);

			_bt_update_transfer_list_view(BT_OUTBOUND_TABLE);
		} else {
			_bt_share_block_sleep(TRUE);
			_bt_set_transfer_indicator(TRUE);
		}
		break;

	case BLUETOOTH_EVENT_OPC_TRANSFER_STARTED:
		DBG("BLUETOOTH_EVENT_OPC_TRANSFER_STARTED \n");

		ret_if(node == NULL);
		name = __get_file_name(send_cnt++, node->file_path);
		snprintf(opc_cnt, sizeof(opc_cnt), "%d/%d",
					send_cnt, node->file_cnt);
		noti = _bt_create_notification(BT_ONGOING_T, SEND_PRIV_ID);
		_bt_set_notification_app_launch(noti, CREATE_PROGRESS,
					NOTI_TR_TYPE_OUT, name, opc_cnt);
		_bt_set_notification_property(noti, QP_NO_TICKER);
		_bt_insert_notification(noti,
					name, opc_cnt,
					BT_ICON_QP_SEND);

		if (ad->tr_next_data == NULL) {
			ERR("ad->tr_next_data is NULL \n");
			break;
		}

		info = (bt_tr_data_t *)(ad->tr_next_data)->data;
		if (info == NULL)
			break;

		ad->current_tr_uid = info->id;
		DBG("ad->current_tr_uid = [%d] \n", ad->current_tr_uid);
		ad->tr_next_data = g_slist_next(ad->tr_next_data);
		break;

	case BLUETOOTH_EVENT_OPC_TRANSFER_PROGRESS:
		client_info = (bt_opc_transfer_info_t *)param->param_data;
		ret_if(client_info == NULL);

		percentage = client_info->percentage;
		_bt_update_notification_progress(NULL,
				SEND_PRIV_ID, percentage);
		_bt_send_message_to_ui(SEND_PRIV_ID, name, percentage,
					FALSE, BLUETOOTH_ERROR_NONE);
		break;

	case BLUETOOTH_EVENT_OPC_TRANSFER_COMPLETE:
		DBG("BLUETOOTH_EVENT_OPC_TRANSFER_COMPLETE \n");
		client_info = (bt_opc_transfer_info_t *)param->param_data;
		ret_if(client_info == NULL);

		DBG("client_info->filename = [%s] \n", client_info->filename);
		DBG("ad->current_tr_uid = [%d] \n", ad->current_tr_uid);

		_bt_send_message_to_ui(SEND_PRIV_ID, name, 100, TRUE, param->result);

		_bt_delete_notification(BT_ONGOING_T, SEND_PRIV_ID);
		_bt_delete_notification(BT_NOTI_T, SEND_NOTI_PRIV_ID);

		DBG("ad->send_data.tr_fail = %d, ad->send_data.tr_success= %d \n",
			ad->send_data.tr_fail, ad->send_data.tr_success);

		if (param->result != BLUETOOTH_ERROR_NONE)
			ad->send_data.tr_fail++;
		else
			ad->send_data.tr_success++;

		noti = _bt_create_notification(BT_NOTI_T,
					SEND_NOTI_PRIV_ID);

		if (noti != NULL) {
			snprintf(str, sizeof(str), BT_TR_STATUS,
				ad->send_data.tr_success,
				ad->send_data.tr_fail);

			DBG("str = [%s] \n", str);
			_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
					NOTI_TR_TYPE_OUT, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE);
			_bt_insert_notification(noti,
						BT_STR_SEND_NOTI, str,
						BT_ICON_QP_SEND);
		}

		if (param->result != BLUETOOTH_ERROR_NONE) {
			_bt_update_sent_data_status(ad->current_tr_uid,
							BT_TR_FAIL);
			_bt_create_warning_popup(param->result);
		} else {
			_bt_update_sent_data_status(ad->current_tr_uid,
							BT_TR_SUCCESS);
		}
		_bt_update_transfer_list_view(BT_OUTBOUND_TABLE);
		break;

	case BLUETOOTH_EVENT_OPC_DISCONNECTED:
		DBG("BLUETOOTH_EVENT_OPC_DISCONNECTED \n");

		if (node->file_cnt > send_cnt) {
			while (NULL != ad->tr_next_data) {
				info = (bt_tr_data_t *)(ad->tr_next_data)->data;
				if (info == NULL)
					break;

				_bt_update_sent_data_status(info->id, BT_TR_FAIL);
				ad->send_data.tr_fail++;
				ad->tr_next_data = g_slist_next(ad->tr_next_data);
			}

			_bt_delete_notification(BT_NOTI_T, SEND_NOTI_PRIV_ID);
			noti = _bt_create_notification(BT_NOTI_T,
					SEND_NOTI_PRIV_ID);

			snprintf(str, sizeof(str), BT_TR_STATUS,
				ad->send_data.tr_success,
				ad->send_data.tr_fail);

			DBG("str = [%s] \n", str);
			_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
					NOTI_TR_TYPE_OUT, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE);
			_bt_insert_notification(noti,
						BT_STR_SEND_NOTI, str,
						BT_ICON_QP_SEND);
		}

		send_cnt = 0;
		_bt_share_block_sleep(FALSE);
		_bt_set_transfer_indicator(FALSE);
		_remove_transfer_info(node);
		DBG("gList len : %d\n",  g_slist_length(bt_transfer_list));
		if (g_slist_length(bt_transfer_list) > 0) {
			DBG("One more job existed !!\n");
			if (!_request_file_send(bt_transfer_list->data)) {
				g_slist_free_full(bt_transfer_list,
							(GDestroyNotify)_free_transfer_info);
				bt_transfer_list = NULL;
			}
		}
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_AUTHORIZE:
		DBG("BT_EVENT_OBEX_TRANSFER_AUTHORIZE \n");
		if (param->result == BLUETOOTH_ERROR_NONE) {
			auth_info = param->param_data;
			server_auth_info.filename = auth_info->filename;
			server_auth_info.length = auth_info->length;
			if (server_auth_info.filename)
				__bt_obex_file_push_auth(&server_auth_info);
		}
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_STARTED:
		DBG("BT_EVENT_OBEX_TRANSFER_STARTED \n");
		transfer_info = param->param_data;
		DBG(" %s \n", transfer_info->type);
		noti  = _bt_create_notification(BT_ONGOING_T,
			transfer_info->transfer_id);
		_bt_set_notification_app_launch(noti, CREATE_PROGRESS,
				NOTI_TR_TYPE_IN, transfer_info->filename, NULL);
		_bt_set_notification_property(noti, QP_NO_TICKER);
		if (0 == g_strcmp0(transfer_info->type, TRANSFER_GET)) {
			_bt_insert_notification(noti,
					transfer_info->filename, NULL,
					BT_ICON_QP_SEND);
			_bt_update_notification_progress(NULL,
					transfer_info->transfer_id, 0);
			_bt_set_transfer_indicator(TRUE);
		} else {
			_bt_insert_notification(noti,
					transfer_info->filename, NULL,
					BT_ICON_QP_RECEIVE);
			_bt_set_transfer_indicator(TRUE);
		}
		_bt_share_block_sleep(TRUE);
		break;

	case BLUETOOTH_EVENT_OBEX_SERVER_TRANSFER_PROGRESS:
		DBG("BT_EVENT_OBEX_TRANSFER_PROGRESS \n");
		if (param->result == BLUETOOTH_ERROR_NONE) {
			transfer_info = param->param_data;
			if (0 == g_strcmp0(transfer_info->type,
						TRANSFER_GET)) {
				_bt_update_notification_progress(NULL,
				  transfer_info->transfer_id,
				  transfer_info->percentage);
			} else {
				  _bt_update_notification_progress(NULL,
				  transfer_info->transfer_id,
				  transfer_info->percentage);
			}

			_bt_send_message_to_ui(transfer_info->transfer_id,
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

		_bt_send_message_to_ui(transfer_info->transfer_id,
			transfer_info->filename,
			transfer_info->percentage, TRUE, param->result);

		if (param->result == BLUETOOTH_ERROR_NONE) {
			_bt_obex_writeclose(param->param_data);
			_bt_update_notification_progress(NULL,
					transfer_info->transfer_id, 100);
		} else {
			DBG("param->result = %d  \n", param->result);
			_bt_create_warning_popup(param->result);
		}

		_bt_delete_notification(BT_ONGOING_T,
				transfer_info->transfer_id);

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

			_bt_delete_notification(BT_NOTI_T,
					RECV_NOTI_PRIV_ID);

			noti  = _bt_create_notification(BT_NOTI_T,
						RECV_NOTI_PRIV_ID);
			_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
				NOTI_TR_TYPE_IN, NULL, NULL);
			_bt_set_notification_property(noti, QP_NO_DELETE);
			_bt_insert_notification(noti,
					BT_STR_RECEIVED_NOTI, str,
					BT_ICON_QP_RECEIVE);
		}

		_bt_update_transfer_list_view(BT_INBOUND_TABLE);
		break;

	default:
		DBG("Unhandled event %x", event);
		break;
	}

}

gboolean _bt_get_default_storage(char *storage)
{
	int val = -1;
	if (-1 == vconf_get_int(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT, (void *)&val)) {
		DBG("vconf error\n");
		return FALSE;
	}
	if (val == 0) /* Phone memory is 0, MMC is 1 */
		g_strlcpy(storage, BT_DOWNLOAD_PHONE_FOLDER, STORAGE_PATH_LEN_MAX);
	else
		g_strlcpy(storage, BT_DOWNLOAD_MMC_FOLDER, STORAGE_PATH_LEN_MAX);
	DBG("Default storage : %s\n", storage);
	return TRUE;
}


void _bt_app_obex_download_dup_file_cb(void *data, void *obj,
				       void *event_info)
{
	bt_obex_server_authorize_into_t *server_auth_info = data;

	DBG("response : %d\n", (int)event_info);

	if ((int)event_info == POPUP_RESPONSE_OK) {
		DBG("OK button pressed \n");
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
	FILE *fd = NULL;
	char text[BT_POPUP_TEXT_LEN] = { 0, };
	bt_app_sys_popup_params_t popup_params = { NULL };
	char storage[STORAGE_PATH_LEN_MAX] = { 0, };

	/* Check if  file is already present */
	_bt_get_default_storage(storage);

	snprintf(temp_filename, BT_FILE_PATH_LEN_MAX, "%s/%s",
		    storage, server_auth_info->filename);
	fd = fopen(temp_filename, "r");

	if (NULL != fd) {
		DBG("opp_info->file_info.file_name  = %s is already exsist\n",
			     server_auth_info->filename);

		/*File exsists: Ask the user if he wants to download the file */
		snprintf(text,
			    BT_POPUP_TEXT_LEN,
			    BT_STR_FILE_EXSIST_MSG, server_auth_info->filename);
		popup_params.title = text;
		popup_params.type = "twobtn";
		_bt_launch_system_popup(BT_APP_EVENT_CONFIRM_MODE_REQUEST,
					&popup_params,
					_bt_app_obex_download_dup_file_cb,
					server_auth_info);
		if (fd != NULL) {
			fclose(fd);
			fd = NULL;
		}
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

static int __bt_vcard_handler(const char *vcard, void *data)
{
	int ret;

	ret = contacts_svc_insert_vcard(0, vcard);
	if (ret < CTS_SUCCESS) {
		ERR("CTS error : %d\n", ret);
	}

	return 0;
}

static gboolean __bt_save_v_object(char *file_path,
					    bt_file_type_t file_type)
{
	retv_if(NULL == file_path, FALSE);
	int ret;

	DBG("file_path = %s, file_type = %d\n", file_path, file_type);

	switch (file_type) {
	case BT_FILE_VCARD:
	{
		DBG("BT_OPP_FILE_VCARD received\n");

		ret = contacts_svc_connect();
		if (ret != CTS_SUCCESS) {
			ERR("[error] = %d \n", ret);
			return FALSE;
		}

		ret = contacts_svc_vcard_foreach(file_path, __bt_vcard_handler, NULL);
		if (ret != CTS_SUCCESS) {
			ERR("[error] = %d \n", ret);
			return FALSE;
		}

		ret = contacts_svc_disconnect();
		if (ret != CTS_SUCCESS) {
			ERR("[error] = %d \n", ret);
			return FALSE;
		}
	}
		break;

	case BT_FILE_VCAL:
        {
		ret = calendar_svc_connect();
                if (ret != CAL_SUCCESS) {
                        ERR("Failed to close Calendar service.\n");
                        return FALSE;
                }

                ret = calendar_svc_calendar_import(file_path, DEFAULT_CALENDAR_ID);
                if (ret != CAL_SUCCESS) {
                        ERR("Failed to import path(%s): error (%d)\n", file_path, ret);
			ret = calendar_svc_close();
	                if ( ret != CAL_SUCCESS) {
	                        ERR("Failed to close Calendar service.\n");
	                }
			return FALSE;
                }
		ret = calendar_svc_close();
                if (ret != CAL_SUCCESS) {
                        ERR("Failed to close Calendar service.\n");
                        return FALSE;
                }
        }
                break;

	default:
		break;
	}

	return TRUE;
}

gboolean _bt_obex_writeclose(bt_obex_server_transfer_info_t *transfer_complete_info)
{
	retv_if(transfer_complete_info->filename == NULL, FALSE);

	char file_path[BT_TEMP_FILE_PATH_LEN_MAX] = { 0, };
	bt_file_type_t file_type = 0;
	char *extn = NULL;
	char storage[STORAGE_PATH_LEN_MAX] ={0, };

	DBG("File name[%s], File name length = [%d]\n",
			 transfer_complete_info->filename,
			 strlen(transfer_complete_info->filename));

	extn = strrchr(transfer_complete_info->filename, '.');
	if (NULL != extn)
		extn++;
	DBG("====== : %s\n", extn);
	file_type = __get_file_type(extn);
	DBG("file type : %d\n", file_type);
	switch (file_type) {
	case BT_FILE_VCARD:
	case BT_FILE_VCAL:
	case BT_FILE_VBOOKMARK:
		_bt_get_default_storage(storage);
		snprintf(file_path, sizeof(file_path), "%s/%s", storage,
			    transfer_complete_info->filename);
		DBG("BT_OPP_FILE_VOBJECT : file_path = %s\n", file_path);
		if (__bt_save_v_object(file_path, file_type)) {
			ecore_file_remove(file_path);
		}
		break;
	default:
		break;
	}
	return FALSE;
}


