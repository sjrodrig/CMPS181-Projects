#include "qe.h"

/**
 * QE = Query Engine
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

unsigned
Tools::getAttributeLength(Attribute &attr, void* data){
	switch (attr.type){
		case TypeInt:
			return INT_SIZE;
		case TypeReal:
			return REAL_SIZE;
		case TypeVarChar:
			return VARCHAR_LENGTH_SIZE + getStringLength(data);
		default:
			return 0;
	}	
}

unsigned
Tools::getStringLength(void* data){
	unsigned stringLength;
	memcpy(&stringLength, data, VARCHAR_LENGTH_SIZE);
	return stringLength;
}

void
Tools::getAttributeValue(Attribute &attr, void* data, void* destination){
	switch (attr.type){
		case TypeInt:
			memcpy(destination, data, INT_SIZE);
		case TypeReal:
			memcpy(destination, data, REAL_SIZE);
		case TypeVarChar:
			memcpy(destination, data, getAttributeLength(attr, data));
	}
}

bool 
Tools::compareValues(int dataInt, CompOp compOp, const void * value) {
	// Checking a condition on an integer is the same as checking it on a float with the same value.
	int valueInt;
	memcpy (&valueInt, value, INT_SIZE);
	float convertedInt = (float)valueInt;

	return compareValues((float) dataInt, compOp, &convertedInt);
}

bool 
Tools::compareValues(float dataFloat, CompOp compOp, const void * value) {
	float valueFloat;
	memcpy (&valueFloat, value, REAL_SIZE);

	switch (compOp) {
		case EQ_OP:  // =
			return dataFloat == valueFloat;
		break;
		case LT_OP:  // <
			return dataFloat < valueFloat;
		break;
		case GT_OP:  // >
			return dataFloat > valueFloat;
		break;
		case LE_OP:  // <=
			return dataFloat <= valueFloat;
		break;
		case GE_OP:  // >=
			return dataFloat >= valueFloat;
		break;
		case NE_OP:  // !=
			return dataFloat != valueFloat;
		break;
		case NO_OP:  // no condition
			return true;
		break;
	}

	// We should never get here.
	return false;
}

bool 
Tools::compareValues(const char * dataString, CompOp compOp, const char * value) {
	
	switch (compOp) {
		case EQ_OP:  // =
			return strcmp(dataString, value) == 0;
		break;
		case LT_OP:  // <
			return strcmp(dataString, value) < 0;
		break;
		case GT_OP:  // >
			return strcmp(dataString, value) > 0;
		break;
		case LE_OP:  // <=
			return strcmp(dataString, value) <= 0;
		break;
		case GE_OP:  // >=
			return strcmp(dataString, value) >= 0;
		break;
		case NE_OP:  // !=
			return strcmp(dataString, value) != 0;
		break;
		case NO_OP:  // no condition
			return true;
		break;
	}

	// We should never get here.
	return false;
}

bool
Tools::checkCondition(vector<Attribute>* attributes, void* data, const Condition &condition){
	// Check that the vector contains information
	if (attributes->empty()){
		return false;
	}

	int found_lhsAttr = -1;
	AttrType lhsAttr_type;
	int found_rhsAttr = -1;
	AttrType rhsAttr_type;
	unsigned offset = 0;

	// Value Holders
	void* lhsValue; //= malloc(PAGE_SIZE);
	void* rhsValue; //= malloc(PAGE_SIZE); 

	// Make it easier to access vector's elements
	vector<Attribute>& attributes_ref = *attributes;

	// Go through the attributes in the record
	for (unsigned i = 0; i < attributes_ref.size(); i++){
		unsigned current_attr_length = getAttributeLength(attributes_ref[i], (char*) data + offset);

		// Retrieve the values for the desired attributes
		if (condition.lhsAttr.compare(attributes_ref[i].name) == 0){
			//getAttributeValue(attributes_ref[i], (char*) data + offset, lhsValue);
			lhsValue = (char*) data + offset;
			lhsAttr_type = attributes_ref[i].type;
			found_lhsAttr = i;
		} else if (condition.bRhsIsAttr && condition.rhsAttr.compare(attributes_ref[i].name) == 0){
			// getAttributeValue(attributes_ref[i], (char*) data + offset, rhsValue);
			rhsValue = (char*) data + offset;
			rhsAttr_type = attributes_ref[i].type;
			found_rhsAttr = i;
		}
		
		offset += current_attr_length;
	}

	if (found_lhsAttr == -1){
		return false;
	} else if (condition.bRhsIsAttr && found_rhsAttr == -1){
		return false;
	}

	// If a value is provided for the RHS, grab it
	if (!condition.bRhsIsAttr){
		//cout << "!" << endl;
		rhsValue = condition.rhsValue.data;
		rhsAttr_type = condition.rhsValue.type;
	}

	// Make sure the values are of the same type
	if (lhsAttr_type != rhsAttr_type){
		return false;
	}

	// Compare the values according to their type
	bool result = false;
	switch(lhsAttr_type){
		case TypeInt:
			int lhsValue_int;
			memcpy(&lhsValue_int, lhsValue, INT_SIZE);
			result = compareValues(lhsValue_int, condition.op, rhsValue);
			break;
		case TypeReal:
			float lhsValue_real;
			memcpy(&lhsValue_real, lhsValue, REAL_SIZE);
			result = compareValues(lhsValue_real, condition.op, rhsValue);
			break;
		case TypeVarChar:
			unsigned lhsValue_string_length = 0;
			string lhsValue_string;
			memcpy(&lhsValue_string_length, lhsValue, VARCHAR_LENGTH_SIZE);
			memcpy(&lhsValue_string, (char*) lhsValue + VARCHAR_LENGTH_SIZE, lhsValue_string_length);
			result = compareValues(lhsValue_string.c_str(), condition.op, (char*) rhsValue + VARCHAR_LENGTH_SIZE);
			break;
	}

	// Clean up & return
	return result;
}

/**
 * Filter out the tuples from the input 
 */
