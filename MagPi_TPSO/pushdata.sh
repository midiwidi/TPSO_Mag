#!/bin/bash

## prior to the first execution, create table with
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

USER=postgres
DB=tpso_mag
BF_FILE=/home/pi/Projects/TPSO_Mag/data/mag.dat

if [[ -z "${TPSO_DB_PASS}" ]]; then
   echo "Environment variable TPSO_DB_PASS doesn't exist. Please create it using 'export TPSO_DB_PASS=mypass'. Replace mypass with the database password for the user postgres."
   exit
fi

if [ -f "$BF_FILE" ]; then
	if [ ! -f "$BF_FILE.push" ] && [ -f "$BF_FILE" ]; then
		mv $BF_FILE $BF_FILE.push
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
	
	INSERT INTO bfield(time, bx, by, bz, clipx, clipy, clipz, sync)
	SELECT *
	FROM bfield_import ON conflict 
	DO NOTHING;
	
	DROP TABLE bfield_import;
	
EOF
	retval=$?
	if [ $retval -eq 0 ]; then
		rm $BF_FILE.push
	fi
fi
