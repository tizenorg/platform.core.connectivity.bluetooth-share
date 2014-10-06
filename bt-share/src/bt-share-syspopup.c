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
#include <stdlib.h>
#include <string.h>
#include "applog.h"
#include "bluetooth-api.h"
#include "bt-share-syspopup.h"
#include "bt-share-main.h"
#include "bt-share-resource.h"
#include "obex-event-handler.h"

extern struct bt_appdata *app_state;
#define BT_SYSPOPUP_EVENT_LEN_MAX 50


static gboolean __bt_system_popup_timer_cb(gpointer user_data)
{
	notification_error_e err = NOTIFICATION_ERROR_NONE;
	notification_h noti = NULL;

	if (NULL == (void *)user_data) {
		ERR("There is some problem with the user data..popup can not be created\n");
		return FALSE;
	}

	INFO("bt system popup timer cb notification");

        noti = notification_create(NOTIFICATION_TYPE_NOTI);
    	if (noti == NULL) {
        	ERR("Failed to create notification \n");
        	return FALSE;
    	}

        err = notification_set_pkgname(noti, BT_SHARE_APP_NAME);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to set pkgname \n");
        	return FALSE;
    	}

    	err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, "bt-syspopup", NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to set notification title \n");
        	return;
    	}

    	err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, "bt system popup timer cb notification", NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to set notification content \n");
        	return;
    	}

        err = notification_insert(noti, NULL);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to insert notification \n");
        	return;
    	}

	return FALSE;
}

int _bt_launch_system_popup(bt_app_event_type_t event_type,
			    bt_app_sys_popup_params_t *popup_params,
			    void *cb,
			    void *data)
{
	notification_error_e err = NOTIFICATION_ERROR_NONE;
	char *body = NULL;
	notification_h noti = NULL;

	char event_str[BT_SYSPOPUP_EVENT_LEN_MAX] = { 0 };
	struct bt_appdata *ad = app_state;

	DBG("+\n");
	if(cb == NULL)
		return -1;

	ad->syspopup_call = 0;

	INFO("bt_launch_system_popup");

        noti = notification_create(NOTIFICATION_TYPE_NOTI);
    	if (noti == NULL) {
        	ERR("Failed to create notification \n");
        	return FALSE;
    	}

        err = notification_set_pkgname(noti, BT_SHARE_APP_NAME);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to set pkgname \n");
        	return FALSE;
    	}

    	err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, "%s", popup_params->title, NOTIFICATION_VARIABLE_TYPE_NONE);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to set notification title \n");
        	return;
    	}

	switch (event_type) {
	case BT_APP_EVENT_CONFIRM_MODE_REQUEST:
		strncpy(event_str, "Confirm request", sizeof(event_str));
    		err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, event_str, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    		if (err != NOTIFICATION_ERROR_NONE) {
        		ERR("Unable to set notification event str \n");
        		return;
    		}
		break;
	case BT_APP_EVENT_FILE_RECEIVED:
		strncpy(event_str, "File received :", sizeof(event_str));
		body = g_strdup_printf("%s %s", event_str, popup_params->file);
    		err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, body, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    		if (err != NOTIFICATION_ERROR_NONE) {
        		ERR("Unable to set notification event str \n");
        		return;
    		}
		g_free(body);		
		break;
	case BT_APP_EVENT_INFORMATION:
		strncpy(event_str, "BT information", sizeof(event_str));
    		err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, event_str, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    		if (err != NOTIFICATION_ERROR_NONE) {
        		ERR("Unable to set notification event str \n");
        		return;
    		}
		break;
	case BT_APP_EVENT_OVERWRITE_REQUEST:
		strncpy(event_str, "Confirm overwrite request", sizeof(event_str));
		body = g_strdup_printf("%s (file : %s)", event_str, popup_params->file);
    		err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, body, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    		if (err != NOTIFICATION_ERROR_NONE) {
        		ERR("Unable to set notification event str \n");
        		return;
    		}
		g_free(body);
		break;
	default:
		break;
	}

        err = notification_insert(noti, NULL);
    	if (err != NOTIFICATION_ERROR_NONE) {
        	ERR("Unable to insert notification \n");
        	return;
    	}

	ad->popups.popup_cb = (bt_app_cb) cb;
	ad->popups.popup_cb_data = data;
	ad->popups.syspopup_request = TRUE;

	DBG("-\n");
	return 0;
}


gboolean _bt_app_popup_memoryfull(gpointer user_data)
{
	bt_app_sys_popup_params_t popup_params = { NULL };

	popup_params.title = BT_STR_MEMORY_FULL;
	popup_params.type = "none";
	_bt_launch_system_popup(BT_APP_EVENT_INFORMATION,
				&popup_params,
				NULL,
				NULL);
	return FALSE;
}


