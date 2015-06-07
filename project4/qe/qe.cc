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

int 
Tools::setConditionToValue(void* data, vector<Attribute> &attributes, const Condition &old_condition, Condition &new_condition){
    // Set the new condition's base information
    new_condition.lhsAttr = old_condition.lhsAttr;
    new_condition.op = old_condition.op;
    new_condition.bRhsIsAttr = false;
    
    // Make sure the attribute's vector is not empty
    if (attributes.empty()){
        return -1;
    }
    
    Value value_info;
    int found_attr = -1;
    unsigned offset = 0;
    // Go through each attribute in the record
    for (unsigned i = 0; i < attributes.size(); i++){
        unsigned current_attr_length = getAttributeLength(attributes[i], (char*) data + offset);
        // If it's the desired record, grab its information
        if (old_condition.rhsAttr.compare(attributes[i].name) == 0){
            found_attr = i;
            value_info.type = attributes[i].type;
            value_info.data = (char*) data + offset;

            // Force end of loop if found
            i = attributes.size();
        }
        offset += current_attr_length;
    }

    if (found_attr == -1){
        return -2;
    }

    new_condition.rhsValue = value_info;
    return SUCCESS;
}

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
	void* lhsValue;
	void* rhsValue; 

	// Make it easier to access vector's elements
	vector<Attribute>& attributes_ref = *attributes;

	// Go through the attributes in the record
	for (unsigned i = 0; i < attributes_ref.size(); i++){
		unsigned current_attr_length = getAttributeLength(attributes_ref[i], (char*) data + offset);

		// Retrieve the values for the desired attributes
		if (condition.lhsAttr.compare(attributes_ref[i].name) == 0){
			lhsValue = (char*) data + offset;
			lhsAttr_type = attributes_ref[i].type;
			found_lhsAttr = i;
		} else if (condition.bRhsIsAttr && condition.rhsAttr.compare(attributes_ref[i].name) == 0){
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
	// Fetch next tuple
	if (filtIter->getNextTuple(data) == QE_EOF) {
		return QE_EOF;
	}

	// If the condition is not met, go to the next tuple recursively 
	if (!Tools::checkCondition(&filterAttributes, data, this->filterOn)){
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

}

int
Project::getNextTuple(void *data) {
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

NLJoin::NLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages) {
	left = leftIn;
    left->getAttributes(left_attrs);
	right = rightIn;
    right->setIterator();
    right->getAttributes(right_attrs);
	joinCondition = condition;
	pages = numPages;
	holder_left = malloc(PAGE_SIZE);
    getNextLeft = true;
}

NLJoin::~NLJoin() {
	free(holder_left);
}

int
NLJoin::getNextTuple(void *data) {
	int tempRetVal = -2;
    void* holder_right = malloc(PAGE_SIZE);

    // Fetch next left tuple if neccessary 
    if (getNextLeft){
	    if (left->getNextTuple(holder_left) == QE_EOF){
            free(holder_right);
		    return QE_EOF;
	    }
        getNextLeft = false;
    }

    // Fetch next right tuple
    if(right->getNextTuple(holder_right) == QE_EOF){
        right->setIterator(); 
        getNextLeft = true;
		tempRetVal = this->getNextTuple(data);
    	free(holder_right);
        return tempRetVal;
    }   
    
    // Make custom condition using value for the desired attribute in the right tuple
    Condition new_condition;
    if (this->joinCondition.bRhsIsAttr) {
        if (Tools::setConditionToValue(holder_right, this->right_attrs, this->joinCondition, new_condition) != SUCCESS) {
    		free(holder_right);
            return -1;
        }
    } else {
    	new_condition = this->joinCondition;
    }

    // Evaluate the custom condition
    if(!Tools::checkCondition(&left_attrs, holder_left, new_condition)){
		tempRetVal = this->getNextTuple(data);
    	free(holder_right);
        return tempRetVal;
    }

    unsigned left_size = RecordBasedFileManager::getRecordSize(left_attrs, holder_left);
    unsigned right_size = RecordBasedFileManager::getRecordSize(right_attrs, holder_right);
    memcpy(data, holder_left, left_size);
    memcpy((char*) data + left_size, holder_right, right_size);

    free(holder_right);
	return SUCCESS;
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

/**
 * EXTRA CREDIT
 */

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
