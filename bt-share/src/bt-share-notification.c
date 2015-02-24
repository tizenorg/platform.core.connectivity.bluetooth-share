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

#include <applog.h>
#include <notification.h>
#include <appsvc.h>
#include <vconf.h>

#include "bt-share-common.h"
#include "bt-share-notification.h"
#include "bt-share-main.h"
#include "bt-share-ipc.h"
#include "obex-event-handler.h"
#include "bluetooth-share-api.h"
#include "bt-share-resource.h"

#define BT_PERCENT_STR_LEN 5
#define BT_PRIV_ID_STR_LEN 8
#define BT_NOTI_STR_LEN_MAX 50

typedef enum {
	CSC_DCM,
	CSC_FTM,
} bt_csc_type_t;


notification_h _bt_insert_notification(struct bt_appdata *ad, bt_notification_type_e type)
{
	notification_h noti = NULL;
	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	notification_type_e noti_type = NOTIFICATION_TYPE_NONE;
	char str[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	char *title = NULL;
	char *content = NULL;
	char *icon_path = NULL;
	char *indicator_icon_path = NULL;
	char *app_id = NULL;
	int noti_id = 0;
	int flag = -1;
	int success = 0;
	int fail = 0;

	DBG("Create notification type : %d", type);

	if (type == BT_SENT_NOTI) {
		success = ad->send_data.tr_success;
		fail = ad->send_data.tr_fail;

		if (success == 1)
			content = "IDS_BT_MBODY_1_FILE_COPIED_PD_FAILED_ABB";
		else
			content = "IDS_BT_MBODY_P1SD_FILES_COPIED_P2SD_FAILED_ABB";

		title = "IDS_BT_MBODY_FILES_SENT_VIA_BLUETOOTH_ABB";
		icon_path = BT_ICON_NOTIFICATION_SENT;
		indicator_icon_path = BT_ICON_NOTIFICATION_SENT_INDICATOR;
		flag = NOTIFICATION_PROP_DISABLE_AUTO_DELETE;
		app_id = NOTI_OPC_APP_ID;
		noti_type = NOTIFICATION_TYPE_NOTI;
	} else if (type == BT_RECEIVED_NOTI) {
		success = ad->recv_data.tr_success;
		fail = ad->recv_data.tr_fail;

		if (success == 1)
			content = "IDS_BT_MBODY_1_FILE_COPIED_PD_FAILED_ABB";
		else
			content = "IDS_BT_MBODY_P1SD_FILES_COPIED_P2SD_FAILED_ABB";

		title = "IDS_BT_MBODY_FILES_RECEIVED_VIA_BLUETOOTH_ABB";
		icon_path = BT_ICON_NOTIFICATION_RECEIVED;
		indicator_icon_path = BT_ICON_NOTIFICATION_RECEIVED_INDICATOR;
		flag = NOTIFICATION_PROP_DISABLE_AUTO_DELETE;
		app_id = NOTI_OPS_APP_ID;
		noti_type = NOTIFICATION_TYPE_NOTI;
	} else if (type == BT_SENDING_NOTI) {
		title = "IDS_BT_MBODY_SEND_VIA_BLUETOOTH_ABB";
		content = "IDS_BT_SBODY_PREPARING_TO_SEND_FILES_ING_ABB";
		icon_path = BT_ICON_NOTIFICATION_SENDING;
		indicator_icon_path = BT_ICON_NOTIFICATION_SENDING_INDICATOR;
		flag = NOTIFICATION_PROP_DISABLE_TICKERNOTI;
		noti_type = NOTIFICATION_TYPE_ONGOING;
	} else if (type == BT_RECEIVING_NOTI) {
		title = "IDS_BT_MBODY_RECEIVE_VIA_BLUETOOTH_ABB";
		content = "IDS_BT_SBODY_PREPARING_TO_RECEIVE_FILES_ING_ABB";
		icon_path = BT_ICON_NOTIFICATION_RECEIVING;
		indicator_icon_path = BT_ICON_NOTIFICATION_RECEIVING_INDICATOR;
		flag = NOTIFICATION_PROP_DISABLE_TICKERNOTI;
		noti_type = NOTIFICATION_TYPE_ONGOING;
	}

	noti = notification_create(noti_type);
	if (!noti) {
		ERR("Fail to notification_create");
		return NULL;
	}

	notification_set_text_domain(noti, BT_COMMON_PKG, BT_COMMON_RES);

	if (title) {
		ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
					NULL, title,
					NOTIFICATION_VARIABLE_TYPE_NONE);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text [%d]", ret);
		}
	}

	if (content) {
		if (type == BT_SENT_NOTI || type == BT_RECEIVED_NOTI) {
			if (success == 1)
				ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
							NULL, content,
							NOTIFICATION_VARIABLE_TYPE_INT, fail,
							NOTIFICATION_VARIABLE_TYPE_NONE);
			else
				ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
							NULL, content,
							NOTIFICATION_VARIABLE_TYPE_INT, success,
							NOTIFICATION_VARIABLE_TYPE_INT, fail,
							NOTIFICATION_VARIABLE_TYPE_NONE);
		} else {
			ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
						NULL, content, NOTIFICATION_VARIABLE_TYPE_NONE);
		}

		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text [%d]", ret);
		}
	}

	if (icon_path) {
		ret = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon_path);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_image [%d]", ret);
		}
	}

	if (indicator_icon_path) {
		ret = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, indicator_icon_path);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_image [%d]", ret);
		}
	}

	if (flag != -1) {
		ret = notification_set_property(noti, flag);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_property [%d]", ret);
		}
	}

	if (app_id) {
		ret = notification_set_pkgname(noti, app_id);
		 if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_pkgname [%d]", ret);
		}
	}

	ret = notification_set_display_applist(noti,
			NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY |
			NOTIFICATION_DISPLAY_APP_INDICATOR);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_set_display_applist [%d]", ret);
	}

	if (type == BT_SENT_NOTI) {
		_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
				NOTI_TR_TYPE_OUT, NULL, NULL, 0);
	} else if (type == BT_RECEIVED_NOTI) {
		_bt_set_notification_app_launch(noti, CREATE_TR_LIST,
				NOTI_TR_TYPE_IN, NULL, NULL, 0);
	}

	ret = notification_insert(noti, &noti_id);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_insert [%d]", ret);
	}

	if (type == BT_SENT_NOTI)
		ad->send_noti_id = noti_id;
	else if (type == BT_RECEIVED_NOTI)
		ad->receive_noti_id = noti_id;

	INFO("Insert %s type: %d ", (type == BT_SENT_NOTI || type == BT_RECEIVED_NOTI) ?
			"Notification" : "Ongoing", noti);

	return noti;
}

