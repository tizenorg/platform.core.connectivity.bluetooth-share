/*
 * bluetooth-share-api
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

#ifndef __APPLOG_H__
#define __APPLOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SYSLOG_INFO(), SYSLOG_ERR(), SYSLOG_DBG() are syslog() wrappers.
 * PRT_INFO(), PRT_ERR(), PRT_DBG() are fprintf() wrappers.
 *
 * If SLP_DEBUG is not defined, SYSLOG_DBG() and PRT_DBG() is ignored.
 *
 * IF SLP_SYSLOG_OUT or SLP_DAEMON is defined,
 *   INFO(), ERR(), DBG() are SYSLOG_XXX()
 * Otherwise,
 *   They are PRT_XXX()
 *
 *
 * warn_if(exrp, fmt, ...)
 *   If expr is true, The fmt string is printed using ERR().
 *
 * ret_if(), retv_if(), retm_if(), retvm_if()
 *   If expr is true, current function return.
 *   Postfix 'v' means that it has a return value and
 *   'm' means that it has output message.
 *
 */

#if defined(SLP_SYSLOG_OUT) || defined(SLP_DAEMON)
#include <syslog.h>
#endif
#include <stdio.h>
#include <dlog.h>

#if defined(SLP_SYSLOG_OUT) || defined(SLP_DAEMON)
#define __LOG(prio, fmt, arg...) \
	do { syslog(prio, fmt, ##arg); } while (0)

#define __LOGD(prio, fmt, arg...) \
	do {
		syslog(prio, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg);
	} while (0)
#else
#define __LOG(prio, fmt, arg...) do { } while (0)
#define __LOGD(prio, fmt, arg...) do { } while (0)
#endif

#define __PRT(prio, fmt, arg...) \
	do { \
		fprintf((LOG_PRI(prio) == LOG_ERR ? stderr : stdout), \
		fmt"\n", ##arg); \
	} while (0)

#define __PRTD(prio, fmt, arg...) \
	do { \
		fprintf((LOG_PRI(prio) == LOG_ERR ? stderr : stdout), \
		"[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
	} while (0)
#define _NOUT (prio, fmt, arg...) do { } while (0)

#ifdef SLP_DEBUG
#define _LOGD __LOGD
#define _LOG  __LOGD
#define _PRTD __PRTD
#define _PRT  __PRTD
#else
#define _LOGD _NOUT
#define _LOG  __LOG
#define _PRTD _NOUT
#define _PRT  __PRT
#endif

#define SYSLOG_INFO(fmt, arg...) _LOG(LOG_INFO, fmt, ##arg)
#define SYSLOG_ERR(fmt, arg...) _LOG(LOG_ERR, fmt, ##arg)
#define SYSLOG_DBG(fmt, arg...) _LOGD(LOG_DEBUG, fmt, ##arg)

#define PRT_INFO(fmt, arg...) \
	LOG(LOG_DEBUG, BT_SHARE, "%s:%d "fmt, __func__, __LINE__, ##arg)

#define PRT_ERR(fmt, arg...) \
	LOG(LOG_DEBUG, BT_SHARE, "%s:%d "fmt, __func__, __LINE__, ##arg)

#define PRT_DBG(fmt, arg...) \
	LOG(LOG_DEBUG, BT_SHARE, "%s:%d "fmt, __func__, __LINE__, ##arg)

#define BT_SHARE				"BT_SHARE"


#if defined(SLP_SYSLOG_OUT) || defined(SLP_DAEMON)
#define INFO SYSLOG_INFO
#define ERR SYSLOG_ERR
#define DBG SYSLOG_DBG
#else
#define INFO(fmt, arg...) \
	SLOG(LOG_DEBUG, BT_SHARE, "%s:%d "fmt, __func__, __LINE__, ##arg)

#define ERR(fmt, arg...) \
	SLOG(LOG_DEBUG, BT_SHARE, "%s:%d "fmt, __func__, __LINE__, ##arg)

#define DBG(fmt, arg...) \
	SLOG(LOG_DEBUG, BT_SHARE, "%s:%d "fmt, __func__, __LINE__, ##arg)
#endif

#ifdef SLP_DEBUG
#define warn_if(expr, fmt, arg...) do { \
		if (expr) { \
			ERR("(%s) -> "fmt, #expr, ##arg); \
		} \
	} while (0)
#define ret_if(expr) do { \
		if (expr) { \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0)
#define retv_if(expr, val) do { \
		if (expr) { \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0)
#define retm_if(expr, fmt, arg...) do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0)
#define retvm_if(expr, val, fmt, arg...) do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0)

#else
#define warn_if(expr, fmt, arg...) do { \
		if (expr) { \
			ERR(fmt, ##arg); \
		} \
	} while (0)
#define ret_if(expr) do { \
		if (expr) { \
			return; \
		} \
	} while (0)
#define retv_if(expr, val) do { \
		if (expr) { \
			return (val); \
		} \
	} while (0)
#define retm_if(expr, fmt, arg...) do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return; \
		} \
	} while (0)
#define retvm_if(expr, val, fmt, arg...) do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return (val); \
		} \
	} while (0)

#endif

#ifdef __cplusplus
}
#endif
#endif				/* __APPLOG_H__ */
