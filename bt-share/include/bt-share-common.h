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

#ifndef __DEF_BLUETOOTH_SHARE_COMMON_H_
#define __DEF_BLUETOOTH_SHARE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	BT_STATUS_OFF         = 0x0000,
	BT_STATUS_ON          = 0x0001,
	BT_STATUS_BT_VISIBLE  = 0x0002,
	BT_STATUS_TRANSFER    = 0x0004,
} bt_status_t;


int _bt_share_block_sleep(gboolean is_block);
int _bt_set_transfer_indicator(gboolean state);


#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_COMMON_H_ */
