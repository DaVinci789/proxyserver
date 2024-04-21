# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread -fsanitize=address

all: proxy multithread_proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

multithread_proxy.o: multithread_proxy.c csapp.h
	$(CC) $(CFLAGS) -c multithread_proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) uriparse.c proxy.o csapp.o -o proxy $(LDFLAGS)

multithread_proxy: multithread_proxy.o csapp.o
	$(CC) $(CFLAGS) uriparse.c multithread_proxy.o csapp.o -o multithread_proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o multithread_proxy proxy core *.tar *.zip *.gzip *.bzip *.gz

