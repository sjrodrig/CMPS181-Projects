#include "Tests.h"

using namespace std;

/**
 * edited by Benjy Strauss
 */

void TEST_RM_0(const string &tableName) {
    // Functions Tested
    // 1. Get Attributes
    cout << "****In Test Case 0****" << endl;
	cout << "tableName is: " << tableName << endl;

    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success);

    for(unsigned i = 0; i < attrs.size(); i++)
    {
        cout << "Attribute Name: " << attrs[i].name << endl;
        cout << "Attribute Type: " << (AttrType)attrs[i].type << endl;
        cout << "Attribute Length: " << attrs[i].length << endl << endl;
    }
    cout<<"** Test Case 0 passed"<<endl;
    return;
}

//	    TEST_RM_1("tbl_employee", 6, "Peters", 24, 170.1, 5000);
void TEST_RM_1(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions tested
    // 1. Insert Tuple **
    // 2. Read Tuple **
    // NOTE: "**" signifies the new functions being tested in this test case. 
    cout << "****In Test Case 1****" << endl;
   
    RID rid; 
    int tupleSize = 0;
    void *tuple = malloc(100);
    void *returnedData = malloc(100);

    // Insert a tuple into a table
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
    cout << "Insert Data of tupleSize: " << tupleSize << endl;
    printTuple(tuple, tupleSize);
    RC rc = rm->insertTuple(tableName, tuple, rid);

    assert(rc == success);
	cout << "insert success" << endl;
    
    // Given the rid, read the tuple from table
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success);

    cout << "Returned Data:" << endl;
    printTuple(returnedData, tupleSize);

    // Compare whether the two memory blocks are the same
    if(memcmp(tuple, returnedData, tupleSize) == 0)
    {
        cout << "****Test case 1 passed****" << endl << endl;
    }
    else
    {
        cout << "****Test case 1 failed****" << endl << endl;
    }

    free(tuple);
    free(returnedData);
    return;
}

void TEST_RM_2(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert tuple
    // 2. Delete Tuple **
    // 3. Read Tuple
    cout << "****In Test Case 2****" << endl;
   
    RID rid; 
    int tupleSize = 0;
    void *tuple = malloc(100);
    void *returnedData = malloc(100);

    // Test Insert the Tuple    
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
    cout << "Insert Data:" << endl;
    printTuple(tuple, tupleSize);
    RC rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success);
    

    // Test Delete Tuple
    rc = rm->deleteTuple(tableName, rid);
    assert(rc == success);
    cout<< "delete data done"<<endl;
    
    // Test Read Tuple
    memset(returnedData, 0, 100);
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc != success);

    cout << "After Deletion." << endl;
    
    // Compare the two memory blocks to see whether they are different
    if (memcmp(tuple, returnedData, tupleSize) != 0)
    {   
        cout << "****Test case 2 passed****" << endl << endl;
    }
    else
    {
        cout << "****Test case 2 failed****" << endl << endl;
    }
        
    free(tuple);
    free(returnedData);
    return;
}

void TEST_RM_3(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert Tuple    
    // 2. Update Tuple **
    // 3. Read Tuple
    cout << "****In Test Case 3****" << endl;
   
    RID rid; 
    int tupleSize = 0;
    int updatedTupleSize = 0;
    void *tuple = malloc(100);
    void *updatedTuple = malloc(100);
    void *returnedData = malloc(100);
   
    // Test Insert Tuple 
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
    RC rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success);
    cout << "Original RID slot = " << rid.slotNum << endl;

    // Test Update Tuple
    prepareTuple(6, "Newman", age, height, 100, updatedTuple, &updatedTupleSize);
    rc = rm->updateTuple(tableName, updatedTuple, rid);
    assert(rc == success);
    cout << "Updated RID slot = " << rid.slotNum << endl;

    // Test Read Tuple 
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success);
    cout << "Read RID slot = " << rid.slotNum << endl;
   
    // Print the tuples 
    cout << "Insert Data:" << endl; 
    printTuple(tuple, tupleSize);

    cout << "Updated data:" << endl;
    printTuple(updatedTuple, updatedTupleSize);

    cout << "Returned Data:" << endl;
    printTuple(returnedData, updatedTupleSize);
    
    if (memcmp(updatedTuple, returnedData, updatedTupleSize) == 0)
    {
        cout << "****Test case 3 passed****" << endl << endl;
    }
    else
    {
        cout << "****Test case 3 failed****" << endl << endl;
    }

    free(tuple);
    free(updatedTuple);
    free(returnedData);
    return;
}