int _bt_update_notification(struct bt_appdata *ad, notification_h noti,
				char *title, char *content, char *icon_path)
{
	retvm_if (!noti, BT_SHARE_FAIL, "noti is NULL");

	INFO("Update noti : %d", noti);
	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	char str[NOTIFICATION_TEXT_LEN_MAX] = { 0 };
	int success = 0;
	int fail = 0;

	if (ad->send_noti == noti) {
		success = ad->send_data.tr_success;
		fail = ad->send_data.tr_fail;

		if (success == 1)
			content = "IDS_BT_MBODY_1_FILE_COPIED_PD_FAILED_ABB";
		else
			content = "IDS_BT_MBODY_P1SD_FILES_COPIED_P2SD_FAILED_ABB";
	} else if (ad->receive_noti == noti) {
		success = ad->recv_data.tr_success;
		fail = ad->recv_data.tr_fail;

		if (success == 1)
			content = "IDS_BT_MBODY_1_FILE_COPIED_PD_FAILED_ABB";
		else
			content = "IDS_BT_MBODY_P1SD_FILES_COPIED_P2SD_FAILED_ABB";
	}

	if (title) {
		ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
					NULL, title,
					NOTIFICATION_VARIABLE_TYPE_NONE);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text [%d]", ret);
		}
	}

	if (content) {
		if (noti == ad->send_noti|| noti == ad->receive_noti) {
			if (success == 1)
				ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
							NULL, content,
							NOTIFICATION_VARIABLE_TYPE_INT, fail,
							NOTIFICATION_VARIABLE_TYPE_NONE);
			else
				ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
							NULL, content,
							NOTIFICATION_VARIABLE_TYPE_INT, success,
							NOTIFICATION_VARIABLE_TYPE_INT, fail,
							NOTIFICATION_VARIABLE_TYPE_NONE);
		} else {
			ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
						NULL, content, NOTIFICATION_VARIABLE_TYPE_NONE);
		}

		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text [%d]", ret);
		}
	}

	if (icon_path) {
		ret = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon_path);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_image [%d]", ret);
		}
	}

	ret = notification_update(noti);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_update [%d]", ret);
	}

	return ret;
}

