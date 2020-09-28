#!/bin/bash

## prior to the first execution create the necessary
## tables using the following commands

#CREATE TABLE bfield (
#	time TIMESTAMP NOT NULL,
#	bx DOUBLE PRECISION  NULL,
#	by DOUBLE PRECISION  NULL,
#	bz DOUBLE PRECISION  NULL,
#	clipx boolean NULL,
#	clipy boolean NULL,
#	clipz boolean NULL,
#	sync boolean NULL,
#	UNIQUE (time)
#);
#SELECT create_hypertable('bfield', 'time');
#GRANT SELECT ON public.bfield TO grafanareader;

#CREATE TABLE bfield_minmax (
#	time TIMESTAMP NOT NULL,
#	bx_min DOUBLE PRECISION  NULL,
#	by_min DOUBLE PRECISION  NULL,
#	bz_min DOUBLE PRECISION  NULL,
#	bx_max DOUBLE PRECISION  NULL,
#	by_max DOUBLE PRECISION  NULL,
#	bz_max DOUBLE PRECISION  NULL,
#	UNIQUE (time)
#);
#SELECT create_hypertable('bfield_minmax', 'time');
#GRANT SELECT ON public.bfield_minmax TO grafanareader;

#CREATE TABLE hk (
#	time TIMESTAMP NOT NULL,
#	mag_start boolean NULL,
#	excit_off boolean NULL,
#	recording boolean NULL,
#	flash_recording_disabled boolean NULL,
#	open_loop boolean NULL,
#	peo boolean NULL,
#	sync_off boolean NULL,
#	sync_src boolean NULL,
#	crc_ok boolean NULL,
#	boot_main boolean NULL,
#	eeprom_page smallint NULL,
#	rate smallint NULL,
#	dbg boolean NULL,
#	temp_e DOUBLE PRECISION  NULL,
#	temp_s DOUBLE PRECISION  NULL,
#	tilt_x DOUBLE PRECISION  NULL,
#	tilt_y DOUBLE PRECISION  NULL,
#	V5p DOUBLE PRECISION  NULL,
#	V5n DOUBLE PRECISION  NULL,
#	V33 DOUBLE PRECISION  NULL,
#	V15 DOUBLE PRECISION  NULL,
#	rd_ptr integer NULL,
#	wr_ptr integer NULL,
#	app_version integer NULL,
#	cfg_version real  NULL,
#	RISC_SW_version real  NULL,
#   time_drift DOUBLE PRECISION  NULL,
#	UNIQUE (time)
#);
#SELECT create_hypertable('hk', 'time');
#GRANT SELECT ON public.hk TO grafanareader;

#CREATE TABLE hk_minmax (
#	time TIMESTAMP NOT NULL,
#	temp_e_min DOUBLE PRECISION  NULL,
#	temp_s_min DOUBLE PRECISION  NULL,
#	tilt_x_min DOUBLE PRECISION  NULL,
#	tilt_y_min DOUBLE PRECISION  NULL,
#	V5p_min DOUBLE PRECISION  NULL,
#	V5n_min DOUBLE PRECISION  NULL,
#	V33_min DOUBLE PRECISION  NULL,
#	V15_min DOUBLE PRECISION  NULL,
#	temp_e_max DOUBLE PRECISION  NULL,
#	temp_s_max DOUBLE PRECISION  NULL,
#	tilt_x_max DOUBLE PRECISION  NULL,
#	tilt_y_max DOUBLE PRECISION  NULL,
#	V5p_max DOUBLE PRECISION  NULL,
#	V5n_max DOUBLE PRECISION  NULL,
#	V33_max DOUBLE PRECISION  NULL,
#	V15_max DOUBLE PRECISION  NULL,
#	UNIQUE (time)
#);
#SELECT create_hypertable('hk_minmax', 'time');
#GRANT SELECT ON public.hk_minmax TO grafanareader;

USER=postgres
DB=tpso_mag
BF_FILE=/home/pi/Projects/TPSO_Mag/data/mag.dat
HK_FILE=/home/pi/Projects/TPSO_Mag/data/hk.dat
BF_MINMAX_FILE=/home/pi/Projects/TPSO_Mag/data/mag.dat.minmax
HK_MINMAX_FILE=/home/pi/Projects/TPSO_Mag/data/hk.dat.minmax
TABLE_BFIELD=bfield
TABLE_BFIELD_MINMAX=bfield_minmax
TABLE_HK=hk
TABLE_HK_MINMAX=hk_minmax

