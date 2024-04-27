# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

RELEASE := 0

CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -fsanitize=address -fsanitize=undefined #-fsanitize=thread

ifeq ($(RELEASE), 1)
	CFLAGS = -Wall -Wextra -pedantic
endif

LDFLAGS = -lpthread 

all: proxy multithread_proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

requests.o: requests.c csapp.h
	$(CC) $(CFLAGS) -c requests.c

validate_uri.o: validate_uri.c
	$(CC) $(CFLAGS) -c validate_uri.c

multithread_proxy.o: multithread_proxy.c csapp.h
	$(CC) $(CFLAGS) -c multithread_proxy.c

proxy: validate_uri.o requests.o proxy.o csapp.o
	$(CC) $(CFLAGS) uriparse.c validate_uri.o requests.o proxy.o csapp.o -o proxy $(LDFLAGS)

multithread_proxy: validate_uri.o requests.o multithread_proxy.o csapp.o
	$(CC) $(CFLAGS) uriparse.c validate_uri.o requests.o multithread_proxy.o csapp.o -o multithread_proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o multithread_proxy proxy core *.tar *.zip *.gzip *.bzip *.gz