int _bt_update_notification_progress(notification_h not,
				int id, int val)
{
	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	ret = notification_update_progress(not, id, (double)val / 100);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_update_progress [%d]\n", ret);
	}
	return ret;
}

int _bt_get_notification_priv_id(notification_h noti)
{
	int group_id = 0;
	int priv_id = 0;
	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	ret = notification_get_id(noti, &group_id, &priv_id);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_get_id [%d]", ret);
	}
	return priv_id;
}

int _bt_delete_notification(notification_h noti)
{
	retv_if(!noti, BT_SHARE_FAIL);
	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	INFO("Delete noti : %d", noti);

	/* In case daemon, give full path */
	ret = notification_delete(noti);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_delete [%d]", ret);
	}

	return ret;
}

int _bt_set_notification_app_launch(notification_h noti,
					bt_notification_launch_type_e launch_type,
					const char *transfer_type,
					const char *filename,
					const char *progress_cnt,
					int transfer_id)
{
	DBG("+\n");
	if (!noti)
		return -1;

	if (!transfer_type)
		return -1;

	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	bundle *b = NULL;
	b = bundle_create();
	if (!b)
		return -1;

	if (launch_type == CREATE_PROGRESS) {
		double percentage = 0;
		char progress[BT_PERCENT_STR_LEN] = { 0 };
		char priv_id_str[BT_PRIV_ID_STR_LEN] = { 0 };

		if (!filename) {
			bundle_free(b);
			return -1;
		}

		ret = notification_get_progress(noti, &percentage);
		if (ret != NOTIFICATION_ERROR_NONE)
			ERR("Fail to notification_get_progress [%d]\n", ret);
		else
			snprintf(progress, BT_PERCENT_STR_LEN, "%d", (int)percentage);

		snprintf(priv_id_str, BT_PRIV_ID_STR_LEN, "%d", transfer_id);

		appsvc_set_pkgname(b, UI_PACKAGE);
		appsvc_add_data(b, "launch-type", "ongoing");
		appsvc_add_data(b, "percentage", progress);
		appsvc_add_data(b, "filename", filename);
		appsvc_add_data(b, "transfer_type", transfer_type);
		appsvc_add_data(b, "transfer_id", priv_id_str);
		if (g_strcmp0(transfer_type, NOTI_TR_TYPE_OUT) == 0)  {
			if (!progress_cnt) {
				bundle_free(b);
				return -1;
			}
			appsvc_add_data(b, "progress_cnt", progress_cnt);
		}
	} else if (launch_type == CREATE_TR_LIST) {
		appsvc_set_pkgname(b, UI_PACKAGE);
		appsvc_add_data(b, "launch-type", "transfer_list");
		appsvc_add_data(b, "transfer_type", transfer_type);
	} else {
		bundle_free(b);
		return -1;
	}

	ret = notification_set_execute_option(noti,
					NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,
					NULL, NULL, b);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_set_execute_option [%d]\n", ret);
	}

	bundle_free(b);
	DBG("-\n");
	return ret;
}

gboolean _bt_update_notification_status(struct bt_appdata *ad )
{
	notification_h noti = NULL;
	notification_list_h list_head = NULL;
	notification_list_h list_traverse = NULL;
	char *app_id = NULL;
	gboolean is_noti_existed = FALSE;
	gboolean send_noti = FALSE;
	gboolean receive_noti = FALSE;
	sqlite3 *db = NULL;
	int group_id = 0;
	int priv_id = 0;
	int ret;

	retv_if (ad == NULL, FALSE);

	/* When bt-share is launched, need to update notification status  */

	notification_get_list(NOTIFICATION_TYPE_NOTI, -1, &list_head);
	list_traverse = list_head;

	while (list_traverse != NULL) {
		noti = notification_list_get_data(list_traverse);
		notification_get_pkgname(noti, &app_id);

		if (g_strcmp0(app_id, NOTI_OPS_APP_ID) == 0) {
			ret = notification_clone(noti, &ad->receive_noti);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Fail to clone notificatoin");
			}
			ret = notification_get_id(noti, &group_id, &priv_id);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Fail to get notification id");
			}
			ad->receive_noti_id = priv_id;
			receive_noti = TRUE;
		} else if (g_strcmp0(app_id, NOTI_OPC_APP_ID) == 0) {
			ret = notification_clone(noti, &ad->send_noti);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Unable to clone notificatoin");
			}
			ret = notification_get_id(noti, &group_id, &priv_id);
			if (ret != NOTIFICATION_ERROR_NONE) {
				ERR("Fail to get notification id");
			}
			ad->send_noti_id = priv_id;
			send_noti = TRUE;
		}

		if (receive_noti || send_noti) {
			time_t ret_time;

			/* Set notication insert_time */
			notification_get_insert_time(noti, &ret_time);
			notification_set_time(noti, ret_time);
			notification_update(noti);
			is_noti_existed = TRUE;
		}

		list_traverse = notification_list_get_next(list_traverse);
	}

	if (list_head != NULL) {
		notification_free_list(list_head);
		list_head = NULL;
	}

	/* If notification item is removed durning BT off, need to remove DB */
	db = bt_share_open_db();
	if (!db)
		return is_noti_existed;

	if (!receive_noti)
		bt_share_remove_all_tr_data(db, BT_DB_INBOUND);
	if (!send_noti)
		bt_share_remove_all_tr_data(db, BT_DB_OUTBOUND);
	bt_share_close_db(db);

	return is_noti_existed;
}

