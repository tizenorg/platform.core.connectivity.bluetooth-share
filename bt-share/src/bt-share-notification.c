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
#include "obex-event-handler.h"
#include "bluetooth-share-api.h"

#define BT_PERCENT_STR_LEN 5
#define BT_PRIV_ID_STR_LEN 8

notification_h _bt_create_notification(bt_qp_type_t type)
{
	DBG("+\n");
	DBG("+Create type : %d\n", type);
	notification_h noti = NULL;
	noti = notification_create(type);
	if (!noti) {
		ERR("Fail to notification_new\n");
		return NULL;
	}
	DBG("noti : %d \n", noti);
	DBG("-\n");
	return noti;
}

int _bt_insert_notification(notification_h noti,
				char *title,
				char *content,
				char *icon_path)
{
	DBG("+\n");
	int noti_id = 0;

	if (!noti)
		return BT_SHARE_FAIL;

	DBG("Insert noti : %d \n", noti);
	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	ret = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon_path);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_set_image\n");
	}

	if (title) {
		ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
								title, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text\n");
		}
	}

	if (content) {
		ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
								content, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text\n");
		}
	}

	ret = notification_insert(noti, &noti_id);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_insert\n");
	}

	DBG("-\n");
	return noti_id;
}

int _bt_update_notification(notification_h noti,
				char *title,
				char *content,
				char *icon_path)
{
	DBG("+\n");
	if (!noti)
		return BT_SHARE_FAIL;

	DBG("Insert noti : %d \n", noti);
	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	ret = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon_path);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_set_image\n");
	}

	if (title) {
		ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
								title, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text\n");
		}
	}

	if (content) {
		ret = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
								content, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
		if (ret != NOTIFICATION_ERROR_NONE) {
			ERR("Fail to notification_set_text\n");
		}
	}

	ret = notification_update(noti);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_update\n");
	}

	DBG("-\n");
	return ret;
}

int _bt_update_notification_progress(void *handle,
				int id,
				int val)
{
	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	ret = notification_update_progress(handle, id, (double)val / 100);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_update_progress\n");
	}
	return ret;
}


gboolean _bt_get_notification_text(int priv_id, char *str)
{
	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	notification_list_h list = NULL;
	notification_h noti = NULL;

	/* Get notification information from notification detail list */
	ret = notification_get_detail_list(BT_SHARE_BIN_PATH,
							     -1,
							     priv_id,
							     -1,
							     &list);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_get_text\n");
		return FALSE;
	}

	if (list) {
		noti = notification_list_get_data(list);
		char *text = NULL;
		notification_get_text(noti,
				      NOTIFICATION_TEXT_TYPE_TITLE,
				      &text);
		g_strlcpy(str, text, NOTIFICATION_TEXT_LEN_MAX);
	} else {
		return FALSE;
	}
	return TRUE;
}

int _bt_delete_notification(notification_h noti)
{
	DBG("+\n");

	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	if (!noti)
		return BT_SHARE_FAIL;

	/* In case daemon, give full path */
	ret = notification_delete(noti);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_delete_by_priv_id\n");
	}
	DBG("-\n");
	return ret;
}

int _bt_set_notification_app_launch(notification_h noti,
					bt_qp_launch_type_t type,
					const char *transfer_type,
					const char *filename,
					const char *progress_cnt)
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

	if (type == CREATE_PROGRESS) {
		int group_id = 0;
		int priv_id = 0;
		double percentage = 0;
		char progress[BT_PERCENT_STR_LEN] = { 0 };
		char priv_id_str[BT_PRIV_ID_STR_LEN] = { 0 };

		if (!filename) {
			bundle_free(b);
			return -1;
		}

		ret = notification_get_progress(noti, &percentage);
		if (ret != NOTIFICATION_ERROR_NONE)
			ERR("Fail to notification_update_progress\n");
		else
			snprintf(progress, BT_PERCENT_STR_LEN, "%d", (int)percentage);

		ret = notification_get_id(noti, &group_id, &priv_id);
		if (ret != NOTIFICATION_ERROR_NONE)
			ERR("Fail to notification_update_progress\n");
		else
			snprintf(priv_id_str, BT_PRIV_ID_STR_LEN, "%d", priv_id);

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
	} else if (type == CREATE_TR_LIST) {
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
		ERR("Fail to notification_set_execute_option\n");
	}

	bundle_free(b);
	DBG("-\n");
	return ret;
}


int _bt_set_notification_property(notification_h noti, int flag)
{
	DBG("+\n");
	if (!noti)
		return -1;

	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	ret = notification_set_property(noti, flag);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_set_property\n");
	}

	ret = notification_set_display_applist(noti,
				 NOTIFICATION_DISPLAY_APP_ALL ^
				 NOTIFICATION_DISPLAY_APP_TICKER);
	if (ret != NOTIFICATION_ERROR_NONE) {
		ERR("Fail to notification_set_display_applist\n");
	}

	DBG("-\n");
	return ret;
}


static void __bt_noti_changed_cb(void *data, notification_type_e type)
{
	DBG("+\n");

	struct bt_appdata *ad = (struct bt_appdata *)data;
	if (NULL == ad)
		return;

	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int count = 0;
	sqlite3 *db = NULL;

	noti_err = notification_get_count(NOTIFICATION_TYPE_NOTI, BT_SHARE_BIN_PATH,
			NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE, &count);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		return;
	}

	/* This callback used only for clear button of notification bar. */
	/* So need to check the clear button of in/outbound list view  */
	if (count >= 1)
		return;

	ad->send_data.tr_fail = 0;
	ad->send_data.tr_success = 0;
	ad->recv_data.tr_fail = 0;
	ad->recv_data.tr_success = 0;

	db = bt_share_open_db();
	if (!db)
		return;

	bt_share_remove_tr_data_by_notification(db, BT_DB_INBOUND);
	bt_share_remove_tr_data_by_notification(db, BT_DB_OUTBOUND);
	DBG("DB record of notification was removed\n");

	ad->send_noti = NULL;
	ad->receive_noti = NULL;

	bt_share_close_db(db);

	DBG("-\n");
}

void _bt_register_notification_cb(struct bt_appdata *ad)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_resister_changed_cb(__bt_noti_changed_cb, ad);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		ERR("notification_resister_changed_cb failed %d \n", noti_err);
	}
}

void _bt_unregister_notification_cb(struct bt_appdata *ad)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_unresister_changed_cb(__bt_noti_changed_cb);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		ERR("notification_unresister_changed_cb failed %d \n", noti_err);
	}
}