void TEST_RM_4(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert tuple
    // 2. Read Attributes **
    cout << "****In Test Case 4****" << endl;
    
    RID rid;    
    int tupleSize = 0;
    void *tuple = malloc(100);
    void *returnedData = malloc(100);
    
    // Test Insert Tuple 
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
    RC rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success);

    // Test Read Attribute
    rc = rm->readAttribute(tableName, rid, "Salary", returnedData);
    assert(rc == success);
 
    cout << "Salary: " << *(int *)returnedData << endl;
    if (memcmp((char *)returnedData, (char *)tuple+18, 4) != 0)
    {
        cout << "****Test case 4 failed" << endl << endl;
    }
    else
    {
        cout << "****Test case 4 passed" << endl << endl;
    }
    
    free(tuple);
    free(returnedData);
    return;
}

void TEST_RM_5(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 0. Insert tuple;
    // 1. Read Tuple
    // 2. Delete Tuples **
    // 3. Read Tuple
    cout << "****In Test Case 5****" << endl;
    
    RID rid;
    int tupleSize = 0;
    void *tuple = malloc(100);
    void *returnedData = malloc(100);
    void *returnedData1 = malloc(100);
   
    // Test Insert Tuple 
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
    RC rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success);

    // Test Read Tuple
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success);
    printTuple(returnedData, tupleSize);

    cout << "Now Deleting..." << endl;

    // Test Delete Tuples
    rc = rm->deleteTuples(tableName);
    assert(rc == success);

    // Test Read Tuple
    memset((char*)returnedData1, 0, 100);
    rc = rm->readTuple(tableName, rid, returnedData1);

    assert(rc != success);
    printTuple(returnedData1, tupleSize);
    
    if(memcmp(tuple, returnedData1, tupleSize) != 0)
    {
        cout << "****Test case 5 passed****" << endl << endl;
    }
    else
    {
        cout << "****Test case 5 failed****" << endl << endl;
    }
       
    free(tuple);
    free(returnedData);
    free(returnedData1);
    return;
}

void TEST_RM_6(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 0. Insert tuple;
    // 1. Read Tuple
    // 2. Delete Table **
    // 3. Read Tuple
    cout << "****In Test Case 6****" << endl;
   
    RID rid; 
    int tupleSize = 0;
    void *tuple = malloc(100);
    void *returnedData = malloc(100);
    void *returnedData1 = malloc(100);
   
    // Test Insert Tuple
    prepareTuple(nameLength, name, age, height, salary, tuple, &tupleSize);
	cout << "****Insert..****" << endl;
	cout << "tableName is: " << tableName << endl;
	cout << "tupleSize is: " << tupleSize << endl;

    RC rc = rm->insertTuple(tableName, tuple, rid);
	cout << "****Inserted****" << endl;

    assert(rc == success);

    // Test Read Tuple 
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success);

    // Test Delete Table
    rc = rm->deleteTable(tableName);
    assert(rc == success);
    cout << "After deletion!" << endl;
    
    // Test Read Tuple 
    memset((char*)returnedData1, 0, 100);
    rc = rm->readTuple(tableName, rid, returnedData1);
    assert(rc != success);
    
    if(memcmp(returnedData, returnedData1, tupleSize) != 0)
    {
        cout << "****Test case 6 passed****" << endl << endl;
    }
    else
    {
        cout << "****Test case 6 failed****" << endl << endl;
    }
        
    free(tuple);
    free(returnedData);    
    free(returnedData1);
    return;
}

