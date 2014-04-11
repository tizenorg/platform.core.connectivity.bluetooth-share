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

#ifndef __DEF_BLUETOOTH_SHARE_RES_H_
#define __DEF_BLUETOOTH_SHARE_RES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libintl.h>
#include <string.h>
#include <tzplatform_config.h>

/*==============  String ================= */
#define BT_COMMON_PKG		"ug-setting-bluetooth-efl"
#define BT_COMMON_RES		"/usr/ug/res/locale"

#define BT_STR_MEMORY_FULL	\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_MEMORYFULL")
#define BT_STR_UNABLE_TO_SEND	\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_SENDINGFAIL")
#define BT_TR_STATUS \
	dgettext(BT_COMMON_PKG, "IDS_BT_BODY_PD_SUCCESSFUL_PD_FAILED")
#define BT_STR_RECEIVED \
	dgettext(BT_COMMON_PKG, "IDS_BT_BODY_RECEIVED")
#define BT_STR_SENT \
	dgettext(BT_COMMON_PKG, "IDS_BT_BODY_SENT")
#define BT_STR_SHARE \
	dgettext(BT_COMMON_PKG, "IDS_BT_BODY_BLUETOOTH_SHARE")
#define BT_STR_BLUETOOTH_ON \
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_ACTIVATED")
#define BT_STR_BLUETOOTH_AVAILABLE \
	dgettext(BT_COMMON_PKG, "IDS_BT_BODY_BLUETOOTH_AVAILABLE")

/*==============  Image ================= */
#define ICONDIR		tzplatform_mkpath(TZ_SYS_RW_APP, \
				"org.tizen.bluetooth-share-ui/res/images")

#define BT_ICON_PATH_MAX	256

#define BT_ICON_QP_SEND		strcat(ICONDIR, "/Q02_icon_BT_transmit.png")
#define BT_ICON_QP_RECEIVE	strcat(ICONDIR, "/Q02_icon_BT_receive.png")
#define BT_ICON_QP_BT_ON	strcat(ICONDIR, "/Q02_Notification_bluetooth.png")


#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_RES_H_ */
