.SILENT:

default:
	gcc -Wall -Wextra -lmraa -lm -g lab4b.c -o lab4b

check: default
	./lab4b << -EOF \n
	OFF
	EOF

clean:
	rm -f lab4b *.tar.gz

dist:
	tar -czf lab4b.tar.gz lab4b.c README Makefile
