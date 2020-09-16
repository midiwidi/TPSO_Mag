#include <stdlib.h>
#include <tomlc99/toml.h>

#include "log.h"
#include "helpers.h"
#include "toml.h"
#include "data_processing.h"

void toml_clean_exit(int ret, FILE *toml_fp, toml_table_t *toml_conf, char *conf_file)
{
	if (toml_fp)
	{
		fclose(toml_fp);
		toml_fp = NULL;
	}

	if (conf_file)
	{
		free(conf_file);
		conf_file = NULL;
	}

	if (toml_conf)
	{
		toml_free(toml_conf);
		toml_conf = NULL;
	}

	clean_exit(ret);
}

int toml_readkey_int(toml_table_t* toml_tab, const char *key, int64_t *toml_int64)
{
	const char* toml_raw;

	/* extract key value */
	if (!(toml_raw = toml_raw_in(toml_tab, key)))
	{
		log_write(LOG_ERR, "can't find configuration key %s", key);
		return ERROR;
	}
	/* convert to integer value */
	if (toml_rtoi(toml_raw, toml_int64))
	{
		log_write(LOG_ERR, "toml_rtoi error");
		return ERROR;
	}
	return NO_ERROR;
}

int toml_readkey_dbl(toml_table_t* toml_tab, const char *key, double *output)
{
	const char* toml_raw;

	/* extract key value */
	if (!(toml_raw = toml_raw_in(toml_tab, key)))
	{
		log_write(LOG_ERR, "can't find configuration key %s", key);
		return ERROR;
	}
	/* convert to double value */
	if (toml_rtod(toml_raw, output))
	{
		log_write(LOG_ERR, "toml_rtod error");
		return ERROR;
	}
	return NO_ERROR;
}

int toml_readkey_str(toml_table_t* toml_tab, const char *key, char **str)
{
	const char* toml_raw;

	/* extract key value */
	if (!(toml_raw = toml_raw_in(toml_tab, key)))
	{
		log_write(LOG_ERR, "can't find configuration key %s", key);
		return ERROR;
	}
	/* convert to string */
	if (toml_rtos(toml_raw, str))
	{
		log_write(LOG_ERR, "toml_rtos error");
		return ERROR;
	}
	return NO_ERROR;
}

