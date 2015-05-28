#ifndef _qe_h_
#define _qe_h_

#include <vector>

#include "../rbf/rbfm.h"
#include "../rm/rm.h"
#include "../ix/ix.h"

# define QE_EOF (-1)  // end of the index scan

/**
 * QueryEngine header
 * @Editor: Benjy Strauss
 * @Editor: Paul Minni
 *
 *
 */

using namespace std;

typedef enum{ MIN = 0, MAX, SUM, AVG, COUNT } AggregateOp;

/**
 * The following functions use  the following
 * format for the passed data.
 *    For int and real: use 4 bytes
 *    For varchar: use 4 bytes for the length followed by the characters
 */

struct Value {
    AttrType type;          // type of value
    void* data;         // value
};


struct Condition {
    string lhsAttr;         // left-hand side attribute
    CompOp op;             // comparison operator
    bool bRhsIsAttr;     // TRUE if right-hand side is an attribute and not a value; FALSE, otherwise.
    string rhsAttr;         // right-hand side attribute if bRhsIsAttr = TRUE
    Value rhsValue;       // right-hand side value if bRhsIsAttr = FALSE
};

// All the relational operators and access methods are iterators.
class Iterator {
public:
    virtual int getNextTuple(void *data) = 0;
    virtual void getAttributes(vector<Attribute> &attrs) const = 0;
    virtual ~Iterator() {};
};

// A wrapper inheriting Iterator over RM_ScanIterator
class TableScan : public Iterator {
public:
	RelationManager &rm;
	RM_ScanIterator *iter;
	string tableName;
	vector<Attribute> attrs;
	vector<string> attrNames;
	RID rid;

	TableScan(RelationManager &rm, const string &tableName, const char *alias = NULL):rm(rm) {
		//Set members
		this->tableName = tableName;

		// Get Attributes from RM
		rm.getAttributes(tableName, attrs);

		// Get Attribute Names from RM
		unsigned i;
		for(i = 0; i < attrs.size(); ++i) {
		// convert to char *
            attrNames.push_back(attrs[i].name);
        }

        // Call rm scan to get iterator
        iter = new RM_ScanIterator();
        rm.scan(tableName, "", NO_OP, NULL, attrNames, *iter);

        // Set alias
        if(alias) this->tableName = alias;
    }

    // Start a new iterator given the new compOp and value
    void setIterator() {
        iter->close();
        delete iter;
        iter = new RM_ScanIterator();
        rm.scan(tableName, "", NO_OP, NULL, attrNames, *iter);
    }

    int getNextTuple(void *data) {
        return iter->getNextTuple(rid, data);
    }

    void getAttributes(vector<Attribute> &attrs) const {
        attrs.clear();
        attrs = this->attrs;
        unsigned i;

        // For attribute in vector<Attribute>, name it as rel.attr
        for(i = 0; i < attrs.size(); ++i) {
            string tmp = tableName;
            tmp += ".";
            tmp += attrs[i].name;
            attrs[i].name = tmp;
         }
     }
	~TableScan() { iter->close(); }
};

/**
 * Base class
 * A wrapper inheriting Iterator over IX_IndexScan
 */
class IndexScan : public Iterator {
public:
    RelationManager &rm;
    RM_IndexScanIterator *iter;
    string tableName;
    string attrName;
    vector<Attribute> attrs;
    char key[PAGE_SIZE];
    RID rid;

    IndexScan(RelationManager &rm, const string &tableName, const string &attrName, const char *alias = NULL):rm(rm) {
		// Set members
		this->tableName = tableName;
		this->attrName = attrName;

		// Get Attributes from RM
		rm.getAttributes(tableName, attrs);

		// Call rm indexScan to get iterator
		iter = new RM_IndexScanIterator();
		rm.indexScan(tableName, attrName, NULL, NULL, true, true, *iter);

		// Set alias
		if(alias) this->tableName = alias;
    }

	// Start a new iterator given the new key range
    void setIterator(void* lowKey, void* highKey, bool lowKeyInclusive, bool highKeyInclusive) {
		iter->close();
		delete iter;
		iter = new RM_IndexScanIterator();
		rm.indexScan(tableName, attrName, lowKey, highKey, lowKeyInclusive, highKeyInclusive, *iter);
    }

	int getNextTuple(void *data) {
		int rc = iter->getNextEntry(rid, key);
        if(rc == 0) {
            rc = rm.readTuple(tableName.c_str(), rid, data);
        }
        return rc;
    }

    void getAttributes(vector<Attribute> &attrs) const {
        attrs.clear();
        attrs = this->attrs;
        unsigned i;

        // For attribute in vector<Attribute>, name it as rel.attr
        for(i = 0; i < attrs.size(); ++i) {
            string tmp = tableName;
            tmp += ".";
            tmp += attrs[i].name;
            attrs[i].name = tmp;
        }
    }

	~IndexScan() { iter->close(); }
};

// Filter operator
class Filter : public Iterator {
public:
	// Iterator of input R, Selection condition
    Filter(Iterator *input, const Condition &condition);
	~Filter();

	int getNextTuple(void *data);
	// For attribute in vector<Attribute>, name it as rel.attr
	void getAttributes(vector<Attribute> &attrs) const;
};

// Projection operator
class Project : public Iterator {
public:
	/**
	 * Iterator of input R
	 * vector containing attribute names
	 */
	Project(Iterator *input, const vector<string> &attrNames);
	~Project();

	int getNextTuple(void *data);
    // For attribute in vector<Attribute>, name it as rel.attr
    void getAttributes(vector<Attribute> &attrs) const;
};

// Nested-Loop join operator
class NLJoin : public Iterator {
public:
	/**
	 * Iterator of input R
	 * TableScan Iterator of input S
	 * Join condition
	 * Number of pages can be used to do join (decided by the optimizer)
	 */
	NLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages);
    ~NLJoin();

    int getNextTuple(void *data);
	// For attribute in vector<Attribute>, name it as rel.attr
	void getAttributes(vector<Attribute> &attrs) const;
};

// Index Nested-Loop join operator
class INLJoin : public Iterator {
public:
	/**
	 * Iterator of input R
	 * TableScan Iterator of input S
	 * Join condition
	 * Number of pages can be used to do join (decided by the optimizer)
	 */
	INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition, const unsigned numPages);
	~INLJoin();

	int getNextTuple(void *data);
	// For attribute in vector<Attribute>, name it as rel.attr
	void getAttributes(vector<Attribute> &attrs) const;
};

// Aggregation operator
class Aggregate : public Iterator {
public:
	/**
	 * Iterator of input R
	 * The attribute over which we are computing an aggregate
	 * Aggregate operation
	 */
	Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op);

	/**
	 * Extra Credit
	 * Iterator of input R
	 * The attribute over which we are computing an aggregate
	 * The attribute over which we are grouping the tuples
	 * Aggregate operation
	 */
	Aggregate(Iterator *input, Attribute aggAttr, Attribute gAttr, AggregateOp op);

	~Aggregate();

	int getNextTuple(void *data);
	/**
	 * Please name the output attribute as aggregateOp(aggAttr)
     * E.g. Relation=rel, attribute=attr, aggregateOp=MAX
	 * output attrname = "MAX(rel.attr)"
	 */
	void getAttributes(vector<Attribute> &attrs) const;
};

#endif
