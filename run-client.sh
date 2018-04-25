#!/bin/bash
gcc src/tcp-client.c -o client/tcp-client 
if [ $? -eq 0 ]; then
	cd client/
	./tcp-client 127.0.0.1 
fi