void TEST_RM_7(const string &tableName)
{
    // Functions Tested
    // 1. Reorganize Page **
    // Insert tuples into one page, reorganize that page, 
    // and use the same tids to read data. The results should 
    // be the same as before. Will check code as well.
    cout << "****In Test Case 7****" << endl;
   
    RID rid; 
    int tupleSize = 0;
    int numTuples = 5;
    void *tuple;
    void *returnedData = malloc(100);

    int sizes[numTuples];
    RID rids[numTuples];
    vector<char *> tuples;

    RC rc = 0;
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);

        // Test Insert Tuple
        float height = (float)i;
        prepareTuple(6, "Tester", 20+i, height, 123, tuple, &tupleSize);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success);

        tuples.push_back((char *)tuple);
        sizes[i] = tupleSize;
        rids[i] = rid;
        if (i > 0) {
            // Since we are inserting 5 tiny tuples into an empty table where the page size is 4kb, all the 5 tuples should be on the first page. 
            assert(rids[i - 1].pageNum == rids[i].pageNum);
        }
        cout << rid.pageNum << endl;
    }
    cout << "After Insertion!" << endl;
    
    int pageid = rid.pageNum;
    rc = rm->reorganizePage(tableName, pageid);
    assert(rc == success);

    // Print out the tuples one by one
    int i = 0;
    for (i = 0; i < numTuples; i++)
    {
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success);
        printTuple(returnedData, tupleSize);

        //if any of the tuples are not the same as what we entered them to be ... there is a problem with the reorganization.
        if (memcmp(tuples[i], returnedData, sizes[i]) != 0)
        {      
            cout << "****Test case 7 failed****" << endl << endl;
            break;
        }
    }
    if(i == numTuples)
    {
        cout << "****Test case 7 passed****" << endl << endl;
    }
    
    // Delete Table    
    rc = rm->deleteTable(tableName);
    assert(rc == success);

    free(returnedData);
    for(i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    return;
}

void TEST_RM_8_A(const string &tableName)
{
    // Functions Tested
    // 1. Simple scan **
    cout << "****In Test Case 8_A****" << endl;

    RID rid;    
    int tupleSize = 0;
    int numTuples = 100;
    void *tuple;
    void *returnedData = malloc(100);

    RID rids[numTuples];
    vector<char *> tuples;
    set<int> ages; 
    RC rc = 0;
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);

        // Insert Tuple
        float height = (float)i;
        int age = 20+i;
        prepareTuple(6, "Tester", age, height, 123, tuple, &tupleSize);
        ages.insert(age);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success);

        tuples.push_back((char *)tuple);
        rids[i] = rid;
    }
    cout << "After Insertion!" << endl;

    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi);
	cout << "rc is: " << rc << endl;
    assert(rc == success);

    cout << "Scanned Data:" << endl;
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        cout << "Age: " << *(int *)returnedData << endl;
        if (ages.find(*(int *)returnedData) == ages.end())
        {
            cout << "****Test case 8_A failed****" << endl << endl;
            rmsi.close();
            free(returnedData);
            for(int i = 0; i < numTuples; i++)
            {
                free(tuples[i]);
            }
            return;
        }
    }
    rmsi.close();

    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    cout << "****Test case 8_A passed****" << endl << endl; 
    return;
}

int TEST_RM_8_B(const string &tableName)
{
    // Functions Tested
    // 1. Simple scan **
    cout << "****In Test Case 8_B****" << endl;

    RID rid;    
    int numTuples = 100;
    void *returnedData = malloc(100);

    set<int> ages; 
    RC rc = 0;
    for(int i = 0; i < numTuples; i++)
    {
        int age = 20+i;
        ages.insert(age);
    }

    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi);
    if(rc != success) {
        cout << "****Test case 8_B failed****" << endl << endl;
        return -1;
    }

    cout << "Scanned Data:" << endl;
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        cout << "Age: " << *(int *)returnedData << endl;
        if (ages.find(*(int *)returnedData) == ages.end())
        {
            cout << "****Test case 8_B failed****" << endl << endl;
            rmsi.close();
            free(returnedData);
            return -1;
        }
    }
    rmsi.close();
    
    // Delete a Table
    rc = rm->deleteTable(tableName);
    if(rc != success) {
        cout << "****Test case 8_B failed****" << endl << endl;
        return -1;
    }

    free(returnedData);
    cout << "****Test case 8_B passed****" << endl << endl; 
    return 0;
}

