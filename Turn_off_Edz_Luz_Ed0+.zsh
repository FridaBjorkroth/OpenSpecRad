#!/bin/zsh
# Script turning off RPI via its wifi

Sourcedir=/home/pi/projects/STS-VIS_rawdata/
Destdir=/Users/Frida/Documents/MAR603_local/
timeout=5

echo "Connecting to RPi_Ed0+..."
networksetup -setairportnetwork en0 RPi_Ed0+ OceanOptics
sleep "$timeout"

echo "Shuts off the Luz"
ssh pi@192.168.4.1  "ssh -i ~/.ssh/id_rsa5 pi@192.168.4.14 sudo shutdown -h now"

echo "Shuts off the Edz"
ssh pi@192.168.4.1  "ssh -i ~/.ssh/id_rsa4 pi@192.168.4.11 sudo shutdown -h now"

echo "Shuts off the Ed0+"
ssh pi@192.168.4.1 sudo shutdown -h now

sleep "$timeout"
echo "Done."



