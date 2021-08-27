Run in following order:

1.	gcc server.c -o server
2.	./server 12005			[If this port no doesn't work, change to some random port no, and change line no 119 of multithreaded_proxy.c]
3. 	gcc multithreaded_proxy.c -o proxy -pthread
4.  ./proxy 127.0.0.1 12006
5.  gcc client.c -o client
6.	gcc 127.0.0.1 12006		[This port no should matches with the port no given in line 3]

-> Connect multiple clients



TO DO:
1. Pass Host no from client to server
2. database.txt improve -> some garbage coming in search results
3. Functions to validate ip_address and domain names, conversion between IPv4 to IPv6
4. Ensure Thread safety
5. See about IP address
6. Veify limit of no of clients connected
 
