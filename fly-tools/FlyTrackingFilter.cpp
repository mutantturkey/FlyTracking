#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#include <list>
#include <fstream>
#include <cassert>
#include <cstdlib>

#include <ImageMagick/Magick++.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_eigen.h>

#include "FrameInfo.h"

using namespace Magick;
using namespace std;
void findObj(Image* img, int x, int y, vector<pair<int,int> > & shape ,bool eightCon=true, bool colorLookingFor=true);
void eightConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
vector<double> covariantDecomposition(vector<pair<int,int> > & points);
pair<int,int> getCentroid(vector<pair<int,int> > & points);
bool isInterface(Image* orig, unsigned int x, unsigned int y);
void writeFrameImage(int fn, string imS);
int roundT(double v) {return int(v+0.5);}


const double PI = atan(1.0)*4.0;
const double FACTOR_EIGEN = 100;

Image* residual;

ostream &operator<<(ostream &out, FlyObject & fO) {
	fO.output(out);
	return out;
}

ostream &operator<<(ostream &out, FrameInfo & fI) {
	fI.output(out);
	return out;
}

vector<vector<pair<int, int> > > shapeVectors;
vector<pair<int,int> > shape;
vector<pair<int, int> > sizeNIndexVector;

void bubbleSort() {
	
	for(int i=1; i<sizeNIndexVector.size(); i++) {
		for(int j=0; j<sizeNIndexVector.size()-i; j++) {
			pair<int, int> a = sizeNIndexVector[j];
			pair<int, int> b = sizeNIndexVector[j+1];
			
			if (a.first < b.first) {
				pair<int, int> c = sizeNIndexVector[j];
				sizeNIndexVector[j] = sizeNIndexVector[j+1];
				sizeNIndexVector[j+1] = c;
			}
		}		
	}

}

void fillResidualWithObj(vector<pair<int, int> > & obj, ColorRGB c)
{
	for (unsigned int i = 0; i<obj.size(); i++)
		residual->pixelColor(obj[i].first, obj[i].second, c);
}

void writeHist(const char* filename, map<unsigned int, unsigned int> & len)
{
	map<unsigned int,unsigned int>::iterator front = len.begin(),
	back = len.end();
	back--;
	
	
	unsigned int first = front->first, last = back->first;
	/*if (cutoff != -1 && cutoff < int(last))
	 last = cutoff;
	 */
	cout << "Min: " << first << endl
	<< "Max: " << last << endl
	<< "Count: " << last-first << endl;
	//vector<unsigned int> hist(last-first, 0);
	vector<unsigned int> hist(last+1, 0);
	
	cout << "hist size: " << hist.size() << endl;
	try{
		for(unsigned int j = 0; j<first; j++) {
			hist[j] = 0;
		}
		for (unsigned int j = first; j<=last; j++)
		{
			/*if ( roundT(j-first) >= int(hist.size()) )
			 hist.resize(j-first,0);
			 hist[roundT(j-first)] = len[j];
			 */
			
			/*if ( roundT(j) >= int(hist.size()) )
			 hist.resize(j,0);
			 hist[roundT(j)] = len[j];
			 */
			hist[j] = len[j];
		}
	}
	catch (...)
	{ cerr << "Bad histogram bucketing" << endl; }
	
	/*if ( (cutoff >= 0) && (cutoff<int(hist.size())) )
	 hist.resize(cutoff);
	 */
	len.clear();
	try
	{
		ofstream fout(filename);
		for (unsigned int i = 0; i<hist.size(); i++) {
			fout << hist[i] << endl;
		}
		fout << first << " " << last << " " << hist.size() << endl;
		fout.close();
	}
	catch (...)
	{ cerr << "Bad memory loc for opening file" << endl; }
}

