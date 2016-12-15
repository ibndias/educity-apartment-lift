while :
do
echo ------------------STARTING WIEGAND READER-------------------------------
sudo rm /var/run/pigpio.pid
sudo /home/pi/wiegand_c #or whatever program you wish to run.
sleep 2
done
sudo rm /var/run/pigpio.pid
