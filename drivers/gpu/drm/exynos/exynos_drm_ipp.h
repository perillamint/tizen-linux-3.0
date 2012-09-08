/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * Authors:
 *	Eunchul Kim <chulspro.kim@samsung.com>
 *	Jinyoung Jeon <jy0.jeon@samsung.com>
 *	Sangmin Lee <lsmin.lee@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _EXYNOS_DRM_IPP_H_
#define _EXYNOS_DRM_IPP_H_

#define for_each_ipp_ops(pos)	\
	for (pos = 0; pos < EXYNOS_DRM_OPS_MAX; pos++)
#define for_each_ipp_planar(pos)	\
	for (pos = 0; pos < EXYNOS_DRM_PLANAR_MAX; pos++)

#define IPP_GET_LCD_WIDTH	_IOR('F', 302, int)
#define IPP_GET_LCD_HEIGHT	_IOR('F', 303, int)
#define IPP_SET_WRITEBACK	_IOW('F', 304, u32)

/* definition of state */
enum drm_exynos_ipp_state {
	IPP_STATE_IDLE,
	IPP_STATE_START,
	IPP_STATE_STOP,
};

/*
 * A structure of buffer information.
 *
 * @gem_objs: Y, Cb, Cr each gem object.
 * @base: Y, Cb, Cr each planar address.
 * @size: Y, Cb, Cr each planar size.
 */
struct drm_exynos_ipp_buf_info {
	void	*gem_objs[EXYNOS_DRM_PLANAR_MAX];
	dma_addr_t	base[EXYNOS_DRM_PLANAR_MAX];
	uint64_t	size[EXYNOS_DRM_PLANAR_MAX];
};

/*
 * A structure of event work information.
 * @work: work structure.
 * @ippdrv: current work ippdrv.
 * @buf_id: id of src, dst buffer.
 */
struct drm_exynos_ipp_event_work {
	struct work_struct	work;
	struct exynos_drm_ippdrv *ippdrv;
	u32	buf_id[EXYNOS_DRM_OPS_MAX];
};

/*
 * A structure of source,destination operations.
 *
 * @set_fmt: set format of image.
 * @set_transf: set transform(rotations, flip).
 * @set_size: set size of region.
 * @set_addr: set address for dma.
 */
struct exynos_drm_ipp_ops {
	int (*set_fmt)(struct device *dev, u32 fmt);
	int (*set_transf)(struct device *dev,
		enum drm_exynos_degree degree,
		enum drm_exynos_flip flip);
	int (*set_size)(struct device *dev, int swap,
		struct drm_exynos_pos *pos, struct drm_exynos_sz *sz);
	int (*set_addr)(struct device *dev,
			 struct drm_exynos_ipp_buf_info *buf_info, u32 buf_id,
		enum drm_exynos_ipp_buf buf);
};

/*
 * A structure of ipp driver.
 *
 * @drv_list: list head for registed sub driver information.
 * @parent_dev: parent device information.
 * @dev: platform device.
 * @drm_dev: drm device.
 * @ipp_id: id of ipp driver.
 * @dedicated: dedicated ipp device.
 * @iommu_used: iommu used status.
 * @ops: source, destination operations.
 * @event_workq: event work queue.
 * @property: current property.
 * @cmd_list: list head for command information.
 * @prop_list: property informations of current ipp driver.
 * @check_property: check property about format, size, buffer.
 * @reset: reset ipp block.
 * @start: ipp each device start.
 * @stop: ipp each device stop.
 * @sched_event: work schedule handler.
 */
struct exynos_drm_ippdrv {
	struct list_head	drv_list;
	struct device	*parent_dev;
	struct device	*dev;
	struct drm_device	*drm_dev;
	u32	ipp_id;
	bool	dedicated;
	bool	iommu_used;
	struct exynos_drm_ipp_ops	*ops[EXYNOS_DRM_OPS_MAX];
	struct workqueue_struct	*event_workq;
	struct drm_exynos_ipp_property	*property;
	struct list_head	cmd_list;
	struct drm_exynos_ipp_prop_list *prop_list;

	int (*check_property)(struct device *dev,
		struct drm_exynos_ipp_property *property);
	int (*reset)(struct device *dev);
	int (*start)(struct device *dev, enum drm_exynos_ipp_cmd cmd);
	void (*stop)(struct device *dev, enum drm_exynos_ipp_cmd cmd);
	void (*sched_event)(struct work_struct *work);
};

#ifdef CONFIG_DRM_EXYNOS_IPP
extern int exynos_drm_ippdrv_register(struct exynos_drm_ippdrv *ippdrv);
extern int exynos_drm_ippdrv_unregister(struct exynos_drm_ippdrv *ippdrv);
extern int exynos_drm_ipp_get_property(struct drm_device *drm_dev, void *data,
					 struct drm_file *file);
extern int exynos_drm_ipp_set_property(struct drm_device *drm_dev, void *data,
					 struct drm_file *file);
extern int exynos_drm_ipp_queue_buf(struct drm_device *drm_dev, void *data,
					 struct drm_file *file);
extern int exynos_drm_ipp_cmd_ctrl(struct drm_device *drm_dev, void *data,
					 struct drm_file *file);
extern int exynos_drm_ippnb_register(struct notifier_block *nb);
extern int exynos_drm_ippnb_unregister(struct notifier_block *nb);
extern int exynos_drm_ippnb_send_event(unsigned long val, void *v);
#else
static inline int exynos_drm_ippdrv_register(struct exynos_drm_ippdrv *ippdrv)
{
	return -ENODEV;
}

static inline int exynos_drm_ippdrv_unregister(struct exynos_drm_ippdrv *ippdrv)
{
	return -ENODEV;
}

static inline int exynos_drm_ipp_get_property(struct drm_device *drm_dev,
						void *data,
						struct drm_file *file_priv)
{
	return -ENOTTY;
}

static inline int exynos_drm_ipp_set_property(struct drm_device *drm_dev,
						void *data,
						struct drm_file *file_priv)
{
	return -ENOTTY;
}

static inline int exynos_drm_ipp_queue_buf(struct drm_device *drm_dev,
						void *data,
						struct drm_file *file)
{
	return -ENOTTY;
}

static inline int exynos_drm_ipp_cmd_ctrl(struct drm_device *drm_dev,
						void *data,
						struct drm_file *file)
{
	return -ENOTTY;
}

static inline int exynos_drm_ippnb_register(struct notifier_block *nb)
{
	return -ENODEV;
}

static inline int exynos_drm_ippnb_unregister(struct notifier_block *nb)
{
	return -ENODEV;
}

static inline int exynos_drm_ippnb_send_event(unsigned long val, void *v)
{
	return -ENOTTY;
}
#endif

#endif /* _EXYNOS_DRM_IPP_H_ */

