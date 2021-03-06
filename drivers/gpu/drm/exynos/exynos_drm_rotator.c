/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Authors:
 *	YoungJun Cho <yj44.cho@samsung.com>
 *	Eunchul Kim <chulspro.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundationr
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#include <drm/drmP.h>
#include <drm/exynos_drm.h>
#include "regs-rotator.h"
#include "exynos_drm.h"
#include "exynos_drm_drv.h"
#include "exynos_drm_ipp.h"
#include "exynos_drm_iommu.h"

enum rot_irq_status {
	ROT_IRQ_STATUS_COMPLETE	= 8,
	ROT_IRQ_STATUS_ILLEGAL	= 9,
};

struct rot_limit {
	u32	min_w;
	u32	min_h;
	u32	max_w;
	u32	max_h;
	u32	align;
};

struct rot_limit_table {
	struct rot_limit	ycbcr420_2p;
	struct rot_limit	rgb888;
};

struct rot_context {
	struct rot_limit_table		*limit_tbl;
	struct clk			*clock;
	struct resource			*regs_res;
	void __iomem			*regs;
	int				irq;
	struct exynos_drm_ippdrv	ippdrv;
	int				cur_buf_id[EXYNOS_DRM_OPS_MAX];
	bool				suspended;
};

static void rotator_reg_set_irq(struct rot_context *rot, bool enable)
{
	u32 val = readl(rot->regs + ROT_CONFIG);

	if (enable == true)
		val |= ROT_CONFIG_IRQ;
	else
		val &= ~ROT_CONFIG_IRQ;

	writel(val, rot->regs + ROT_CONFIG);
}

static u32 rotator_reg_get_format(struct rot_context *rot)
{
	u32 val = readl(rot->regs + ROT_CONTROL);

	val &= ROT_CONTROL_FMT_MASK;

	return val;
	}

static enum rot_irq_status rotator_reg_get_irq_status(struct rot_context *rot)
{
	u32 val = readl(rot->regs + ROT_STATUS);

	val = ROT_STATUS_IRQ(val);

	if (val == ROT_STATUS_IRQ_VAL_COMPLETE)
		return ROT_IRQ_STATUS_COMPLETE;
	else
		return ROT_IRQ_STATUS_ILLEGAL;
}

static void rotator_reg_set_irq_status_clear(struct rot_context *rot,
						enum rot_irq_status status)
{
	u32 val = readl(rot->regs + ROT_STATUS);

	val |= ROT_STATUS_IRQ_PENDING((u32)status);

	writel(val, rot->regs + ROT_STATUS);
}

static void rotator_reg_get_dump(struct rot_context *rot)
{
	u32 val, i;

	for (i = 0; i <= ROT_DST_CROP_POS; i += 0x4) {
		val = readl(rot->regs + i);
		DRM_DEBUG_KMS("[%s] [0x%x] : 0x%x\n", __func__, i, val);
	}
}

static irqreturn_t rotator_irq_handler(int irq, void *arg)
{
	struct rot_context *rot = arg;
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	struct drm_exynos_ipp_cmd_node *c_node = ippdrv->cmd;
	struct drm_exynos_ipp_event_work *event_work = c_node->event_work;
	enum rot_irq_status irq_status;

	/* Get execution result */
	irq_status = rotator_reg_get_irq_status(rot);
	rotator_reg_set_irq_status_clear(rot, irq_status);

	if (irq_status == ROT_IRQ_STATUS_COMPLETE) {
		event_work->ippdrv = ippdrv;
		event_work->buf_id[EXYNOS_DRM_OPS_DST] =
			rot->cur_buf_id[EXYNOS_DRM_OPS_DST];
		queue_work(ippdrv->event_workq,
			(struct work_struct *)event_work);
	} else {
		DRM_ERROR("the SFR is set illegally\n");
		rotator_reg_get_dump(rot);
	}

	return IRQ_HANDLED;
}

