#include<iostream>
#include<vector>

using namespace std;

void largestIncreasingPositiveDotProductSeq(vector<double> velocityDirs, int &startIndex, int &endIndex) {
	
	int positiveVelSeqSize = 0;
	int flag = false;
	int maxSeqSize = 0;
	int st = 0;
	for (int j=0; j<velocityDirs.size()-1; j++) {
		double prevVel = velocityDirs[j];
		double currVel  = velocityDirs[j+1];
		
		double dotProd = prevVel*currVel;
		
		if( dotProd > 0 && flag == false) {
			st = j;
			positiveVelSeqSize++;
			flag = true;
			cout << "In first if positiveSize "<<positiveVelSeqSize<<endl;
			
		} else if (dotProd > 0 && flag == true) {
			positiveVelSeqSize++;
			cout << "In second if positive "<<positiveVelSeqSize<<endl;
		} else {
			positiveVelSeqSize = 0;
			flag = false;
			cout << "Else\n";			
			
		}
		
		if (positiveVelSeqSize > maxSeqSize) {
			maxSeqSize = positiveVelSeqSize;
			startIndex = st;
			endIndex = st+positiveVelSeqSize;
			cout << "maxseq updated \npositiveSize "<<positiveVelSeqSize<<endl;
			cout << "st "<<startIndex<<endl;
			cout << "end "<<endIndex<<endl;
		}

		
		
	}
	
}

int main () {

	vector<double> test(4);
	test[0]=0;
	test[1]=7;
	test[2]=0;
	test[3]=7;
	/*test[4]=5;
	test[5]=2;
	test[6]=-3;
	test[7]=-4;
	test[8]=2;
	test[9]=3;
	 */
	int st;
	int endIndex;
	
	largestIncreasingPositiveDotProductSeq(test,st, endIndex );
	for (int i=st; i<=endIndex; i++) {
		cout << test[i]<<endl;
	}
	cout << st << endl;
	cout << endIndex<<endl;
	
	/*cout << "index"<<endl;
	int size = 0;
	int i;
	int end = 14;
	st = 0;
	int c;
	for (i=end; i>=st; i=i-5) {
		c = i;
		cout << c << endl;
		size++;
	}
	if ((c-st) != 0) {
		cout <<"additional " <<st<<endl;
		size++;
	}
	cout << "last i "<<i;
	cout << "size = ";
	cout << size <<endl;
	 */
}
