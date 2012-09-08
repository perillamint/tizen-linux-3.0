/*
 *  Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt224s_grande.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include "mxt224s_grande_dev.h"


#if DUAL_TSP
#define FLIP_NOTINIT		-1
#define FLIP_OPEN		1
#define FLIP_CLOSE		0
/* Slave addresses */
#define MXT224S_ADDR_MAIN		0x4b
#define MXT224S_ADDR_SUB		0x4a
/* TSP_SEL value : GPIO to switch Main tsp or Sub tsp */
#define TSP_SEL_toMAIN		0
#define TSP_SEL_toSUB		1

static int Flip_status_tsp;
static int Tsp_current_addr;

static int Tsp_main_initialized;
static int Tsp_sub_initialized;
static int Tsp_probe_passed;
#endif
int16_t sumsize;

/* add for protection code */
/*#######################################*/
/*#######################################*/

/* variable related protection code */
/*#######################################*/
static int treat_median_error_status;
/*#######################################*/

int touch_is_pressed;
EXPORT_SYMBOL(touch_is_pressed);
static bool g_debug_switch;
static u8 threshold;
static int firm_status_data;

static u16 pre_x[][4] = {{0}, };
static u16 pre_y[][4] = {{0}, };


#if DEBUG_INFO
static u8	*object_type_name[63] = {
	[5]	= "GEN_MESSAGEPROCESSOR_T5",
	[6]	= "GEN_COMMANDPROCESSOR_T6",
	[7]	= "GEN_POWERCONFIG_T7",
	[8]	= "GEN_ACQUIRECONFIG_T8",
	[9]	= "TOUCH_MULTITOUCHSCREEN_T9",
	[15]	= "TOUCH_KEYARRAY_T15",
	[18]	= "SPT_COMCONFIG_T18",
	[23]	= "TOUCH_PROXIMITY_T23",
	[25]	= "SPT_SELFTEST_T25",
	[37]	= "DEBUG_DIAGNOSTICS_T37",
	[38]	= "USER_DATA_T38",
	[40]	= "PROCI_GRIPSUPPRESSION_T40",
	[42]	= "PROCI_TOUCHSUPPRESSION_T42",
	[44]	= "MESSAGECOUNT_T44",
	[46]	= "SPT_CTECONFIG_T46",
	[47]	= "PROCI_STYLUS_T47",
	[55]	= "PROCI_ADAPTIVETHRESHOLD_T55",
	[56]	= "PROCI_SHIELDLESS_T56",
	[57]	= "SPT_GENERICDATA_T57",
	[61]	= "SPT_TIMER_T61",
	[62]	= "PROCG_NOISESUPPRESSION_T62",
};
#endif

extern struct class *sec_class;
#ifdef CONFIG_READ_FROM_FILE
int mxt_download_config(struct mxt_data *data, const char *fn);
#endif
/* declare function proto type */
/*
static void mxt_ta_probe(int ta_status);
static void report_input_data(struct mxt_data *data);
*/
struct object_t *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct object_t *object;
	int i;

	for (i = 0; i < data->objects_len; i++) {
		object = data->objects + i;
		if (object->object_type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type T%u\n", type);
	return NULL;
}

int read_mem(struct mxt_data *data, u16 reg, u8 len, u8 *buf)
{
	int ret;
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.len = 2,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};
#if DUAL_TSP
		msg[0].addr=Tsp_current_addr; 
		msg[1].addr=Tsp_current_addr; 
#endif

	ret = i2c_transfer(data->client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("i2c failed ret = %d, %d %d\n", ret, Tsp_current_addr, data->client->addr);
		return ret;
	}
	return ret == 2 ? 0 : -EIO;
}

int write_mem(struct mxt_data *data, u16 reg, u8 len, const u8 *buf)
{
	int ret;
	u8 tmp[len + 2];

	put_unaligned_le16(cpu_to_le16(reg), tmp);
	memcpy(tmp + 2, buf, len);

#if DUAL_TSP
	data->client->addr =Tsp_current_addr; 
#endif
	ret = i2c_master_send(data->client, tmp, sizeof(tmp));
	if (ret < 0){
		pr_err("i2c write failed ret = %d, %d\n", ret, data->client->addr);
		return ret;
		}

	return ret == sizeof(tmp) ? 0 : -EIO;
}

static int __devinit mxt_reset(struct mxt_data *data)
{
	u8 buf = 1u;
	return write_mem(data, data->cmd_proc + CMD_RESET_OFFSET, 1, &buf);
}

static int __devinit mxt_backup(struct mxt_data *data)
{
	u8 buf = 0x55u;
	return write_mem(data, data->cmd_proc + CMD_BACKUP_OFFSET, 1, &buf);
}

static int get_object_info(struct mxt_data *data, u8 object_type, u16 *size,
				u16 *address)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			*size = data->objects[i].size + 1;
			*address = data->objects[i].i2c_address;
			return 0;
		}
	}

	return -ENODEV;
}
int mxt_read_object(struct mxt_data *data,
				u8 type, u8 offset, u8 *val)
{
	struct object_t *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->i2c_address;

	return read_mem(data, reg + offset, 1, val);
}

int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct object_t *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	if (offset >= object->size * object->instances) {
		dev_err(&data->client->dev, "Tried to write outside object T%d"
			" offset:%d, size:%d\n", type, offset, object->size);
		return -EINVAL;
	}
	reg = object->i2c_address;
	return write_mem(data, reg + offset, 1, &val);
}

#if CHECK_ANTITOUCH
void mxt_t61_timer_set(struct mxt_data *data, u8 mode, u8 cmd, u16 msPeriod)
{
	int ret = 0;
	u8 buf[5] = {3, 0, 0, 0, 0};
	u16 size = 0;
	u16 obj_address = 0;

	get_object_info(data, SPT_TIMER_T61,
			&size, &obj_address);

	buf[1] = cmd;
	buf[2] = mode;
	buf[3] = msPeriod & 0xFF;
	buf[4] = (msPeriod >> 8) & 0xFF;

	ret = write_mem(data, obj_address, 5, buf);
	if (ret)
		pr_err("%s write error T%d address[0x%x]\n",
			__func__, SPT_TIMER_T61, obj_address);

	pr_info("T61 Timer Enabled %d\n", msPeriod);
}

void mxt_t8_cal_set(struct mxt_data *data, u8 mstime)
{
	int ret = 0;
	u16 size = 0;
	u16 obj_address = 0;

	if (mstime) {
		data->pdata->check_autocal = 1;
		pr_info("T8 Autocal Enabled %d\n", mstime);
	} else {
		data->pdata->check_autocal = 0;
		pr_info("T8 Autocal Disabled %d\n", mstime);
	}

	get_object_info(data, GEN_ACQUISITIONCONFIG_T8,
									&size, &obj_address);


	ret = write_mem(data, obj_address + 4, 1, &mstime);
	if (ret)
		pr_err("%s write error T%d address[0x%x]\n",
			__func__, SPT_TIMER_T61, obj_address);
}


static int diff_two_point(s16 x, s16 y, s16 oldx, s16 oldy)
{
	s16 diffx, diffy;
	s16 distance;

	diffx = x-oldx;
	diffy = y-oldy;
	distance = abs(diffx) + abs(diffy);

	return distance;
}


static void mxt_check_coordinate(struct mxt_data *data, u8 detect, u8 id, s16 x, s16 y)
{
	if (detect) {
		data->tcount[id] = 0;
		data->distance[id] = 0;
	}
	if (data->tcount[id] >= 1) {
		data->distance[id] = diff_two_point(x, y,
					data->touchbx[id], data->touchby[id]);
#if ITDEV
		if (debug_enabled)
			pr_info("[TSP]Check Distance ID:%d, %d\n",
					id, data->distance[id]);
#endif
	}
	if (data->tcount[id] > 10000)
		data->tcount[id] = 1;
	else
	data->tcount[id]++;

	data->touchbx[id] = x;
	data->touchby[id] = y;

	if (id >= data->old_id)
		data->max_id = id;
	else
		data->max_id = data->old_id;

	data->old_id = id;
}

#endif	/* CHECK_ANTITOUCH */

#if TOUCH_BOOSTER
void mxt_set_dvfs_off(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data, dvfs_dwork.work);

	if (mxt_enabled) {
		disable_irq(data->client->irq);
		if (touch_cpu_lock_status
			&& !tsp_press_status){
			exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
			touch_cpu_lock_status = 0;
		}
		enable_irq(data->client->irq);
	}
}

void mxt_set_dvfs_on(struct mxt_data *data)
{
	cancel_delayed_work(&data->dvfs_dwork);
	if (cpu_lv < 0)
		exynos_cpufreq_get_level(TOUCH_BOOSTER_LIMIT_CLK, &cpu_lv);
	exynos_cpufreq_lock(DVFS_LOCK_ID_TSP, cpu_lv);
	touch_cpu_lock_status = 1;
}
#endif /* - TOUCH_BOOSTER */

static int write_config(struct mxt_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address = 0;
	u16 size = 0;

	ret = get_object_info(data, type, &size, &address);

	if (size == 0 && address == 0)
		return 0;
	else
		return write_mem(data, address, size, cfg);
}

static int check_instance(struct mxt_data *data, u8 object_type)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type)
			return data->objects[i].instances;
	}
	return 0;
}

static int init_write_config(struct mxt_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address = 0;
	u16 size = 0;
	u8 *temp;

	ret = get_object_info(data, type, &size, &address);

	if ((size == 0) || (address == 0)) {
		pr_err("%s error object_type(%d)\n", __func__, type);
		return -ENODEV;
	}

	ret = write_mem(data, address, size, cfg);

	if (check_instance(data, type)) {
#if DEBUG_INFO
		pr_info("exist instance1 object (%d)\n", type);
#endif
		temp = kmalloc(size * sizeof(u8), GFP_KERNEL);
		memset(temp, 0, size);
		ret |= write_mem(data, address+size, size, temp);
		kfree(temp);
	}

	return ret;
}

