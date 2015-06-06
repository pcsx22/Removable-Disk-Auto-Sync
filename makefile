all: 
	g++ mSync.cpp -l sqlite3 -o mSync
	sudo cp $(shell pwd)"/mSync" "/usr/local/bin"
	echo "Hellp"
