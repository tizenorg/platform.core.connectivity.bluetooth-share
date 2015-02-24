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

#ifndef __DEF_BLUETOOTH_SHARE_NOTI_H_
#define __DEF_BLUETOOTH_SHARE_NOTI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <notification.h>
#include <tzplatform_config.h>
#include "bt-share-main.h"

#define BT_SHARE_BIN_PATH tzplatform_mkpath(TZ_SYS_BIN, "bluetooth-share")
#define NOTIFICATION_TEXT_LEN_MAX 100
#define NOTI_TR_TYPE_IN "inbound"
#define NOTI_TR_TYPE_OUT "outbound"
#define NOTI_OPS_APP_ID	"bluetooth-share-opp-server"
#define NOTI_OPC_APP_ID	"bluetooth-share-opp-client"
#define QP_NO_APP_LAUNCH       NOTIFICATION_PROP_DISABLE_APP_LAUNCH

/* Priv_id should be unique. */
enum {
	SEND_PRIV_ID,			/* ongoing type for opp client */
	RECV_NOTI_PRIV_ID = 99999,	/* notification type for obex server */
	SEND_NOTI_PRIV_ID	/* notification type for obex client */
};

typedef enum {
	BT_SENT_NOTI,
	BT_SENDING_NOTI,
	BT_RECEIVED_NOTI,
	BT_RECEIVING_NOTI,
} bt_notification_type_e;

typedef enum {
	CREATE_PROGRESS,
	CREATE_TR_LIST
} bt_notification_launch_type_e;


notification_h _bt_insert_notification(struct bt_appdata *ad, bt_notification_type_e type);

int _bt_update_notification(struct bt_appdata *ad, notification_h noti,
				char *title,
				char *content,
				char *icon_path);

int _bt_update_notification_progress(notification_h noti,
				int id,
				int val);

int _bt_get_notification_priv_id(notification_h noti);

int _bt_delete_notification(notification_h noti);

int _bt_set_notification_app_launch(notification_h noti,
					bt_notification_launch_type_e launch_type,
					const char *transfer_type,
					const char *filename,
					const char *progress_cnt,
					int transfer_id);

gboolean _bt_update_notification_status(struct bt_appdata *ad );

void _bt_register_notification_cb(struct bt_appdata *ad);

void _bt_unregister_notification_cb(struct bt_appdata *ad);


#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_NOTI_H_ */
