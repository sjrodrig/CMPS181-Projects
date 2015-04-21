#include <fstream>
#include <iostream>
#include <cassert>

#include "FileManager.h"
#include "RecordBasedFile.h"
#include "RelationManager.h"

using namespace std;

void rbfTest() {
  PagedFileManager *pfm = PagedFileManager::instance();
  RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();

  // write your own testing cases here
}


int main() {
	cout << "test..." << endl;

	rbfTest();
	// other tests go here

	cout << "OK" << endl;
	
	exit(0);
}
