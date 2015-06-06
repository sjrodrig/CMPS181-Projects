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

Edits to Provided Code:

Line 700: rbfm.cc: (fix leaks)
free(dataString);


Bugs and Unintended Features:
	Crashes on test 4 unless run inside Valgrind.  Inside Valgrind, this crash does not occur.
	Tests 1-3 always pass.  Test 4 always passes inside valgrind.
	Test cases 3-10 have been slightly modified to print a line saying they fail if they fail

How it works:
	The model we used involves the concept of pipelining.  Each iterator takes an iterator as input, and then it is input into another iterator.  Each iterator performs tasks on the data (projection/selection/join) like people in an assembly line, so once all the iterators have touched the data, the data has the result of all of the operations.

------------------------------------------------------------------------------------------------------------
    //Depending on the implementation of NLJoin being used...
	//Won't be called, since METHOD = 1 now, don't comment out 
	if(METHOD == 2) {
		//Load everything into the right Vector
		void* data;
		rvIndex = -1;
		justStarted = true;
		leftData = NULL;
		while(right->getNextTuple(data) == 0) {
	 		rightVect.push_back(data);
	 	}
	}

cout << "NLJoin::getNextTuple()" << endl;
	//check the method being used
	if(method != 2) {
		cout << "unimplemented method" << endl;
		return QE_EOF;
	}

	cout << "<><>NLJoin::getNextTuple()<><>" << endl;
	//-121 to be unique for debugging purposes
	int retVal = -121;

	//if we were just created 
	if(justStarted == true) {
		//get the data from the iterator
		retVal = left->getNextTuple(leftData);
		if(retVal != 0) {
			cout << "No tuples in left iterator!" << endl;
			return retVal;
		}
		justStarted = false;
	}

	vector<Attribute> l_atts;
	left->getAttributes(l_atts);
	vector<Attribute> r_atts;
	right->getAttributes(r_atts);

	//increment rvIndex to start to make sure we don't return the same tuple twice
loopToMatch:
	for(rvIndex++; rvIndex < rightVect.size(); rvIndex++ ) {
		void* matchMe = rightVect.at(rvIndex);

		bool leftPass = Tools::checkCondition(&l_atts, leftData, joinCondition);
		bool rightPass = Tools::checkCondition(&r_atts, matchMe, joinCondition);

		if(leftPass == true && rightPass == true) {
			data = Tools::mergeVoidStars(leftData, matchMe, l_atts, r_atts);

			return SUCCESS;
		}
	}

	retVal = left->getNextTuple(leftData);
	if(retVal != 0) {
		//means we are done
		return retVal;
	}

	rvIndex = -1;
	//look for another match
	goto loopToMatch;
	return retVal;


------------------------------------------------------------------------------------------------------------
void*
Tools::mergeVoidStars(void* left, void* right, vector<Attribute> lattrs, vector<Attribute> rattrs) {
	unsigned leftSize = 0;
	unsigned rightSize = 0;
	void* retVal;

	for(unsigned index = 0; index < lattrs.size(); index++) {
		leftSize += lattrs.at(index).length;
	}

	for(unsigned index = 0; index < rattrs.size(); index++) {
		rightSize += rattrs.at(index).length;
	}

	unsigned totalSize = leftSize + rightSize;
	retVal = malloc(totalSize);
	memcpy(retVal, left, leftSize);
	memcpy(retVal+leftSize, right, rightSize);
	return retVal;
}
------------------------------------------------------------------------------------------------------------