int read_config(char* conf_file, struct config *cfg)
{
	FILE* toml_fp = NULL;
	char toml_errbuf[200];
	toml_table_t* toml_conf = NULL;
	toml_table_t* toml_tab = NULL;

	int64_t toml_int64;
	uint8_t ui8_tmp;
	uint16_t ui16_tmp;
	double dbl_tmp;
	int ret = NO_ERROR;

	if (!(toml_fp = fopen(conf_file, "r")))
	{
		// open conf file specified via parameter failed
		if (!(toml_fp = fopen(DEFAULT_CONFIG_FILE, "r")))
		{
			// if no config file was specified and the default config file couldn't be found,
			// then initialize config with default values
			cfg->avg_N_bfield = 6000;
			cfg->avg_N_hk = 240;
			cfg->downsample_mode = AVERAGING;
			cfg->loglevel = LOG_NOTICE;
			//cfg->offset //done at struct declaration
			//cfg->orth = //done at struct declaration
			//cfg->scale = //done at struct declaration
			//cfg->telemetry_resolution = //done at struct declaration
			cfg->timestamp_position = TIMESTAMP_AT_CENTER;


			//asprintf(&cfg->device, "%s", DEFAULT_BLOCK_DEVICE);


			if (conf_file)
					free(conf_file);
			return NO_ERROR;
		}
		else
			// open default conf file succeeded
			log_write(LOG_INFO, "using default config file %s", DEFAULT_CONFIG_FILE);
	}
	else
		// open conf file specified via parameter succeeded
		log_write(LOG_INFO, "using config file %s", conf_file);

	if (conf_file)
		free(conf_file);

	toml_conf = toml_parse_file(toml_fp, toml_errbuf, sizeof(toml_errbuf));

	fclose(toml_fp);
	toml_fp = NULL;

	if (!toml_conf)
	{
		log_write(LOG_ERR, "error parsing the config file (%s)", toml_errbuf);
		ret = ERROR;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_int(toml_conf, "version_major", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->version_major = toml_int64;
	if((ret=toml_readkey_int(toml_conf, "version_minor", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->version_minor = toml_int64;

	// Reading the configuration from the *.toml file is divided into several blocks
	// This is done to make sure that the order in which keys are read is such that
	// keys which other keys depend on are read first before the key which depend on
	// other keys are read.

	/********************************************************************************/
	/* 1st Block: keys, which don't depend on other keys                            */
	/********************************************************************************/
	// locate the [Gateway] table
	if (!(toml_tab = toml_table_in(toml_conf, "Gateway")))
	{
		log_write(LOG_ERR, "can't find configuration table 'Gateway'");
		ret = ERROR;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_int(toml_tab, "width", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_width(toml_int64, &cfg->cam.num_cols)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_widthheight(toml_int64, "pixel per line", &cfg->core.pixels_per_line)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_widthheight(toml_int64, "test pattern generator width", &cfg->core.tptrngen_size_x)))
			toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "loglevel", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_loglevel(toml_int64, &cfg->loglevel)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	setlogmask(LOG_UPTO (cfg->loglevel));

	/* locate the [swir.camera] table */
	if (!(toml_tab = toml_table_in(toml_tab, "camera")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.camera");
		ret = EXIT_TOML_SERTAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_int(toml_tab, "baud", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_baud(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg0.bits.baud = ui8_tmp;
	if((ret=toml_readkey_int(toml_tab, "trigger_mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_trigger_mode(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg0.bits.trigger_mode = ui8_tmp;
	if((ret=toml_readkey_int(toml_tab, "shutter_mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_shutter_mode(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg0.bits.shutter_mode = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "integration_mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_integr_mode(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.integration_mode.bits.mode = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "gain", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_gain(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg2.bits.gain = ui8_tmp;
	if((ret=toml_readkey_int(toml_tab, "asm", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_asm(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg2.bits.as_mode = ui8_tmp;
	if((ret=toml_readkey_int(toml_tab, "cds", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_cds(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg2.bits.cds = ui8_tmp;
	if((ret=toml_readkey_int(toml_tab, "amp_mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_amp_mode(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.cfg2.bits.amp_mode = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "x_start", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_xstart(toml_int64, &cfg->cam.first_col)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "y_start", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_ystart(toml_int64, &cfg->cam.first_line)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "tec_mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_tec_mode(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.tec.bits.mode = ui8_tmp;
	if((ret=toml_readkey_int(toml_tab, "tec_temp", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_tec_temp(toml_int64, &ui16_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->cam.tec.bits.temp_low = ui16_tmp & 0x03;
	cfg->cam.tec.bits.temp_high = ui16_tmp >> 2 & 0x0F;

	if((ret=toml_readkey_int(toml_tab, "power_on_delay", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_pwrondelay(toml_int64, &cfg->pwron_delay)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	// locate the [Calibration] table
	if (!(toml_tab = toml_table_in(toml_conf, "Calibration")))
	{
		log_write(LOG_ERR, "can't find configuration table 'Calibration'");
		ret = EXIT_TOML_SWIRTAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}
	/* locate the [swir.core] table */
	if (!(toml_tab = toml_table_in(toml_tab, "core")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.core");
		ret = EXIT_TOML_CORETAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_str(toml_tab, "block_device", &cfg->device)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "fb_ovfl_handling_off", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_ovf_hndl(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->core.control.bits.fb_ovf_handling_off = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "arith_bit_discard", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_discard_bits(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->core.control.bits.discard_bits = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "sol_source", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_sol_src(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->core.control.bits.sol_source = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "data_source", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_data_src(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->core.control.bits.data_source = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "burst_length", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_burst_len(toml_int64, &cfg->core.burst_length)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "burst_period", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_burst_period(toml_int64, &cfg->core.burst_period)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_dbl(toml_tab, "tptrngen_pixel_clk", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_pclk(dbl_tmp, &cfg->core.tptrngen_pixel_clk)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "tptrngen_random_seed", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_seed(toml_int64, &cfg->tptrn_params.random_seed)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "tptrngen_checker_width", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_tile_size(toml_int64, &cfg->tptrn_params.checker_width)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if((ret=toml_readkey_int(toml_tab, "tptrngen_checker_height", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_tile_size(toml_int64, &cfg->tptrn_params.checker_height)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if((ret=toml_readkey_int(toml_tab, "tptrngen_checker_color2", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_color(toml_int64, &cfg->tptrn_params.checker_color2)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if((ret=toml_readkey_int(toml_tab, "tptrngen_checker_color1", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_color(toml_int64, &cfg->tptrn_params.checker_color1)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_dbl(toml_tab, "tptrngen_gradient_inc", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_grd_inc(dbl_tmp, &cfg->tptrn_params.gradient_inc)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if((ret=toml_readkey_int(toml_tab, "tptrngen_gradient_startcolor", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_color(toml_int64, &cfg->tptrn_params.gradient_startcolor)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if((ret=toml_readkey_int(toml_tab, "tptrngen_gradient_endcolor", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_color(toml_int64, &cfg->tptrn_params.gradient_endcolor)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "interrupt_mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_int_mode(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->core.interrupt_config.bits.mode = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "interrupt_nframes", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_int_nframes(toml_int64, &ui8_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	cfg->core.interrupt_config.bits.frame_count = ui8_tmp;

	if((ret=toml_readkey_int(toml_tab, "arith_limit", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_px_limit(toml_int64, &cfg->core.arith_limit)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "arith_offset", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_px_offset(toml_int64, &cfg->core.arith_offset)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	/********************************************************************************/
	/* 2nd Block: keys, which depend on keys read in previous block                 */
	/********************************************************************************/
	// locate the [swir] table
	if (!(toml_tab = toml_table_in(toml_conf, "swir")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.x");
		ret = EXIT_TOML_SWIRTAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_dbl(toml_tab, "fps", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_fps(dbl_tmp, cfg->cam.cfg0.bits.trigger_mode, &cfg->cam.fps)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_fps(dbl_tmp, cfg->cam.cfg0.bits.trigger_mode, &cfg->core.trigger_period)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	if((ret=toml_readkey_int(toml_tab, "height", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_height(toml_int64, &cfg->cam.num_lines)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_pixels_per_frame(toml_int64, cfg->core.pixels_per_line, &cfg->core.pixels_per_frame)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_widthheight(toml_int64, "test pattern generator height", &cfg->core.tptrngen_size_y)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	/* locate the [swir.camera] table */
	if (!(toml_tab = toml_table_in(toml_tab, "camera")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.camera");
		ret = EXIT_TOML_SERTAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_dbl(toml_tab, "trigger_delay", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_trigger_delay(dbl_tmp, cfg->cam.cfg0.bits.shutter_mode, cfg->cam.cfg0.bits.trigger_mode, cfg->cam.cfg2.bits.cds, &cfg->cam.trigger_delay)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	// locate the [swir] table
	if (!(toml_tab = toml_table_in(toml_conf, "swir")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.x");
		ret = EXIT_TOML_SWIRTAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}
	/* locate the [swir.core] table */
	if (!(toml_tab = toml_table_in(toml_tab, "core")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.core");
		ret = EXIT_TOML_CORETAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}

	if((ret=toml_readkey_dbl(toml_tab, "tptrngen_lval_low_duration", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_tptrngen_lval_low(dbl_tmp, cfg->core.tptrngen_pixel_clk, &cfg->core.tptrngen_lval_low)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	/********************************************************************************/
	/* 3nd Block: keys, which depend on keys read in previous block                 */
	/********************************************************************************/
	// locate the [swir] table
	if (!(toml_tab = toml_table_in(toml_conf, "swir")))
	{
		log_write(LOG_ERR, "can't find configuration table swir.x");
		ret = EXIT_TOML_SWIRTAB;
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	}
	if((ret=toml_readkey_dbl(toml_tab, "exposure_time", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_cam_exposure_time(dbl_tmp, cfg->cam.cfg0.bits.trigger_mode, &cfg->cam.exposure_time)))
		toml_clean_exit(ret, toml_fp, toml_conf, conf_file);
	if ((ret=cfg_conv_dn_core_exposure_time(dbl_tmp, cfg->cam.cfg0.bits.trigger_mode, cfg->core.trigger_period, &cfg->core.trigger_width)))
			toml_clean_exit(ret, toml_fp, toml_conf, conf_file);

	/* done with toml_conf */
	toml_free(toml_conf);
	toml_conf = NULL;

	return EXIT_NO_ERROR;
}
