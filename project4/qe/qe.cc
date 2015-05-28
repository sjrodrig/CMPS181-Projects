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

}

Filter::~Filter() {

}

int
Filter::getNextTuple(void *data) {
	return QE_EOF;
}

void
Filter::getAttributes(vector<Attribute> &attrs) const {

}

Project::Project(Iterator *input, const vector<string> &attrNames) {

}

Project::~Project() {

}

int
Project::getNextTuple(void *data) {

}

void
Project::getAttributes(vector<Attribute> &attrs) const {

}

NLJoin::NLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages) {

}

NLJoin::~NLJoin() {

}

int
NLJoin::getNextTuple(void *data) {
	return QE_EOF;
}

void
NLJoin::getAttributes(vector<Attribute> &attrs) const {

}

INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition, const unsigned numPages) {

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

}

//EXTRA CREDIT
Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute gAttr, AggregateOp op) {

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
