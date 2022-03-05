
# TPSO_Mag

## MagPi_TPSO

Software, scripts and configurations for the Magson fluxgate magnetometer MFG-1S at the TPSO observatory near Victor Harbor in South Australia

## Data_Server
Scripts and configuration for the data server and web frontend

### Annual data export (backup) of database content of the TPSO magnetometer as CSV files
**Export Data**

The exported csv files for a whole year would get quite big and we might run out of disk space. That's why we split the export in 2 halfs (01.01.YYYY ... 31.07.YYYY and 01.08.YYYY ... 31.12.YYYY)

1. Edit the export script /home/midiwidi/Projects/TPSO/tpso_db_export.sh and make sure the DB user postgres password is specified and adjust start and end date to YYYY-01-01 ... YYYY-07-31 (format YYYY-MM-DD)
2. Delete the folder /home/midiwidi/Projects/TPSO/export/YYYY (YYYY is the year to backup) if there is one
3. run /home/midiwidi/Projects/TPSO/tpso_db_export.sh and wait until it finishes
4. Optional: check /home/midiwidi/Projects/TPSO/export/YYYY for the exported data
5. cd to /home/midiwidi/Projects/TPSO/export/YYYY and zip the data with 'zip -r YYYY.zip YYYY'
6. Download the zip file using WinSCP and extract the data

... and repeat everything with the second half of the year

7. Edit the export script /home/midiwidi/Projects/TPSO/tpso_db_export.sh and adjust start and end date to YYYY-08-01 ... YYYY-12-31 (format YYYY-MM-DD)
8. Delete the folder /home/midiwidi/Projects/TPSO/export/YYYY and the zip file
9. run /home/midiwidi/Projects/TPSO/tpso_db_export.sh and wait until it finishes
10. Optional: check /home/midiwidi/Projects/TPSO/export/YYYY for the exported data
11. cd to /home/midiwidi/Projects/TPSO/export/YYYY and zip the data with 'zip -r YYYY.zip YYYY'
12. Download the zip file using WinSCP and extract the data (into the same directory as the data of the first half of the year)
13. Delete the folder /home/midiwidi/Projects/TPSO/export/YYYY and the zip file

**Generate Preview Images**

14. Edit the export script /home/midiwidi/Projects/TPSO/tpso_db_export_thumbnails.sh and adjust start and end date to YYYY-01-01 ... YYYY-12-31 (format YYYY-MM-DD)
15. Run /home/midiwidi/Projects/TPSO/tpso_db_export_thumbnails.sh and wait until it finishes
16. Optional: check /home/midiwidi/Projects/TPSO/export/YYYY/thumbnails for the exported images
17. Download the image files using WinSCP
18. Delete the folder /home/midiwidi/Projects/TPSO/export/YYYY including the image files

## Periodic removal of old data
Every hour, the script tpso_db_delete_oldest.sh is started by a cronjob. It removes data which is older than 426 days (1 year and 2 month). The 2 month should be enough time for the annual export so that no data is lost. 

crontab -e

    0 * * * * /home/midiwidi/Projects/TPSO/tpso_db_delete_oldest.sh

