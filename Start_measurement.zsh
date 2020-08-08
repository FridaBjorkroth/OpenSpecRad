#!/bin/zsh
# Script for starting the data collection

timeout=5
ttimeout=60

##########################
##   CONNECT TO ED0+    ##
##########################

echo "Connecting to RPi_Ed0+..."
networksetup -setairportnetwork en0 RPi_Ed0+ OceanOptics
sleep "$timeout"

################################
##   LIST DATAFILES ON Ed0+    ##
################################

echo "\nListing the data files on Ed0+:"
ssh pi@192.168.4.1 ls /home/pi/projects/Ed0+_raw_data/

#################################
###   LIST DATAFILES ON EDZ    ##
#################################

echo "\nListing the data files on Edz:"
ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 ls /home/pi/projects/Edz_raw_data/"

################################
##   LIST DATAFILES ON LUZ    ##
################################

echo "\nListing the data files on Luz:"
ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 ls /home/pi/projects/Luz_raw_data/"


############################
##   START MEASUREMENT    ##
############################

echo "\nDo you wish to start a new measurement? (y/n)"
read answer
if [[ $answer == y* ]]; then
	echo "Starting Edz"

	ssh pi@192.168.4.1  "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 /home/pi/projects/C_Seabreeze/In-water_measurement_Edzinteractive" < /Users/Frida/Desktop/Input_settings_Edz.txt > /dev/null 2>&1 &
	
	echo "Starting Luz"
	ssh pi@192.168.4.1  "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 /home/pi/projects/C_Seabreeze/In-water_measurement_Luzinteractive" < /Users/Frida/Desktop/Input_settings_Luz.txt > /dev/null 2>&1 &

	echo "\nRelease the in-water instrument and let it free-fall.\n"

	
	echo "Starting Ed0+ and displaying processes:\n"
	ssh pi@192.168.4.1 /home/pi/projects/C_Seabreeze/Above-water_measurement_Ed0interactive < /Users/Frida/Desktop/Input_settings_AW.txt
	
	##sleep "$ttimeout"
else
	echo "\nExit.\n"
	exit 1
	
fi


###################################
##   PING IN-WATER INSTRUMENT    ##
###################################

echo "\nSearching for connection to the in-water instrument.\n"

# Maximum number to try.
#((count = 100))
# [[ $count -ne 0 ]]

while true; do
    ssh pi@192.168.4.1 ping -c 1 192.168.4.11
    rc=$?
    if [[ $rc -eq 0 ]] ; then 		# If Edz is reached, try to reach the Luz.
        ssh pi@192.168.4.1 ping -c 1 192.168.4.14
	echo "\nEdz is within reach."
	if [[ $rc -eq 0 ]] ; then	# If equal, then the connection to both Edz and Luz is successful.
		echo "\nLuz is also within reach."
		echo "\nThe in-water instrument is back up."
		#((count = 1))		# Ending the while.
		break
	fi
	         
    fi
    #((count = count - 1))                  # Reduces count.
    sleep "$ttimeout"
done


######################
##   ERROR CHECK    ##
######################

echo "\nLook for any error during the measurement.."

EFILE=/home/pi/projects/Edz_raw_data/Error_log.txt
LFILE=/home/pi/projects/Luz_raw_data/Error_log.txt
AFILE=/home/pi/projects/Ed0+_raw_data/Error_log.txt

##########################
##   ERROR CHECK EDZ    ##
##########################

ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 test -f '$EFILE'"
rc=$?
if [[ $rc -eq 0 ]]; then
	echo "\nEdz error found:"
	ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 cat '$EFILE'"
fi

##########################
##   ERROR CHECK LUZ    ##
##########################

ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 test -f '$LFILE'"
rc=$?
if [[ $rc -eq 0 ]]; then
	echo "\nLuz error found:"
	ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 cat '$LFILE'"
fi

###########################
##   ERROR CHECK ED0+    ##
###########################

ssh pi@192.168.4.1 test -f "$AFILE"
rc=$?
if [[ $rc -eq 0 ]]; then
	echo "\nEd0+ error found:"
	ssh pi@192.168.4.1 cat "$AFILE"    
fi


##############################
##   GET ED0+ DATA FILES    ##
##############################

echo "\nDo you want to download the data files from Ed0+? (y/n)"
read answer
if [[ $answer == y* ]]; then
	scp -r pi@192.168.4.1:/home/pi/projects/Ed0+_raw_data/ /Users/Frida/Documents/MAR603_local/
	echo "\nDone."
fi

#############################
##   GET EDZ DATA FILES    ##
#############################

echo "\nDo you want to download the data files from Edz? (y/n)"
read answer
if [[ $answer == y* ]]; then
	scp -i ~/.ssh/id_rsa4 -r -oProxyCommand="ssh -W %h:%p pi@192.168.4.1" pi@\192.168.4.11:/home/pi/projects/Edz_raw_data/ /Users/Frida/Documents/MAR603_local/
	echo "\nDone."
fi

#############################
##   GET LUZ DATA FILES    ##
#############################

echo "\nDo you want to download the data files from Luz? (y/n)"
read answer
if [[ $answer == y* ]]; then
	scp -i ~/.ssh/id_rsa5 -r -oProxyCommand="ssh -W %h:%p pi@192.168.4.1" pi@\192.168.4.14:/home/pi/projects/Luz_raw_data/ /Users/Frida/Documents/MAR603_local/
	echo "\nDone."
fi




echo "\nEnd."



