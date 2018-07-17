#Name: Zixuan Wang
#EMAIL: zixuan14@g.ucla.edu
.SILENT:

default:
	gcc -lz -Wall -Wextra lab1b-client.c -o lab1b-client
	gcc -lz -Wall -Wextra lab1b-server.c -o lab1b-server

clean:
	rm -f lab1b-client lab1b-server lab1b.tar.gz

dist:
	tar -czf lab1b.tar.gz lab1b-client.c lab1b-server.c README Makefile