//	    TEST_RM_9("tbl_employee4", rids, sizes);
void TEST_RM_9(const string &tableName, vector<RID> &rids, vector<int> &sizes)
{
    // Functions Tested:
    // 1. getAttributes
    // 2. insert tuple
    cout << "****In Test case 9****" << endl;

    RID rid; 
    void *tuple = malloc(1000);
    int numTuples = 2000;

    // GetAttributes
    vector<Attribute> attrs;
	//cout << "FLAG-" << endl;
    RC rc = rm->getAttributes(tableName, attrs);
	//cout << "FLAG+" << endl;
    assert(rc == success);
	//cout << "attrs.size() " << attrs.size() << endl;	

    for(unsigned i = 0; i < attrs.size(); i++)
    {
        cout << "Attribute Name: " << attrs[i].name << endl;
        cout << "Attribute Type: " << (AttrType)attrs[i].type << endl;
        cout << "Attribute Length: " << attrs[i].length << endl << endl;
    }

    // Insert 2000 tuples into table
    for(int i = 0; i < numTuples; i++)
    {
		//if(i > 337) { cout << "i is: " << i << endl; }
        // Test insert Tuple
        int size = 0;
        memset(tuple, 0, 1000);
        prepareLargeTuple(i, tuple, &size);

        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success);

        rids.push_back(rid);
        sizes.push_back(size);        
    }
    cout << "****Test case 9 passed****" << endl << endl;
    free(tuple);
    writeRIDsToDisk(rids);
    writeSizesToDisk(sizes);
}

void TEST_RM_10(const string &tableName, vector<RID> &rids, vector<int> &sizes)
{
    // Functions Tested:
    // 1. read tuple
    cout << "****In Test case 10****" << endl;

    int numTuples = 2000;
    void *tuple = malloc(1000);
    void *returnedData = malloc(1000);
    
    readRIDsFromDisk(rids, numTuples);
    readSizesFromDisk(sizes, numTuples);

    RC rc = 0;
    for(int i = 0; i < numTuples; i++)
    {
		cout << "i is: " << i << endl;
        memset(tuple, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success);

        int size = 0;
        prepareLargeTuple(i, tuple, &size);
		cout << "size should be: " << size << endl;

        int copy_val = memcmp(returnedData, tuple, sizes[i]);

        cout << "copy_val = " << copy_val << endl;

        if(copy_val != 0)
        {
            cout << "****Test case 10 failed****" << endl << endl;
            return;
        }
    }
    cout << "****Test case 10 passed****" << endl << endl;

    free(tuple);
    free(returnedData);
}

void TEST_RM_11(const string &tableName, vector<RID> &rids, vector<int> &sizes)
{
    // Functions Tested:
    // 1. update tuple
    // 2. read tuple
    cout << "****In Test case 11****" << endl;

    int numTuples = 2000;
    RC rc = 0;
    void *tuple = malloc(1000);
    void *returnedData = malloc(1000);
    
    readRIDsFromDisk(rids, numTuples);
    readSizesFromDisk(sizes, numTuples);

    // Update the first 1000 tuples
    int size = 0;
    for(int i = 0; i < 1000; i++)
    {
        memset(tuple, 0, 1000);
        RID rid = rids[i];

        prepareLargeTuple(i+10, tuple, &size);
        rc = rm->updateTuple(tableName, tuple, rid);
        assert(rc == success);

        sizes[i] = size;
        rids[i] = rid;
    }
    cout << "Updated!" << endl;

    // Read the recrods out and check integrity
    for(int i = 0; i < 1000; i++)
    {
        memset(tuple, 0, 1000);
        memset(returnedData, 0, 1000);
        prepareLargeTuple(i+10, tuple, &size);
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success);

        if(memcmp(returnedData, tuple, sizes[i]) != 0)
        {
            cout << "****Test case 11 failed****" << endl << endl;
            return;
        }
    }
    cout << "****Test case 11 passed****" << endl << endl;

    free(tuple);
    free(returnedData);
}