#if TREAT_ERR
static int change_config(struct mxt_data *data,
			u16 reg, u8 offeset, u8 change_value)
{
	u8 value = 0;

	value = change_value;
	return write_mem(data, reg+offeset, 1, &value);
}
#endif

static u32 __devinit crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16)byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32)data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}

static int __devinit calculate_infoblock_crc(struct mxt_data *data,
							u32 *crc_pointer)
{
	u32 crc = 0;
	u8 mem[7 + data->objects_len * 6];
	int status;
	int i;

	status = read_mem(data, 0, sizeof(mem), mem);

	if (status)
		return status;

	for (i = 0; i < sizeof(mem) - 1; i += 2)
		crc = crc24(crc, mem[i], mem[i + 1]);

	*crc_pointer = crc24(crc, mem[i], 0) & 0x00FFFFFF;

	return 0;
}

static uint8_t calibrate_chip_e(void)
{
	u8 cal_data = 1;
	int ret = 0;
	/* send calibration command to the chip */
	ret = write_mem(copy_data,
		copy_data->cmd_proc + CMD_CALIBRATE_OFFSET,
		1, &cal_data);
	/* set flag for calibration lockup
	recovery if cal command was successful */
	if (!ret)
		pr_info("calibration success!!!\n");
	return ret;
}

#if TREAT_ERR
static void treat_error_status(void)
{
	bool ta_status = 0;
	u16 size;
	u16 obj_address = 0;
	int error = 0;
	struct mxt_data *data = copy_data;

	data->read_ta_status(&ta_status);

	if (treat_median_error_status) {
		pr_err("Error status already treated\n");
		return;
	} else
		treat_median_error_status = 1;

	pr_info("Error status TA is[%d]\n", ta_status);

	if (ta_status) {
#if !(FOR_BRINGUP)
		get_object_info(data,
			GEN_POWERCONFIG_T7, &size, &obj_address);
		/* 1:ACTVACQINT */
		error = change_config(data, obj_address, 1, 255);

		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
		/* 0:CHRGTIME */
		error |= change_config(data, obj_address, 0, 64);

		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 50);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 0);

		get_object_info(data,
			PROCI_TOUCHSUPPRESSION_T42, &size, &obj_address);
		/* 0:CTRL */
		error |= change_config(data, obj_address, 0, 3);

		get_object_info(data,
			SPT_CTECONFIG_T46, &size, &obj_address);
		/* 2:IDLESYNCSPERX */
		error |= change_config(data, obj_address, 2, 48);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address, 3, 48);

		get_object_info(data,
			PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		/* 2:CALCFG */
		error |= change_config(data, obj_address, 2, 114);
		/* 3:BASEFREQ */
		error |= change_config(data, obj_address, 3, 15);
		/* 8:MFFREQ[0] */
		error |= change_config(data, obj_address, 8, 3);
		/* 9:MFFREQ[1] */
		error |= change_config(data, obj_address, 9, 5);
		/* 10:NLGAIN*/
		error |= change_config(data, obj_address, 10, 96);
		/* 11:NLTHR*/
		error |= change_config(data, obj_address, 11, 30);
		/* 17:GCMAXADCSPERX */
		error |= change_config(data, obj_address, 17, 100);
		/* 34:BLEN[0] */
		error |= change_config(data, obj_address, 34, 80);
		/* 35:TCHTHR[0] */
		error |= change_config(data, obj_address, 35, 40);
		/* 36:TCHDI[0] */
		error |= change_config(data, obj_address, 36, 2);
		/* 39:MOVFILTER[0] */
		error |= change_config(data, obj_address, 39, 65);
		/* 41:MRGHYST[0] */
		error |= change_config(data, obj_address, 41, 40);
		/* 42:MRGTHR[0] */
		error |= change_config(data, obj_address, 42, 50);
		/* 43:XLOCLIP[0] */
		error |= change_config(data, obj_address, 43, 5);
		/* 44:XHICLIP[0] */
		error |= change_config(data, obj_address, 44, 5);
		/* 51:JUMPLIMIT[0] */
		error |= change_config(data, obj_address, 51, 25);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 15);
#endif
		if (error < 0)
			pr_err("failed to write error status\n");
	} else {
#if !(FOR_BRINGUP)
		get_object_info(data,
			GEN_POWERCONFIG_T7, &size, &obj_address);
		/* 1:ACTVACQINT */
		error = change_config(data, obj_address, 1, 255);

		get_object_info(data,
			GEN_ACQUISITIONCONFIG_T8, &size, &obj_address);
		/* 0:CHRGTIME */
		error |= change_config(data, obj_address, 0, 64);

		/* 8:ATCHFRCCALTHR*/
		error |= change_config(data, obj_address, 8, 50);
		/* 9:ATCHFRCCALRATIO*/
		error |= change_config(data, obj_address, 9, 0);

		get_object_info(data,
			TOUCH_MULTITOUCHSCREEN_T9, &size, &obj_address);
		/* 31:TCHHYST */
		error |= change_config(data, obj_address, 31, 15);

		get_object_info(data,
			PROCI_TOUCHSUPPRESSION_T42, &size, &obj_address);
		/* 0:CTRL */
		error |= change_config(data, obj_address, 0, 3);

		get_object_info(data,
			SPT_CTECONFIG_T46, &size, &obj_address);
		/* 2:IDLESYNCSPERX */
		error |= change_config(data, obj_address, 2, 48);
		/* 3:ACTVSYNCSPERX */
		error |= change_config(data, obj_address, 3, 48);

		get_object_info(data,
			PROCG_NOISESUPPRESSION_T48, &size, &obj_address);
		/* 2:CALCFG */
		error |= change_config(data, obj_address, 2, 242);
		/* 3:BASEFREQ */
		error |= change_config(data, obj_address, 3, 15);
		/* 8:MFFREQ[0] */
		error |= change_config(data, obj_address, 8, 3);
		/* 9:MFFREQ[1] */
		error |= change_config(data, obj_address, 9, 5);
		/* 10:NLGAIN*/
		error |= change_config(data, obj_address, 10, 112);
		/* 11:NLTHR*/
		error |= change_config(data, obj_address, 11, 25);
		/* 17:GCMAXADCSPERX */
		error |= change_config(data, obj_address, 17, 100);
		/* 34:BLEN[0] */
		error |= change_config(data, obj_address, 34, 112);
		/* 35:TCHTHR[0] */
		error |= change_config(data, obj_address, 35, 40);
		/* 41:MRGHYST[0] */
		error |= change_config(data, obj_address, 41, 40);
		/* 42:MRGTHR[0] */
		error |= change_config(data, obj_address, 42, 50);
		/* 51:JUMPLIMIT[0] */
		error |= change_config(data, obj_address, 51, 25);
		/* 52:TCHHYST[0] */
		error |= change_config(data, obj_address, 52, 15);
#endif
		if (error < 0)
			pr_err("failed to write error status\n");
	}
}
#endif

static void mxt_ta_probe(int ta_status)
{
#ifdef CONFIG_READ_FROM_FILE
	struct mxt_data *data = copy_data;
	if (ta_status)
		mxt_download_config(data, MXT_TA_CFG_NAME);
	else
		mxt_download_config(data, MXT_BATT_CFG_NAME);
#else	
#if 1//!(FOR_BRINGUP)
	u16 obj_address = 0;
	u16 size;
	int error = 0;
	u8 value = 0;

	struct mxt_data *data = copy_data;

	if (!mxt_enabled) {
		pr_err("%s mxt_enabled is 0\n", __func__);
		return;
	}

	if (ta_status) {
		write_config(copy_data, copy_data->pdata->t9_config_chrg[0], copy_data->pdata->t9_config_chrg + 1);
		write_config(copy_data, copy_data->pdata->t47_config_chrg[0], copy_data->pdata->t47_config_chrg + 1);
		write_config(copy_data, copy_data->pdata->t62_config_chrg[0], copy_data->pdata->t62_config_chrg + 1);
#ifdef CONFIG_READ_FROM_FILE
		mxt_download_config(data, MXT_TA_CFG_NAME);
#endif
	} else {
		write_config(copy_data, copy_data->pdata->t9_config_batt[0], copy_data->pdata->t9_config_batt + 1);
		write_config(copy_data, copy_data->pdata->t47_config_batt[0], copy_data->pdata->t47_config_batt + 1);
		write_config(copy_data, copy_data->pdata->t62_config_batt[0], copy_data->pdata->t62_config_batt + 1);
#ifdef CONFIG_READ_FROM_FILE
		mxt_download_config(data, MXT_BATT_CFG_NAME);
#endif
	}
#endif
#if 0
    get_object_info(data, PROCG_NOISESUPPRESSION_T62,
    	&size, &obj_address);

	//Read CALCFG1 for Setting CHRGON
	read_mem(data, obj_address+1, 1, &value);


	value &= 0xFE;
	error = write_mem(data, obj_address+1, 1, &value);


		value |= 0x01;
		error = write_mem(data, obj_address+1, 1, &value);
#endif

#endif
	pr_info("%s : threshold[%d]\n", __func__, threshold);
};

static uint8_t reportid_to_type(struct mxt_data *data, u8 report_id, u8 *instance)
{
	struct report_id_map_t *report_id_map;
	report_id_map = data->rid_map;

	if (report_id <= data->max_report_id) {
		*instance = report_id_map[report_id].instance;
		return report_id_map[report_id].object_type;
	} else
		return 0;
}

