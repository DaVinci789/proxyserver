# proxyserver
## for cs428

The important file is proxy.c. It's currently the single threaded proxy server.

The other .cc files are files you can read to better understand how proxy.c works.
- old\_proxy.cc: essentially the connection handler ripped out from tiny
- send\_request.cc: how to make arbitrary http calls from C.

proxy.c is essentially old\_proxy.cc and send\_request.cc mashed together.

loglist.h is an experimental multithreaded capable ~~2d array made up of far too many linked lists~~ logging system.

TODO:
comment more stuff.
make multithreaded version.
