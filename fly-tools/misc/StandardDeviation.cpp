#include<iostream>
#include<fstream>
#include<iomanip>
#include<cmath>
#include<vector>
using namespace std;

vector<double> currentHistogramValues;

int main(int argc, char* argv[]) {

	// argv[0] name of the executable
	// argv[1] input file containing the list of files
	// argv[2] output file containing the standard deviation and file name pair
	// argv[3] data file path
	// argv[4] data file postfix of metricname
	
	if (argc < 3) {
		cout<<"Please provide the parameters ./executable iputfilename outputfilename"<<endl;
		exit(0);
	}
	
	ifstream inputFileNames(argv[1]);
	if (inputFileNames.fail() == true) {
		cout << "Input File can not be opened"<<endl;
		exit(0);
	}
	
	ofstream outputFile(argv[2]);
	outputFile<<left<<setw(20)<<"File Name"<<left<<setw(20)<<"Standard deviation"<<left<<setw(20)<<"Mean"<<endl;
	
	string prefixPath(argv[3]);
	
	string metricName(argv[4]);
	
	string currentFileName;
	while(inputFileNames>>currentFileName) {
		
		string currentFileWithExtension = prefixPath + currentFileName + "/" + currentFileName + "_"+ metricName +".txt";
		ifstream currentFile(currentFileWithExtension.c_str());
		if (currentFile.fail() == true) {
			cout << currentFileName + " cannot be opened"<<endl;
			exit(0);
		}
		
		// N = sum(H_i) for i = 0 to M-1, M = number of samples in the histogram
		// mean = 1/N*(sum(i*H_i)) for i = 0 to M-1
		double currentValueOfHistogram;
		double sumOfValues = 0.0;
		currentHistogramValues.clear();
		double i = 0.0;
		double N = 0.0;
		double M = 0.0;
		while (currentFile >> currentValueOfHistogram) {
			sumOfValues = sumOfValues + i*currentValueOfHistogram;
			currentHistogramValues.push_back(currentValueOfHistogram);
			
			N = N + currentValueOfHistogram;
			i=i+1.0;
		}
		
		M = currentHistogramValues.size();
		// mean
		double mean = sumOfValues/N;
		
		
		// sigma^2 = (sum( (i-mean)^2*H_i ) )/(N-1) for i = 0 to M-1
		double standardDev = 0.0;
		double sumSquaredResults = 0.0;
		int j = 0;
		for (i=0.0; i<M; i=i+1.0) {
			sumSquaredResults += pow((i-mean), 2.0)*currentHistogramValues[j];
			j++;
		}
		
		// standard deviation
		
		standardDev = sumSquaredResults/(N-1);
		standardDev = sqrt(standardDev);
		
	
		outputFile<<left<<setw(20)<<currentFileName<<left<<setw(20)<<standardDev<<left<<setw(20)<<mean<<endl;
		
		currentFile.close();
	}
	
	inputFileNames.close();
	outputFile.close();

}