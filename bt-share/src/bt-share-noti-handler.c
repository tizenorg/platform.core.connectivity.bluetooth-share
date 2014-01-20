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

/* For multi-user support */
#include <tzplatform_config.h>

#include "applog.h"
#include "bluetooth-api.h"
#include "bt-share-noti-handler.h"


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

		bluetooth_obex_server_set_root(root_path);
		bluetooth_obex_server_set_destination_path(download_path);
	}
}

void _bt_init_vconf_notification(void)
{
	int ret;
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT,
			__bt_default_memory_changed_cb, NULL);
	if (ret < 0) {
		DBG("vconf_notify_key_changed failed\n");
	}
}

void _bt_deinit_vconf_notification(void)
{
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_DEFAULT_MEM_BLUETOOTH_INT, 
			(vconf_callback_fn) __bt_default_memory_changed_cb);
	return;
}

