
To compile program, run 'make'.
Can then run with arbitrary readers-writers data files (see readw.c for details) from command line:
$ ./readw <data-file>

Can run in readers-first and writers-first mode, depending on which #define statement is uncommented at the beginning of the source code. Uncomment 'READER' for readers-first mode OR uncomment WRITER for writers-first mode. If no #define statement uncommented, program runs without any mutual exclusion.