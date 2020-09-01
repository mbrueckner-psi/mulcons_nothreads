# mulcons_nothreads

This is a small server simulating a detector server.
Compile it with gcc -Wall server.c -o server
And run it with ./server

Then start a second and third terminal window.
In the second window start
telnet localhost 8888
and type
a
which starts the "acquisition".
You see a countdown on the server side. The connection
remains open and telnet is still running.

In the third window open another telnet session
telnet localhost 8888
and type
t

This returns the "temperature" (always 50°C).

Server command list:
a	starts acquisition
s	stops the acquisition and closes the connection
t	returns temperature
x	status of acquisition
