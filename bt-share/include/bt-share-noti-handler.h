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

#ifndef __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_
#define __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_PATH_LEN_MAX 30
#define BT_DEFAULT_MEM_PHONE 0
#define BT_DEFAULT_MEM_MMC 1

#define BT_DOWNLOAD_PHONE_FOLDER tzplatform_mkpath(TZ_USER_CONTENT, "Downloads")
#define BT_DOWNLOAD_MMC_FOLDER tzplatform_mkpath(TZ_SYS_STORAGE, "sdcard")
#define BT_DOWNLOAD_MEDIA_FOLDER tzplatform_getenv(TZ_USER_CONTENT)
#define BT_FTP_FOLDER tzplatform_mkpath(TZ_SYS_SHARE, "bt-ftp")

void _bt_init_vconf_notification(void);
void _bt_deinit_vconf_notification(void);
#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_ */
