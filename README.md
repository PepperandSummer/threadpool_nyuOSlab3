# threadpool_nyuOSlab3
## Data compression is the process of encoding information using fewer bits than the original representation. Run-length encoding (RLE) is a simple yet effective compression algorithm: repeated data are stored as a single data and the count. In this lab, you will build a parallel run-length encoder called Not Your Usual ENCoder, or nyuenc for short.
## For more details, please see this link.
## https://cs.nyu.edu/courses/spring23/CSCI-GA.2250-002/nyuenc
Milestone 1: sequential RLE
You will first implement nyuenc as a single-threaded program. The encoder reads from one or more files specified as command-line arguments and writes to STDOUT. Thus, the typical usage of nyuenc would use shell redirection to write the encoded output to a file.

Milestone 2: parallel RLE
Next, you will parallelize the encoding using POSIX threads. In particular, you will implement a thread pool for executing encoding tasks.
You should use mutexes, condition variables, or semaphores to realize proper synchronization among threads. Your code must be free of race conditions. You must not perform busy waiting, and you must not use sleep(), usleep(), or nanosleep().