int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		cerr << "Usage: executablename <inputFile.txt> <ratio_largest_to_second_largest> <InputLocationOfMaskImage> <outputFolderName>" << endl; // input file contains name of the
		// input image files
		return -1;
	}
	
	MagickCore::SetMagickResourceLimit(MagickCore::MemoryResource, 1536);
	MagickCore::SetMagickResourceLimit(MagickCore::MapResource, 2048);
	
	string fileName;
	ifstream inputFile(argv[1]);
	if (inputFile.fail() ) {
		cout << "cannot open the input file that contains name of the input images\n";
		exit(1);
	}

	// get the output file name along with the location from argv[4]
	string outputFileLocation(argv[4]);
	string outputFileName = outputFileLocation + "final/outputFile.txt";
	ofstream outputFile(outputFileName.c_str());
	
	// get the location of the input mask file
	string inputMaskFileLocation(argv[3]);
	
	int frameCounter = 0;
	// use 15, 20 or 10
	// 15 found to be working correct
	double ratioSecondLargestToLargest = atof(argv[2]);
	
	cout << "Ratio is 1/"<<ratioSecondLargestToLargest<< " = "<<(1/ratioSecondLargestToLargest)<<endl;
	
	outputFile<<"Ratio given 1/"<<ratioSecondLargestToLargest<<" = "<<(1/ratioSecondLargestToLargest)<<endl;
	
	ratioSecondLargestToLargest = 1/ratioSecondLargestToLargest;
	// to find the largest object avg size
	
	long totalSize = 0;
	
	char buffer[100];
	
	while (inputFile>>fileName) {
		
		string savedFileName = fileName;
	
		// get the input mask file
//		fileName = "input/"+fileName;
		fileName =  inputMaskFileLocation + fileName;

		Image* img = new Image(fileName.c_str());
		int width = img->columns(),height = img->rows();
		Image* imgForFilter;
		sprintf(buffer,"%ix%i",width,height);
		
		// residual image is initialized with black representing not visited.
		residual = new Image(buffer, "black");
		
		imgForFilter = new Image(buffer, "white");
		cout << "Reading "<<savedFileName<<endl;
		cout << "Filter wxh "<<width<<","<<height<<endl;
		shape.clear();
		// find the black background from location (0,0)
		findObj(img, 0, 0, shape, false, false);
		int s = shape.size();
		if (s > 0)
			cout << "black object size is "<<s;
		for (int i=0; i<s; i++) {
			imgForFilter->pixelColor(shape[i].first, shape[i].second, "black");
		}
		
		// store the intermediate file in temp folder under the output location
//		string oFilteredFileName = "output/filtered/temp/"+outputFile;
		string oFilteredFileName =  outputFileLocation +"temp/"+savedFileName;
		
		cout << "Saving the filtered image "<< oFilteredFileName<<endl;
		imgForFilter->write(oFilteredFileName.c_str());

		delete residual;
		/*
		residual = new Image(buffer, "black");
		
		shapeVectors.clear();
		sizeNIndexVector.clear();
		
		// find the two largest object
		int objectCounter = 0;
		for (int x=0; x<width; x++) {
			for (int y=0; y<height; y++) {
				//find the white object now using eight connected neighbours
				shape.clear();
				findObj(imgForFilter, x, y, shape, true, true);
				int s = shape.size();
				if (s>0) {
					shapeVectors.push_back(shape);
					cout << "new object pushed back at position "<<x<<","<<y<<" of size "<<s<<endl;
					pair<int, int> si(s, objectCounter);
					sizeNIndexVector.push_back(si);
					objectCounter++;
				}

			}
		}
		
		// sort the sizes and take the largest to find average largest
		cout<<"shapeVectors size = "<<shapeVectors.size()<<endl;
		
		bubbleSort();
		
		cout <<"Largest object size is "<<sizeNIndexVector[0].first<<endl;
		
		// add the largest size to sum
		totalSize += static_cast<long>(sizeNIndexVector[0].first);
		
		cout << "Current total size is "<<totalSize<<" for object "<<(frameCounter+1)<<endl;
		
		cout << "-----------------------------------------------------------"<<endl;
		*/
		
		frameCounter++;
		
		delete imgForFilter;

//		delete residual;
		
		delete img;
	

	}
	
	inputFile.close();
	
	/*
	avgLargestSize = static_cast<long> (totalSize/frameCounter);
	cout << "Average largest size is "<<avgLargestSize<<endl;
	*/
	
	// previous loop calculates the average largest size
	 
	inputFile.open(argv[1]);
	
	if (inputFile.fail() == true) {
		cout << "Cannot open the input file again"<<endl;
		exit(1);
	}
	
	
	while (inputFile>>fileName) {
		
		//string inputFileName = "output/filtered/temp/"+fileName;
		string inputFileName =  outputFileLocation + "temp/"+fileName;
		
		Image* img = new Image(inputFileName.c_str());
		int width = img->columns();
		int height = img->rows();
		
		outputFile<<"File name is "<<inputFileName<<endl;
		outputFile<<"----------------------------------------------\n";
		
		sprintf(buffer,"%ix%i",width,height);
		
		// residual image is initialized with black representing not visited.
		residual = new Image(buffer, "black");
		
		cout << "Reading "<<inputFileName<<endl;
		cout << "Filter wxh "<<width<<","<<height<<endl;
		
		shapeVectors.clear();
		sizeNIndexVector.clear();

		// find the objects and sort according to size
		int objectCounter = 0;
		for (int x=0; x<width; x++) {
			for (int y=0; y<height; y++) {
				// find the white object using eight connected
				shape.clear();
				findObj(img, x, y, shape, true, true);
				int s = shape.size();
				
				if (s > 0) {
					
					shapeVectors.push_back(shape);
					cout << "New object found at position ("<<x<<","<<y<<") of size "<<s<<endl;
					pair<int, int> si(s, objectCounter);
					sizeNIndexVector.push_back(si);
					objectCounter++;
				
				}
					
			}
		}
		
		cout << "Total object found "<<sizeNIndexVector.size()<<endl;
		
		outputFile<<"Total objects found "<<sizeNIndexVector.size()<<endl;
		
		bubbleSort();
		
		// take the largest object
		double currentLargestSize = static_cast<double>(sizeNIndexVector[0].first);
		
		double secondLargest = 0;
		double ratio = 0;
		if (sizeNIndexVector.size() > 1) {
			
			secondLargest = static_cast<double>(sizeNIndexVector[1].first);
			ratio = secondLargest/currentLargestSize;
			cout << "Ratio is "<<secondLargest<<"/"<<currentLargestSize<<" = "<<ratio<<endl;
			outputFile<<"secondLargest = "<<secondLargest<<"\ncurrentLargest = "<<currentLargestSize<<"\nRatio = "<<ratio<<endl;
			
		}
		
		// find the largest to second largest ratio if it is less than the defined ratio then
		// the objects are single object
				
		int numberOfObjects = 0;
		
		if (sizeNIndexVector.size() == 1) {
			cout << "Frame contains one object\n";
			outputFile << "Frame contains one object\n";
			numberOfObjects = 1;
		}
		else if (ratio <= ratioSecondLargestToLargest ) {
			cout << "Single object calculated in the frame because current ratio = "<<ratio<<" is less than defined ratio = "<<ratioSecondLargestToLargest<<endl;
			outputFile<< "Single object calculated in the frame because current ratio = "<<ratio<<" is less than defined ratio = "<<ratioSecondLargestToLargest<<endl;
			numberOfObjects = 1;
		} else { 
			cout << "Two objects in the frame\n";
			outputFile<<"Two objects in the frame\n";
			numberOfObjects = 2;
		}
		
		cout << "Total object found "<<numberOfObjects<<endl;
		outputFile << "Total object found "<<numberOfObjects<<endl;
		
		Image* imgFinal = new Image(buffer, "black");
				
		for (int n=0; n<numberOfObjects; n++) {
			
			int totalPoints = sizeNIndexVector[n].first;
			
			for (int i=0; i<totalPoints; i++) {
				
				imgFinal->pixelColor(shapeVectors[ sizeNIndexVector[n].second ][i].first, shapeVectors[ sizeNIndexVector[n].second ][i].second, "white");
			}
			

		}
		
		//string finalImageName = "output/filtered/final/"+fileName;
		
		string finalImageName = outputFileLocation + "final/"+fileName;
		
		imgFinal->write( finalImageName.c_str() );
		
		
		
		
		// writing the single in red
		if (numberOfObjects == 1) {
			
			Image* singleObjectFinal = new Image(buffer, "black");
		
			int totalPoints = sizeNIndexVector[0].first;
			
			cout << "Output the single object of size = "<<totalPoints<<endl;
			
			for (int i=0; i<totalPoints; i++) {
				
				singleObjectFinal->pixelColor(shapeVectors[ sizeNIndexVector[0].second ][i].first, shapeVectors[ sizeNIndexVector[0].second ][i].second, "red");
			}
			
			//string singleImageName = "output/filtered/single/"+fileName;
			string singleImageName = outputFileLocation + "single/"+fileName;
			
			singleObjectFinal->write(singleImageName.c_str());
			
			delete singleObjectFinal;
			
		}
		
		outputFile<<"----------------------------------------------------\n";
		
			
		delete img;
		
		delete residual;
		
		delete imgFinal;
		
		
	}
	
	inputFile.close();
	outputFile.close();
	
	return 0;
}