Filter::Filter(Iterator* input, const Condition &condition) {
	filtIter = input;
	filterOn = condition;
	filtIter->getAttributes(filterAttributes);
}

Filter::~Filter() {

}

int
Filter::getNextTuple(void *data) {
cout << "Filter::getNextTuple" << endl;
	int retVal = -1;

	// Fetch next tuple
	if (filtIter->getNextTuple(data) == QE_EOF) {
		return QE_EOF;
	}

	// If the condition is not met, go to the next tuple recursively 
	if (!Tools::checkCondition(&filterAttributes, data, this->filterOn)){
		retVal = filtIter->getNextTuple(data);
		return retVal;
	}

	return SUCCESS;
}

void
Filter::getAttributes(vector<Attribute> &attrs) const {
	filtIter->getAttributes(attrs);
}

Project::Project(Iterator *input, const vector<string> &attrNames) {
	projIter = input;
	attributeNames = attrNames;
}

Project::~Project() {

}

int
Project::getNextTuple(void *data) {
	cout << "Project::getNextTuple" << endl;
	void* holder = malloc(PAGE_SIZE);

	// Fetch next tuple
	if (projIter->getNextTuple(holder) == QE_EOF){
		free(holder);
		return QE_EOF;
	}

	// If no attributes were given, just end here (failsafe)
	if (attributeNames.empty()){
		free(holder);
		return SUCCESS;
	}

	// Get the tuple's attributes
	vector<Attribute> tuple_attrs;
	projIter->getAttributes(tuple_attrs);

	// Go through the desired attributes
	unsigned offset = 0;
	for(unsigned i = 0; i < attributeNames.size(); i++){
		bool found_value = false;
		unsigned value_size = 0;
		unsigned internal_offset = 0;

		// Go through all the attributes in the record
		for (unsigned j = 0; j < tuple_attrs.size(); j++){
			// If it's the desired attribute, grab its value

			if (attributeNames[i].compare(tuple_attrs[j].name) == 0){
				Tools::getAttributeValue(tuple_attrs[j], (char*) holder + internal_offset, (char*) data + offset);
				found_value = true;
				// Make sure the for-loop ends if the attribute has been found
				j = tuple_attrs.size();
			}
			value_size = Tools::getAttributeLength(tuple_attrs[j], (char*) holder + internal_offset);
			internal_offset += value_size;
		}

		if (found_value){
			offset += value_size;
		}
	}

	free(holder);
	return SUCCESS;
}

