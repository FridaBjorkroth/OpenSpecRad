#!/bin/zsh
# Script for deleting the data files at Ed, Lu and Ed0+.

timeout=5

##########################
##   CONNECT TO ED0+    ##
##########################

echo "\nConnecting to RPi_Ed0+..."
networksetup -setairportnetwork en0 RPi_Ed0+ OceanOptics
sleep "$timeout"

#################################
###   LIST DATAFILES ON EDZ    ##
#################################

echo "\nListing the data files on Edz:"
ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 ls /home/pi/projects/Edz_raw_data/"

echo "\nDo you want to delete all Edz data files? (y/n)\n"
read answer
if [[ $answer == y* ]]; then
	ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 rm -v /home/pi/projects/Edz_raw_data/*"
	echo "\nDone."	
fi


################################
##   LIST DATAFILES ON LUZ    ##
################################

echo "\nListing the data files on Luz:"
ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 ls /home/pi/projects/Luz_raw_data/"

echo "\nDo you want to delete all Luz data files? (y/n)\n"
read answer
if [[ $answer == y* ]]; then
	ssh pi@192.168.4.1 "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 rm -v /home/pi/projects/Luz_raw_data/*"
	echo "\nDone."	
fi

################################
##   LIST DATAFILES ON Ed0+   ##
################################

echo "\nListing the data files on Ed0+:"
ssh pi@192.168.4.1 ls /home/pi/projects/Ed0+_raw_data/


echo "\nDo you want to delete all Ed0+ data files? (y/n)\n"
read answer
if [[ $answer == y* ]]; then
	ssh pi@192.168.4.1 'rm -v /home/pi/projects/Ed0+_raw_data/*'
	echo "\nDone."	
fi

echo "\nEnd."