Pair:
Benjamin (Benjy) Strauss
Paul Minni

Known Issues:
Test case 3 crashes because it appears to have been written by a drunk monkey.  The test file is deleted, and then the program crashes because it can't open this file.  This is a problem in the test case, and not our program, and it sets everything else up for failure.

Note that when inserting the data, put the length of the varchar at the front.

How to run:

Compiling:
"make remake": compile with our custom main
"make one" OR "make remake1": compile test case 1
"make two" OR "make remake2": compile test case 2
"make purge": delete the executable and object files.

Running:
"./db181" runs the executable.  No parameters are taken.







