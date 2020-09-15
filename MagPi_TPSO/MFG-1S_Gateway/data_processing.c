#include "helpers.h"
#include "globals.h"
#include "data_processing.h"
#include "log.h"

int process_cmd_reply_packet(uint8_t* packet)
{
	//log_write(LOG_ERR, "CMD reply packet");
	return NO_ERROR;
}

int process_HK_packet(struct hk_packet* packet)
{
	static struct hk_accu_t hk_accu = {.t_first=0, .t_last=0, .t_center=0,
									   .temp_e=0, .temp_s=0, .tilt_x=0, .tilt_y=0,
									   .V5p=0, .V5n=0, .V33=0, .V15=0,
									   .count=0};
	double temp_e, temp_s, tilt_x, tilt_y, V5p, V5n, V33, V15;

	temp_e = g_config.scale.temp_e * packet->temp_e + g_config.offset.temp_e;
	temp_s = g_config.scale.temp_s * packet->temp_s + g_config.offset.temp_s;
	tilt_x = g_config.scale.tilt_x * packet->tilt_x + g_config.offset.tilt_x;
	tilt_y = g_config.scale.tilt_y * packet->tilt_y + g_config.offset.tilt_y;
	V5p = g_config.scale.V5p * packet->V5p + g_config.offset.V5p;
	V5n = g_config.scale.V5n * packet->V5n + V5p * g_config.offset.V5n;
	V33 = g_config.scale.V33 * packet->V33 + g_config.offset.V33;
	V15 = g_config.scale.V15 * packet->V15 + g_config.offset.V15;

	if (g_config.avg_N_hk != 0)
	{
		if (hk_accu.count == 0)
		{
			hk_accu.t_first = packet->sec + packet->subsec / 65536.0;
			hk_accu.t_center = hk_accu.t_first;
			hk_accu.temp_e = temp_e;
			hk_accu.temp_s = temp_s;
			hk_accu.tilt_x = tilt_x;
			hk_accu.tilt_y = tilt_y;
			hk_accu.V5p = V5p;
			hk_accu.V5n = V5n;
			hk_accu.V33 = V33;
			hk_accu.V15 = V15;

			//firmware version, status, rd_ptr and wr_ptr is not averaged (last samples are taken)
		}
		else
		{
			hk_accu.t_center += packet->sec + packet->subsec / 65536.0;
			hk_accu.temp_e += temp_e;
			hk_accu.temp_s += temp_s;
			hk_accu.tilt_x += tilt_x;
			hk_accu.tilt_y += tilt_y;
			hk_accu.V5p += V5p;
			hk_accu.V5n += V5n;
			hk_accu.V33 += V33;
			hk_accu.V15 += V15;
			//firmware version, status, rd_ptr and wr_ptr is not averaged (last samples are taken)
		}
		hk_accu.count++;

		if (hk_accu.count >= g_config.avg_N_hk)
		{
			hk_accu.t_last = packet->sec + packet->subsec / 65536.0;
			hk_accu.t_center = hk_accu.t_center / hk_accu.count;

			switch(g_config.timestamp_position)
			{
				case TIMESTAMP_AT_FIRST_SAMPLE:
					g_hk_data.t = hk_accu.t_first;
					break;
				case TIMESTAMP_AT_CENTER:
					g_hk_data.t = hk_accu.t_center;
					break;
				case TIMESTAMP_AT_LAST_SAMPLE:
					g_hk_data.t = hk_accu.t_last;
					break;
			}
			g_hk_data.firmware_version = packet->firmware_version;
			g_hk_data.temp_e = hk_accu.temp_e / hk_accu.count;
			g_hk_data.temp_s = hk_accu.temp_s / hk_accu.count;
			g_hk_data.tilt_x = hk_accu.tilt_x / hk_accu.count;
			g_hk_data.tilt_y = hk_accu.tilt_y / hk_accu.count;
			g_hk_data.V5p = hk_accu.V5p / hk_accu.count;
			g_hk_data.V5n = hk_accu.V5n / hk_accu.count;
			g_hk_data.V33 = hk_accu.V33 / hk_accu.count;
			g_hk_data.V15 = hk_accu.V15 / hk_accu.count;
			g_hk_data.status.raw = packet->status.raw;
			g_hk_data.rd_ptr = packet->rd_ptr2 << 16 | packet->rd_ptr1 << 8 | packet->rd_ptr0;
			g_hk_data.wr_ptr = packet->wr_ptr2 << 16 | packet->wr_ptr1 << 8 | packet->wr_ptr0;
			hk_accu.count = 0;

			g_hk_data.updated = 1;
		}
	}
	else
		g_hk_data.updated = 0;

	return NO_ERROR;
}

