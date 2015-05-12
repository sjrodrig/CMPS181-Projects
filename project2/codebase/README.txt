CMPS181 - Spring 2015
PROJECT 2

Pair Programming:
Benjamin (Benjy) Strauss
Paul-Valentin Mini (pcamille)

All required functions are implemented. If you run into any errors, 
double check that the correct tables were initialized (for testing).

All test cases were copied directly from the materials, and all
function EXACTLY as they did as provided when called properly.

To (re)compile: type “make remake”
To call a test case, type “./db181” then the name of the test case,
preceded by a hyphen: i.e. “-create” “-00” “-01” “-08a” et cetera.

The scan free is data pointer when closed (close()), do NOT free them
yourself when testing or you will get a double free error.

reorganizePage() is currently not supported.
reorganizeFile() is currently not supported.

Known issues:
	If you run the tests in order, 10 and 11 will fail.  To get 10
and 11 to work, recompile/clean with "make remake" and then "./db181 -create"
and execute Test 09 (“./db181 -09”) before running them. Test 09 inserts data 
used in Test 10 and 11.

Test 16 doesn’t make any sense since it doesn’t make the table that it
looks for.

Note that when inserting the data, put the length of the varchar 
at the front.





