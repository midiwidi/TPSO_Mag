#!/bin/bash

start_date='2021-02-09'
end_date='2021-12-31'
base_dir=/home/midiwidi/Projects/TPSO/export   #no '/' at the end

export PGPASSWORD=DB_passwd_goes_here

start_date=$(date -d $start_date +%Y%m%d)
end_date=$(date -d $end_date +%Y%m%d)

curr_date=$start_date

while [[ $curr_date -le $end_date ]]
do
    year=$(date -d $curr_date +%Y)
	month=$(date -d $curr_date +%m)
	day=$(date -d $curr_date +%d)
	echo "exporting $(date -d $curr_date +%Y-%m-%d)"

	if [ ! -d "$base_dir/$year" ]
	then
    		mkdir -m 777 "$base_dir/$year"
	fi

	if [ ! -d "$base_dir/$year/$month" ]
    then
            mkdir -m 777 "$base_dir/$year/$month"
    fi

	psql -U postgres -d tpso_mag -c \
	"COPY (SELECT \
	time, bx,by, bz, clipx::int, clipy::int, clipz::int, sync::int FROM bfield \
	WHERE time BETWEEN '$year-$month-$day 00:00:00.000'::timestamp AND \
	'$year-$month-$day 23:59:59.999999999') TO '$base_dir/$year/$month/$year-$month-${day}_bfield.csv' csv header"

	psql -U postgres -d tpso_mag -c \
	"COPY (SELECT \
	* FROM bfield_minmax \
	WHERE time BETWEEN '$year-$month-$day 00:00:00.000'::timestamp AND \
	'$year-$month-$day 23:59:59.999999999') TO '$base_dir/$year/$month/$year-$month-${day}_bfield_minmax.csv' csv header"

	psql -U postgres -d tpso_mag -c \
	"COPY (SELECT \
	time, mag_start::int, excit_off::int, recording::int, flash_recording_disabled::int, open_loop::int, \
	peo::int, sync_off::int, sync_src::int, crc_ok::int, boot_main::int, eeprom_page, rate, dbg::int, \
	temp_e, temp_s, tilt_x, tilt_y, v5p, v5n, v33, v15, rd_ptr, wr_ptr, app_version, cfg_version, \
	risc_sw_version, time_drift FROM hk \
	WHERE time BETWEEN '$year-$month-$day 00:00:00.000'::timestamp AND \
	'$year-$month-$day 23:59:59.999999999') TO '$base_dir/$year/$month/$year-$month-${day}_hk.csv' csv header"

	psql -U postgres -d tpso_mag -c \
	"COPY (SELECT \
	* FROM hk_minmax \
	WHERE time BETWEEN '$year-$month-$day 00:00:00.000'::timestamp AND \
	'$year-$month-$day 23:59:59.999999999') TO '$base_dir/$year/$month/$year-$month-${day}_hk_minmax.csv' csv header"

	curr_date=$(date -d"$curr_date + 1 day" +"%Y%m%d")

done
