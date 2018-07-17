.SILENT:

default:
	gcc -Wall -Wextra -g lab0.c -o lab0

dist:
	tar -czf lab0-304546929.tar.gz lab0.c Makefile backtrace.png breakpoint.png README

clean:
	rm -f lab0 lab0-304546929.tar.gz

check: default inputTest outputTest SegfaultTest catchTest falseArg diffTest exit0Test

inputTest:
	echo "Zixuan loves Ai Xinnan!" > in.txt; \
	chmod -r in.txt; \
	./lab0 --input=in.txt --output=out.txt &> /dev/null; \
	if [[ $$? -ne 2 ]]; then \
	echo "Test failed: return the false exit code for the input error"; \
	else \
	echo "Test pass: return the correct exit code for the input error"; \
	fi
	rm -f *.txt

outputTest:
	echo "Zixuan loves Ai Xinnan!" > in.txt; \
	touch out.txt; \
	chmod u-w out.txt; \
	./lab0 --input=in.txt --output=out.txt &> /dev/null; \
	if [[ $$? -ne 3 ]]; then \
	echo "Test failed: return the false exit code for the output error"; \
	else \
	echo "Test pass: return the correct exit code for the output error"; \
	fi
	chmod u=rwx out.txt
	rm in.txt out.txt

SegfaultTest:
	./lab0 --segfault &> /dev/null; \
	if [[ $$? -ne 139 ]]; then \
	echo "segfault test fails."; \
	else \
	echo "segfault test pass."; \
	fi

catchTest:
	./lab0 --segfault --catch &> /dev/null; \
	if [[ $$? -ne 4 ]]; then \
	echo "Test failed: return the false exit code for the catch error"; \
	else \
	echo "Test pass: return the correct exit code for the catch error"; \
	fi

falseArg:
	./lab0 --falsearg &> /dev/null; \
	if [[ $$? -ne 1 ]]; then \
	echo "Test failed: return the false exit code for unrecognized argument"; \
	else \
	echo "unrecognized argument test succeeds."; \
	fi

diffTest:
	echo "Zixuan loves Ai Xinnan!" > in.txt; \
	./lab0 --input=in.txt --output=out.txt &> /dev/null; \
	cmp in.txt out.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "Test pass: input and output are identical."; \
	else \
	echo "Test failed: input and output are different."; \
	fi
	rm in.txt out.txt

exit0Test:
	echo "Zixuan loves Ai Xinnan!" > in.txt; \
	./lab0 --input=in.txt --output=out.txt &> /dev/null; \
	if [[ $$? -ne 0 ]]; then \
	echo "Test failed: the successful program did not exit with 0."; \
	else \
	echo "Test pass: the program exit with 0 successfully."; \
	fi
	rm in.txt out.txt
