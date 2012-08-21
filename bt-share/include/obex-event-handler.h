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

#ifndef __BT_EVENT_HANDLER_H__
#define __BT_EVENT_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth-api.h"

#define BT_FILE_PATH_LEN_MAX	(4096 + 10)
#define BT_TEMP_FILE_PATH_LEN_MAX 262
#define BT_TEXT_LEN_MAX 255

typedef enum {
	BT_FILE_IMAGE,	/**<IMAGE */
	BT_FILE_VCARD,	/**<VCARD */
	BT_FILE_VCAL,	/**<VCAL */
	BT_FILE_VBOOKMARK,	/**<VBOOKMARK */
	BT_FILE_VMEMO,
	BT_FILE_DOC,	/**<DOC, */
	BT_FILE_OTHER	/**<OTHER*/
} bt_file_type_t;

void _bt_share_event_handler(int event, bluetooth_event_param_t *param,
			       void *user_data);

void _bt_obex_server_event_handler(int event,
				       bluetooth_event_param_t *param,
				       void *user_data);

void _bt_app_obex_download_dup_file_cb(void *data, void *obj,
				       void *event_info);

gboolean _bt_get_default_storage(char *storage);


#ifdef __cplusplus
}
#endif
#endif				/* __BT_EVENT_HANDLER_H__ */