static void rotator_align_size(struct rot_context *rot, u32 fmt, u32 *hsize,
								u32 *vsize)
{
	struct rot_limit_table *limit_tbl = rot->limit_tbl;
	struct rot_limit *limit;
	u32 mask, val;

	/* Get size limit */
	if (fmt == ROT_CONTROL_FMT_RGB888)
		limit = &limit_tbl->rgb888;
	else
		limit = &limit_tbl->ycbcr420_2p;

	/* Get mask for rounding to nearest aligned val */
	mask = ~((1 << limit->align) - 1);

	/* Set aligned width */
	val = ROT_ALIGN(*hsize, limit->align, mask);
	if (val < limit->min_w)
		*hsize = ROT_MIN(limit->min_w, mask);
	else if (val > limit->max_w)
		*hsize = ROT_MAX(limit->max_w, mask);
	else
		*hsize = val;

	/* Set aligned height */
	val = ROT_ALIGN(*vsize, limit->align, mask);
	if (val < limit->min_h)
		*vsize = ROT_MIN(limit->min_h, mask);
	else if (val > limit->max_h)
		*vsize = ROT_MAX(limit->max_h, mask);
	else
		*vsize = val;
}

static int rotator_src_set_fmt(struct device *dev, u32 fmt)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 val;

	val = readl(rot->regs + ROT_CONTROL);
	val &= ~ROT_CONTROL_FMT_MASK;

	switch (fmt) {
	case DRM_FORMAT_NV12:
		val |= ROT_CONTROL_FMT_YCBCR420_2P;
		break;
	case DRM_FORMAT_XRGB8888:
		val |= ROT_CONTROL_FMT_RGB888;
		break;
	default:
		DRM_ERROR("invalid image format\n");
		return -EINVAL;
	}

	writel(val, rot->regs + ROT_CONTROL);

	return 0;
}

static int rotator_src_set_size(struct device *dev, int swap,
						struct drm_exynos_pos *pos,
						struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 fmt, hsize, vsize;
	u32 val;

	/* Get format */
	fmt = rotator_reg_get_format(rot);

	/* Align buffer size */
	hsize = sz->hsize;
	vsize = sz->vsize;
	rotator_align_size(rot, fmt, &hsize, &vsize);

	/* Set buffer size configuration */
	val = ROT_SET_BUF_SIZE_H(vsize) | ROT_SET_BUF_SIZE_W(hsize);
	writel(val, rot->regs + ROT_SRC_BUF_SIZE);

	/* Set crop image position configuration */
	val = ROT_CROP_POS_Y(pos->y) | ROT_CROP_POS_X(pos->x);
	writel(val, rot->regs + ROT_SRC_CROP_POS);
	val = ROT_SRC_CROP_SIZE_H(pos->h) | ROT_SRC_CROP_SIZE_W(pos->w);
	writel(val, rot->regs + ROT_SRC_CROP_SIZE);

	return 0;
}

static int rotator_src_set_addr(struct device *dev,
				struct drm_exynos_ipp_buf_info *buf_info,
		u32 buf_id, enum drm_exynos_ipp_buf_type buf_type)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 val, fmt, hsize, vsize;
	int i;

	/* Set current buf_id */
	rot->cur_buf_id[EXYNOS_DRM_OPS_SRC] = buf_id;

	switch (buf_type) {
	case IPP_BUF_ENQUEUE:
		/* Set address configuration */
		for_each_ipp_planar(i)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_format(rot);

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
					(addr[EXYNOS_DRM_PLANAR_CB] == 0x00)) {

			val = readl(rot->regs + ROT_SRC_BUF_SIZE);
			hsize = ROT_GET_BUF_SIZE_W(val);
			vsize = ROT_GET_BUF_SIZE_H(val);

			/* Set cb planar */
			addr[EXYNOS_DRM_PLANAR_CB] =
				addr[EXYNOS_DRM_PLANAR_Y] + hsize * vsize;
		}

		for_each_ipp_planar(i)
			writel(addr[i], rot->regs + ROT_SRC_BUF_ADDR(i));
		break;
	case IPP_BUF_DEQUEUE:
		for_each_ipp_planar(i)
			writel(0x0, rot->regs + ROT_SRC_BUF_ADDR(i));
		break;
	default:
		/* Nothing to do */
		break;
	}

	return 0;
}

