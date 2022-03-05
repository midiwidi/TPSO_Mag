#!/bin/bash

# Maximum age of newest timestamp in the DB. If it is older something
# is wrong and deletion of old data is skipped 
t_max_age=$((1*24*3600))	# 1 day 

# Maximum error between system time and external source. If the error is bigger
# the system time is incorrect and deletion of old data is skipped
max_sys_time_err=10	# 10s

# Maximum age of data before it is deleted from the database
max_db_data_age=$((426*24*3600))	# 426 days = 1 year and 2 month (should be enough time for an annual backup)

export PGPASSWORD=DB_passwd_goes_here

t_db_newest=$(($(psql -U postgres -d tpso_mag -t -c "SELECT extract(epoch from time::timestamp(0)) FROM bfield ORDER BY time DESC LIMIT 1")))
t_system=$(($(date -u '+%s')))
t_extern=$(($(date -d "$(curl -s --head http://google.com | grep ^Date: | sed 's/Date: //g')" '+%s')))

#echo "t db newest = "$(date -d @$t_db_newest '+%Y-%m-%d %H:%M:%S')" ("$t_db_newest")"
#echo "t system = "$(date -d @$t_system '+%Y-%m-%d %H:%M:%S')" ("$t_system")"
#echo "t extern = "$(date -d @$t_extern '+%Y-%m-%d %H:%M:%S')" ("$t_extern")"

t_diff=$(($t_system - $t_db_newest))
t_diff_sysext=$(($t_system - $t_extern))

if [[ $t_diff_sysext -gt $max_sys_time_err ]] || [[ $t_diff_sysext -lt $((-$max_sys_time_err)) ]]
then
	logger -p user.error "TPSO DB: system time error ("$t_diff_sysext") too big, skipping deletion of old data"
	exit 1
fi

if [[ $t_diff -lt 0 ]]
then
	logger -p user.error "TPSO DB: last timestamp in DB is newer than current time, skipping deletion of old data"
	exit 2
fi

if [[ $t_diff -gt $t_max_age ]]
then
	logger -p user.error "TPSO DB: last timestamp in DB is older than "$t_max_age"s ("$t_diff"s), skipping deletion of old data"
	exit 3
fi

t_delete=$(($t_system - $max_db_data_age))
logger -p user.info "TPSO DB: all data from before "$(date -d @$t_delete '+%Y-%m-%d %H:%M:%S')" will be deleted"

count=$(($(psql -U postgres -d tpso_mag -t -c "DELETE FROM bfield WHERE time < TO_TIMESTAMP("$t_delete")" | tr -dc '0-9')))
logger -p user.info "TPSO DB - table 'bfield': "$count" rows deleted"

count=$(($(psql -U postgres -d tpso_mag -t -c "DELETE FROM bfield_minmax WHERE time < TO_TIMESTAMP("$t_delete")" | tr -dc '0-9')))
logger -p user.info "TPSO DB - table 'bfield_minmax': "$count" rows deleted"

count=$(($(psql -U postgres -d tpso_mag -t -c "DELETE FROM hk WHERE time < TO_TIMESTAMP("$t_delete")" | tr -dc '0-9')))
logger -p user.info "TPSO DB - table 'hk': "$count" rows deleted"

count=$(($(psql -U postgres -d tpso_mag -t -c "DELETE FROM hk_minmax WHERE time < TO_TIMESTAMP("$t_delete")" | tr -dc '0-9')))
logger -p user.info "TPSO DB - table 'hk_minmaxS': "$count" rows deleted"
