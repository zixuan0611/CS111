.SILENT:

default:
	gcc -Wall -Wextra -lmraa -lm -g -o lab4c_tcp lab4c_tcp.c
	gcc -Wall -Wextra -lmraa -lm -lssl -lcrypto -g -o lab4c_tls lab4c_tls.c

clean:
	rm -f lab4c.tar.gz lab4c_tcp lab4c_tls

dist:
	tar -czf lab4c.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