void
Project::getAttributes(vector<Attribute> &attrs) const {
	// Make sure the vector is clean
	attrs.clear();

	// Get the iterator's attributes
	vector<Attribute> iter_attrs;
	projIter->getAttributes(iter_attrs);

	// Go through the desired attributes
	for(unsigned i = 0; i < attributeNames.size(); i++){
		// Go through all the attributes in the iterator
		for (unsigned j = 0; j < iter_attrs.size(); i++){
			// If it's the desired attribute, push it onto the vector
			if (attributeNames[i].compare(iter_attrs[j].name) == 0){
				attrs.push_back(iter_attrs[j]);
				// Make sure the for-loop ends if the attribute has been found
				j = iter_attrs.size();
			}
		}
	}
}

/*******************************************************************/
NLJoin::NLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages) {
	left = leftIn;
	right = rightIn;
	joinCondition = condition;
	pages = numPages;

	//Depending on the implementation of NLJoin being used...
	if(method == 2) {
		//Load everything into the right Vector
		void* data;
		rvIndex = 0;
		justStarted = true;
		leftData = NULL;
		while(right->getNextTuple(data) == 0) {
			rightVect.push_back(data);
		}
	}
}

NLJoin::~NLJoin() {
	free(leftData);
}

int
NLJoin::getNextTuple(void *data) {
	//check the method being used
	if(method != 2) {
		cout << "unimplemented method" << endl;
		return QE_EOF;
	}

	//-121 to be unique for debugging purposes
	int retVal = -121;

	//if we were just created 
	if(justStarted == true) {
		retVal = left->getNextTuple(leftData);
		if(retVal != 0) {
			cout << "No tuples in left iterator." << endl;
			return retVal;
		}
		justStarted = false;
	}

loopToMatch:
	for(; rvIndex < rightVect.size(); rvIndex++ ) {

		void* matchMe = rightVect.at(rvIndex);


/*
get the data from the iterator
search the table until a match is found

keep going until another match is found.
if we complete the table, get another data piece from the vector*/




	}

	retVal = left->getNextTuple(leftData);
	if(retVal != 0) {
		//means we are done
		return retVal;
	}

	//run look for another match
	goto loopToMatch;
	return retVal;
}
/*******************************************************************/
void
NLJoin::getAttributes(vector<Attribute> &attrs) const {
	left->getAttributes(attrs);

	vector<Attribute> temp;
	right->getAttributes(temp);

	for(unsigned rightIndex = 0; rightIndex < temp.size(); rightIndex++) {
		attrs.push_back(temp.at(rightIndex));
	}
}
/*******************************************************************/
INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition, const unsigned numPages) {
	left = leftIn;
	right = rightIn;
	joinCondition = condition;
	pages = numPages;
}

INLJoin::~INLJoin() {

}

int
INLJoin::getNextTuple(void *data) {
	return QE_EOF;
}

void
INLJoin::getAttributes(vector<Attribute> &attrs) const {
	left->getAttributes(attrs);

	vector<Attribute> temp;
	right->getAttributes(temp);

	for(unsigned rightIndex = 0; rightIndex < temp.size(); rightIndex++) {
		attrs.push_back(temp.at(rightIndex));
	}
}

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op) {
	aggrIter = input;
	aggrAttr = aggAttr;
	oper = op;
}

/**
 * EXTRA CREDIT
 * Correctness unknown
 */
Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute gAttr, AggregateOp op) {
	Aggregate(input, aggAttr, op);
	aggrAttr2 = gAttr;
}

Aggregate::~Aggregate() {

}

int
Aggregate::getNextTuple(void *data) {
	return QE_EOF;
}

void
Aggregate::getAttributes(vector<Attribute> &attrs) const {

}


// ... the rest of your implementations go here
