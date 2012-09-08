/* linux/arch/arm/mach-exynos/dispfreq_opp_exynos4.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS4 - Display frequency scaling support with OPP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* This feature was derived from exynos4 display of devfreq
 * which was made by Mr Myungjoo Ham.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/opp.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/pm_qos_params.h>

#include <linux/list.h>
#include <linux/pm_qos_params.h>
#include <mach/cpufreq.h>
#include <mach/dev.h>
#include <linux/device.h>

#define EXYNOS4_DISPLAY_ON	1
#define EXYNOS4_DISPLAY_OFF	0

static struct pm_qos_request_list qos_wrapper[DVFS_LOCK_ID_END];

/* Wrappers for obsolete legacy kernel hack (busfreq_lock/lock_free) */
int exynos4_busfreq_lock(unsigned int nId, enum busfreq_level_request lvl)
{
	s32 qos_value;

	if (WARN(nId >= DVFS_LOCK_ID_END, "incorrect nId."))
		return -EINVAL;
	if (WARN(lvl >= BUS_LEVEL_END, "incorrect level."))
		return -EINVAL;

	switch (lvl) {
	case BUS_L0:
		qos_value = 400000;
		break;
	case BUS_L1:
		qos_value = 267000;
		break;
	case BUS_L2:
		qos_value = 133000;
		break;
	default:
		qos_value = 0;
	}

	if (qos_wrapper[nId].pm_qos_class == 0) {
		pm_qos_add_request(&qos_wrapper[nId],
				   PM_QOS_BUS_QOS, qos_value);
	} else {
		pm_qos_update_request(&qos_wrapper[nId], qos_value);
	}

	return 0;
}

void exynos4_busfreq_lock_free(unsigned int nId)
{
	if (WARN(nId >= DVFS_LOCK_ID_END, "incorrect nId."))
		return;

	if (qos_wrapper[nId].pm_qos_class)
		pm_qos_update_request(&qos_wrapper[nId],
				      PM_QOS_BUS_DMA_THROUGHPUT_DEFAULT_VALUE);
}