if [[ -z "${TPSO_DB_PASS}" ]]; then
   logger "[pushdata] Environment variable TPSO_DB_PASS doesn't exist. Please create it using 'export TPSO_DB_PASS=mypass'. Replace mypass with the database password for the user postgres."
   exit
fi

if [ -f "$BF_FILE" ]; then
	if [ ! -f "$BF_FILE.push" ] && [ -f "$BF_FILE" ]; then
		mv $BF_FILE $BF_FILE.push
	else
		logger "[pushdata] $BF_FILE.push exists, indicating a missing server connection"
	fi

	PGPASSWORD=${TPSO_DB_PASS} psql -U $USER -h localhost $DB <<EOF
	
	CREATE TEMPORARY TABLE bfield_import (
		time TIMESTAMP NOT NULL,
		bx DOUBLE PRECISION NULL,
		by DOUBLE PRECISION NULL,
		bz DOUBLE PRECISION NULL,
		clipx boolean NULL,
		clipy boolean NULL,
		clipz boolean NULL,
		sync boolean NULL
	);
	
	\COPY bfield_import from '$BF_FILE.push' with delimiter E'\t' null as 'NULL';
	
	INSERT INTO $TABLE_BFIELD(time, bx, by, bz, clipx, clipy, clipz, sync)
	SELECT *
	FROM bfield_import ON conflict 
	DO NOTHING;
	
	DROP TABLE bfield_import;
	
EOF
	retval=$?
	if [ $retval -eq 0 ]; then
		rm $BF_FILE.push
	else
		logger "[pushdata] error inserting the B-field data into the DB, keeping $BF_FILE.push" 
	fi
fi

if [ -f "$BF_MINMAX_FILE" ]; then
	if [ ! -f "$BF_MINMAX_FILE.push" ] && [ -f "$BF_MINMAX_FILE" ]; then
		mv $BF_MINMAX_FILE $BF_MINMAX_FILE.push
	else
		logger "[pushdata] $BF_MINMAX_FILE.push exists, indicating a missing server connection"
	fi

	PGPASSWORD=${TPSO_DB_PASS} psql -U $USER -h localhost $DB <<EOF
	
	CREATE TEMPORARY TABLE bfield_import (
        time TIMESTAMP NOT NULL,
        bx_min DOUBLE PRECISION  NULL,
        by_min DOUBLE PRECISION  NULL,
        bz_min DOUBLE PRECISION  NULL,
        bx_max DOUBLE PRECISION  NULL,
        by_max DOUBLE PRECISION  NULL,
        bz_max DOUBLE PRECISION  NULL
	);
	
	\COPY bfield_import from '$BF_MINMAX_FILE.push' with delimiter E'\t' null as 'NULL';
	
	INSERT INTO $TABLE_BFIELD_MINMAX(time, bx_min, by_min, bz_min, bx_max, by_max, bz_max)
	SELECT *
	FROM bfield_import ON conflict 
	DO NOTHING;
	
	DROP TABLE bfield_import;
	
EOF
	retval=$?
	if [ $retval -eq 0 ]; then
		rm $BF_MINMAX_FILE.push
	else
		logger "[pushdata] error inserting the B-field min/max data into the DB, keeping $BF_MINMAX_FILE.push"
	fi
fi

