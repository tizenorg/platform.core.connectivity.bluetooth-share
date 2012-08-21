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

#ifndef __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_
#define __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_PATH_LEN_MAX 30


#define BT_DOWNLOAD_PHONE_FOLDER "/opt/media/Downloads"
#define BT_DOWNLOAD_MMC_FOLDER "/opt/storage/sdcard"
#define BT_DOWNLOAD_MEDIA_FOLDER "/opt/media"
#define BT_FTP_FOLDER "/opt/share/bt-ftp"

void _bt_init_vconf_notification(void);
#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_ */
