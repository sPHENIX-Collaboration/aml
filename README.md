
   The Advanced Multi-Threaded Logger (aml)

This used to be the backbone of the PHENIX data logging. It would take
multiple input streams from various machines and write out a single
datastream.

In one aspect, it operates like a ftp server that accepts an incomimg
connection, forks itself, and that fork handles this particular
connection and terminates when done.

Instead of forking, the aml generates a new thread that deals with the
incoming connection. In this way, it is still the same process that
can easily deal with the writing of the incoming data to a single output.

The network part of it can be optimized much better to teh properties
of the incoming data, and since we are writing only one output file,
the disk throughput can be much higher. 