static int __devinit mxt_init_touch_driver(struct mxt_data *data)
{
	struct object_t *object_table;
	struct report_id_map_t *report_id_map_t;
	u32 read_crc = 0;
	u32 calc_crc;
	u16 crc_address;
	u16 dummy;
	int i, j;
	u8 id[ID_BLOCK_SIZE];
	int ret;
	u8 type_count = 0;
	u8 tmp;
	int cur_rep_id, start_report_id;

	ret = read_mem(data, 0, sizeof(id), id);
	if (ret)
	{
		printk("[TSP]  mxt_init_touch_driver() read_mem fail : %d, addr:%#02x\n", ret, Tsp_current_addr);
		return ret;
	}

	pr_info("family = %#02x, variant = %#02x, version "
			"= %#02x, build = %#02x, "
			"matrix X,Y size = %d,%d\n"
			, id[0], id[1], id[2], id[3], id[4], id[5]);

	data->family_id = id[0];
	data->tsp_variant = id[1];
	data->tsp_version = id[2];
	data->tsp_build = id[3];
	data->objects_len = id[6];

	object_table = kmalloc(data->objects_len * sizeof(*object_table),
				GFP_KERNEL);
	if (!object_table)
		return -ENOMEM;

	ret = read_mem(data, OBJECT_TABLE_START_ADDRESS,
			data->objects_len * sizeof(*object_table),
			(u8 *)object_table);
	if (ret)
		goto err;

	data->max_report_id = 0;

	for (i = 0; i < data->objects_len; i++) {
		object_table[i].i2c_address =
			le16_to_cpu(object_table[i].i2c_address);
		data->max_report_id += object_table[i].num_report_ids *
						(object_table[i].instances + 1);
		tmp = 0;
		if (object_table[i].num_report_ids) {
			tmp = type_count + 1;
			type_count += object_table[i].num_report_ids *
						(object_table[i].instances + 1);
		}
		switch (object_table[i].object_type) {
		case TOUCH_MULTITOUCHSCREEN_T9:
			data->finger_type = tmp;
			pr_info("Finger type = %d\n",
						data->finger_type);
			break;
		case GEN_MESSAGEPROCESSOR_T5:
#if ITDEV
			data->msg_proc_addr = object_table[i].i2c_address;
#endif
			data->msg_object_size = object_table[i].size + 1;
			break;
		}
	}
	if (data->rid_map_alloc) {
		data->rid_map_alloc = false;
		kfree(data->rid_map);
	}
	data->rid_map = kmalloc((sizeof(report_id_map_t) * data->max_report_id + 1), GFP_KERNEL);

	if (!data->rid_map) {
		kfree(object_table);
		return -ENOMEM;
	}
	data->rid_map_alloc = true;
	data->rid_map[0].instance = 0;
	data->rid_map[0].object_type = 0;
	cur_rep_id = 1;

	for (i = 0; i < data->objects_len; i++) {
		if (object_table[i].num_report_ids != 0) {
			for (j = 0; j <= object_table[i].instances; j++) {
				for (start_report_id = cur_rep_id;
				     cur_rep_id <
				     (start_report_id +
				      object_table[i].num_report_ids);
				     cur_rep_id++) {
					data->rid_map[cur_rep_id].instance = j;
					data->rid_map[cur_rep_id].object_type =
					    object_table[i].object_type;
				}
			}
		}
	}
	data->objects = object_table;

	/* Verify CRC */
	crc_address = OBJECT_TABLE_START_ADDRESS +
			data->objects_len * OBJECT_TABLE_ELEMENT_SIZE;

#ifdef __BIG_ENDIAN
#error The following code will likely break on a big endian machine
#endif
	ret = read_mem(data, crc_address, 3, (u8 *)&read_crc);
	if (ret)
		goto err;

	read_crc = le32_to_cpu(read_crc);

	ret = calculate_infoblock_crc(data, &calc_crc);
	if (ret)
		goto err;

	if (read_crc != calc_crc) {
		pr_err("CRC error\n");
		ret = -EFAULT;
		goto err;
	}

	ret = get_object_info(data, GEN_MESSAGEPROCESSOR_T5, &dummy,
					&data->msg_proc);
	if (ret)
		goto err;

	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &dummy,
					&data->cmd_proc);
	if (ret)
		goto err;

#if DEBUG_INFO
	pr_info("maXTouch: %d Objects\n",
			data->objects_len);

	for (i = 0; i < data->objects_len; i++) {
		pr_info("Type:\t\t\t[%d]: %s\n",
			 object_table[i].object_type,
			 object_type_name[object_table[i].object_type]);
		pr_info("\tAddress:\t0x%04X\n",
			 object_table[i].i2c_address);
		pr_info("\tSize:\t\t%d Bytes\n",
			 object_table[i].size);
		pr_info("\tInstances:\t%d\n",
			 object_table[i].instances);
		pr_info("\tReport Id's:\t%d\n",
			 object_table[i].num_report_ids);
	}
#endif

	return 0;

err:
	kfree(object_table);
	return ret;
}


static void report_input_data(struct mxt_data *data)
{
	int i;
	int count = 0;
	int report_count = 0;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
					MT_TOOL_FINGER, false);
		} else {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
					MT_TOOL_FINGER, true);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					data->fingers[i].x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					data->fingers[i].y);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					data->fingers[i].z);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE,
					 data->fingers[i].w);
			 input_report_abs(data->input_dev, ABS_MT_SUMSIZE,
			 sumsize);
			input_report_abs(data->input_dev, ABS_MT_COMPONENT,
			data->fingers[i].component);
		}

		report_count++;

#if SHOW_COORDINATE
		switch (data->fingers[i].state) {
		case MXT_STATE_PRESS:
			pr_info("P: "
				"id[%d],x=%d,y=%d,w=%d, z=%d\n",
				i, data->fingers[i].x, data->fingers[i].y
				, data->fingers[i].w, data->fingers[i].z);
			break;
/*
		case MXT_STATE_MOVE:
			pr_info("M: "
				"id[%d],x=%d,y=%d,w=%d,mc=%d\n",
				i, data->fingers[i].x, data->fingers[i].y
				, data->fingers[i].w, data->fingers[i].mcount);
			break;
*/
		case MXT_STATE_RELEASE:
			pr_info("R: "
				"id[%d],mc=%d\n",
				i, data->fingers[i].mcount);
			break;
		default:
			break;
		}
#else
		if (data->fingers[i].state == MXT_STATE_PRESS)
			pr_info("P: id[%d],w=%d\n"
				, i, data->fingers[i].w);
		else if (data->fingers[i].state == MXT_STATE_RELEASE)
			pr_info("R: id[%d],mc=%d\n"
				, i, data->fingers[i].mcount);
#endif
		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			data->fingers[i].state = MXT_STATE_INACTIVE;
			data->fingers[i].mcount = 0;
		} else {
			data->fingers[i].state = MXT_STATE_MOVE;
			count++;
		}
	}
	if (report_count > 0) {
#if ITDEV
		if (!driver_paused)
#endif
			input_sync(data->input_dev);
	}

	if (count)
		touch_is_pressed = 1;
	else
		touch_is_pressed = 0;

#if TOUCH_BOOSTER
	if (count == 0) {
		sumsize = 0;
		if (touch_cpu_lock_status) {
			cancel_delayed_work(&data->dvfs_dwork);
			schedule_delayed_work(&data->dvfs_dwork,
				msecs_to_jiffies(TOUCH_BOOSTER_TIME));
		}
		tsp_press_status = 0;
	} else
		tsp_press_status = 1;
#endif
	data->finger_mask = 0;
}

#if CHECK_ANTITOUCH
static u16 mxt_dist_check(struct mxt_data *data)
{
	int i;
	u16 dist_sum = 0;

	for (i = 0; i <= data->max_id; i++) {
		if (data->distance[i] < 3)
			dist_sum++;
		else
			dist_sum = 0;
		}

	for (i = data->max_id + 1; i < MAX_USING_FINGER_NUM; i++)
		data->distance[i] = 0;
#if ITDEV
	if (debug_enabled) {
		pr_info("[TSP] dist_sum = %d\n", dist_sum);
		pr_info("[TSP] dis0 = %d, dis1 = %d, dis2 = %d,"
			"dis3 = %d, dis4 = %d, dis5 = %d\n",
			data->distance[0], data->distance[1],
			data->distance[2], data->distance[3],
			data->distance[4], data->distance[5]);
	}
#endif
	return dist_sum;
}
#endif

