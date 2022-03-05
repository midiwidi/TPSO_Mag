##!/bin/bash

start_date='2021-02-09'
end_date='2021-12-31'
base_dir=/home/midiwidi/Projects/TPSO/export   #no '/' at the end


mkdir -p "$base_dir/thumbnails/bfield"
mkdir -p "$base_dir/thumbnails/housekeeping"

start_date=$(date -d $start_date +%Y%m%d)
end_date=$(date -d $end_date +%Y%m%d)

#############################################################################
# calculuate week start date to be the first day of the week the start date is in
#############################################################################
doy=$((10#$(date -d $start_date +%j)))
week_day_offset=$(((($doy-1)/7)*7))
year=$(date -d $start_date +%Y)
week_start_date=$(date -d "$year-01-01" +%Y%m%d)
week_start_date=$(date -d "$week_start_date + $week_day_offset day" +"%Y%m%d")
#############################################################################

while [[ $week_start_date -le $end_date ]]
do
	#echo -e -n "week start = $week_start_date 00:00:00, "
	week_end_date=$(date -d "$week_start_date + 6 day" +"%Y%m%d")
	#echo -e -n "week end = $week_end_date 23:59:59, "
	doy=$((10#$(date -d $week_start_date +%j)))
	#echo -e -n "doy = $doy, "
	printf -v week "%02d" $((($doy-1)/7+1))
	#echo "week = $week"

	bfield_fname=$base_dir"/thumbnails/bfield/week"$week"_"$(date -d $week_start_date +%Y-%m-%d)"_to_"$(date -d $week_end_date +%Y-%m-%d)".png"
	#echo $bfield_fname
	hk_fname=$base_dir"/thumbnails/housekeeping/week"$week"_"$(date -d $week_start_date +%Y-%m-%d)"_to_"$(date -d $week_end_date +%Y-%m-%d)".png"
	#echo $hk_fname

	#Get B-Field Image
	echo "Getting B-field Image of week $week"
	wget -q -O $bfield_fname "localhost:3000/render/d/Dls2Cv1nk/magnetic-field?orgId=3&width=1800&height=1590&kiosk=tv&from="$(date -d $week_start_date +%s)"000&to="$(date -d $week_end_date +%s)"000"

	#Get HK Image
	echo "Getting HK Image of week $week"
	wget -q -O $hk_fname "localhost:3000/render/d/fQdrqD1nk/housekeeping?orgId=3&width=1800&height=2690&kiosk=tv&from="$(date -d $week_start_date +%s)"000&to="$(date -d $week_end_date +%s)"000"

	week_start_date=$(date -d "$week_start_date + 7 day" +"%Y%m%d")
done
