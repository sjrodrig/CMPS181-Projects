#include "Tools.h"

using namespace std;

void
Tools::printNBytes(byte* data, int size) {
	cout << "Printing:" << endl;
	byte outByte;
	double zeros = ceil( log10( (double) size) );
	int num_zeros = (int) zeros;
	num_zeros--;
	double top = pow(10, zeros);

	for(int aa = 0; aa < size; aa++) {
		outByte = data[aa];
		cout << "[";
		
		if( aa == 0 ) {
			for(int bb = 0; bb < num_zeros+1; bb++) {
				cout << "0";
			}
		} else {
			for(int spacer = aa; spacer < top; spacer *= 10) {
				cout << "0";
			}
		}

		cout << aa << "]";
 		cout << (bitset<8>) outByte;
		if ((aa+1) % 4 == 0) {
			cout << endl;
		} else {
			cout << " ";
		}
	}
	cout << endl;
}

void
Tools::fprintNBytes(string filename, byte* data, int size) {
	ofstream outfile;
	outfile.open(filename.c_str());

	byte outByte;
	double zeros = ceil( log10( (double) size) );
	int num_zeros = (int) zeros;
	num_zeros--;
	double top = pow(10, zeros);

	for(int aa = 0; aa < size; aa++) {
		outByte = data[aa];

		//if(outByte == 0) { continue; }

		outfile << "[";
		
		if( aa == 0 ) {
			for(int bb = 0; bb < num_zeros+1; bb++) {
				outfile << "0";
			}
		} else {
			for(int spacer = aa; spacer < top; spacer *= 10) {
				outfile << "0";
			}
		}

		outfile << aa << "]";
 		outfile << (bitset<8>) outByte;
		if ((aa+1) % 4 == 0) {
			outfile << endl;
		} else {
			outfile << " ";
		}
	}
	outfile << endl;
	outfile.close();
}