static irqreturn_t mxt_irq_thread(int irq, void *ptr)
{
	struct mxt_data *data = ptr;
	int id, i;
	u8 msg[data->msg_object_size];
	u8 touch_message_flag = 0;
	u8 object_type, instance;
	u16 dist_sum = 0;
#if CHECK_ANTITOUCH
	u16 tch_area = 0, atch_area = 0;
#endif

#if DEBUG_INFO
	pr_info("[TSP] mxt_irq_thread()\n");
#endif
	do {
		touch_message_flag = 0;
		if (read_mem(data, data->msg_proc, sizeof(msg), msg)) {
#if TOUCH_BOOSTER
			if (touch_cpu_lock_status) {
				exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
				touch_cpu_lock_status = 0;
			}
#endif
			return IRQ_HANDLED;
		}
#if ITDEV
		if (debug_enabled)
			print_hex_dump(KERN_INFO, "MXT MSG:",
			DUMP_PREFIX_NONE, 16, 1, msg, sizeof(msg), false);
#endif
		object_type = reportid_to_type(data, msg[0] , &instance);

		if (object_type == GEN_COMMANDPROCESSOR_T6) {
			if (msg[1] == 0x00) /* normal mode */
				pr_info("normal mode\n");
			if ((msg[1]&0x04) == 0x04) /* I2C checksum error */
				pr_info("I2C checksum error\n");
			if ((msg[1]&0x08) == 0x08) /* config error */
				pr_info("config error\n");
			if ((msg[1]&0x10) == 0x10) {
				/* calibration */
				pr_info("calibration is"
					" on going !!\n");			

#if CHECK_ANTITOUCH
			/* After Calibration */
			pr_info("[TSP] mxt_check_coordinate Disable autocal = 0\n");
			mxt_t8_cal_set(data, 0);
			data->pdata->check_antitouch = 1;
			mxt_t61_timer_set(data,
						MXT_T61_TIMER_ONESHOT,
				MXT_T61_TIMER_CMD_STOP, 0);
			data->pdata->check_timer = 0;
			data->pdata->check_calgood = 0;
                        data->touch_coin_check = 0;
#endif
			}
			if ((msg[1]&0x20) == 0x20) /* signal error */
				pr_info("signal error\n");
			if ((msg[1]&0x40) == 0x40) /* overflow */
				pr_info("overflow detected\n");
			if ((msg[1]&0x80) == 0x80) /* reset */
				pr_info("[TSP] reset is ongoing\n");
		}

		if (object_type == PROCI_TOUCHSUPPRESSION_T42) {
			if ((msg[1] & 0x01) == 0x00) {
				/* Palm release */
				pr_info("palm touch released\n");
				touch_is_pressed = 0;

			} else if ((msg[1] & 0x01) == 0x01) {
				/* Palm Press */
				pr_info("palm touch detected\n");
				touch_is_pressed = 1;
				touch_message_flag = 1;
			}
		}
		if (object_type == PROCI_EXTRATOUCHSCREENDATA_T57) {
#if CHECK_ANTITOUCH
			tch_area = msg[3] | (msg[4] << 8);
			atch_area = msg[5] | (msg[6] << 8);
			sumsize = msg[1] + (msg[2] << 8);

			data->Report_touch_number = 0;
			for (i = 0; i < data->num_fingers; i++) {
				if ((data->fingers[i].state != \
					MXT_STATE_INACTIVE) &&
					(data->fingers[i].state != \
					MXT_STATE_RELEASE))

					data->Report_touch_number++;

			}
			pr_info("TCHAREA=%d, ATCHAREA=%d, sumsize=%d, Report_touch_number:%d\n", tch_area, atch_area, sumsize, data->Report_touch_number);

			if (data->pdata->check_antitouch) {
				if (tch_area) {
					pr_info("TCHAREA=%d\n", tch_area);

					/* First Touch After Calibration */
					if (data->pdata->check_timer == 0) {
						mxt_t61_timer_set(data,
							MXT_T61_TIMER_ONESHOT,
							MXT_T61_TIMER_CMD_START, 5000);
						data->pdata->check_timer = 1;

						//mxt_t8_cal_set(data, 5*3);

				}
				}

				if ((tch_area < atch_area) ||
					(tch_area > 15) ||
					((tch_area <= 15) &&
					(tch_area < atch_area - 3))) {
					mxt_t8_cal_set(data, 3);
				}
			}
			else
			{
				if ((atch_area - tch_area) > 8) {
					if (tch_area < 35) {
						pr_info("[TSP]Cal Not Good %d %d\n",
							atch_area, tch_area);
						calibrate_chip_e();
				}
					 }
				if ((tch_area - atch_area) >= 40) {
					pr_info("[TSP]Cal Not Good 2 - %d %d\n",
							atch_area, tch_area);
					calibrate_chip_e();
					 }
				}
	// CoinCheck
			if((tch_area == 0)&&(atch_area == 0)&&(data->Report_touch_number > 0))
			{
				if(!data->touch_coin_check)
				{
					dist_sum = mxt_dist_check(data);
					if (dist_sum == (data->Report_touch_number))
					{
						if(sumsize <= 15){
							if (data->touch_area_cnt >= 3) {
								pr_info("[TSP] Multi touch Cal maybe bad contion : Set autocal = 5\n");
								mxt_t8_cal_set(data, 5);
								data->touch_area_cnt = 0;
								data->touch_coin_check = 1;
							} else {
								data->touch_area_cnt++;
							}
						}
						else {
							data->touch_area_cnt = 0;
						}
					}
				}
			}
#endif	/* CHECK_ANTITOUCH */
			}

#if CHECK_ANTITOUCH
		if (object_type == SPT_TIMER_T61) {
			if ((msg[1] & 0xa0) == 0xa0) {

				if (data->pdata->check_antitouch) {
					pr_info("SPT_TIMER_T61 Stop\n");
					data->pdata->check_antitouch = 0;
					data->pdata->check_timer = 0;
					mxt_t8_cal_set(data, 0);
					data->pdata->check_calgood = 0;
					}
				}
			}
#endif	/* CHECK_ANTITOUCH */
		if (object_type == TOUCH_MULTITOUCHSCREEN_T9) {
			id = msg[0] - data->finger_type;

			/* If not a touch event, then keep going */
			if (id < 0 || id >= data->num_fingers)
					continue;

			if (data->finger_mask & (1U << id))
				report_input_data(data);

			if (msg[1] & RELEASE_MSG_MASK) {
				data->fingers[id].z = 0;
				data->fingers[id].w = msg[5];
				data->finger_mask |= 1U << id;
				data->fingers[id].state = MXT_STATE_RELEASE;
			} else if ((msg[1] & DETECT_MSG_MASK) && (msg[1] &
					(PRESS_MSG_MASK | MOVE_MSG_MASK | VECTOR_MSG_MASK))) {
#if TOUCH_BOOSTER
				if (!touch_cpu_lock_status)
					mxt_set_dvfs_on(data);
#endif
				touch_message_flag = 1;
				data->fingers[id].z = msg[6];
				data->fingers[id].w = msg[5];
				if (data->fingers[id].w == 0)
					data->fingers[id].w = 1;

				data->fingers[id].x =
					(((msg[2] << 4) | (msg[4] >> 4))
					>> data->x_dropbits);

				data->fingers[id].y =
					(((msg[3] << 4) | (msg[4] & 0xF))
					>> data->y_dropbits);
#if HIGH_RESOLUTION
				/* high X resolution version*/
				data->fingers[id].x = (u16)((data->fingers[id].x * 480) / 4096);             /* 800 -> 480 */
				data->fingers[id].y = (u16)((data->fingers[id].y * 800) / 4096);	 /* 480 -> 800 */
#endif

				data->finger_mask |= 1U << id;

				if (msg[1] & PRESS_MSG_MASK) {
					data->fingers[id].state = MXT_STATE_PRESS;
					data->fingers[id].mcount = 0;
#if CHECK_ANTITOUCH
		                        mxt_check_coordinate(data, 1, id,
					data->fingers[id].x,
					data->fingers[id].y);
#endif

				} else if (msg[1] & MOVE_MSG_MASK) {
					data->fingers[id].mcount += 1;
#if CHECK_ANTITOUCH
					mxt_check_coordinate(data, 0, id,
					data->fingers[id].x,
					data->fingers[id].y);
#endif
				}
					data->fingers[id].component = msg[7];

			} else if ((msg[1] & SUPPRESS_MSG_MASK)
			&& (data->fingers[id].state != MXT_STATE_INACTIVE)) {
				data->fingers[id].z = 0;
				data->fingers[id].w = msg[5];
				data->fingers[id].state = MXT_STATE_RELEASE;
				data->finger_mask |= 1U << id;
			} else {
				/* ignore changed amplitude message */
				if (!((msg[1] & DETECT_MSG_MASK)
					&& (msg[1] & AMPLITUDE_MSG_MASK)))
					pr_err("Unknown state %#02x %#02x\n",
						msg[0], msg[1]);
				continue;
			}
		}
	} while (!gpio_get_value(data->gpio_read_done));

	if (data->finger_mask)
		report_input_data(data);


	return IRQ_HANDLED;
}

static int mxt_internal_suspend(struct mxt_data *data)
{
	int i;
	int count = 0;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
		count++;
	}
	if (count)
		report_input_data(data);

#if TOUCH_BOOSTER
	cancel_delayed_work(&data->dvfs_dwork);
	tsp_press_status = 0;
	if (touch_cpu_lock_status) {
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
		touch_cpu_lock_status = 0;
	}
#endif
	data->power_off();

	return 0;
}

static int mxt_internal_resume(struct mxt_data *data)
{
	data->power_on();

	return 0;
}

#if DUAL_TSP
void samsung_switching_tsp_suspend(void)
{
	static const u8 sleep_power_cfg[3]={0,0,0};
	int ret;
	int i=0; 
			
	/******************************************************/
	/*	  One TSP has to enter suspend mode					*/
	/******************************************************/
	printk("[TSP] ++++samsung_switching_tsp_suspend()\n");
	if (Flip_status_tsp == FLIP_OPEN) {/* Sub_TSP need to enter suspend-mode*/
		Tsp_current_addr = MXT224S_ADDR_SUB;
		gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toSUB);
	} else {/* Main_TSP need to enter suspend-mode*/
		Tsp_current_addr = MXT224S_ADDR_MAIN;
		gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toMAIN);
	}

	do {
		ret = write_config(copy_data, GEN_POWERCONFIG_T7, sleep_power_cfg);
		msleep(20);
		printk(KERN_ERR "[TSP] %s, i=%d, ret=%d \n", __func__, i, ret);
		i++;
	} while (ret && i < 10);


	if (Flip_status_tsp == FLIP_OPEN) { /* return to Main_TSP*/
		Tsp_current_addr = MXT224S_ADDR_MAIN;
		gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toMAIN);
	} else {/* return to Sub_TSP*/
		Tsp_current_addr = MXT224S_ADDR_SUB;
		gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toSUB);
	}
	copy_data->client->addr =Tsp_current_addr;

	return;

}