static int rotator_dst_set_transf(struct device *dev,
					enum drm_exynos_degree degree,
					enum drm_exynos_flip flip)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 val;

	/* Set transform configuration */
	val = readl(rot->regs + ROT_CONTROL);
	val &= ~ROT_CONTROL_FLIP_MASK;

	switch (flip) {
	case EXYNOS_DRM_FLIP_VERTICAL:
		val |= ROT_CONTROL_FLIP_VERTICAL;
		break;
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		val |= ROT_CONTROL_FLIP_HORIZONTAL;
		break;
	default:
		/* Flip None */
		break;
	}

	val &= ~ROT_CONTROL_ROT_MASK;

	switch (degree) {
	case EXYNOS_DRM_DEGREE_90:
		val |= ROT_CONTROL_ROT_90;
		break;
	case EXYNOS_DRM_DEGREE_180:
		val |= ROT_CONTROL_ROT_180;
		break;
	case EXYNOS_DRM_DEGREE_270:
		val |= ROT_CONTROL_ROT_270;
		break;
	default:
		/* Rotation 0 Degree */
		break;
	}

	writel(val, rot->regs + ROT_CONTROL);

	/* Check degree for setting buffer size swap */
	if ((degree == EXYNOS_DRM_DEGREE_90) ||
		(degree == EXYNOS_DRM_DEGREE_270))
		return 1;
	else
		return 0;
}

static int rotator_dst_set_size(struct device *dev, int swap,
						struct drm_exynos_pos *pos,
						struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 val, fmt, hsize, vsize;

	/* Get format */
	fmt = rotator_reg_get_format(rot);

	/* Align buffer size */
	hsize = sz->hsize;
	vsize = sz->vsize;
	rotator_align_size(rot, fmt, &hsize, &vsize);

	/* Set buffer size configuration */
	val = ROT_SET_BUF_SIZE_H(vsize) | ROT_SET_BUF_SIZE_W(hsize);
	writel(val, rot->regs + ROT_DST_BUF_SIZE);

	/* Set crop image position configuration */
	val = ROT_CROP_POS_Y(pos->y) | ROT_CROP_POS_X(pos->x);
	writel(val, rot->regs + ROT_DST_CROP_POS);

	return 0;
}

static int rotator_dst_set_addr(struct device *dev,
				struct drm_exynos_ipp_buf_info *buf_info,
		u32 buf_id, enum drm_exynos_ipp_buf_type buf_type)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 val, fmt, hsize, vsize;
	int i;

	/* Set current buf_id */
	rot->cur_buf_id[EXYNOS_DRM_OPS_DST] = buf_id;

	switch (buf_type) {
	case IPP_BUF_ENQUEUE:
		/* Set address configuration */
		for_each_ipp_planar(i)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_format(rot);

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
					(addr[EXYNOS_DRM_PLANAR_CB] == 0x00)) {
			/* Get buf size */
			val = readl(rot->regs + ROT_DST_BUF_SIZE);

			hsize = ROT_GET_BUF_SIZE_W(val);
			vsize = ROT_GET_BUF_SIZE_H(val);

			/* Set cb planar */
			addr[EXYNOS_DRM_PLANAR_CB] =
				addr[EXYNOS_DRM_PLANAR_Y] + hsize * vsize;
		}

		for_each_ipp_planar(i)
			writel(addr[i], rot->regs + ROT_DST_BUF_ADDR(i));
		break;
	case IPP_BUF_DEQUEUE:
		for_each_ipp_planar(i)
			writel(0x0, rot->regs + ROT_DST_BUF_ADDR(i));
		break;
	default:
		/* Nothing to do */
		break;
	}

	return 0;
}

