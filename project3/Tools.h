#ifndef _Tools_h_
#define _Tools_h_

#include <bitset>
#include <math.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string>

/**
 * Tools for debugging
 * @author: Benjamin (Benjy) Strauss
 */

using std::string;
typedef unsigned char byte;

class Tools {
public:
	void printNBytes(byte* data, int size);
	void fprintNBytes(string filename, byte* data, int size);

};

#endif
