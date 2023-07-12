Problem statement : Distributed Erasure Coded Engine using Intel ISA-L

Code includes implementation of PUT, GET, LIST.
In the implementation, data chunks and parity are getting generated with the ratio of 8:3.

In Put function, File is getting divided in 8 chunks and 3 parities are being generated using Intel ISA-L library.

In Get function, all 8 data chunks are combined to retrieve the original file. 
In case of data loss, for the regeneration of chunks, the following scenarios are covered:
  1. If only chunks are deleted 
  2. If only parities are deleted 
  3. If both data chunks and parities are deleted  

In List function, objects are getting listed from the hash table. 

Checksum is used to check if the regenerated file is same as original file. 
The code works on image, audio and video files along with text files. The data chunks are getting regenerated without any noise.

The application works with docker as well. The file to be input is built with the container. The rest of the execution remains same as above.