static struct exynos_drm_ipp_ops rot_src_ops = {
	.set_fmt	=	rotator_src_set_fmt,
	.set_size	=	rotator_src_set_size,
	.set_addr	=	rotator_src_set_addr,
};

static struct exynos_drm_ipp_ops rot_dst_ops = {
	.set_transf	=	rotator_dst_set_transf,
	.set_size	=	rotator_dst_set_size,
	.set_addr	=	rotator_dst_set_addr,
};

static int rotator_init_prop_list(struct drm_exynos_ipp_prop_list **prop_list)
{
	DRM_DEBUG_KMS("%s\n", __func__);

	if (!prop_list) {
		DRM_ERROR("empty prop_list.\n");
		return -EINVAL;
	}

	*prop_list = kzalloc(sizeof(**prop_list), GFP_KERNEL);
	if (!*prop_list) {
		DRM_ERROR("failed to alloc property list.\n");
		return -ENOMEM;
	}
	/*ToDo fix support function list*/
	(*prop_list)->version = 1;
	(*prop_list)->flip = (1 << EXYNOS_DRM_FLIP_VERTICAL) |
				(1 << EXYNOS_DRM_FLIP_HORIZONTAL);
	(*prop_list)->degree = (1 << EXYNOS_DRM_DEGREE_0) |
				(1 << EXYNOS_DRM_DEGREE_90) |
				(1 << EXYNOS_DRM_DEGREE_180) |
				(1 << EXYNOS_DRM_DEGREE_270);
	(*prop_list)->csc = 0;
	(*prop_list)->crop = 0;
	(*prop_list)->scale = 0;

	return 0;
}

static int rotator_ippdrv_check_property(struct device *dev,
				struct drm_exynos_ipp_property *property)
{
	struct drm_exynos_ipp_config *src_config =
					&property->config[EXYNOS_DRM_OPS_SRC];
	struct drm_exynos_ipp_config *dst_config =
					&property->config[EXYNOS_DRM_OPS_DST];
	struct drm_exynos_pos *src_pos = &src_config->pos;
	struct drm_exynos_pos *dst_pos = &dst_config->pos;
	struct drm_exynos_sz *src_sz = &src_config->sz;
	struct drm_exynos_sz *dst_sz = &dst_config->sz;
	bool swap = false;

	/* Check format configuration */
	if (src_config->fmt != dst_config->fmt) {
		DRM_DEBUG_KMS("[%s]not support csc feature\n", __func__);
		return -EINVAL;
	}

	switch (src_config->fmt) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]not support format\n", __func__);
		return -EINVAL;
	}

	/* Check transform configuration */
	if (src_config->degree != EXYNOS_DRM_DEGREE_0) {
		DRM_DEBUG_KMS("[%s]not support source-side rotation\n",
								__func__);
		return -EINVAL;
	}

	switch (dst_config->degree) {
	case EXYNOS_DRM_DEGREE_90:
	case EXYNOS_DRM_DEGREE_270:
		swap = true;
	case EXYNOS_DRM_DEGREE_0:
	case EXYNOS_DRM_DEGREE_180:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]invalid degree\n", __func__);
		return -EINVAL;
	}

	if (src_config->flip != EXYNOS_DRM_FLIP_NONE) {
		DRM_DEBUG_KMS("[%s]not support source-side flip\n", __func__);
		return -EINVAL;
	}

	switch (dst_config->flip) {
	case EXYNOS_DRM_FLIP_NONE:
	case EXYNOS_DRM_FLIP_VERTICAL:
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]invalid flip\n", __func__);
		return -EINVAL;
	}

	/* Check size configuration */
	if ((src_pos->x + src_pos->w > src_sz->hsize) ||
		(src_pos->y + src_pos->h > src_sz->vsize)) {
		DRM_DEBUG_KMS("[%s]out of source buffer bound\n", __func__);
		return -EINVAL;
	}

	if (swap) {
		if ((dst_pos->x + dst_pos->h > dst_sz->vsize) ||
			(dst_pos->y + dst_pos->w > dst_sz->hsize)) {
			DRM_DEBUG_KMS("[%s]out of destination buffer bound\n",
								__func__);
			return -EINVAL;
		}

		if ((src_pos->w != dst_pos->h) || (src_pos->h != dst_pos->w)) {
			DRM_DEBUG_KMS("[%s]not support scale feature\n",
								__func__);
			return -EINVAL;
		}
	} else {
		if ((dst_pos->x + dst_pos->w > dst_sz->hsize) ||
			(dst_pos->y + dst_pos->h > dst_sz->vsize)) {
			DRM_DEBUG_KMS("[%s]out of destination buffer bound\n",
								__func__);
			return -EINVAL;
		}

		if ((src_pos->w != dst_pos->w) || (src_pos->h != dst_pos->h)) {
			DRM_DEBUG_KMS("[%s]not support scale feature\n",
								__func__);
			return -EINVAL;
		}
	}

	return 0;
}

