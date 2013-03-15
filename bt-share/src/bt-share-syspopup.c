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
#include <stdlib.h>
#include <string.h>
#include <syspopup_caller.h>
#include "applog.h"
#include "bluetooth-api.h"
#include "bt-share-syspopup.h"
#include "bt-share-main.h"
#include "bt-share-resource.h"
#include "obex-event-handler.h"




extern struct bt_appdata *app_state;
#define BT_POPUP_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS 200
#define BT_SYSPOPUP_EVENT_LEN_MAX 50




static gboolean __bt_system_popup_timer_cb(gpointer user_data)
{
	int ret = 0;
	bundle *b = (bundle *) user_data;

	if (NULL == b) {
		ERR("There is some problem with the user data..popup can not be created\n");
		return FALSE;
	}
	ret = syspopup_launch("bt-syspopup", b);

	if (0 > ret) {
		ERR("launching sys-popup failed\n");
		return TRUE;
	} else {
		DBG("Hurray Popup launched \n");
		bundle_free(b);
		return FALSE;
	}
}

int _bt_launch_system_popup(bt_app_event_type_t event_type,
			    bt_app_sys_popup_params_t *popup_params,
			    void *cb,
			    void *data)
{
	int ret = 0;
	bundle *b = NULL;
	char event_str[BT_SYSPOPUP_EVENT_LEN_MAX] = { 0 };
	struct bt_appdata *ad = app_state;

	DBG("+\n");
	if(cb == NULL)
		return -1;

	b = bundle_create();
	if(b == NULL)
		return -1;

	bundle_add(b, "title", popup_params->title);
	bundle_add(b, "type", popup_params->type);
	bundle_add(b, "file", popup_params->file);
	bundle_add(b, "device_name", popup_params->device_name);

	switch (event_type) {
	case BT_APP_EVENT_CONFIRM_MODE_REQUEST:
		strncpy(event_str, "app-confirm-request", sizeof(event_str));
		break;
	case BT_APP_EVENT_FILE_RECEIVED:
		strncpy(event_str, "file-received", sizeof(event_str));
		break;
	case BT_APP_EVENT_INFORMATION:
		strncpy(event_str, "bt-information", sizeof(event_str));
		break;
	case BT_APP_EVENT_OVERWRITE_REQUEST:
		strncpy(event_str, "confirm-overwrite-request", sizeof(event_str));
		break;
	default:
		break;
	}

	bundle_add(b, "event-type", event_str);

	/*The system popup launch function is not able to launch second popup
	  * if first popup is being processed still, this due to the check
	  * in AUL module to restrict multiple launching of syspopup,
	  * to solve this problem after discussion it is decided that  if
	  * the popup launch fails then it will be retried
	  * after small timeout. */
	ret = syspopup_launch("bt-syspopup", b);
	if (0 > ret) {
		ERR("Popup launch failed...retry = %d\n", ret);
		g_timeout_add(BT_POPUP_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS,
			      (GSourceFunc) __bt_system_popup_timer_cb, b);
	} else {
		bundle_free(b);
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


