# mulcons_nothreads

This is a small server simulating a detector server.
Compile it with
```sh
gcc -Wall server.c -o server
```
And run it with
```sh
./server
```

Then start a second and third terminal window.
In the second window start
```sh
telnet localhost 8888
```
and type a
which starts the "acquisition".
You see a countdown on the server side. The connection
remains open and telnet is still running.

In the third window open another telnet session
```sh
telnet localhost 8888
```
and type t
This returns the "temperature" (always 50°C) while the "acquisition" is still running.

Server command list:
* a	- starts acquisition
* s	- stops the acquisition and closes the connection
* t	- returns temperature
* x	- status of acquisition