void findObj(Image* img, int x, int y, vector<pair<int,int> > & shape ,bool eightCon, bool colorLookingFor)
{
	assert(residual != NULL);

	if (eightCon == true)
		eightConnObj(img, x, y, shape, colorLookingFor);
	else {
		fourConnObj(img, x, y, shape, colorLookingFor);
	}
}

int barrier = 1000;
void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color)
{
	int width = img->columns(),height = img->rows();
	
	// boundary violation check
	if ( (x >= (width)) || (x < 0) || (y >= (height) ) || (y < 0) )
		return;
	
	// residualpixel.mono() == true implies it is visited. Otherwise not visited.
	ColorMono residualpixel = ColorMono(residual->pixelColor(x,y));
	// originalpixel.mono() == true implies it is an object pixel. Otherwise it is blank region pixel.
	ColorMono originalpixel = ColorMono(img->pixelColor(x,y));
	
	// If the current pixel is already visited then return
	if (residualpixel.mono() == true)
		return;
	
	// Else if current pixel is not visited and it is black, which means it is not an object pixel; so return
	else if (residualpixel.mono() == false && originalpixel.mono() != color)
		return;
	// If current pixel is not visited and its value is white, which means a new object is found.
	else if (residualpixel.mono() == false && originalpixel.mono() == color) {
		// Save the coordinates of the current pixel into the vector and make the pixel visited in the residual image
		pair<int,int> p;
		p.first = x;
		p.second = y;
		obj.push_back(p);
		
//		if (obj.size() > barrier) {
//			cout<<obj.size()<<endl;
//			barrier = barrier + 1000;
//		}
		// setting the residual image at pixel(x,y) to white.
		residual->pixelColor(x,y, ColorMono(true));
		
		// Recursively call all of it's eight neighbours.
		fourConnObj(img, x+1, y, obj, color);
		fourConnObj(img, x, y-1, obj, color);
		
		fourConnObj(img, x-1, y, obj, color);
		fourConnObj(img, x, y+1, obj, color);
	}
	
}

void eightConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color)
{
	int width = img->columns(),height = img->rows();
	
	// boundary violation check
	if ( (x >= (width)) || (x < 0) || (y >= (height) ) || (y < 0) )
		return;
	
	// residualpixel.mono() == true implies it is visited. Otherwise not visited.
	ColorMono residualpixel = ColorMono(residual->pixelColor(x,y));
	// originalpixel.mono() == true implies it is an object pixel. Otherwise it is blank region pixel.
	ColorMono originalpixel = ColorMono(img->pixelColor(x,y));
	
	// If the current pixel is already visited then return
	if (residualpixel.mono() == true)
		return;
	
	// Else if current pixel is not visited and it is black, which means it is not an object pixel; so return
	else if (residualpixel.mono() == false && originalpixel.mono() != color)
		return;
	// If current pixel is not visited and its value is white, which means a new object is found.
	else if (residualpixel.mono() == false && originalpixel.mono() == color) {
		// Save the coordinates of the current pixel into the vector and make the pixel visited in the residual image
		pair<int,int> p;
		p.first = x;
		p.second = y;
		obj.push_back(p);

//		if (obj.size() > barrier) {
//			//cout<<obj.size()<<endl;
//			barrier = barrier + 1000;
//		}
		// setting the residual image at pixel(x,y) to white.
		residual->pixelColor(x,y, ColorMono(true));
		
		// Recursively call all of it's eight neighbours.
		eightConnObj(img, x+1, y, obj, color);
		eightConnObj(img, x+1, y-1, obj, color);
		eightConnObj(img, x, y-1, obj, color);
		eightConnObj(img, x-1, y-1, obj, color);
		
		eightConnObj(img, x-1, y, obj, color);
		eightConnObj(img, x-1, y+1, obj, color);
		eightConnObj(img, x, y+1, obj, color);
		eightConnObj(img, x+1, y+1, obj, color);		

	}
	
}

// Aspect Ratio
pair<int,int> getCentroid(vector<pair<int,int> > & points)
{
	pair<int,int> centroid;
	centroid.first = 0;
	centroid.second = 0;
	
	for (unsigned int i = 0; i<points.size(); i++)
	{
		centroid.first += points[i].first;
		centroid.second += points[i].second;
	}
	
	centroid.first = roundT(double(centroid.first)/points.size());
	centroid.second = roundT(double(centroid.second)/points.size());
	
	return centroid;
}


