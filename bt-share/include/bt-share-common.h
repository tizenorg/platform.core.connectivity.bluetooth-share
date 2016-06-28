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

#ifndef __DEF_BLUETOOTH_SHARE_COMMON_H_
#define __DEF_BLUETOOTH_SHARE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

/* Define Error type */
#define BT_SHARE_FAIL -1
#define BT_SHARE_ERROR_NONE 0

#define BT_DEFAULT_MEM_PHONE 0
#define BT_DEFAULT_MEM_MMC 1

#define STORAGE_PATH_LEN_MAX 255

#define BT_DOWNLOAD_DEFAULT_PHONE_FOLDER "/home/owner/content/Downloads"
#define BT_DOWNLOAD_DEFAULT_MMC_FOLDER "/opt/media/SDCardA1"
#define BT_DOWNLOAD_DEFAULT_MEDIA_FOLDER "/home/owner/content/Downloads"
//#define BT_DOWNLOAD_DEFAULT_MEDIA_FOLDER tzplatform_getenv(TZ_USER_CONTENT)

/*
#define BT_FTP_FOLDER tzplatform_mkpath(TZ_SYS_SHARE, "bt-ftp")
*/

#define BT_DOWNLOAD_PHONE_ROOT "/home/owner/content/"
#define BT_DOWNLOAD_MMC_ROOT "/opt/media/SDCardA1"
#define BT_FTP_FOLDER "/opt/share/bt-ftp"
#define BT_FTP_FOLDER_PHONE "/opt/share/bt-ftp/Media/"
#define BT_FTP_FOLDER_MMC "/opt/share/bt-ftp/SD_External/"

//#define BT_TMP_DIR "/opt/home/owner/content/Downloads/.bluetooth/"
#define BT_TMP_DIR "/home/owner/content/Downloads/"
#define BT_TMP_FILE BT_TMP_DIR"bluetooth_content_share"
#define TXT_FILE_NAME BT_TMP_DIR"bluetooth_content_share.txt"
#define HTML_FILE_NAME BT_TMP_DIR"bluetooth_content_share.html"
#define TXT_FILE_FORMAT BT_TMP_DIR"bluetooth_content_share%s.txt"
#define HTML_FILE_FORMAT BT_TMP_DIR"bluetooth_content_share%s.html"
#define HTML_FORMAT "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\"/></head><body><a href=\"%s\">%s</a></body></html>"

//#define BT_CONTACT_SHARE_TMP_DIR "/opt/home/owner/content/Downloads/.bluetooth/"
#define BT_CONTACT_SHARE_TMP_DIR "/home/owner/content/Downloads/"

typedef enum {
	BT_HTTP_FILE,
	BT_TXT_FILE,
} bt_file_t;

typedef enum {
	BT_STATUS_OFF         = 0x0000,
	BT_STATUS_ON          = 0x0001,
	BT_STATUS_BT_VISIBLE  = 0x0002,
	BT_STATUS_TRANSFER    = 0x0004,
} bt_status_t;


int _bt_share_block_sleep(gboolean is_block);
int _bt_set_transfer_indicator(gboolean state);
char *_bt_share_create_transfer_file(char *text);
void _bt_remove_tmp_file(char *file_path);
void _bt_remove_vcf_file(char *file_path);
char *_bt_share_get_storage_path(int storage_type);

#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_COMMON_H_ */
