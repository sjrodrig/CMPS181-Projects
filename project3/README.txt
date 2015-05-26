Pair:
Benjamin (Benjy) Strauss
Paul Minni

Known Issues:
Test case 4 crashes...

Note that when inserting the data, put the length of the varchar at the front.

How to run:

Compiling:
"make remake": compile with our custom main
"make one" OR "make remake1": compile test case 1
"make two" OR "make remake2": compile test case 2
"make purge": delete the executable and object files.

Running:
"./db181" runs the executable.  No parameters are taken.


Psuedo-Code for insert() below:
	IS NODE A PARENT (NON-LEAF)? getPageType(pageData)
		TRUE
			Find i? Cycle tree using key info? getSonPageID()
			Call insert recursivly
			IS newChildEntry == null
				TRUE
					Return;
				FALSE
					Does parent page have space?
						TRUE
							insert newChildEntry
							set newChildEntry = null
							Return;
						FALSE
							Split page? 
							Dont forget to setPageType()
		FALSE
			Does leaf page have space? (using page header and its space offset)
				TRUE
					Insert Key & RID?
					Set newChildEntry to null
				FALSE
					Split page in half
					setPageType() of new page
					Insert into first or second half?
					Set newChildEntry to middle value being copied up
			