int process_bfield_packet(struct bfield_packet* packet)
{
	static struct bfield_accu_t bfield_accu = {.t_first=0, .t_last=0, .t_center=0, .bx=0, .by=0, .bz=0, .count=0};
	int32_t bx_raw, by_raw, bz_raw;
	double bx, by, bz;

	bx_raw = extend_sign_bit( *((uint32_t*)&packet->bx0) & 0xFFFFFF, 24);
	by_raw = extend_sign_bit( *((uint32_t*)&packet->by0) & 0xFFFFFF, 24);
	bz_raw = extend_sign_bit( *((uint32_t*)&packet->bz0) & 0xFFFFFF, 24);

	bx = g_config.scale.x * g_config.telemetry_resolution * bx_raw + g_config.offset.x;
	by = g_config.scale.y * g_config.telemetry_resolution * by_raw + g_config.offset.y + g_config.telemetry_resolution * bx_raw * g_config.orth.xy;
	bz = g_config.scale.z * g_config.telemetry_resolution * bz_raw + g_config.offset.z + g_config.telemetry_resolution * bx_raw * g_config.orth.xz + g_config.telemetry_resolution * by_raw * g_config.orth.yz;

	if (g_config.avg_N_bfield != 0)
	{
		if (bfield_accu.count == 0)
		{
			bfield_accu.t_first = packet->sec + packet->subsec / 65536.0;
			bfield_accu.t_center = bfield_accu.t_first;
			bfield_accu.bx = bx;
			bfield_accu.by = by;
			bfield_accu.bz = bz;
			g_bfield_data.status.raw = packet->status.raw; //clear stick bits
		}
		else
		{
			bfield_accu.t_center += packet->sec + packet->subsec / 65536.0;
			bfield_accu.bx += bx;
			bfield_accu.by += by;
			bfield_accu.bz += bz;
			g_bfield_data.status.raw |= packet->status.raw; // make status bits sticky
		}
		bfield_accu.count++;

		if (bfield_accu.count >= g_config.avg_N_bfield)
		{
			bfield_accu.t_last = packet->sec + packet->subsec / 65536.0;
			bfield_accu.t_center = bfield_accu.t_center / bfield_accu.count;

			switch(g_config.timestamp_position)
			{
				case TIMESTAMP_AT_FIRST_SAMPLE:
					g_bfield_data.t = bfield_accu.t_first;
					break;
				case TIMESTAMP_AT_CENTER:
					g_bfield_data.t = bfield_accu.t_center;
					break;
				case TIMESTAMP_AT_LAST_SAMPLE:
					g_bfield_data.t = bfield_accu.t_last;
					break;
			}

			g_bfield_data.bx = bfield_accu.bx / bfield_accu.count;
			bfield_accu.bx = 0;
			g_bfield_data.by = bfield_accu.by / bfield_accu.count;
			bfield_accu.by = 0;
			g_bfield_data.bz = bfield_accu.bz / bfield_accu.count;
			bfield_accu.bz = 0;
			g_bfield_data.status.raw |= packet->status.raw; // make status bits sticky
			bfield_accu.count = 0;

			g_bfield_data.updated = 1;
		}
	}

	return NO_ERROR;
}

int process_massmem_packet(uint8_t* packet)
{
	//log_write(LOG_ERR, "massmem packet");
	return NO_ERROR;
}

int process_gps_packet(uint8_t* packet)
{
	//log_write(LOG_ERR, "GPS packet");
	return NO_ERROR;
}