void TEST_RM_12(const string &tableName, vector<RID> &rids)
{
    // Functions Tested
    // 1. delete tuple
    // 2. read tuple
    cout << "****In Test case 12****" << endl;

    int numTuples = 2000;
    RC rc = 0;
    void * returnedData = malloc(1000);
    
    readRIDsFromDisk(rids, numTuples);

    // Delete the first 1000 tuples
    for(int i = 0; i < 1000; i++)
    {
        rc = rm->deleteTuple(tableName, rids[i]);
        assert(rc == success);

        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc != success);
    }
    cout << "After deletion!" << endl;

    for(int i = 1000; i < 2000; i++)
    {
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success);
    }
    cout << "****Test case 12 passed****" << endl << endl;

    free(returnedData);
}

void TEST_RM_13(const string &tableName)
{
    // Functions Tested
    // 1. scan
    cout << "****In Test case 13****" << endl;

    RM_ScanIterator rmsi;
    vector<string> attrs;
    attrs.push_back("attr5");
    attrs.push_back("attr12");
    attrs.push_back("attr28");
   
    RC rc = rm->scan(tableName, "", NO_OP, NULL, attrs, rmsi); 
    assert(rc == success);

    RID rid;
    int j = 0;
    void *returnedData = malloc(1000);

    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        if(j % 200 == 0)
        {
            int offset = 0;

            cout << "Real Value: " << *(float *)(returnedData) << endl;
            offset += 4;
        
            int size = *(int *)((char *)returnedData + offset);
            cout << "String size: " << size << endl;
            offset += 4;

            char *buffer = (char *)malloc(size + 1);
            memcpy(buffer, (char *)returnedData + offset, size);
            buffer[size] = 0;
            offset += size;
    
            cout << "Char Value: " << buffer << endl;

            cout << "Integer Value: " << *(int *)((char *)returnedData + offset ) << endl << endl;
            offset += 4;

            free(buffer);
        }
        j++;
        memset(returnedData, 0, 1000);
    }
    rmsi.close();
    cout << "Total number of tuples: " << j << endl << endl;

    cout << "****Test case 13 passed****" << endl << endl;
    free(returnedData);
}

void TEST_RM_14(const string &tableName, vector<RID> &rids)
{
    // Functions Tested
    // 1. reorganize page
    // 2. delete tuples
    // 3. delete table
    cout << "****In Test case 14****" << endl;

	int numTuples = 2000;
    readRIDsFromDisk(rids, numTuples);

    RC rc;
    rc = rm->reorganizePage(tableName, rids[1000].pageNum);
    assert(rc == success);

    rc = rm->deleteTuples(tableName);
    assert(rc == success);

    rc = rm->deleteTable(tableName);
    assert(rc == success);

    cout << "****Test case 14 passed****" << endl << endl;
}

void TEST_RM_15(const string &tableName) {

    cout << "****In Test case 15****" << endl;

    RID rid;
    int tupleSize = 0;
    int numTuples = 500;
    void *tuple;
    void *returnedData = malloc(100);
    int ageVal = 25;

    RID rids[numTuples];
    vector<char *> tuples;

    RC rc = 0;
    int age;
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);

        // Insert Tuple
        float height = (float)i;

        age = (rand()%20) + 15;

        prepareTuple(6, "Tester", age, height, 123, tuple, &tupleSize);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success);

        tuples.push_back((char *)tuple);
        rids[i] = rid;
    }
    cout << "After Insertion!" << endl;

    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, attr, GT_OP, &ageVal, attributes, rmsi);
    assert(rc == success);

    cout << "Scanned Data:" << endl;

    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        cout << "Age: " << *(int *)returnedData << endl;
        assert ( (*(int *) returnedData) > ageVal );
    }
    rmsi.close();

    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }

    cout << "****Test case 15 passed****" << endl << endl;
}

