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
#include <vconf.h>
#include <unistd.h>

/* For multi-user support */
#include <tzplatform_config.h>

#include "applog.h"
#include "bluetooth-api.h"
#include "bt-share-noti-handler.h"
#include "bt-share-main.h"


static void __bt_default_memory_changed_cb(keynode_t *node, void *data)
{
	int default_memory = 0;
	char *root_path = NULL;
	char *download_path = NULL;

	DBG("__bt_default_memory_changed_cb\n");

	ret_if (node == NULL);

	DBG("key=%s\n", vconf_keynode_get_name(node));

	if (vconf_keynode_get_type(node) == VCONF_TYPE_INT) {
		/* Phone memory is 0, MMC is 1 */
		default_memory = vconf_keynode_get_int(node);
		root_path = default_memory ? BT_DOWNLOAD_MMC_FOLDER : BT_DOWNLOAD_MEDIA_FOLDER;
		download_path = default_memory ? BT_DOWNLOAD_MMC_FOLDER : BT_DOWNLOAD_PHONE_FOLDER;

		if (access(download_path, W_OK) != 0) {
			if (mkdir(download_path, 0755) < 0) {
				DBG("mkdir fail![%s]", download_path);
			}
		}

		bluetooth_obex_server_set_root(root_path);
		bluetooth_obex_server_set_destination_path(download_path);
	}
}

static void __bt_mmc_status_changed_cb(keynode_t *node, void *data)
{
	DBG("+");

	int mmc_status = 0;
	int default_memory = 0;
	int ret = 0;
	retm_if(!node || !data, "invalid param!");
	DBG("key=%s", vconf_keynode_get_name(node));
	struct bt_appdata *ad = (struct bt_appdata *)data;

	if (vconf_keynode_get_type(node) == VCONF_TYPE_INT) {
		/* Phone memory is 0, MMC is 1 */
		mmc_status = vconf_keynode_get_int(node);

		if (mmc_status == VCONFKEY_SYSMAN_MMC_REMOVED ||
				mmc_status == VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED) {
			retm_if(vconf_get_int(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
						&default_memory) != 0,
					"vconf_get_int failed");

			if (ad->opc_noti) {	/* sending case */
				DBG("cancel outbound transfer");
				bluetooth_opc_cancel_push();
				ret = _bt_delete_notification(ad->opc_noti);
				if (ret == NOTIFICATION_ERROR_NONE) {
					ad->opc_noti = NULL;
					ad->opc_noti_id = 0;
				}
			} else {		/* receiving case */
				DBG("cancel inbound transfer");
				if (default_memory == BT_DEFAULT_MEM_MMC) {
					_bt_obex_cancel_transfer(data);
					retm_if(vconf_set_int(
								VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
								BT_DEFAULT_MEM_PHONE) != 0, "vconf_set_int failed");
					DBG("Default Memory set to Phone");
				}
			}
		}
	}
	DBG("-");
}

void _bt_init_vconf_notification(void *data)
{
	int ret;

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
			__bt_default_memory_changed_cb, NULL);
	if (ret < 0) {
		DBG("vconf_notify_key_changed init failed");
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
			__bt_mmc_status_changed_cb, data);
	if (ret < 0) {
		DBG("vconf_notify_key_changed init failed");
	}
}

void _bt_deinit_vconf_notification(void)
{
	int ret;
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
			(vconf_callback_fn) __bt_default_memory_changed_cb);
	if (ret < 0) {
		DBG("vconf_notify_key_changed deinit failed");
	}

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
			(vconf_callback_fn) __bt_mmc_status_changed_cb);
	if (ret < 0) {
		DBG("vconf_notify_key_changed deinit failed");
	}
}