void samsung_switching_tsp_resume(void)
{
	bool ta_status = 0;
	int ret;
	int i=0;
	struct mxt_data *data = copy_data;
	
	printk("[TSP]%s : addr:%02x, tspsel :%d\n", __FUNCTION__, Tsp_current_addr, gpio_get_value(GPIO_TSP_SEL));

	if (Tsp_main_initialized == 0) {
	/******************************************************/
	/*				Main TSP or Sub TSP ini						*/
	/******************************************************/
		Tsp_main_initialized = 1;

		printk("[TSP] samsung_switching_tsp_resume() : Main TSP init	#############\n");
		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
		#ifdef CONFIG_READ_FROM_FILE
		else{
			mxt_download_config(data, MXT_BATT_CFG_NAME);
		}
		#endif
		calibrate_chip_e();
	}

	if (Tsp_sub_initialized == 0) {
	/******************************************************/
	/*				Sub TSP init								*/
	/******************************************************/
		Tsp_sub_initialized = 1;

		printk("[TSP] samsung_switching_tsp_resume() :Sub TSP init		#############\n");
		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
		#ifdef CONFIG_READ_FROM_FILE
		else{
			mxt_download_config(data, MXT_BATT_CFG_NAME);
		}
		#endif
		calibrate_chip_e();
	}


	do {
		ret = write_config(copy_data, GEN_POWERCONFIG_T7, copy_data->power_cfg);
		msleep(20);
		printk(KERN_ERR "[TSP] %s, i=%d,r=%d \n",__func__, i, ret);
		i++;
	} while (ret && i < 10);


	return;
}

void samsung_switching_tsp(int flip)
{
	if (Tsp_probe_passed == 0)
		return;

	printk("[TSP]%s\n", __FUNCTION__);
	printk("[TSP] Flip_status_tsp:%s, flip:%d(hallSW:%d)\n", Flip_status_tsp ? "FLIP OPEN" : "FLIP OPEN", flip, gpio_get_value(GPIO_HALL_SW));
	printk( "[TSP] tspsel:%d, addr:%02x, current addr:%02x\n", gpio_get_value(GPIO_TSP_SEL), copy_data->client->addr, Tsp_current_addr);

	if (Flip_status_tsp == FLIP_NOTINIT) {
		Flip_status_tsp = flip;
		samsung_switching_tsp_suspend();
		return;
	}

	disable_irq(copy_data->client->irq); /* do not accept tsp irq before folder open/close complete */
	if (mxt_enabled == 0) {
		Flip_status_tsp = flip;
	} else {
		if (Flip_status_tsp != flip) {
			Flip_status_tsp = flip;
			samsung_switching_tsp_suspend();
			samsung_switching_tsp_resume();
		}
	}
	enable_irq(copy_data->client->irq); /* enable tsp irq again */

	return;
}
EXPORT_SYMBOL(samsung_switching_tsp);

#endif // DUAL_TSP

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt_suspend	NULL
#define mxt_resume	NULL

static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);

	if (mxt_enabled == 1) {
		pr_info("%s\n", __func__);
		mxt_enabled = 0;
		touch_is_pressed = 0;
		disable_irq(data->client->irq);
		mxt_internal_suspend(data);
		Tsp_main_initialized = 0;
		Tsp_sub_initialized = 0;
	} else
		pr_err("%s. but touch already off\n", __func__);
}

static void mxt_late_resume(struct early_suspend *h)
{
	bool ta_status = 0;
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);

	if (mxt_enabled == 0) {
		pr_info("[TSP]        +%s\n", __func__);
		mxt_internal_resume(data);

		mxt_enabled = 1;

#if DUAL_TSP
	if (Flip_status_tsp == FLIP_OPEN) {
	/******************************************************/
	/*				Main TSP or Sub TSP init					    */
	/******************************************************/
		Tsp_main_initialized = 1;

		Tsp_current_addr = MXT224S_ADDR_MAIN;
	        gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toMAIN);
		printk("[TSP] mxt_late_resume() : Main TSP init		#############\n");
		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
		#ifdef CONFIG_READ_FROM_FILE
		else{
			mxt_download_config(data, MXT_BATT_CFG_NAME);
		}
		#endif
		calibrate_chip_e();
	} else {
	/******************************************************/
	/*				Sub TSP init							    */
	/******************************************************/
		Tsp_sub_initialized = 1;

		Tsp_current_addr = MXT224S_ADDR_SUB;
	        gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toSUB);
		printk("[TSP] mxt_late_resume() :Sub TSP init		#############\n");
		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
		#ifdef CONFIG_READ_FROM_FILE
		else{
			mxt_download_config(data, MXT_BATT_CFG_NAME);
		}
		#endif
		calibrate_chip_e();
	}

/******************************************************/
/*	  One TSP has to enter suspend mode					    */
/******************************************************/
		samsung_switching_tsp_suspend();

/******************************************************/
		treat_median_error_status = 0;

#else
		if (data->read_ta_status) {
			data->read_ta_status(&ta_status);
			pr_info("ta_status is %d\n", ta_status);
			mxt_ta_probe(ta_status);
		}
#ifdef CONFIG_READ_FROM_FILE
		else{
			mxt_download_config(data, MXT_BATT_CFG_NAME);
		}
#endif
		treat_median_error_status = 0;
		calibrate_chip_e();
#endif
		enable_irq(data->client->irq);
		pr_info("[TSP]        +%s\n", __func__);
	} else
		pr_err("%s. but touch already on\n", __func__);
}

#else
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	mxt_enabled = 0;
	touch_is_pressed = 0;
#if TOUCH_BOOSTER
	tsp_press_status = 0;
#endif
	return mxt_internal_suspend(data);
}

static int mxt_resume(struct device *dev)
{
	int ret = 0;
	bool ta_status = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	ret = mxt_internal_resume(data);

	mxt_enabled = 1;

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		pr_info("ta_status is %d\n", ta_status);
		mxt_ta_probe(ta_status);
	}
#ifdef CONFIG_READ_FROM_FILE
	else{
		mxt_download_config(data, MXT_BATT_CFG_NAME);
	}
#endif
	return ret;
}
#endif

#if FORCE_RELEASE
static void Mxt_force_released(void)
{
	struct mxt_data *data = copy_data;
	int i;

	if (!mxt_enabled) {
		pr_err("mxt_enabled is 0\n");
		return;
	}

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
	}
	report_input_data(data);

	calibrate_chip_e();
};
#endif

#if SYSFS
static ssize_t mxt_debug_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	g_debug_switch = !g_debug_switch;
	return 0;
}

static ssize_t mxt_object_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	unsigned int object_type;
	unsigned int object_register;
	unsigned int register_value;
	u8 value;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	sscanf(buf, "%u%u%u", &object_type, &object_register, &register_value);
	pr_info("object type T%d", object_type);
	pr_info("object register ->Byte%d\n", object_register);
	pr_info("register value %d\n", register_value);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	if (ret) {
		pr_err("fail to get object_info\n");
		return count;
	}

	size = 1;
	value = (u8)register_value;
	write_mem(data, address+(u16)object_register, size, &value);
	read_mem(data, address+(u16)object_register, (u8)size, &val);

	pr_info("T%d Byte%d is %d\n",
		object_type, object_register, val);
	return count;
}

static ssize_t mxt_object_show(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	unsigned int object_type;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	u16 i;
	sscanf(buf, "%u", &object_type);
	pr_info("object type T%d\n", object_type);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	if (ret) {
		pr_err("fail to get object_info\n");
		return count;
	}
	for (i = 0; i < size; i++) {
		read_mem(data, address+i, 1, &val);
		pr_info("Byte %u --> %u\n", i, val);
	}
	return count;
}
static unsigned char test_node[5] = {15, 23, 145, 239, 247};
static int index_reference;
static int index_delta;

static void diagnostic_chip(u8 mode)
{
	int error;
	u16 t6_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	u16 t37_address = 0;

	ret = get_object_info(copy_data,
		GEN_COMMANDPROCESSOR_T6, &size_one, &t6_address);

	size_one = 1;
	error = write_mem(copy_data, t6_address+5, (u8)size_one, &mode);
	/* QT602240_COMMAND_DIAGNOSTIC, mode); */
	if (error < 0) {
		pr_err("error %s: write_object\n", __func__);
	} else {
		get_object_info(copy_data,
			DEBUG_DIAGNOSTIC_T37, &size_one, &t37_address);
		size_one = 1;
		/* pr_info("diagnostic_chip setting success\n"); */
		read_mem(copy_data, t37_address, (u8)size_one, &value);
		/* pr_info("dianostic_chip mode is %d\n",value); */
	}
}

static uint8_t read_uint16_t(struct mxt_data *data, uint16_t address, uint16_t *buf)
{
	uint8_t status;
	uint8_t temp[2];

	status = read_mem(data, address, 2, temp);
	*buf = ((uint16_t)temp[1]<<8) + (uint16_t)temp[0];

	return status;
}

static void read_dbg_data(uint8_t dbg_mode , uint16_t node, uint16_t *dbg_data)
{
	u8 read_page, read_point;
	uint8_t mode, page;
	u16 size;
	u16 diagnostic_addr = 0;

	if (!mxt_enabled) {
		pr_err("read_dbg_data. mxt_enabled is 0\n");
		return;
	}

	get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &diagnostic_addr);

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(20);

	do {
		if (read_mem(copy_data, diagnostic_addr, 1, &mode)) {
			pr_info("READ_MEM_FAILED\n");
			return;
		}
	} while (mode != MXT_CTE_MODE);

	diagnostic_chip(dbg_mode);
	msleep(20);

	do {
		if (read_mem(copy_data, diagnostic_addr, 1, &mode)) {
			pr_info("READ_MEM_FAILED\n");
			return;
		}
	} while (mode != dbg_mode);

	for (page = 1; page <= read_page; page++) {
		diagnostic_chip(MXT_PAGE_UP);
		msleep(20);
		do {
			if (read_mem(copy_data,
				diagnostic_addr + 1, 1, &mode)) {
				pr_info("READ_MEM_FAILED\n");
				return;
			}
		} while (mode != page);
	}

	if (read_uint16_t(copy_data, diagnostic_addr + read_point, dbg_data)) {
		pr_info("READ_MEM_FAILED\n");
		return;
	}
}


