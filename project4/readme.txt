README and Report

Changes to starter code
(0) Added new makefile targets "purge" and "remake"
(1) Modified iterator classes to include local variables
(2) Moved method implementations to "qe.cc"
(3) Implemented unimplemented methods in qe.cc and rm.cc
(4) Added helper methods to qe.h and qe.cc
	(Specifically those in the Tools.h class)

Makefile Targets:
> remake: recompile as if the program has never run
> purge: clean, clear all generated files

Bugs and Unintended Features:
	Crashes on test 4 unless run inside Valgrind.  Inside Valgrind, this crash does not occur.
	Tests 1-3 always pass.  Test 4 always passes inside valgrind.
	Test cases 3-10 have been slightly modified to print a line saying they fail if they fail

How it works:
	The model we used involves the concept of pipelining.  Each iterator takes an iterator as input, and then it is input into another iterator.  Each iterator performs tasks on the data (projection/selection/join) like people in an assembly line, so once all the iterators have touched the data, the data has the result of all of the operations.




