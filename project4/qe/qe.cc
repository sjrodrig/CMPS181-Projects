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
getAttributeLength(Attribute &attr, void* data){
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
getStringLength(void* data){
	unsigned stringLength;
	memcpy(&stringLength, data, VARCHAR_LENGTH_SIZE);
	return stringLength;
}

void
getAttributeValue(Attribute &attr, void* data, void* destination){
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
compareValues(int dataInt, CompOp compOp, const void * value) {
	// Checking a condition on an integer is the same as checking it on a float with the same value.
	int valueInt;
	memcpy (&valueInt, value, INT_SIZE);
	float convertedInt = (float)valueInt;

	return compareValues((float) dataInt, compOp, &convertedInt);
}

bool 
compareValues(float dataFloat, CompOp compOp, const void * value) {
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
compareValues(const char * dataString, CompOp compOp, const char * value) {
	switch (compOp)
	{
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
checkCondition(vector<Attribute>* attributes, void* data, const Condition &condition){
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
	void* lhsValue = malloc(PAGE_SIZE);
	void* rhsValue = malloc(PAGE_SIZE); 

	// Make it easier to access vector's elements
	vector<Attribute>& attributes_ref = *attributes;

	// Go through the attributes in the record
	for (unsigned i = 0; i < attributes_ref.size(); i++){
		unsigned current_attr_length = getAttributeLength(attributes_ref[i], (char*) data + offset);

		// Retrieve the values for the desired attributes
		if (condition.lhsAttr.compare(attributes_ref[i].name) == 0){
			getAttributeValue(attributes_ref[i], (char*) data + offset, lhsValue);
			lhsAttr_type = attributes_ref[i].type;
			found_lhsAttr = i;
		} else if (condition.bRhsIsAttr && condition.rhsAttr.compare(attributes_ref[i].name) == 0){
			getAttributeValue(attributes_ref[i], (char*) data + offset, rhsValue);
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
		case TypeReal:
			float lhsValue_real;
			memcpy(&lhsValue_real, lhsValue, REAL_SIZE);
			result = compareValues(lhsValue_real, condition.op, rhsValue);
		case TypeVarChar:
			unsigned lhsValue_string_length = 0;
			string lhsValue_string;
			memcpy(&lhsValue_string_length, lhsValue, VARCHAR_LENGTH_SIZE);
			memcpy(&lhsValue_string, (char*) lhsValue + VARCHAR_LENGTH_SIZE, lhsValue_string_length);
			result = compareValues(lhsValue_string.c_str(), condition.op, (char*) rhsValue + VARCHAR_LENGTH_SIZE);
	}

	// Clean up & return
	free(lhsValue);
	free(rhsValue);
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
	delete filtIter;
}

int
Filter::getNextTuple(void *data) {
	// Fetch next tuple
	if (filtIter->getNextTuple(data) == QE_EOF) {
		return QE_EOF;
	}

	// If the condition is not met, go to the next tuple recursively 
	if (!checkCondition(&filterAttributes, data, this->filterOn)){
		return this->getNextTuple(data);
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
	delete projIter;
}

int
Project::getNextTuple(void *data) {
	return QE_EOF;
}

void
Project::getAttributes(vector<Attribute> &attrs) const {
	// attrs = attributeNames;
}

NLJoin::NLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages) {
	left = leftIn;
	right = rightIn;
	joinCondition = condition;
	pages = numPages;
}

NLJoin::~NLJoin() {
	delete left;
	delete right;
}

int
NLJoin::getNextTuple(void *data) {
	return QE_EOF;
}

void
NLJoin::getAttributes(vector<Attribute> &attrs) const {
	left->getAttributes(attrs);

	vector<Attribute> temp;
	right->getAttributes(temp);

	for(unsigned rightIndex = 0; rightIndex < temp.size(); rightIndex++) {
		attrs.push_back(temp.at(rightIndex));
	}
}

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

	for(int rightIndex = 0; rightIndex < temp.size(); rightIndex++) {
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
	delete aggrIter;
}

int
Aggregate::getNextTuple(void *data) {
	return QE_EOF;
}

void
Aggregate::getAttributes(vector<Attribute> &attrs) const {

}


// ... the rest of your implementations go here