vector<double> covariantDecomposition(vector<pair<int,int> > & points)
{
	unsigned int i,j,k;
	pair<int,int> centroid = getCentroid(points);
	vector<double> retval;
	
	gsl_matrix* matrice = gsl_matrix_alloc(2, 2);
	
	double sumX2 = 0, sumXY = 0, sumY2 = 0;
	for (k = 0; k<points.size(); k++)
	{
		sumX2 += pow(double(points[k].first - centroid.first),2.0);
		sumY2 += pow(double(points[k].second - centroid.second),2.0);
		// should we take the absolute value of X*Y
		sumXY += (points[k].first - centroid.first) * (points[k].second - centroid.second);
	}
	gsl_matrix_set(matrice, 0, 0, roundT(sumX2/points.size()));
	gsl_matrix_set(matrice, 0, 1, roundT(sumXY/points.size()));
	gsl_matrix_set(matrice, 1, 0, roundT(sumXY/points.size()));
	gsl_matrix_set(matrice, 1, 1, roundT(sumY2/points.size()));
	
	//	outputMatrix("Covariant", matrice);
	
	// This function allocates a workspace for computing eigenvalues of n-by-n
	// real symmetric matrices. The size of the workspace is O(2n).
	gsl_eigen_symmv_workspace* eigenSpace = gsl_eigen_symmv_alloc(2);
	gsl_vector* eigenVal = gsl_vector_alloc(2);
	gsl_matrix* eigenVec = gsl_matrix_alloc(2, 2);
	// This function computes the eigenvalues and eigenvectors of the real
	// symmetric matrix A. Additional workspace of the appropriate size must be
	// provided in w. The diagonal and lower triangular part of A are destroyed
	// during the computation, but the strict upper triangular part is not
	// referenced. The eigenvalues are stored in the vector eval and are unordered.
	// The corresponding eigenvectors are stored in the columns of the matrix evec.
	// For example, the eigenvector in the first column corresponds to the first
	// eigenvalue. The eigenvectors are guaranteed to be mutually orthogonal and
	// normalised to unit magnitude.
	gsl_eigen_symmv (matrice, eigenVal, eigenVec, eigenSpace);
	gsl_eigen_symmv_free (eigenSpace);
	
	gsl_eigen_symmv_sort(eigenVal, eigenVec, GSL_EIGEN_SORT_VAL_ASC);
	
	for (i = 0; i<eigenVal->size; i++)
		retval.push_back(gsl_vector_get(eigenVal, i));
	
	for (j = 0; j<eigenVec->size2; j++)
		for (i = 0; i<eigenVec->size1; i++)
			retval.push_back(gsl_matrix_get(eigenVec, i, j));
	
	retval.push_back(static_cast<double>(centroid.first));
	retval.push_back(static_cast<double> (centroid.second));
	
//	for (i=0; i<2; i++) {
//		gsl_vector_view evec_i = gsl_matrix_column (eigenVec, i);
//		//printf ("eigenvalue = %g\n", eval_i);
//		cout<<"eigenvector = \n";
//		gsl_vector_fprintf (stdout, &evec_i.vector, "%g");
//	}
	
	gsl_vector_free(eigenVal);
	gsl_matrix_free(matrice);
	gsl_matrix_free(eigenVec);
	
	return retval;
}

// isInterface for binary image
bool isInterface(Image* orig, unsigned int x, unsigned int y)
{
	// Get the current pixel's color
	ColorMono currentpixel = (ColorMono)orig->pixelColor(x,y);
	// If the current pixel is black pixel then it is not boundary pixel
	// error check
	if (currentpixel.mono() == false)
		return false;
	
	// If the current pixel is not black then it is white. So, now we need
	// to check whether any four of its neighbor pixels (left, top, right,
	// bottom ) is black. If any of this neighbor is black then current
	// pixel is a neighbor pixel. Otherwise current pixel is not neighbor
	// pixel.
	
	ColorMono leftneighborpixel = (ColorMono)orig->pixelColor(x-1,y);
	ColorMono topneighborpixel = (ColorMono)orig->pixelColor(x,y-1);
	ColorMono rightneighborpixel = (ColorMono)orig->pixelColor(x+1,y);
	ColorMono bottomneighborpixel = (ColorMono)orig->pixelColor(x,y+1);
	
	// If leftneighborpixel is black and currentpixel is white then it is
	// boundary pixel
	if ( leftneighborpixel.mono() != currentpixel.mono())
		return true;
	// If topneighborpixel is black and currentpixel is white then it is
	// boundary pixel
	else if (topneighborpixel.mono() != currentpixel.mono())
		return true;
	// If rightneighborpixel is black and currentpixel is white then it
	// is boundary pixel
	else if (rightneighborpixel.mono() != currentpixel.mono())
		return true;
	// If bottomneighborpixel is black and currentpixel is white then it
	// is boundary pixel
	else if (bottomneighborpixel.mono() != currentpixel.mono())
		return true;
	// Else all of its neighbor pixels are white so it can not be a
	// boundary pixel
	else
		return false;
	
}