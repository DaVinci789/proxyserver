# proxyserver for cs428

## Important Files:
- requests.c
  This file has the handles getting a connection from the client and forwarding it to the internet.
- proxy.c
  This is the sequential proxy
- multithreaded\_proxy.c
  This is the multithreaded proxy
- loglist.h
  An experimental multithreaded capable ~~2d array made up of far too many linked lists~~ logging system.

The other .cc files are files you can read to better understand how proxy.c works.
- old\_proxy.cc: essentially the connection handler ripped out from tiny
- send\_request.cc: how to make arbitrary http calls from C.

proxy.c is essentially old\_proxy.cc and send\_request.cc mashed together.

TODO:
comment more stuff.