void TEST_RM_16(const string &tableName)
{
    cout << "****In Test Case 16****" << endl;

    // Get Catalog Attributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success);

    // print attribute name
    cout << "Catalog schema: (";
    for(unsigned i = 0; i < attrs.size(); i++)
    {
        if (i < attrs.size() - 1) cout << attrs[i].name << ", ";
        else cout << attrs[i].name << ")" << endl << endl;
    }

    RID rid;
    void *returnedData = malloc(100);

    // Set up the iterator
    RM_ScanIterator rmsi;
    vector<string> projected_attrs;
    for (int i = 0; i < attrs.size(); i++){
      projected_attrs.push_back(attrs[i].name);
    }

    rc = rm->scan(tableName, "", NO_OP, NULL, projected_attrs, rmsi);
    assert(rc == success);

    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        rbfm->printRecord(attrs, returnedData);
    }
    rmsi.close();

    free(returnedData);
    cout<<"** Test case 16 is over"<<endl;
    return;
}

void
createTables() {
	// Basic Functions
	cout << endl << "Create Tables ..." << endl;

	// Create Table tbl_employee
	createTable("tbl_employee");

	cout << endl << "Create Tables2 ..." << endl;
	// Create Table tbl_employee2
	createTable("tbl_employee2");


	cout << endl << "Create Tables3 ..." << endl;
	// Create Table tbl_employee3
	createTable("tbl_employee3");

	cout << endl << "Create Tables4 ..." << endl;
	// Create Table tbl_employee4
	createLargeTable("tbl_employee4");
}

/*********************************************************************************/

/**
 * The testing structure
 *
 *
 */

int main(int argc, char** argv) {
	//preliminary fields;
    string catalog_table_name = "Tables";
    vector<RID> rids;
    vector<int> sizes;

	if(argc != 2) {
		cout << "Error: please select a test to perform.\nSee specification in readme." << endl;
		exit(0);
	}

	int testSelector = -2;
	string param = argv[1];
	cout << param << endl;

	if(param.size() == 3) { //-0X or -1X
		testSelector = (param.at(1) - 48) * 10;
		testSelector += (param.at(2) - 48);
	} else if(param.size() == 4) { //-08a or -08b
		if(param.at(3) == 'a') {
			testSelector = 8;
		} else {
			testSelector = 108;
		}
	} else if(param.size() == 7) { //-08a or -08b
		testSelector = -1;
	}

	//cout << "selecting: " << testSelector << endl;

	switch(testSelector) {
	case -1:
	    createTables();
		break;
	case 0:
    	TEST_RM_0("tbl_employee");
		break;
	case 1:
	    TEST_RM_1("tbl_employee", 6, "Peters", 24, 170.1, 5000);
		break;
	case 2:
		TEST_RM_2("tbl_employee", 6, "Victor", 22, 180.2, 6000);
		break;
	case 3:
	    TEST_RM_3("tbl_employee", 6, "Thomas", 28, 187.3, 4000);
		break;
	case 4:
	    TEST_RM_4("tbl_employee", 6, "Veekay", 27, 171.4, 9000);
		break;
	case 5:
	    TEST_RM_5("tbl_employee", 6, "Dillon", 29, 172.5, 7000);
		break;
	case 6:
	    TEST_RM_6("tbl_employee", 6, "Martin", 26, 173.6, 8000);
		break;
	case 7:
	    TEST_RM_7("tbl_employee2");
		break;
	case 8:
	    TEST_RM_8_A("tbl_employee3");
		break;
	case 9:
	    TEST_RM_9("tbl_employee4", rids, sizes);
		break;
	case 10:
	    TEST_RM_10("tbl_employee4", rids, sizes);
		break;
	case 11:
	    TEST_RM_11("tbl_employee4", rids, sizes);
		break;
	case 12:
	    TEST_RM_12("tbl_employee4", rids);
		break;
	case 13:
	    TEST_RM_13("tbl_employee4");
		break;
	case 14:
	    TEST_RM_14("tbl_employee4", rids);
		break;
	case 15:
		createTable("tbl_b_employee");
	    TEST_RM_15("tbl_b_employee4");
		break;
	case 16:
    	TEST_RM_16(catalog_table_name);
		break;
	case 108:
	    TEST_RM_8_B("tbl_employee3");
		break;
	default:
		cout << "User error: Parameter not recognized." << endl;
	}

	cout << "Operations Complete." << endl;
	return 0;
}

