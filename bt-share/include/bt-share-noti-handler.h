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

void _bt_init_vconf_notification(void *data);
void _bt_deinit_vconf_notification(void);
#ifdef __cplusplus
}
#endif
#endif				/* __DEF_BLUETOOTH_SHARE_NOTI_HANDLER_H_ */