static int rotator_ippdrv_start(struct device *dev, enum drm_exynos_ipp_cmd cmd)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 val;

	if (rot->suspended) {
		DRM_ERROR("suspended state\n");
		return -EPERM;
	}

	if (cmd != IPP_CMD_M2M) {
		DRM_ERROR("not support cmd: %d\n", cmd);
		return -EINVAL;
	}

	/* Set interrupt enable */
	rotator_reg_set_irq(rot, true);

	val = readl(rot->regs + ROT_CONTROL);
	val |= ROT_CONTROL_START;

	writel(val, rot->regs + ROT_CONTROL);

	return 0;
}

static int __devinit rotator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot;
	struct resource *res;
	struct exynos_drm_ippdrv *ippdrv;
	int ret;

	rot = kzalloc(sizeof(*rot), GFP_KERNEL);
	if (!rot) {
		dev_err(dev, "failed to allocate rot\n");
		return -ENOMEM;
	}

	rot->limit_tbl = (struct rot_limit_table *)
				platform_get_device_id(pdev)->driver_data;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to find registers\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	rot->regs_res = request_mem_region(res->start, resource_size(res),
								dev_name(dev));
	if (!rot->regs_res) {
		dev_err(dev, "failed to claim register region\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	rot->regs = ioremap(res->start, resource_size(res));
	if (!rot->regs) {
		dev_err(dev, "failed to map register\n");
		ret = -ENXIO;
		goto err_ioremap;
	}

	rot->irq = platform_get_irq(pdev, 0);
	if (rot->irq < 0) {
		dev_err(dev, "faild to get irq\n");
		ret = rot->irq;
		goto err_get_irq;
	}

	ret = request_threaded_irq(rot->irq, NULL, rotator_irq_handler,
					IRQF_ONESHOT, "drm_rotator", rot);
	if (ret < 0) {
		dev_err(dev, "failed to request irq\n");
		goto err_get_irq;
	}

	rot->clock = clk_get(dev, "rotator");
	if (IS_ERR_OR_NULL(rot->clock)) {
		dev_err(dev, "faild to get clock\n");
		ret = PTR_ERR(rot->clock);
		goto err_clk_get;
	}

	pm_runtime_enable(dev);

	ippdrv = &rot->ippdrv;
	ippdrv->dev = dev;
	ippdrv->ops[EXYNOS_DRM_OPS_SRC] = &rot_src_ops;
	ippdrv->ops[EXYNOS_DRM_OPS_DST] = &rot_dst_ops;
	ippdrv->check_property = rotator_ippdrv_check_property;
	ippdrv->start = rotator_ippdrv_start;
	ret = rotator_init_prop_list(&ippdrv->prop_list);
	if (ret < 0) {
		dev_err(dev, "failed to init property list.\n");
		goto err_ippdrv_register;
	}

	DRM_INFO("%s:ippdrv[0x%x]\n", __func__, (int)ippdrv);

	platform_set_drvdata(pdev, rot);

	ret = exynos_drm_ippdrv_register(ippdrv);
	if (ret < 0) {
		dev_err(dev, "failed to register drm rotator device\n");
		kfree(ippdrv->prop_list);
		goto err_ippdrv_register;
	}

	dev_info(dev, "The exynos rotator is probed successfully\n");

	return 0;

err_ippdrv_register:
	pm_runtime_disable(dev);
	clk_put(rot->clock);
err_clk_get:
	free_irq(rot->irq, rot);
err_get_irq:
	iounmap(rot->regs);
err_ioremap:
	release_resource(rot->regs_res);
	kfree(rot->regs_res);
err_get_resource:
	kfree(rot);
	return ret;
}

static int __devexit rotator_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot = dev_get_drvdata(dev);
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;

	kfree(ippdrv->prop_list);
	exynos_drm_ippdrv_unregister(ippdrv);

	pm_runtime_disable(dev);
	clk_put(rot->clock);

	free_irq(rot->irq, rot);

	iounmap(rot->regs);

	release_resource(rot->regs_res);
	kfree(rot->regs_res);

	kfree(rot);

	return 0;
}