int read_all_data(uint16_t dbg_mode)
{
	struct mxt_data *data = copy_data;
	struct mxt_data_sysfs *sysfs_data = data->sysfs_data;
	u8 read_page, read_point;
	u16 max_value = MIN_VALUE, min_value = MAX_VALUE;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;
	u8 val;
	bool dual_x_mode = 0;

	ret = get_object_info(copy_data, PROCG_NOISESUPPRESSION_T62, &size, &object_address);
	read_mem(copy_data, object_address + 3, 1, &val);
	if (val & 0x10)
		dual_x_mode = 1;

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &object_address);
	/*jerry no need to leave it */
	msleep(50); /* msleep(20);  */

	for (read_page = 0 ; read_page < 6; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data, object_address + (u16)read_point, 2, data_buffer);
			sysfs_data->reference[num] = ((u16)data_buffer[1]<<8)	+ (u16)data_buffer[0];

			if ((sysfs_data->reference[num] > MIN_VALUE) || (sysfs_data->reference[num] < MAX_VALUE)) {
				state = 1;
				printk(KERN_ERR
					"[TSP] mxt_refer_node[%3d] = %5d\n",
					num, sysfs_data->reference[num]);
				}

				if (data_buffer[0] != 0) {
					if (sysfs_data->reference[num] != 0) {
						if (sysfs_data->reference[num] > max_value)
							max_value = sysfs_data->reference[num];
						if (sysfs_data->reference[num] < min_value)
							min_value = sysfs_data->reference[num];
					}
				}
			if (num > 250)
				goto out;
			num++;
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(20);
	}

out:
	if ((max_value - min_value) > 4000) {
		printk(KERN_ERR
			"[TSP] diff = %d, max_value = %d, min_value = %d\n",
			(max_value - min_value), max_value, min_value);
		state = 1;
	}

	sysfs_data->ref_max_data = max_value;
	sysfs_data->ref_min_data = min_value;
	return !state;
}

int read_all_delta_data(uint16_t dbg_mode)
{
	struct mxt_data *data = copy_data;
	struct mxt_data_sysfs *sysfs_data = data->sysfs_data;
	u8 read_page, read_point;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;

	if (!mxt_enabled) {
		pr_err("%s : mxt_enabled is 0\n", __func__);
		return 1;
	}

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &object_address);
/*jerry no need to leave it */
/*
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
*/
	msleep(50); /* msleep(20);  */


	/* 768/64 */
	for (read_page = 0 ; read_page < 6; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data,
				object_address + (u16)read_point, 2, data_buffer);
				sysfs_data->delta[num] = ((uint16_t)data_buffer[1]<<8)
					+ (uint16_t)data_buffer[0];
					printk(KERN_ERR
						"[TSP] mxt_delta_node[%3d] = %5d\n",
						num, sysfs_data->delta[num]);

			if (abs(sysfs_data->delta[num]) > abs(sysfs_data->delta_max_data)) {
				sysfs_data->delta_max_node = num;
				sysfs_data->delta_max_data = sysfs_data->delta[num];
			}
			num++;

			/* all node => 24 * 32 = 768 => (12page * 64) */
			/*if ((read_page == 11) && (node == 64))
				break;*/
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(35);
	}

	return state;
}

int find_channel(uint16_t dbg_mode)
{
	u8 read_page, read_point;
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };
	u8 node = 0;
	int state = 0;
	int num = 0;
	int ret;
	u16 size;
	u16 delta_val = 0;
	u16 max_val = 0;

	if (!mxt_enabled) {
		pr_err("%s : mxt_enabled is 0\n", __func__);
		return 1;
	}

	/* Page Num Clear */
	diagnostic_chip(MXT_CTE_MODE);
	msleep(30);/* msleep(20);  */

	diagnostic_chip(dbg_mode);
	msleep(30);/* msleep(20);  */

	ret = get_object_info(copy_data,
		DEBUG_DIAGNOSTIC_T37, &size, &object_address);
/*jerry no need to leave it */
/*
	for (i = 0; i < 5; i++) {
		if (data_buffer[0] == dbg_mode)
			break;

		msleep(20);
	}
*/
	msleep(50); /* msleep(20);  */


	/* 768/64 */
	for (read_page = 0 ; read_page < 12; read_page++) {
		for (node = 0; node < 64; node++) {
			read_point = (node * 2) + 2;
			read_mem(copy_data,
				object_address+(u16)read_point, 2, data_buffer);
				delta_val = ((uint16_t)data_buffer[1]<<8)
					+ (uint16_t)data_buffer[0];

				if (delta_val > 32767)
					delta_val = 65535 - delta_val;
				if (delta_val > max_val) {
					max_val = delta_val;
					state = (read_point - 2)/2 +
						(read_page * 64);
				}

			num++;

			/* all node => 24 * 32 = 768 => (12page * 64) */
			/*if ((read_page == 11) && (node == 64))
				break;*/
		}
		diagnostic_chip(MXT_PAGE_UP);
		msleep(35);
	}

	return state;
}

static ssize_t find_channel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int status = 0;

	status = find_channel(MXT_DELTA_MODE);

	return sprintf(buf, "%u\n", status);
}

static struct device *sec_touchscreen;
static struct device *mxt_noise_test;
/*
	top_left, top_right, center, bottom_left, bottom_right
*/

#endif
static int mxt_check_bootloader(struct i2c_client *client,
					unsigned int state)
{
	u8 val;
	u8 temp;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1)
		return -EIO;

	if (val & 0x20)	{
		if (i2c_master_recv(client, &temp, 1) != 1)
			return -EIO;

		if (i2c_master_recv(client, &temp, 1) != 1)
			return -EIO;
		val &= ~0x20;
	}

	if ((val & 0xF0) == MXT_APP_CRC_FAIL) {
		pr_info("MXT_APP_CRC_FAIL\n");
		if (i2c_master_recv(client, &val, 1) != 1)
			return -EIO;

		if (val & 0x20) {
			if (i2c_master_recv(client, &temp, 1) != 1)
				return -EIO;

			if (i2c_master_recv(client, &temp, 1) != 1)
				return -EIO;
			val &= ~0x20;
		}
	}

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
	case MXT_WAITING_FRAME_DATA:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		pr_err("Unvalid bootloader mode state\n");
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		pr_err("%s: i2c send failed\n",
			__func__);

		return -EIO;
	}

	return 0;
}

static int mxt_fw_write(struct i2c_client *client,
				const u8 *data, unsigned int frame_size)
{
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		pr_err("%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_load_fw(struct mxt_data *dev, const char *fn)
{
	struct mxt_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	u16 obj_address = 0;
	u16 size_one;
	u8 value;
	unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;

	pr_info("mxt_load_fw start from header!!!\n");
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_mXT;
	fw->size = sizeof(firmware_mXT);
#else
	const struct firmware *fw = NULL;

	pr_info("mxt_load_fw startl!!!\n");
	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		pr_err("Unable to open firmware %s\n", fn);
		return ret;
	}
#endif
	/* Change to the bootloader mode */
	object_register = 0;
	value = (u8)MXT_BOOT_VALUE;
	ret = get_object_info(data,
		GEN_COMMANDPROCESSOR_T6, &size_one, &obj_address);
	if (ret) {
		pr_err("fail to get object_info\n");
		release_firmware(fw);
		return ret;
	}
	size_one = 1;
	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);
	msleep(MXT_SW_RESET_TIME);

	/* Change to slave address of bootloader */
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < fw->size) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			check_wating_frame_data_error++;
			if (check_wating_frame_data_error > 10) {
				pr_err("firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				pr_err("check_wating_frame_data_error = %d, retry\n",
					check_wating_frame_data_error);
				continue;
			}
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_err("check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("Updated %d bytes / %zd bytes\n",
			pos, fw->size);

		msleep(20);
	}

out:
#if READ_FW_FROM_HEADER
	kfree(fw);
#else
	release_firmware(fw);
#endif
	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	return ret;
}

static int mxt_load_fw_bootmode(struct device *dev, const char *fn)
{
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;

	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;
	pr_info("mxt_load_fw start from header!!!\n");
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_mXT;
	fw->size = sizeof(firmware_mXT);
#else
	const struct firmware *fw = NULL;
	pr_info("mxt_load_fw start!!!\n");

	ret = request_firmware(&fw, fn, &client->dev);
	if (ret) {
		pr_err("Unable to open firmware %s\n", fn);
		return ret;
	}
#endif
	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < fw->size) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			check_wating_frame_data_error++;
			if (check_wating_frame_data_error > 10) {
				pr_err("firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				pr_err("check_wating_frame_data_error = %d, retry\n",
					check_wating_frame_data_error);
				continue;
			}
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_err("check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("Updated %d bytes / %zd bytes\n",
			pos, fw->size);

		msleep(20);
	}

out:
#if READ_FW_FROM_HEADER
	kfree(fw);
#else
	release_firmware(fw);
#endif
	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	return ret;
}


#if FOR_DEBUGGING_TEST_DOWNLOADFW_BIN
#include <linux/uaccess.h>

#define MXT_FW_BIN_NAME "/sdcard/mxt.bin"