static void __bt_notification_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	DBG("__bt_notification_changed_cb");

	retm_if (data == NULL, "Invalid data");
	struct bt_appdata *ad = (struct bt_appdata *)data;
	gboolean is_sent_noti_exist = FALSE;
	gboolean is_received_noti_exist = FALSE;
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int group_id;
	int priv_id;
	sqlite3 *db = NULL;

	retm_if (op_list == NULL, "Invalid op_list");

	if (type != NOTIFICATION_TYPE_NOTI ||
		(op_list->type != NOTIFICATION_OP_DELETE &&
		op_list->type != NOTIFICATION_OP_DELETE_ALL))
		return;

	if (ad->send_noti == NULL && ad->receive_noti == NULL)
		return;

	noti_err = notification_get_list(type, -1, &noti_list);
	ret_if (noti_err != NOTIFICATION_ERROR_NONE);

	noti_list = notification_list_get_head(noti_list);
	while (noti_list) {
		noti = notification_list_get_data(noti_list);
		noti_err  = notification_get_id(noti, &group_id, &priv_id);
		if (noti_err == NOTIFICATION_ERROR_NONE) {
			if (ad->send_noti_id == priv_id)
				is_sent_noti_exist = TRUE;
			else if (ad->receive_noti_id == priv_id)
				is_received_noti_exist = TRUE;
		}
		noti_list = notification_list_get_next(noti_list);
	}
	notification_free_list(noti_list);

	db = bt_share_open_db();
	retm_if(!db, "fail to open db!");

	if (is_sent_noti_exist == FALSE) {
		DBG("Not found sent notification. Delete outbound db");
		ad->send_noti = NULL;
		ad->send_noti_id = 0;
		if (bt_share_remove_tr_data_by_notification(db,
			BT_DB_OUTBOUND) == BT_SHARE_ERR_NONE) {
			ad->send_data.tr_fail = 0;
			ad->send_data.tr_success = 0;
		}
	}
	if (is_received_noti_exist == FALSE) {
		DBG("Not found recv notification. Delete inbound db");
		ad->receive_noti = NULL;
		ad->receive_noti_id = 0;
		if (bt_share_remove_tr_data_by_notification(db,
			BT_DB_INBOUND) == BT_SHARE_ERR_NONE) {
			ad->recv_data.tr_fail = 0;
			ad->recv_data.tr_success = 0;
		}
	}

	bt_share_close_db(db);

	DBG("SEND: Success[%d] Fail[%d] ||||| RECEIVE: Success[%d] Fail[%d]",
			ad->send_data.tr_success, ad->send_data.tr_fail,
			ad->recv_data.tr_success, ad->recv_data.tr_fail);

}

void _bt_register_notification_cb(struct bt_appdata *ad)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_register_detailed_changed_cb(__bt_notification_changed_cb, ad);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		ERR("notification_resister_changed_cb failed [%d]\n", noti_err);
	}
}

void _bt_unregister_notification_cb(struct bt_appdata *ad)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_unregister_detailed_changed_cb(__bt_notification_changed_cb, NULL);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		ERR("notification_unresister_changed_cb failed [%d]\n", noti_err);
	}
}