if [ -f "$HK_FILE" ]; then
	if [ ! -f "$HK_FILE.push" ] && [ -f "$HK_FILE" ]; then
		mv $HK_FILE $HK_FILE.push
	else
		logger "[pushdata] $HK_FILE.push exists, indicating a missing server connection"
	fi

	PGPASSWORD=${TPSO_DB_PASS} psql -U $USER -h localhost $DB <<EOF
	
	CREATE TEMPORARY TABLE hk_import (
        time TIMESTAMP NOT NULL,
        mag_start boolean NULL,
        excit_off boolean NULL,
        recording boolean NULL,
        flash_recording_disabled boolean NULL,
        open_loop boolean NULL,
        peo boolean NULL,
        sync_off boolean NULL,
        sync_src boolean NULL,
        crc_ok boolean NULL,
        boot_main boolean NULL,
        eeprom_page smallint NULL,
        rate smallint NULL,
        dbg boolean NULL,
        temp_e DOUBLE PRECISION  NULL,
        temp_s DOUBLE PRECISION  NULL,
        tilt_x DOUBLE PRECISION  NULL,
        tilt_y DOUBLE PRECISION  NULL,
        V5p DOUBLE PRECISION  NULL,
        V5n DOUBLE PRECISION  NULL,
        V33 DOUBLE PRECISION  NULL,
        V15 DOUBLE PRECISION  NULL,
        rd_ptr integer NULL,
        wr_ptr integer NULL,
        app_version integer NULL,
        cfg_version real  NULL,
        RISC_SW_version real  NULL,
        time_drift DOUBLE PRECISION  NULL
    );
	
	\COPY hk_import from '$HK_FILE.push' with delimiter E'\t' null as 'NULL';
	
	INSERT INTO $TABLE_HK(time, mag_start, excit_off, recording, flash_recording_disabled, open_loop, peo, sync_off, sync_src, crc_ok, boot_main, eeprom_page, rate, dbg, temp_e, temp_s, tilt_x, tilt_y, V5p, V5n, V33, V15, rd_ptr, wr_ptr, app_version, cfg_version, RISC_SW_version, time_drift)
	SELECT *
	FROM hk_import ON conflict 
	DO NOTHING;
	
	DROP TABLE hk_import;
	
EOF
	retval=$?
	if [ $retval -eq 0 ]; then
		rm $HK_FILE.push
	else
		logger "[pushdata] error inserting the HK data into the DB, keeping $HK_FILE.push" 
	fi
fi

if [ -f "$HK_MINMAX_FILE" ]; then
	if [ ! -f "$HK_MINMAX_FILE.push" ] && [ -f "$HK_MINMAX_FILE" ]; then
		mv $HK_MINMAX_FILE $HK_MINMAX_FILE.push
	else
		logger "[pushdata] $HK_MINMAX_FILE.push exists, indicating a missing server connection"
	fi

	PGPASSWORD=${TPSO_DB_PASS} psql -U $USER -h localhost $DB <<EOF
	
	CREATE TEMPORARY TABLE hk_import (
        time TIMESTAMP NOT NULL,
        temp_e_min DOUBLE PRECISION  NULL,
        temp_s_min DOUBLE PRECISION  NULL,
        tilt_x_min DOUBLE PRECISION  NULL,
        tilt_y_min DOUBLE PRECISION  NULL,
        V5p_min DOUBLE PRECISION  NULL,
        V5n_min DOUBLE PRECISION  NULL,
        V33_min DOUBLE PRECISION  NULL,
        V15_min DOUBLE PRECISION  NULL,
        temp_e_max DOUBLE PRECISION  NULL,
        temp_s_max DOUBLE PRECISION  NULL,
        tilt_x_max DOUBLE PRECISION  NULL,
        tilt_y_max DOUBLE PRECISION  NULL,
        V5p_max DOUBLE PRECISION  NULL,
        V5n_max DOUBLE PRECISION  NULL,
        V33_max DOUBLE PRECISION  NULL,
        V15_max DOUBLE PRECISION  NULL
    );
	
	\COPY hk_import from '$HK_MINMAX_FILE.push' with delimiter E'\t' null as 'NULL';
	
	INSERT INTO $TABLE_HK_MINMAX(time, temp_e_min, temp_s_min, tilt_x_min, tilt_y_min, V5p_min, V5n_min, V33_min, V15_min, temp_e_max, temp_s_max, tilt_x_max, tilt_y_max, V5p_max, V5n_max, V33_max, V15_max)
	SELECT *
	FROM hk_import ON conflict 
	DO NOTHING;
	
	DROP TABLE hk_import;
	
EOF
	retval=$?
	if [ $retval -eq 0 ]; then
		rm $HK_MINMAX_FILE.push
	else
		logger "[pushdata] error inserting the HK min/max data into the DB, keeping $HK_MINMAX_FILE.push" 
	fi
fi