static int mxt_download(const u8 *pBianry, const u32 unLength)
{
	struct mxt_data *data = copy_data;
	struct i2c_client *client = copy_data->client;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	u16 obj_address = 0;
	u16 size_one;
	u8 value;
	unsigned int object_register;
	int check_frame_crc_error = 0;
	int check_wating_frame_data_error = 0;

	pr_info("mxt_download start!!!\n");

	/* Change to the bootloader mode */
	object_register = 0;
	value = (u8)MXT_BOOT_VALUE;
	ret = get_object_info(data,
		GEN_COMMANDPROCESSOR_T6, &size_one, &obj_address);
	if (ret) {
		pr_err("fail to get object_info\n");
		return ret;
	}
	size_one = 1;

	write_mem(data, obj_address+(u16)object_register, (u8)size_one, &value);
	msleep(MXT_SW_RESET_TIME);

	/* Change to slave address of bootloader */
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;
	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);

	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < unLength) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			check_wating_frame_data_error++;
			if (check_wating_frame_data_error > 10) {
				pr_err("firm update fail. wating_frame_data err\n");
				goto out;
			} else {
				pr_info("check_wating_frame_data_error=%d, retry\n",
					check_wating_frame_data_error);
				continue;
			}
		}

		frame_size = ((*(pBianry + pos) << 8) | *(pBianry + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, pBianry + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			check_frame_crc_error++;
			if (check_frame_crc_error > 10) {
				pr_err("firm update fail. frame_crc err\n");
				goto out;
			} else {
				pr_info("check_frame_crc_error = %d, retry\n",
					check_frame_crc_error);
				continue;
			}
		}

		pos += frame_size;

		pr_info("Updated %d bytes / %zd bytes\n", pos, unLength);

		msleep(20);
	}

out:
	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	return ret;
}

int mxt_binfile_download(void)
{
	int nRet = 0;
	int retry_cnt = 0;
	long fw1_size = 0;
	unsigned char *fw_data1;
	struct file *filp;
	loff_t	pos;
	int	ret = 0;
	mm_segment_t oldfs;
	spinlock_t	lock;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(MXT_FW_BIN_NAME, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("file open error:%d\n", (s32)filp);
		return -1;
	}

	fw1_size = filp->f_path.dentry->d_inode->i_size;
	pr_info("Size of the file : %ld(bytes)\n", fw1_size);

	fw_data1 = kmalloc(fw1_size, GFP_KERNEL);
	memset(fw_data1, 0, fw1_size);

	pos = 0;
	memset(fw_data1, 0, fw1_size);
	ret = vfs_read(filp, (char __user *)fw_data1, fw1_size, &pos);

	if (ret != fw1_size) {
		pr_err("Failed to read file %s (ret = %d)\n",
			MXT_FW_BIN_NAME, ret);
		kfree(fw_data1);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(oldfs);

	for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
		pr_info("ADB - MASTER CHIP Firmware update! try : %d",
			retry_cnt+1);
		nRet = mxt_download((const u8 *)fw_data1, (const u32)fw1_size);
		if (nRet)
			continue;
		break;
	}

	kfree(fw_data1);
	return nRet;
}
#endif

int set_mxt_firm_update_store(struct mxt_data *data, const char *buf, size_t size)
{
	u8 **tsp_config = (u8 **)data->pdata->config;
	int i = 0;
	int error = 1;
	pr_info("set_mxt_update_show start!!\n");

	disable_irq(data->client->irq);
	firm_status_data = 1;
#if FOR_DEBUGGING_TEST_DOWNLOADFW_BIN
	error = mxt_binfile_download();
#else
	if (data->tsp_version >= firmware_latest[0]
		&& data->tsp_build >= firmware_latest[1]) {
		pr_err("latest firmware\n");
		firm_status_data = 2;
		enable_irq(data->client->irq);
		return error;
	}
	pr_info("fm_update\n");
	error = mxt_load_fw(data, MXT_FW_NAME);
#endif

	if (error) {
		firm_status_data = 3;
		pr_err("The firmware update failed(%d)\n", error);
		return error;
	} else {
		firm_status_data = 2;
		pr_info("The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT_SW_RESET_TIME);

		mxt_init_touch_driver(data);
		/* mxt224_initialize(data); */
	}

	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
		error = init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
		if (error) {
			pr_err("init_write_config error\n");
			firm_status_data = 3;
			return error;
		}
	}

	error = mxt_backup(data);
	if (error) {
		pr_err("mxt_backup fail!!!\n");
		return error;
	}

	/* reset the touch IC. */
	error = mxt_reset(data);
	if (error) {
		pr_err("mxt_reset fail!!!\n");
		return error;
	}

	msleep(MXT_SW_RESET_TIME);
	enable_irq(data->client->irq);

	return 0;
}

static ssize_t set_mxt_firm_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

	int count;
	pr_info("Enter firmware_status_show by Factory command\n");

	if (firm_status_data == 1)
		count = sprintf(buf, "DOWNLOADING\n");
	else if (firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");

	return count;
}

static int __devinit mxt_init_config(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	int i;
	bool ta_status = 0;
	u16 size;
	u16 obj_address = 0;

	u8 **tsp_config;

	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret >= 0) {
		pr_info("boot mode. firm update excute\n");
		mxt_load_fw_bootmode(NULL, MXT_FW_NAME);
		msleep(MXT_SW_RESET_TIME);
	} else {
		if (client->addr == MXT_BOOT_LOW)
			client->addr = MXT_APP_LOW;
		else
			client->addr = MXT_APP_HIGH;
	}

	ret = mxt_init_touch_driver(data);

	if (ret) {
		pr_err("chip initialization failed\n");
		goto err_init_drv;
	}

	/* tsp_family_id - 0x82 : MXT224S series */
	if (data->family_id == 0x82) {
		tsp_config = (u8 **)data->pdata->config;

#if DUAL_TSP
	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
	if (tsp_config[i][0] == GEN_POWERCONFIG_T7)
		data->power_cfg = tsp_config[i] + 1;
}
#endif
#if !(FOR_BRINGUP)
	data->t48_config_batt = pdata->t48_config_batt;
	data->t48_config_chrg = pdata->t48_config_chrg;
	data->tchthr_batt = pdata->tchthr_batt;
	data->tchthr_charging = pdata->tchthr_charging;
	data->calcfg_batt = pdata->calcfg_batt;
	data->calcfg_charging = pdata->calcfg_charging;
#endif
#if UPDATE_ON_PROBE
#if !(FOR_DEBUGGING_TEST_DOWNLOADFW_BIN)
	if (data->tsp_version < firmware_latest[0]
		|| (data->tsp_version == firmware_latest[0]
			&& data->tsp_build != firmware_latest[1])) {
		pr_info("force firmware update\n");
		if (mxt_load_fw(NULL, MXT_FW_NAME))
			goto err_config;
		else {
			msleep(MXT_SW_RESET_TIME);
			mxt_init_touch_driver(data);
		}
	}
#endif
#endif
	} else {
		pr_err("ERROR : There is no valid TSP ID\n");
		goto err_config;
	}

  // Read USER DATA[0] for Enable&Disable to write configuration
	get_object_info(data, SPT_USERDATA_T38, &size, &obj_address);
	read_mem(data, obj_address + 0, 1, &data->disable_config_write);


	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
#if FOR_DEBUGGING_TEST_DOWNLOADFW_BIN
	if (data->disable_config_write == 0)
		ret = init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
	else
		ret = 0;
#else
	ret = init_write_config(data, tsp_config[i][0],
						tsp_config[i] + 1);
#endif
	/*12/03/29 Temporary set as comment*/
	/*if (ret)
		goto err_config;*/

	if (tsp_config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
		/* Are x and y inverted? */
		if (tsp_config[i][10] & 0x1) {
			data->x_dropbits =
				(!(tsp_config[i][22] & 0xC)) << 1;
			data->y_dropbits =
				(!(tsp_config[i][20] & 0xC)) << 1;
		} else {
			data->x_dropbits =
				(!(tsp_config[i][20] & 0xC)) << 1;
			data->y_dropbits =
				(!(tsp_config[i][22] & 0xC)) << 1;
			}
		}
	}
	ret = mxt_backup(data);
	if (ret)
		goto err_backup;

	/* reset the touch IC. */
	ret = mxt_reset(data);
	if (ret)
		goto err_reset;

	msleep(MXT_SW_RESET_TIME);

	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		pr_info("ta_status is %d\n", ta_status);
		mxt_ta_probe(ta_status);
	}
#ifdef CONFIG_READ_FROM_FILE
	else{
		mxt_download_config(data, MXT_BATT_CFG_NAME);
	}
#endif
	calibrate_chip_e();

	return 0;

	err_reset:
		pr_info("mxt ierr_reset \n");
	err_backup:
		pr_info("mxt err_reset \n");
	err_config:
		kfree(data->objects);
		pr_info("mxt err_config \n");
	err_init_drv:
		gpio_free(data->gpio_read_done);
		pr_info("mxt err_init_drv \n");

	return ret;

}

static int __devinit mxt_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct mxt224s_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	int ret;
	int i=0;


	pr_info("%s +++\n", __func__);

	touch_is_pressed = 0;
#if TOUCH_BOOSTER
	tsp_press_status = 0;
#endif
#if DUAL_TSP
	Flip_status_tsp = FLIP_NOTINIT;
	Tsp_current_addr = MXT224S_ADDR_MAIN;
#endif

	if (!pdata) {
		pr_err("missing platform data\n");
		return -ENODEV;
	}

	if (pdata->max_finger_touches <= 0)
		return -EINVAL;

	data = kzalloc(sizeof(*data) + pdata->max_finger_touches *
					sizeof(*data->fingers), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->pdata = pdata;
	data->num_fingers = pdata->max_finger_touches;
	data->power_on = pdata->power_on;
	data->power_off = pdata->power_off;
#if 1 //!(FOR_BRINGUP)
	data->register_cb = pdata->register_cb;
#endif
	data->read_ta_status = pdata->read_ta_status;
	mutex_init(&data->lock);
	data->client = client;
	i2c_set_clientdata(client, data);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		pr_err("input device allocation failed\n");
		goto err_alloc_dev;
	}
	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "sec_touchscreen";

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(MT_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, MAX_USING_FINGER_NUM);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->min_x,
			     pdata->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->min_y,
			     pdata->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, pdata->min_z,
			     pdata->max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, pdata->min_w,
			     pdata->max_w, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_SUMSIZE, 0, 16 * 26, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_COMPONENT, 0, 255, 0, 0);
	ret = input_register_device(input_dev);
	if (ret) {
		input_free_device(input_dev);
		goto err_reg_dev;
	}

	data->gpio_read_done = pdata->gpio_read_done;

	data->power_on();

	copy_data = data;