struct rot_limit_table rot_limit_tbl = {
	.ycbcr420_2p = {
		.min_w = 32,
		.min_h = 32,
		.max_w = SZ_32K,
		.max_h = SZ_32K,
		.align = 3,
	},
	.rgb888 = {
		.min_w = 8,
		.min_h = 8,
		.max_w = SZ_8K,
		.max_h = SZ_8K,
		.align = 2,
	},
};

struct platform_device_id rotator_driver_ids[] = {
	{
		.name		= "exynos-rot",
		.driver_data	= (unsigned long)&rot_limit_tbl,
	},
	{},
};

static int rotator_power_on(struct rot_context *rot, bool enable)
{
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	struct drm_device *drm_dev = ippdrv->drm_dev;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;
	int ret;

	DRM_DEBUG_KMS("%s:\n", __func__);

	if (enable) {
		/* activate iommu */
		ret = exynos_drm_iommu_activate(drm_priv->vmm, ippdrv->dev);
		if (ret) {
			DRM_ERROR("failed to activate iommu\n");
			return -EFAULT;
		}

		clk_enable(rot->clock);
		rot->suspended = false;
	} else {
		clk_disable(rot->clock);

		/* deactivate iommu */
		exynos_drm_iommu_deactivate(drm_priv->vmm, ippdrv->dev);
		rot->suspended = true;
	}

	return 0;
}


#ifdef CONFIG_PM_SLEEP
static int rotator_suspend(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	DRM_DEBUG_KMS("%s:\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;
	/* ToDo */
	return rotator_power_on(rot, false);
}

static int rotator_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	DRM_DEBUG_KMS("%s:\n", __func__);

	if (!pm_runtime_suspended(dev))
		return rotator_power_on(rot, true);
	/* ToDo */
	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int rotator_runtime_suspend(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	DRM_DEBUG_KMS("%s:\n", __func__);

	/* ToDo */
	return  rotator_power_on(rot, false);
}

static int rotator_runtime_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	DRM_DEBUG_KMS("%s:\n", __func__);
	/* ToDo */
	return  rotator_power_on(rot, true);
}
#endif

static const struct dev_pm_ops rotator_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rotator_suspend, rotator_resume)
	SET_RUNTIME_PM_OPS(rotator_runtime_suspend, rotator_runtime_resume,
									NULL)
};

struct platform_driver rotator_driver = {
	.probe		= rotator_probe,
	.remove		= __devexit_p(rotator_remove),
	.id_table	= rotator_driver_ids,
	.driver		= {
		.name	= "exynos-rot",
		.owner	= THIS_MODULE,
		.pm	= &rotator_pm_ops,
	},
};
