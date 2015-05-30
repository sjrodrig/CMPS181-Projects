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

/**
 * Filter out the tuples from the input 
 */
Filter::Filter(Iterator* input, const Condition &condition) {
	filtIter = input;
	filterOn = condition;
}

Filter::~Filter() {
	delete filtIter;
}

int
Filter::getNextTuple(void *data) {
	return QE_EOF;
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

}

void
Project::getAttributes(vector<Attribute> &attrs) const {
	attrs = attrNames;
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
	leftIn->getAttributes(attrs);

	vector<Attribute> temp;
	rightIn->getAttributes(temp);

	for(int rightIndex = 0; rightIndex < temp.size(); rightIndex++) {
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

}

void
INLJoin::getAttributes(vector<Attribute> &attrs) const {

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