#if 1//!(FOR_BRINGUP)
	data->register_cb(mxt_ta_probe);
#endif

#if DUAL_TSP
	Tsp_main_initialized = 0;
	Tsp_sub_initialized = 0;
	mxt_enabled = 0;
	Tsp_probe_passed  = 0;
/******************************************************/
/*              Main TSP init                                                                      */
/******************************************************/
	Tsp_current_addr = MXT224S_ADDR_MAIN;
	gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toMAIN);
	copy_data->client->addr = Tsp_current_addr;
	printk("[TSP]mxt_probe() :  Main TSP init    #############  \n");
	ret = mxt_init_config(data);
	if (ret) {
		pr_err("[TSP] chip config initialization failed\n");
		return ret;
	}
	Tsp_main_initialized = 1;
/******************************************************/
/*				Sub TSP init						           */
/******************************************************/
	Tsp_current_addr = MXT224S_ADDR_SUB;
	gpio_set_value(GPIO_TSP_SEL, TSP_SEL_toSUB);
	copy_data->client->addr = Tsp_current_addr;
	printk("[TSP]mxt_probe() :   Sub TSP init    #############  \n");
	ret = mxt_init_config(data);
	if (ret) {
		pr_err("[TSP] chip config initialization failed\n");
		return ret;
	}
	Tsp_sub_initialized = 1;
/******************************************************/
/*	  One TSP has to enter suspend mode					    */
/******************************************************/
	/* In flip module, 1st flip-value-scan will be  executed precisely.*/
	/* Then, samsung_switching_tsp() will be called... */

/******************************************************/

#else
	ret = mxt_init_config(data);
	if (ret) {
		pr_err("[TSP] chip config initialization failed\n");
		return ret;
	}
#endif

	for (i = 0; i < data->num_fingers; i++)
		data->fingers[i].state = MXT_STATE_INACTIVE;

	ret = request_threaded_irq(client->irq, NULL, mxt_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt_ts", data);

	if (ret < 0)
		goto err_irq;

	ret = mxt_sysfs_init(client);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to creat sysfs\n");
		goto err_free_mem;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	mxt_enabled = 1;
	Tsp_probe_passed  = 1;
#if TOUCH_BOOSTER
		INIT_DELAYED_WORK(&data->dvfs_dwork,
			mxt_set_dvfs_off);
#endif


	return 0;
err_free_mem:
	kfree(data->objects);
	kfree(data->rid_map);
err_irq:
	pr_info("mxt err_irq \n");
err_init_drv:
	gpio_free(data->gpio_read_done);
	pr_info("mxt err_init_drv \n");
/* err_gpio_req:
	data->power_off();
	input_unregister_device(input_dev); */
err_reg_dev:
	pr_info("mxt err_reg_dev \n");

err_alloc_dev:
	pr_info("mxt err_alloc_dev \n");
	kfree(data);
	return ret;
}

#ifdef CONFIG_READ_FROM_FILE
#if DOWNLOAD_CONFIG
static struct object_t *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct object_t *object;
	int i;

	for (i = 0; i < data->objects_len; i++) {
		object = data->objects + i;
		if (object->object_type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type T%u\n", type);
	return NULL;
}

static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	u8 buf[3];

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	buf[2] = val;

	printk("[TSP] mxt_write_reg %d %d\n", reg, val);

	if (i2c_master_send(client, buf, 3) != 3) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

int mxt_download_config(struct mxt_data *data, const char *fn)
{
	struct device *dev = &data->client->dev;
	struct mxt_info cfg_info;
	struct object_t *object;
#ifdef CONFIG_READ_FROM_SDCARD
	struct firmware *cfg = NULL;
#else
	const struct firmware *cfg = NULL;
#endif
	int ret;
	int offset;
	loff_t pos;
	int i;
	unsigned long info_crc, config_crc;
	unsigned int type, instance, size, object_size, instance_size;
	u8 val;
	u16 reg;

#ifdef CONFIG_READ_FROM_SDCARD
	struct file *filp;
	long cfg_size = 0;
	unsigned char *cfg_data;
	mm_segment_t oldfs;
	
	oldfs = get_fs();
	set_fs(get_ds());	
	
	printk("[TSP] mxt_download_config %s\n", fn);
	
	filp = filp_open(fn, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("file open error:%d\n", (s32)filp);
		return -1;
	}
	
	cfg_size = filp->f_path.dentry->d_inode->i_size;
	pr_info("Size of the Cfg file : %ld(bytes)\n", cfg_size);

	cfg_data = kmalloc(cfg_size, GFP_KERNEL);
	memset(cfg_data, 0, cfg_size);

	pos = 0;
	ret = vfs_read(filp, (char __user *)cfg_data, cfg_size, &pos);

	if (ret != cfg_size) {
		pr_err("Failed to read Cfg file %s (ret = %d)\n",
			fn, ret);
		kfree(cfg_data);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(oldfs);
	
	//firmware struct
	cfg = kzalloc(sizeof(struct firmware), GFP_KERNEL);
	cfg->data = cfg_data;
	cfg->size = cfg_size;		
#else
	ret = request_firmware(&cfg, fn, dev);
	if (ret < 0) {
		dev_err(dev, "Failure to request config file %s\n", fn);
		return 0;
	}
#endif		

	if (strncmp(cfg->data, MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
		dev_err(dev, "Unrecognised config file\n");
		ret = -EINVAL;
		goto release;
	}

	pos = strlen(MXT_CFG_MAGIC);

	/* Load information block and check */
	for (i = 0; i < sizeof(struct mxt_info); i++) {
		ret = sscanf(cfg->data + pos, "%hhx%n",
			     (unsigned char *)&cfg_info + i,
			     &offset);
		if (ret != 1) {
			dev_err(dev, "Bad format\n");
			ret = -EINVAL;
		}

		pos += offset;
	}

	if (cfg_info.family_id != data->family_id) {
		dev_err(dev, "Family ID mismatch! %x %x\n", cfg_info.family_id, data->family_id);
		ret = -EINVAL;
	}

	if (cfg_info.variant_id != data->tsp_variant) {
		dev_err(dev, "Variant ID mismatch! %x %x\n", cfg_info.variant_id, data->tsp_variant);
		ret = -EINVAL;
	}

	if (cfg_info.version != data->tsp_version)
		dev_err(dev, "Warning: version mismatch! %x %x\n", cfg_info.version, data->tsp_version);

	if (cfg_info.build != data->tsp_build)
		dev_err(dev, "Warning: build num mismatch! %x %x\n", cfg_info.build, data->tsp_build);

	ret = sscanf(cfg->data + pos, "%lx%n", &info_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format\n");
		ret = -EINVAL;
	}
	pos += offset;

	/* Check config CRC */
	ret = sscanf(cfg->data + pos, "%lx%n", &config_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format\n");
		ret = -EINVAL;
	}
	pos += offset;

	while (pos < cfg->size) {
		/* Read type, instance, length */
		ret = sscanf(cfg->data + pos, "%x %x %x%n",
			     &type, &instance, &size, &offset);
		if (ret == 0) {
			/* EOF */
			ret = 1;
			goto release;
		} else if (ret < 0) {
			dev_err(dev, "Bad format\n");
			ret = -EINVAL;
			goto release;
		}
		pos += offset;

		object = mxt_get_object(data, type);
		if (!object) {
			ret = -EINVAL;
			goto release;
		}

		object_size = object->size+1;
		instance_size = object->instances+1;

		if (size > object_size) {
			dev_err(dev, "Object length exceeded!\n");
			ret = -EINVAL;
			goto release;
		}

		if (instance >= instance_size) {
			dev_err(dev, "Object instances exceeded!\n");
			ret = -EINVAL;
			goto release;
		}

		reg = object->i2c_address + object_size * instance;

		for (i = 0; i < size; i++) {
			ret = sscanf(cfg->data + pos, "%hhx%n",
				     &val,
				     &offset);
			if (ret != 1) {
				dev_err(dev, "Bad format\n");
				ret = -EINVAL;
				goto release;
			}

			ret = mxt_write_reg(data->client, reg + i, val);
			if (ret)
				goto release;

			pos += offset;
		}

		/* If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated */
		if (size < object_size) {
			dev_info(dev, "Warning: zeroing %d byte(s) in T%d\n",
				 object->size - size, type);

			for (i = size + 1; i < object_size; i++) {
				ret = mxt_write_reg(data->client, reg + i, 0);
				if (ret)
					goto release;
			}
		}
	}

release:
	
#ifdef CONFIG_READ_FROM_SDCARD
	kfree(cfg);	
	kfree(cfg_data);
#else
	release_firmware(cfg);
#endif	
	return ret;
}

#endif
#endif

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	kfree(data->objects);
	gpio_free(data->gpio_read_done);
	data->power_off();
	input_unregister_device(data->input_dev);
	kfree(data);

	return 0;
}

static struct i2c_device_id mxt_idtable[] = {
	{MXT_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static const struct dev_pm_ops mxt_pm_ops = {
	.suspend = mxt_suspend,
	.resume = mxt_resume,
};

static struct i2c_driver mxt_i2c_driver = {
	.id_table = mxt_idtable,
	.probe = mxt_probe,
	.remove = __devexit_p(mxt_remove),
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MXT_DEV_NAME,
		.pm	= &mxt_pm_ops,
	},
};

static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_i2c_driver);
}

static void __exit mxt_exit(void)
{
	i2c_del_driver(&mxt_i2c_driver);
}
module_init(mxt_init);
module_exit(mxt_exit);

MODULE_DESCRIPTION("Atmel MaXTouch driver");
MODULE_AUTHOR("ki_won.kim<ki_won.kim@samsung.com>");
MODULE_LICENSE("GPL");
