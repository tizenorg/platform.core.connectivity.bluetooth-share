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
#include "applog.h"
#include "bluetooth-api.h"
#include "bt-share-syspopup.h"
#include "bt-share-main.h"
#include "bt-share-resource.h"
#include "obex-event-handler.h"

extern struct bt_appdata *app_state;
#define BT_POPUP_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS 200
#define BT_SYSPOPUP_EVENT_LEN_MAX 50
#define BT_SYSPOPUP_MAX_CALL 10

static gboolean __bt_system_popup_timer_cb(gpointer user_data)
{
	if (NULL == (void *)user_data) {
		err("there is some problem with the user data..popup can not be created\n");
		return FALSE;
	}

	INFO("bt system popup timer cb notification");	

	return FALSE;
}

int _bt_launch_system_popup(bt_app_event_type_t event_type,
			    bt_app_sys_popup_params_t *popup_params,
			    void *cb,
			    void *data)
{
	char event_str[BT_SYSPOPUP_EVENT_LEN_MAX] = { 0 };
        struct bt_appdata *ad = app_state;

	switch (event_type) {
	case BT_APP_EVENT_CONFIRM_MODE_REQUEST:
		strncpy(event_str, "app-confirm-request", sizeof(event_str));
		break;
	}

	INFO("bt_launch_system_popup");

	if (cb != NULL)
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


