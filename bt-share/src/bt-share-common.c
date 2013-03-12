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

#include <pmapi.h>
#include <glib.h>
#include "vconf-keys.h"
#include "applog.h"
#include "bt-share-common.h"
#include <vconf.h>

int _bt_share_block_sleep(gboolean is_block)
{
	static int block_sleep_count = 0;
	int result = -1;

	DBG("is_block [%d], block_sleep_count[%d]\n", is_block,
		     block_sleep_count);

	if (is_block) {
		if (block_sleep_count < 0) {
			DBG("block_sleep_count[%d] is invalid. It is set to 0.\n",
				     block_sleep_count);
			block_sleep_count = 0;
		} else if (block_sleep_count == 0) {
			result = pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
			if (result != 0) {
				DBG("LCD Lock is failed with result code [%d]\n", result);
			}
		} else {
			result = 0;
		}

		if (result == 0) {
			block_sleep_count++;
		}
	} else {
		if (block_sleep_count <= 0) {
			DBG("block_sleep_count[%d] is invalid. It is set to 0.\n",
				     block_sleep_count);
			block_sleep_count = 0;
		} else if (block_sleep_count == 1) {
			result = pm_unlock_state(LCD_OFF, PM_RESET_TIMER);
			if (result != 0) {
				DBG("LCD Unlock is failed with result code [%d]\n",
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
		DBG("Get vconf failed\n");
		return -1;
	}

	if(state == TRUE) {
		block_cnt++;
		if(bt_device_state & BT_STATUS_TRANSFER)
			return 0;
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
		DBG("Set vconf failed\n");
		return -1;
	}
	return 0;
}

