#!/bin/bash
./kill.sh
gcc src/tcp-ftp.c -o server/tcp-server 
if [ $? -eq 0 ]; then
	echo server/ServerV server/ServerW server/ServerX server/ServerZ | xargs -n 1 cp server/tcp-server
	rm server/tcp-server
	(cd server/ServerV && gnome-terminal -x ./tcp-server) &
	(cd server/ServerW && gnome-terminal -x ./tcp-server) &
	(cd server/ServerX && gnome-terminal -x ./tcp-server) &
	(cd server/ServerZ && gnome-terminal -x ./tcp-server) &
	(gnome-terminal -e ./run-client.sh) 
fi
