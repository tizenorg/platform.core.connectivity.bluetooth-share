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

#ifndef __DEF_BLUETOOTH_SHARE_RES_H_
#define __DEF_BLUETOOTH_SHARE_RES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libintl.h>

/*==============  String ================= */
#define BT_COMMON_PKG		"ug-setting-bluetooth-efl"
#define BT_COMMON_RES		"/usr/ug/res/locale"

#define BT_STR_FILE_EXSIST_MSG	\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_PS_ALREADY_EXISTS_OVERWRITE_Q")
#define BT_STR_FILE_DELETE_AUTHORIZE_MSG	\
	dgettext(BT_COMMON_PKG, "Delete %s by %s ?")
#define BT_STR_RECEIVING	dgettext(BT_COMMON_PKG, "IDS_BT_POP_RECEIVING_ING")
#define BT_STR_SENDING		\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_SHARING_ING")
#define BT_STR_MEMORY_FULL	\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_MEMORYFULL")
#define BT_STR_RECEIVCED		\
	dgettext(BT_COMMON_PKG, "IDS_BT_BODY_RECEIVED")
#define BT_STR_PD_FILES_RECEIVED	\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_PD_FILES_RECEIVED")
#define BT_STR_UNABLE_TO_SEND	\
	dgettext(BT_COMMON_PKG, "IDS_BT_POP_SENDINGFAIL")

#define BT_STR_SEND_NOTI	"Bluetooth share: Sent"
#define BT_STR_RECEIVED_NOTI	"Bluetooth share: Received"
#define BT_TR_STATUS "%d successful, %d failed"

/*==============  Image ================= */
#define ICONDIR	"/opt/apps/org.tizen.bluetooth-share-ui/res/images"
#define BT_ICON_PATH_MAX	256

#define BT_ICON_QP_SEND		ICONDIR"/Q02_icon_BT_transmit.png"
#define BT_ICON_QP_RECEIVE	ICONDIR"/Q02_icon_BT_receive.png"
#define BT_ICON_QP_BT_ON		ICONDIR"/Q02_QuickPanel_icon_bluetooth_noti.png"


#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_RES_H_ */
