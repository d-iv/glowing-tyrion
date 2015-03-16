

CC=gcc
DEBUG=-g
CFLAGS=$(DEBUG) -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement

PROGS= ftclient ftserver
OBJS = ftclient.o ftserver.o
SRCS = ftclient.c ftserver.c

all: $(PROGS)

bash:
	bash primeTest.bash
	
cleantxt:
	rm -f *.txt 
	
ftclient: ftclient.c
	gcc -D_POSIX_SOURCE -std=c99 ftclient.c -o ftclient

ftserver: ftserver.c
	gcc -D_POSIX_SOURCE -std=c99 ftserver.c -o ftserver

serve: ftserver
	ftserver
	
client: ftclient
	ftclient
	
viewmem:
	ls -l /dev/shm
	

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(@:.o=.c)

clean: 
	rm -f $(PROGS) *.o *~ *.ar *toc.txt

# TAR ########################################################	
tar:
	make clean
	make cleantext
	tar cvfj ../Homework6-ericksdo.tar.bzip ../Homework6

cleantext:
	rm -f *.txt

	
viewLost:
	ps aux | grep $(LOGNAME) | grep -v root

	
