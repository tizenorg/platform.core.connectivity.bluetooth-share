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

#include <device/power.h>
#include <glib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vconf.h>
#include <Ecore_File.h>

#include "vconf-keys.h"
#include "applog.h"
#include "bt-share-common.h"

int _bt_share_block_sleep(gboolean is_block)
{
	static int block_sleep_count = 0;
	int result = -1;

	DBG("is_block [%d], block_sleep_count[%d]\n", is_block,
		     block_sleep_count);

	if (is_block) {
		if (block_sleep_count < 0) {
			INFO("block_sleep_count[%d] is invalid. It is set to 0.\n",
				     block_sleep_count);
			block_sleep_count = 0;
		} else if (block_sleep_count == 0) {
			result = device_power_request_lock(POWER_LOCK_CPU, 0);
			if (result != 0) {
				INFO("LCD Lock is failed with result code [%d]\n", result);
			}
		} else {
			result = 0;
		}

		if (result == 0) {
			block_sleep_count++;
		}
	} else {
		if (block_sleep_count <= 0) {
			INFO("block_sleep_count[%d] is invalid. It is set to 0.\n",
				     block_sleep_count);
			block_sleep_count = 0;
		} else if (block_sleep_count == 1) {
			result = device_power_release_lock(POWER_LOCK_CPU);
			if (result != 0) {
				INFO("LCD Unlock is failed with result code [%d]\n",
					     result);
			}
		} else {
			result = 0;
		}

		if (result == 0) {
			block_sleep_count--;
		}
	}

	DBG("result [%d]\n", result);
	return result;
}



int _bt_set_transfer_indicator(gboolean state)
{
	int bt_device_state;
	static int block_cnt = 0;
	int ret;

	ret = vconf_get_int(VCONFKEY_BT_STATUS, (void *)&bt_device_state);
	if (ret != 0) {
		ERR("Get vconf failed\n");
		return -1;
	}

	if(state == TRUE) {
		block_cnt++;
		retv_if(bt_device_state & BT_STATUS_OFF, 0);
		retv_if(bt_device_state & BT_STATUS_TRANSFER, 0);
		bt_device_state |= BT_STATUS_TRANSFER;
	} else {
		if(block_cnt > 0)
			block_cnt--;
		if(block_cnt != 0)
			return 0;
		bt_device_state ^= BT_STATUS_TRANSFER;
	}

	ret = vconf_set_int(VCONFKEY_BT_STATUS, bt_device_state);
	if (ret != 0) {
		ERR("Set vconf failed\n");
		return -1;
	}
	return 0;
}

static char *__bt_share_get_transfer_file_name(int file_type)
{
	int count = 0;
	char *appendix;
	char *file;
	char *file_format;
	char *file_name;

	if (file_type == BT_HTTP_FILE) {
		file_name = HTML_FILE_NAME;
		file_format = HTML_FILE_FORMAT;
	} else {
		file_name = TXT_FILE_NAME;
		file_format = TXT_FILE_FORMAT;
	}

	file = g_strdup(file_name);

	/* While the file exists, increase the file name */
	while (access(file, F_OK) == 0) {
		g_free(file);

		appendix = g_strdup_printf("_%d", count);
		file = g_strdup_printf(file_format, appendix);
		g_free(appendix);

		count++;
	}

	return file;
}

void _bt_remove_tmp_file(char *file_path)
{
	if (g_str_has_prefix(file_path, BT_TMP_FILE) == TRUE) {
		DBG_SECURE("Remove the file: %s", file_path);
		ecore_file_remove(file_path);
	}
}

void _bt_remove_vcf_file(char *file_path)
{
	if (g_str_has_prefix(file_path, BT_CONTACT_SHARE_TMP_DIR) == TRUE) {
		DBG_SECURE("Remove the file: %s", file_path);
		ecore_file_remove(file_path);
	}
}

char *_bt_share_create_transfer_file(char *text)
{
	int fd;
	int file_type;
	char *file = NULL;
	char *content;
	char *url_sheme;
	ssize_t write_size;

	retv_if(text == NULL, NULL);

	url_sheme = g_uri_parse_scheme(text);

	if (url_sheme) {
		/* HTTP file generate */
		g_free(url_sheme);
		file_type = BT_HTTP_FILE;
	} else {
		/* TXT file generate */
		file_type = BT_TXT_FILE;
	}

	file = __bt_share_get_transfer_file_name(file_type);
	retv_if(file == NULL, NULL);

	fd = open(file, O_RDWR | O_CREAT, 0755);

	if (fd < 0) {
		ERR("Fail to open the file : %s", file);
		goto fail;
	}

	if (file_type == BT_HTTP_FILE) {
		content = g_strdup_printf(HTML_FORMAT, text, text);
	} else {
		content = g_strdup(text);
	}

	DBG_SECURE("content: \n%s", content);

	write_size = write(fd, content, strlen(content));
	g_free(content);
	close(fd);

	if (write_size < 0) {
		ERR("Fail to write in file");
		goto fail;
	}

	return file;
fail:
	g_free(file);
	return NULL;
}

