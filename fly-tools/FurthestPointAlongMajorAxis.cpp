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

const double PI = atan(1.0)*4.0;
const double FACTOR_EIGEN = 100;
const int STUCKING_TO_A_SINGLE_BLOB = 1;
const int SEPARATING_FROM_SINGLE_BLOB = 2;


Image* residual;

vector<FlyObject > fOVector;
vector<FrameInfo > fIVector;
// temporary store of the frames for finding initial head direction
vector<FrameInfo > fIForHeadVector;
// since for the first time the head is automatically need to be set
int startIndexToFindNewHead=0;
int endIndexToFindNewHead;
double initLargeLocationX=-1;
double initLargeLocationY=-1;
double initSmallLocationX=-1;
double initSmallLocationY=-1;
pair<double, double> largeCollisionBeforeDirection;
pair<double, double> smallCollisionBeforeDirection;

// start location in the vector where two object get together
int startIndexSingleBlob=-1;
int endIndexSingleBlob =-1;

// indices of the new set of sequences
int startOfATrackSequence = 0; // fix it later on inside main
int endOfATrackSequence=-1;
int sequenceSize=1;

// indices for printing the one object frames
int startOfAOneObject = -1;
int endOfAOneObject = -1;

vector<string> fnVector;
string inputFileName;

// GLOBAL PATHS
string maskImagePath;
string origImagePath;
string finalOutputPath;

vector<pair<double, double> > velocityDirectionsF;
vector<pair<double, double> > velocityDirectionsS;

pair<double, double> avgVelocityF;
pair<double, double> avgVelocityS;

pair<double, double> overAllVelocityF;
pair<double, double> overAllVelocityS;

vector<pair<double, double> > evDirectionF;
vector<pair<double, double> > evDirectionS;

void initSequence(){
	
	startOfATrackSequence = -1;
	endOfATrackSequence = -1;
	sequenceSize = 1;
	

}

ostream &operator<<(ostream &out, FlyObject & fO) {
	fO.output(out);
	return out;
}

ostream &operator<<(ostream &out, FrameInfo & fI) {
	fI.output(out);
	return out;
}

void bubbleSort(vector<FlyObject > & fov) {
	
	//FlyObject a,b,c;
	for(int i=1; i<fov.size(); i++) {
		for(int j=0; j<fov.size()-i; j++) {
			FlyObject a = fov[j];
			FlyObject b = fov[j+1];
			
			if (a.getArea() < b.getArea()) {
				FlyObject c = fov[j];
				fov[j] = fov[j+1];
				fov[j+1] = c;
			
			}
			
		}
		
	}

}

void findObj(Image* img, int x, int y, vector<pair<int,int> > & shape ,bool eightCon=true, bool colorLookingFor=true);
void eightConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
vector<double> covariantDecomposition(vector<pair<int,int> > & points);
pair<int,int> getCentroid(vector<pair<int,int> > & points);
bool isInterface(Image* orig, unsigned int x, unsigned int y);
void writeFrameImage(int fn, string imS);
void drawTheFlyObject(FrameInfo currentFI, string fileName, int isFirst, bool singleBlob=false);
void drawTheSequence(int startIndex, int endIndex, int isFirst, bool singleBlob = false);
double euclideanDist(FlyObject a, FlyObject b);
bool identifyFlyObjectFromFrameToFrame(FrameInfo prevFI, FrameInfo& currentFI, bool gotRidOfSingleBlob=false) ;
int roundT(double v) {return int(v+0.5);}
void determineHeadDirection(int fileCounter);


void normalizeVector(pair<double,double> &a);
double calculateDotProduct(pair<double, double> v, pair<double, double> eV);
void calculateHeadVector(FlyObject fO, pair<double,double> &headDirection);

/*
void lookAt(int x, int y, jzImage& img)
{
	int imageWidth =img.width();
	int imageHeight = img.height();
	// if current pixel is white
	if (img.pixelColor(x,y) == 1) {
		// if it was the first white pixel then
		if (!inWhite)
		{
			// check whether it started as a white pixel; if it started as white pixel then this line segment should
			// not be counted. because it is part of the larger white blob
			if ( (x == 0 || x == (imageWidth -1) ) || ( y == 0 || y == (imageHeight - 1)))
				startedWhite = true;
			inWhite = true;
			x_init = x;
			y_init = y;
		}
		// if we are on a white region
		//else {
		 
		 //}
		 
	}
	
	if (img.pixelColor(x,y) == 0) {
		// if we are going through a white line and reached the black pixel
		if (inWhite)
		{
			// if the line started as a white pixel then discard the line fragment
			if (startedWhite == false) {
				int val = roundT(dist(x_init, y_init, x, y));
				//cout<<" val is = " << val << " at (x0,y0) = " << x_init << "," << y_init << " to (x1,y1) = " << x << "," << y <<endl;
				len[val]++;
			} else
				startedWhite = false;
			
			inWhite = false;
		}
	}
}

void drawLine(int x0, int y0, int x1, int y1, jzImage& img)
{
	inWhite = false;
	startedWhite = false;
	
	// always go from x0 -> x1
	if (x0 > x1)
	{
		int temp = x0;
		x0 = x1;
		x1 = temp;
		temp = y0;
		y0 = y1;
		y1 = temp;
	}
	
	int dx, dy, d, x, y, incrE, incrNE;
	dx = x1 - x0;
	dy = y1 - y0;
	y = y0;
	x = x0;
	
	// if they are on a vertical line
	if (dx == 0)
	{
		if (y0 < y1)
			for (int i = y0; i<=y1; i++)
				lookAt(x,i,img);
		else
			for (int i = y1; i<=y0; i++)
				lookAt(x,i,img);
		return;
	}
	
	// if they are on a horizontal line
	if (dy == 0)
	{
		for (int i = x0; i<=x1; i++)
			lookAt(i,y,img);
		return;
	}
	
	int dir = 0;
	double m = double(dy)/double(dx);
	if (m >= 1.0)
		dir = 1;
	else if ( (m < 0.0) && (m > -1.0) )
		dir = 2;
	else if (m <= -1.0)
		dir = 3;
	
	switch(dir)
	{
			// when slope m: 0< m <1
		case 0:
			d = dy*2 - dx;
			incrE = dy*2;
			incrNE = (dy - dx)*2;
			x = x0;
			y = y0;
			
			lookAt(x,y,img);
			
			while (x<x1)
			{
				if (d <= 0)
				{
					d+= incrE;
					x++;
				}
				else
				{
					d+= incrNE;
					x++;
					y++;
				}
				lookAt(x,y,img);
			}
			break;
			// when slope m: m >= 1
		case 1:
			d = dx*2 - dy;
			incrE = dx*2;
			incrNE = (dx - dy)*2;
			x = x0;
			y = y0;
			
			lookAt(x,y,img);
			
			while (y<y1)
			{
				if (d <= 0)
				{
					d+= incrE;
					y++;
				}
				else
				{
					d+= incrNE;
					x++;
					y++;
				}
				lookAt(x,y,img);
			}
			break;
		case 2:
			d = dy*2 + dx;
			incrE = dy*2;
			incrNE = (dy + dx)*2;
			x = x0;
			y = y0;
			
			lookAt(x,y,img);
			
			while (x<x1)
			{
				if (d >= 0)
				{
					d+= incrE;
					x++;
				}
				else
				{
					d+= incrNE;
					x++;
					y--;
				}
				
				lookAt(x,y,img);
			}
			break;
			// since initially we swapped the P0 with P1 so that P0 always holds values with smaller x cooridinate
			// so we are sure that m<0 is the result of y0 being greater that
		case 3:
			d = dx*2 + dy;
			incrE = dx*2;
			incrNE = (dx + dy)*2;
			x = x0;
			y = y0;
			
			lookAt(x,y,img);
			
			while (y>y1)
			{
				if (d <= 0)
				{
					d+= incrE;
					y--;
				}
				else
				{
					d+= incrNE;
					x++;
					y--;
				}
				lookAt(x,y,img);
			}
			break;
	}
	
	
	if (inWhite)
	{
		// it is a fraction of the blob so dont count it
		//len[dist(x_init,y_init,x,y)+1]++;
		inWhite = false;
		startedWhite = false;
	}
}*/

bool isInMaleBlob;
bool isInBlackZone;
bool isInFemaleBlob;
bool isAtTheBoundary;
int maskImageHeight;
int maskImageWidth;
void putPixel(Image* maskImage, int x, int y, int color) {
	
	ColorMono currPixelColor = ColorMono(maskImage->pixelColor(x,y));
	
	// if current pixel is still inside the male fly do nothing
	// if current pixel is black and prior to that it was inside the male fly then enter the black zone
	if (currPixelColor.mono() == false and isInMaleBlob == true) {
		isInBlackZone = true;
		isInMaleBlob = false;
		cout << "Enters black zone "<<x<<","<<y<<endl;
	} else if (currPixelColor.mono() == true and isInBlackZone == true) {
		isInFemaleBlob = true;
		cout << "Hit the female fly"<<" "<<x<<","<<y<<endl;
	} else if (currPixelColor.mono() == false and isInBlackZone == true and ( x == 0 || x == (maskImageWidth-1) || y == 0 || y == (maskImageHeight-1) ) ) {
		isAtTheBoundary = true;
		cout << "Is at the boundary"<<endl;
	}

	
}


int draw_line_bm(Image* maskImage, int x0, int y0, int x1, int y1) {
	
	// mark the flag to indicate that it is still inside the male blob
	isInMaleBlob = true;
	isInBlackZone = false;
	isInFemaleBlob = false;
	isAtTheBoundary = false;
	maskImageHeight = maskImage->rows();
	maskImageWidth = maskImage->columns();
	
	int x, y;
	int dx, dy;
	dx = x1 - x0;
	dy = y1 - y0;
	double m = static_cast<double> (dy)/static_cast<double> (dx);
	
	int octant = -1;
	if (( m >= 0 and m <= 1) and x0 < x1 ) { 
		cout << "Octant 1"<<endl;
		octant = 1;
	} else if ((m > 1) and (y0 < y1)) {
		cout << "Octant 2"<<endl;
		octant = 2;
	} else if ((m < -1) and (y0 < y1)) {
		cout << "Octant 3"<<endl;
		octant = 3;
	} else if ((m <=0 and m >= -1) and (x0 > x1)) {
		cout << "Octant 4"<<endl;
		octant = 4;
	} else if ((m > 0 and m <=1) and (x0 > x1) ) {
		cout << "Octant 5"<<endl;
		octant = 5;
	}else if ((m > 1) and (y0 > y1) ) {
		cout << "Octant 6"<<endl;
		octant = 6;
	}else if ((m < -1) and (y0 > y1) ) {
		cout << "Octant 7"<<endl;
		octant = 7;
	} else if ((m <=0 and m >= -1) and (x0 < x1) ) {
		cout << "Octant 8"<<endl;
		octant = 8;
	}
	
	int d;
	int delE, delN, delW, delNE, delNW, delSW, delS, delSE;
	
	dx = abs(dx);
	dy = abs(dy);
	
	switch (octant) {
		case 1:
			//----------------------------
			d = 2*dy - dx;
			delE = 2*dy;
			delNE = 2*(dy-dx);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x, y, 1);
			
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			while (x<=x1) {
				// if we choose E because midpoint is above the line
				if (d <= 0) {
					
					d = d + delE;
					x = x+1;
				} else {
					
					d = d + delNE;
					x = x + 1;
					y = y + 1;
				}
				
				putPixel(maskImage,x, y, 1);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
			}
			break;
			
		case 2:
			
			d = 2*dx - dy;
			delN = 2*dx;
			delNE = 2*(dx-dy);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x, y, 2);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			
			while (y<=y1) {
				// if we choose N because midpoint is above the line
				if (d<=0) {
					d = d + delN;
					y = y + 1;
				} else {
					d = d + delNE;
					x = x + 1;
					y = y + 1;
				}
				putPixel(maskImage,x, y, 2);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
			}
			break;
			
		case 3:
			
			d = dy - 2*dx;
			delN = 2*dx;
			delNW = 2*(dx-dy);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x, y, 3);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			
			while (y<=y1) {
				
				if (d <= 0) {
					
					d = d + delN;
					//cout << "Going N d <= 0\n";
					y = y + 1;
					
				} else {
					d = d + delNW;
					//cout << "Going NW d > 0\n";
					y = y + 1;
					x = x - 1;
				}
				putPixel(maskImage,x, y, 3);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
				
			}
			
			break;
			
		case 4:
			
			d = -dx + 2*dy;
			//cout << "-dx + 2*dy = "<<(-dx+2*dy)<<endl;
			delW = 2*dy;
			//cout << "-2*dy = "<<(2*dy)<<endl;
			delNW = 2*(-dx + dy);
			//cout << "2*(-dx + dy) = "<<(2*(-dx + dy))<<endl;
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x, y, 4);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;

			
			while ( x >= x1 ) {
				
				if (d <= 0) {
					
					d = d + delW;
					x = x - 1;
					//cout << "At pixel(" <<x<<","<<y<<") Going W since d <= 0 "<<d<<endl;
					
					
				} else {
					d = d + delNW;
					
					//cout << "At pixel(" <<x<<","<<y<<") Going NW since d > 0 "<<d<<endl;
					
					x = x - 1;
					y = y + 1;
				}
				
				putPixel(maskImage,x, y, 4);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
			}
			
			break;
			
			/*case 4:
			 
			 d = dx - 2*dy;
			 cout << "dx - 2*dy = "<<(dx-2*dy)<<endl;
			 delW = -2*dy;
			 cout << "-2*dy = "<<(-2*dy)<<endl;
			 delNW = 2*(dx - dy);
			 cout << "2*(dx - dy) = "<<(2*(dx - dy))<<endl;
			 
			 x = x0;
			 y = y0;
			 
			 putpixel(x, y, 4);
			 
			 while ( x >= x1 ) {
			 
			 if (d <= 0) {
			 
			 d = d + delW;
			 x = x - 1;
			 cout << "At pixel(" <<x<<","<<y<<") Going W since d <= 0 "<<d<<endl;
			 
			 
			 } else {
			 d = d + delNW;
			 
			 cout << "At pixel(" <<x<<","<<y<<") Going NW since d > 0 "<<d<<endl;
			 
			 x = x - 1;
			 y = y + 1;
			 }
			 
			 putpixel(x, y, 4);
			 }
			 
			 break;
			 */
			
		case 5:
			
			d = -dx + 2*dy;
			delW = 2*dy;
			delSW = 2*(-dx+dy);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x, y, 5);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			
			while (x>=x1) {
				
				if (d<=0) {
					
					//cout << "Going W since d<0"<<endl;
					d = d + delW;
					x = x - 1;
					
				} else {
					d = d + delSW;
					
					x = x - 1;
					
					y = y - 1;
					
					//cout << "Going SW since d > 0"<<endl;
					
				}
				
				putPixel(maskImage,x, y, 5);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
				
			}
			
			break;
			
			
		case 6:
			
			d = 2*dx - dy;
			delS = 2*dx;
			delSW = 2*(dx-dy);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x,y,6);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			
			while (y>=y1) {
				
				if (d<=0) {
					d = d + delS;
					y = y -1;
				}
				else {
					d = d + delSW;
					y = y -1;
					x = x -1;
				}
				
				putPixel(maskImage,x, y, 6);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
				
			}
			
			break;
			
		case 7:
			
			d = 2*dx - dy;
			delS = 2*dx;
			delSE = 2*(dx-dy);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x,y,7);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			
			while (y>=y1) {
				
				if (d<=0) {
					d = d + delS;
					y = y -1;
				}
				else {
					d = d + delSE;
					y = y - 1;
					x = x + 1;
				}
				
				putPixel(maskImage,x, y, 7);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;				
				
			}
			
			break;
			
		case 8:
			
			d = 2*dy - dx;
			delE = 2*dy;
			delSE = 2*(dy - dx);
			
			x = x0;
			y = y0;
			
			putPixel(maskImage,x,y,8);
			if (isInFemaleBlob == true)
				return 1;
			else if (isAtTheBoundary == true)
				return 2;
			
			
			while (x<=x1) {
				
				if (d<=0) {
					d = d + delE;
					x = x + 1;
				}
				else {
					d = d + delSE;
					y = y - 1;
					x = x + 1;
				}
				
				putPixel(maskImage,x, y, 8);
				if (isInFemaleBlob == true)
					return 1;
				else if (isAtTheBoundary == true)
					return 2;
				
				
			}
			
			break;
			
		default:
			cout << "No octant which should be a bug\n";
			exit(0);
			break;

			
	}
	
	return 0;
	
}


void fillResidualWithObj(vector<pair<int, int> > & obj, ColorRGB c)
{
	for (unsigned int i = 0; i<obj.size(); i++)
		residual->pixelColor(obj[i].first, obj[i].second, c);
}
double euclideanDist(pair<int, int > newLocation, pair<int, int> initLocation) {
	
	double temp = pow((newLocation.first - initLocation.first), 2.0) + pow((newLocation.second - initLocation.second), 2.0);
	temp = sqrt(temp);
	return temp;

}
bool calculateDisplacement(FrameInfo prevFI, FrameInfo currentFI) {
	
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	
	FlyObject pFLargeFO = pFOVector[0];
	FlyObject pFSmallFO = pFOVector[1];
	
	FlyObject cFLargeFO = cFOVector[0];
	FlyObject cFSmallFO = cFOVector[1];
	
	pair<int, int> cFLargeCentroid = cFLargeFO.getCentroid();
	pair<int, int> initLarge(initLargeLocationX, initLargeLocationY);
	
	double largeDist = euclideanDist(cFLargeCentroid, initLarge);
	cout << "Large object current position "<<cFLargeCentroid.first<<" , "<<cFLargeCentroid.second<<endl;
	cout << "Large object went to "<<largeDist<<endl;
	
	pair<int, int> cFSmallCentroid = cFSmallFO.getCentroid();
	pair<int, int> initSmall(initSmallLocationX, initSmallLocationY);
	
	double smallDist = euclideanDist(cFSmallCentroid, initSmall);
	cout << "Small object current position "<<cFSmallCentroid.first<<" , "<<cFSmallCentroid.second<<endl;
	cout << "Small object went to "<<smallDist<<endl;
	
	// try first checking only one distance change
	// 20 pixel distance is too much to get away. It creates oscillation of forward and  backward movement
	// in the small fly since it follows the larger.
//	if (largeDist >= 20 || smallDist >=20)
//		return true;
//	else
//		return false;
	if (largeDist >= 5 || smallDist >=5)
		return true;
	else
		return false;
	
	
}

int sequenceCondition(FrameInfo prevFI,FrameInfo currentFI) {
	bool prevFIsSingleBlob = prevFI.getIsSingleBlob();
	bool currentFIsSingleBlob = currentFI.getIsSingleBlob();
	
	if (prevFIsSingleBlob == false and currentFIsSingleBlob == true)
		return STUCKING_TO_A_SINGLE_BLOB;
	else if (prevFIsSingleBlob == true and currentFIsSingleBlob == false)
		return SEPARATING_FROM_SINGLE_BLOB;
	else
		return -1;
	
}


bool isStuckToSingleBlob(FrameInfo prevFI,FrameInfo currentFI) {
	bool prevFIsSingleBlob = prevFI.getIsSingleBlob();
	bool currentFIsSingleBlob = currentFI.getIsSingleBlob();
	
	if (prevFIsSingleBlob == false and currentFIsSingleBlob == true)
		return true;
	else 
		return false;
	
}


bool isStartOfNewHeadDirectionSearch(FrameInfo prevFI, FrameInfo currentFI) {

	bool prevFIsSingleBlob = prevFI.getIsSingleBlob();
	bool currentFIsSingleBlob = currentFI.getIsSingleBlob();
	
	if (prevFIsSingleBlob == true and currentFIsSingleBlob == false)
		return true;
	else 
		return false;
	
}

void separateObjectAndInitForNewHeadCalculation(FrameInfo prevFI, FrameInfo currentFI) {
	
	// ASSUMPTION : the bigger object retains the previous frame head direction. And the smaller frame head calculation starts
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	// previous frame should have only one frame when this function is called
	if (pFOVector.size() > 1) {
		cerr<<"Previous frame does contain more than one fly object in the funciton separateObjectAndInitForNewHeadCalculation()"<<endl;
	}
	
	FlyObject pFLargeFO = pFOVector[0];

	FlyObject cFLargeFO = cFOVector[0];
	FlyObject cFSmallFO = cFOVector[1];
	
	pair<int, int> currentFLargeCentroid = cFLargeFO.getCentroid();
	initLargeLocationX = currentFLargeCentroid.first;
	initLargeLocationY = currentFLargeCentroid.second;

	cout << "Init location of the large object when get departed first ("<<initLargeLocationX<<" , "<<initLargeLocationY<<")"<<endl;
	
	pair<int, int> currentFSmallCentroid = cFSmallFO.getCentroid();
	initSmallLocationX = currentFSmallCentroid.first;
	initSmallLocationY = currentFSmallCentroid.second;
	
	
	cout << "Init location of the small object when get departed first ("<<initSmallLocationX<<" , "<<initSmallLocationY<<")"<<endl;
	
	// Later on may try to find the object that moves toward the previous frames direction
	
}


pair<double, double> windowLargeHeadDirection;


void computeHeadBackward(FrameInfo nextFI, FrameInfo &currentFI) {
	vector<FlyObject > nextFOVector = nextFI.getFOVector();
	vector<FlyObject > currentFOVector = currentFI.getFOVector();
	
	// next frame objects
	FlyObject nextFLargeFO = nextFOVector[0];
	FlyObject nextFSmallFO = nextFOVector[1];
	
	// current frame objects
	FlyObject currentFLargeFO = currentFOVector[0];
	FlyObject currentFSmallFO = currentFOVector[1];
	
	// larger
	pair<double, double> cLEV = currentFLargeFO.getMajorAxisEV();
	normalizeVector(cLEV);
	// historical head
	pair<double, double> nLHH = nextFLargeFO.getHead();
	// next frame head direction
//	pair<double, double> nLHD;
//	calculateHeadVector(nextFLargeFO, nLHD);
	// take the minimum angle with the historical head
	pair<int, int> nCentroid = nextFLargeFO.getCentroid();
	cout << "Next Large was at "<<nCentroid.first<<","<<nCentroid.second<<endl;
	cout << "Next large Historical Head was at direction "<<nLHH.first<<","<<nLHH.second<<endl;
	double largeDotProdNHWCEV = calculateDotProduct(nLHH, cLEV);
	pair<double, double> cLREV(-cLEV.first, -cLEV.second);
	if (largeDotProdNHWCEV >=0) {
		cout << "Current eigen is in direction with the historical head"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*cLEV.first+0.9*nLHH.first;
		double newHeadSecond = 0.1*cLEV.second+0.9*nLHH.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		currentFLargeFO.setHead(newHead);
		currentFLargeFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		//identifiedELEV = eLEV;
		
	} else {
		cout << "Current eigen is in opposite direction of the historical head\n";
		// record this vector
		double newHeadFirst = 0.1*cLREV.first+0.9*nLHH.first;
		double newHeadSecond = 0.1*cLREV.second+0.9*nLHH.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		currentFLargeFO.setHead(newHead);
		currentFLargeFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV
		//identifiedELEV = eLEV;
		
	}

	
	// small
	
	pair<double, double> cSEV = currentFSmallFO.getMajorAxisEV();
	normalizeVector(cSEV);
	// small historical head
	pair<double, double> nSHH = nextFSmallFO.getHead();
	// next frame head direction
//	pair<double, double> nSHD;
//	calculateHeadVector(nextFSmallFO, nSHD);
	// take the minimum angle with the historical head
	pair<int,int> nSCentroid = nextFSmallFO.getCentroid();
	cout << "Next small centroid at "<<nSCentroid.first<<","<<nSCentroid.second<<endl;
	cout << "Next small historical head direction "<<nSHH.first<<","<<nSHH.second<<endl;
	
	double smallDotProdNHWCEV = calculateDotProduct(nSHH, cSEV);
	pair<double, double> cSREV(-cSEV.first, -cSEV.second);
	if (smallDotProdNHWCEV >=0) {
		cout << "Current eigen is in direction with the historical head"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*cSEV.first+0.9*nSHH.first;
		double newHeadSecond = 0.1*cSEV.second+0.9*nSHH.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		currentFSmallFO.setHead(newHead);
		currentFSmallFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		//identifiedELEV = eLEV;
		
	} else {
		cout << "Current eigen is in opposite direction of the historical head\n";
		// record this vector
		double newHeadFirst = 0.1*cSREV.first+0.9*nSHH.first;
		double newHeadSecond = 0.1*cSREV.second+0.9*nSHH.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		currentFSmallFO.setHead(newHead);
		currentFSmallFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV
		//identifiedELEV = eLEV;
		
	}
	
	
	// update the currentFrame
	// update the fIForHeadVector update
	vector<FlyObject > updatedFOVector;
	updatedFOVector.push_back(currentFLargeFO);
	updatedFOVector.push_back(currentFSmallFO);
	
	currentFI.setFOVector(updatedFOVector);
	
	
}

void calculateNewHeadDirection() {
	
	cout << "Start calculating new head direction for large,small fly objects"<<endl;
	cout << "StartIndex "<<startIndexToFindNewHead<<" endIndex "<<endIndexToFindNewHead<<endl;
	
	FrameInfo firstFI = fIForHeadVector[0];
	FrameInfo endFI = fIForHeadVector[endIndexToFindNewHead-startIndexToFindNewHead];
	
	// movement direction detection
	vector<FlyObject > firstFOVector = firstFI.getFOVector();
	vector<FlyObject > endFOVector = endFI.getFOVector();
	
	// init frame objects
	FlyObject firstFLargeFO = firstFOVector[0];
	FlyObject firstFSmallFO = firstFOVector[1];
	
	// end frame objects
	FlyObject endFLargeFO = endFOVector[0];
	FlyObject endFSmallFO = endFOVector[1];
	
	// large
	pair<int, int > firstFLargeFOCentroid = firstFLargeFO.getCentroid();
	pair<int, int > endFLargeFOCentroid = endFLargeFO.getCentroid();
	double largeDirectionX = endFLargeFOCentroid.first - firstFLargeFOCentroid.first;
	double largeDirectionY = endFLargeFOCentroid.second - firstFLargeFOCentroid.second;
	pair<double, double > largeDirection(largeDirectionX, largeDirectionY);
	normalizeVector(largeDirection);

	
	// find for the large object
	cout << "For the larger end frame head finding \n";
	pair<double, double> eLEV = endFLargeFO.getMajorAxisEV();
	// normalize the eigenvector
	normalizeVector(eLEV);
	pair<double,double> identifiedELEV;
	
//	// the head from the previous frame
//	pair<double, double> pLH = pFLargeFO.getHead();
	
	// find the whether the object is moving forward or backward
	double largerDotProdMovDirAndLCBD = calculateDotProduct(largeDirection, largeCollisionBeforeDirection);
	
	// find the dot product with the large object movement direction with end frames major axis both direction
	double largerDotProdMovDirAndELEV = calculateDotProduct(largeDirection, eLEV);
	pair<double,double> eLREV(-eLEV.first, -eLEV.second);
	//double largerDotProdMovDirAndELEV = calculateDotProduct(pLH, clREV);
	if (largerDotProdMovDirAndELEV >=0 and largerDotProdMovDirAndLCBD >= 0) {
		cout << "Current eigen is in direction with the movement direction and moving forward \n";
		// record this vector for history
		double newHeadFirst = 0.1*eLEV.first+0.9*largeDirection.first;
		double newHeadSecond = 0.1*eLEV.second+0.9*largeDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFLargeFO.setHead(newHead);
		endFLargeFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		identifiedELEV = eLEV;
		
	} else if (largerDotProdMovDirAndELEV <0 and largerDotProdMovDirAndLCBD >=0){
		cout << "Current eigen is in opposite direction of the movement direction and moving forward\n";
		// record this vector
		double newHeadFirst = 0.1*eLREV.first+0.9*largeDirection.first;
		double newHeadSecond = 0.1*eLREV.second+0.9*largeDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFLargeFO.setHead(newHead);
		endFLargeFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV to reverse direction of the current EV
		identifiedELEV = eLREV;
		
	}

	// moving backward
	else if (largerDotProdMovDirAndELEV <0 and largerDotProdMovDirAndLCBD <0) {
		cout << "Current eigen is in direction with the movement direction and moving backward"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*eLEV.first+0.9*largeDirection.first;
		double newHeadSecond = 0.1*eLEV.second+0.9*largeDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFLargeFO.setHead(newHead);
		endFLargeFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		identifiedELEV = eLEV;
		
	} else if (largerDotProdMovDirAndELEV >=0 and largerDotProdMovDirAndLCBD <0 ) {
		cout << "Current eigen is in opposite direction of movement direction and moving backward\n";
		// record this vector
		double newHeadFirst = 0.1*eLREV.first+0.9*largeDirection.first;
		double newHeadSecond = 0.1*eLREV.second+0.9*largeDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFLargeFO.setHead(newHead);
		endFLargeFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV to reverse direction of the current EV
		identifiedELEV = eLREV;
		
	}
	
	
	
	
	// small
	pair<int, int > firstFSmallFOCentroid = firstFSmallFO.getCentroid();
	pair<int, int > endFSmallFOCentroid = endFSmallFO.getCentroid();
	double smallDirectionX = endFSmallFOCentroid.first - firstFSmallFOCentroid.first;
	double smallDirectionY = endFSmallFOCentroid.second - firstFSmallFOCentroid.second;
	pair<double, double > smallDirection(smallDirectionX, smallDirectionY);
	cout << "Small Direction "<<smallDirectionX<<","<<smallDirectionY<<endl;
	normalizeVector(smallDirection);
	cout << "Normalized small Direction "<<smallDirection.first<<","<<smallDirection.second<<endl;
	pair<double, double> smallRevDirection(-smallDirection.first, -smallDirection.second);
	cout << "Normalized small Reverse Direction "<<smallRevDirection.first<<","<<smallRevDirection.second<<endl;
	
	// find for the small object
	pair<double, double> eSEV = endFSmallFO.getMajorAxisEV();
	// normalize the eigenvector
	normalizeVector(eSEV);
	pair<double,double> identifiedESEV;
	
	// find the whether the object is moving forward or backward
	double smallerDotProdMovDirAndSCBD = calculateDotProduct(smallDirection, smallCollisionBeforeDirection);
	
	// find the dot product with the small object movement direction with end frames major axis both direction
	double smallerDotProdMovDirAndESEV = calculateDotProduct(smallDirection, eSEV);
	pair<double,double> eSREV(-eSEV.first, -eSEV.second);

	if (smallerDotProdMovDirAndESEV >=0 && smallerDotProdMovDirAndSCBD >=0) {
		cout << "Current eigen is in direction with the movement direction and moving forward "<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*eSEV.first+0.9*smallDirection.first;
		double newHeadSecond = 0.1*eSEV.second+0.9*smallDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFSmallFO.setHead(newHead);
		endFSmallFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		identifiedESEV = eSEV;
		
	} else if (smallerDotProdMovDirAndESEV <0 and smallerDotProdMovDirAndSCBD >=0) {
		cout << "Current eigen is in opposite direction of the movement direction and moving forward \n";
		// record this vector
		double newHeadFirst = 0.1*eSREV.first+0.9*smallDirection.first;
		double newHeadSecond = 0.1*eSREV.second+0.9*smallDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFSmallFO.setHead(newHead);
		endFSmallFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV
		identifiedESEV = eSREV;
		
	} else if (smallerDotProdMovDirAndESEV <0 and smallerDotProdMovDirAndSCBD <0  ) {
		cout << "Current eigen is in direction with the movement direction and moving backward"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*eSEV.first+0.9*smallRevDirection.first;
		double newHeadSecond = 0.1*eSEV.second+0.9*smallRevDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFSmallFO.setHead(newHead);
		endFSmallFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		identifiedESEV = eSEV;
		
	} else if (smallerDotProdMovDirAndESEV >=0 and smallerDotProdMovDirAndSCBD <0 ) {
		cout << "Current eigen is in opposite direction with the movement direction and moving backward\n";
		// record this vector
		double newHeadFirst = 0.1*eSREV.first+0.9*smallRevDirection.first;
		double newHeadSecond = 0.1*eSREV.second+0.9*smallRevDirection.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		endFSmallFO.setHead(newHead);
		endFSmallFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV
		identifiedESEV = eSREV;
		
	}

	
	
	// update the fIForHeadVector update
	vector<FlyObject > updatedFOVector;
	updatedFOVector.push_back(endFLargeFO);
	updatedFOVector.push_back(endFSmallFO);
	
	endFI.setFOVector(updatedFOVector);
	
	// update the vector with the correct info
	fIForHeadVector[endIndexToFindNewHead-startIndexToFindNewHead] = endFI;
	
	// calculate the head directions from end to first backwards
	FrameInfo nextFI = fIForHeadVector[endIndexToFindNewHead-startIndexToFindNewHead];
	
	cout << "fIForHeadVector size "<<fIForHeadVector.size()<<endl;
	
	for (int i=fIForHeadVector.size()-2; i>=0; i--) {
		
		cout << "Calculating backwards head direction i "<<i<<endl;
		FrameInfo currentFI = fIForHeadVector[i];
		computeHeadBackward(nextFI, currentFI);
		// update the frame after calculating the information
		fIForHeadVector[i] = currentFI;
		
		nextFI = currentFI;
	
	}
	
	cout << "New head direction done size "<<fIForHeadVector.size()<<endl;
	cout << "Copying into the fIVector from position "<<startIndexToFindNewHead<<" to "<<endIndexToFindNewHead<<endl;
	for (int i=0; i<=(endIndexToFindNewHead-startIndexToFindNewHead); i++) {
		cout << "Copying to fIVector[(startIndex + i)="<<(startIndexToFindNewHead+i)<<" from fIForNewHead[i="<<i<<endl;
		fIVector[startIndexToFindNewHead+i] = fIForHeadVector[i];
	}
}

void drawTheSequence(int startIndex, int endIndex, int isFirst, bool singleBlob) {

	cout << "Should draw "<<isFirst<<endl;
	//ifstream inputFile;
	//inputFile.open(inputFileName.c_str());
	/*if (inputFile.fail() ) {
		cout << "cannot open the input file that contains name of the input images\n";
		exit(1);
	}*/
	
	string fileName = fnVector[startIndex];
	//inputFile>>fileName;
	//inputFileName = "output/identified/"+fileName;
	FrameInfo prevFI = fIVector[startIndex];
	cout << "Extracting information for image "<< prevFI.getFrameNo() << endl;
	cout << "----------------------------------------\n";
	cout<<prevFI;
	drawTheFlyObject(prevFI, fileName, isFirst, singleBlob);
	
	for (int i=startIndex+1; i<=endIndex; i++) {
		FrameInfo nextFI = fIVector[i];
		cout << "Extracting information for image "<< nextFI.getFrameNo() << endl;
		cout << "----------------------------------------\n";
		//FrameInfo na = &nextFI;
		cout<<nextFI;
		//inputFile>>fileName;
		fileName = fnVector[i];
		//	inputFileName = "output/identified/"+fileName;
		drawTheFlyObject(nextFI, fileName,  isFirst, singleBlob);
	}
	
	//inputFile.close();
	
}

void normalizeVector(pair<double,double> &a) {
	double temp = a.first*a.first + a.second*a.second;
	temp = sqrt(temp);
	if (temp != 0) {
		a.first = a.first/temp;
		a.second = a.second/temp;
	}
}
double calculateDotProduct(pair<double, double> v, pair<double, double> eV) {
	return (v.first*eV.first + v.second*eV.second);
}
void calculateHeadVector(FlyObject fO, pair<double,double> &headDirection) {
	bool headDirectionBool = fO.getHeadIsInDirectionMAEV();
	pair<double,double> fOMajorAxis = fO.getMajorAxisEV();
	if (headDirectionBool == true) {
		headDirection.first = fOMajorAxis.first;
		headDirection.second = fOMajorAxis.second;
	} else {
		headDirection.first = -fOMajorAxis.first;
		headDirection.second = -fOMajorAxis.second;	
	}
}
void determineHeadDirection(int fileCounter) {
	
	FrameInfo prevFI = fIVector[fileCounter-1];
	FrameInfo currentFI = fIVector[fileCounter];
	
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	
	FlyObject pFLargeFO = pFOVector[0];
	FlyObject pFSmallFO = pFOVector[1];
	
	FlyObject cFLargeFO = cFOVector[0];
	FlyObject cFSmallFO = cFOVector[1];
	
	// calculate velocity
	pair<int,int> pFLCentroid = pFLargeFO.getCentroid();
	pair<int,int> pFSCentroid = pFSmallFO.getCentroid();
	
	pair<int,int> cFLCentroid = cFLargeFO.getCentroid();
	pair<int,int> cFSCentroid = cFSmallFO.getCentroid();
	
	double velocityXLarge = static_cast<double> (cFLCentroid.first) - static_cast<double> (pFLCentroid.first);
	double velocityYLarge = static_cast<double> (cFLCentroid.second) - static_cast<double> (pFLCentroid.second);
	
	double velocityXSmall = static_cast<double> (cFSCentroid.first) - static_cast<double> (pFSCentroid.first);
	double velocityYSmall = static_cast<double> (cFSCentroid.second) - static_cast<double> (pFSCentroid.second);
	
	pair<double,double> cLVV(velocityXLarge,velocityYLarge);
	pair<double,double> cSVV(velocityXSmall,velocityYSmall);
	
	cFLargeFO.setVelocityV(cLVV);
	cFSmallFO.setVelocityV(cSVV);
	
	// normalize the velocity
	cFLargeFO.normalizeVelocity();
	cFSmallFO.normalizeVelocity();
	
	// determine the head direction for larger object
	pair<double, double> cLEV = cFLargeFO.getMajorAxisEV();
	// normalize the eigenvector
	normalizeVector(cLEV);
	pair<double,double> identifiedCLEV;
	
	// the head from the previous frame
	pair<double, double> pLH = pFLargeFO.getHead();
	
	// find the dot product with the previous frame head with current major axis both direction
	double largerDotProdPrevHeadAndCLEV = calculateDotProduct(pLH, cLEV);
	pair<double,double> cLREV(-cLEV.first, -cLEV.second);
	//double largerDotProdPrevHeadAndCLREV = calculateDotProduct(pLH, clREV);
	if (largerDotProdPrevHeadAndCLEV >=0) {
		cout << "Current eigen is in direction with the historical head"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*cLEV.first+0.9*pLH.first;
		double newHeadSecond = 0.1*cLEV.second+0.9*pLH.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		cFLargeFO.setHead(newHead);
		cFLargeFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV
		identifiedCLEV = cLEV;
		
	} else {
		cout << "Current eigen is in opposite direction of the historical head\n";
		// record this vector
		double newHeadFirst = 0.1*cLREV.first + 0.9*pLH.first;
		double newHeadSecond =0.1*cLREV.second+0.9*pLH.second;
		pair<double, double> newHead(newHeadFirst,newHeadSecond);
		cFLargeFO.setHead(newHead);
		cFLargeFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV to reverse direction of the current EV
		identifiedCLEV = cLREV;
		
	}
	
	
	// tracking the forward and backward movement of the fly object
	double largeDotProd = calculateDotProduct(cLVV, identifiedCLEV);
	cout << "largerDotProd "<<largeDotProd<<endl;
	if (largeDotProd >= 0) {
		cout<<"Larger Dot prod is positive"<<endl;
		//		cFLargeFO.setHeadIsInDirectionMAEV(true);
	} else {
		cout<<"Larger Dot prod is negative"<<endl;
		//		cFLargeFO.setHeadIsInDirectionMAEV(false);
	}
	
	
	pair<double, double > cSEV = cFSmallFO.getMajorAxisEV();
	normalizeVector(cSEV);
	pair<double,double> identifiedCSEV;
	// head from the previous frame
	pair<double,double> pSH = pFSmallFO.getHead();
	
	// find the dot product with the previous frame head with current major axis both direction
	double smallerDotProdPrevHeadAndCSEV = calculateDotProduct(pSH, cSEV);
	pair<double, double> cSREV(-cSEV.first, -cSEV.second);
	if (smallerDotProdPrevHeadAndCSEV >=0) {
		cout << "Current eigen is in direction with the historical head for the smaller fly object"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*cSEV.first + 0.9*pSH.first;
		double newHeadSecond = 0.1*cSEV.second + 0.9*pSH.second;
		pair<double,double> newHead(newHeadFirst, newHeadSecond);
		cFSmallFO.setHead(newHead);
		cFSmallFO.setHeadIsInDirectionMAEV(true);
		// set the identified EV to current EV
		identifiedCSEV = cSEV;
	} else {
		cout << "Current eigen is in direction with the historical head for the smaller fly object"<<endl;
		// record this vector for history
		double newHeadFirst = 0.1*cSREV.first + 0.9*pSH.first;
		double newHeadSecond = 0.1*cSREV.second + 0.9*pSH.second;
		pair<double,double> newHead(newHeadFirst, newHeadSecond);
		cFSmallFO.setHead(newHead);
		cFSmallFO.setHeadIsInDirectionMAEV(false);
		// set the identified EV to reverse direction of current EV
		identifiedCSEV = cSREV;
		
	}
	
	// to detect the swift change of head direction with 2 frames
	// find previous frame's HD
	pair<double, double> previousHeadDirection;
	calculateHeadVector(pFSmallFO, previousHeadDirection);
	// find the current frame's HD
	pair<double, double> currentHeadDirection;
	calculateHeadVector(cFSmallFO, currentHeadDirection);
	
	// calculate the dot product of previous head direction and current head direction.
	double previousHDDotCurrentHD = calculateDotProduct(previousHeadDirection, currentHeadDirection);
	if (previousHDDotCurrentHD < 0 and fileCounter > 1) {
		
		// now check if velocity direction( for the future historical velocity might be considered too) and
		// current head direction dot product is also less than zero
		// asssumption that current velocity and new head direction are towards same direction
		//		pair<double, double> currentVelocity = cFSmallFO.getVelocityV();
		//		double currentVVDotCurrentHD = calculateDotProduct(currentVelocity, currentHeadDirection);
		//		if (currentVVDotCurrentHD < 0) 
		{
			// toggle current head direction
			bool currentHeadDirectionBool = cFSmallFO.getHeadIsInDirectionMAEV();
			if (currentHeadDirectionBool == true)
				cFSmallFO.setHeadIsInDirectionMAEV(false);
			else {
				cFSmallFO.setHeadIsInDirectionMAEV(true);
				
			}
			
			// update the historical head
			// reset historical head to conform to the swift change in head direction
			bool cFSmallFOFinalHeadDirectionBool = cFSmallFO.getHeadIsInDirectionMAEV();
			if (cFSmallFOFinalHeadDirectionBool == true) {
				// record this vector for history
				double newHeadFirst = cSEV.first;
				double newHeadSecond = cSEV.second;
				pair<double,double> newHead(newHeadFirst, newHeadSecond);
				cFSmallFO.setHead(newHead);
				
			} else {
				// record this vector for history
				double newHeadFirst = cSREV.first;
				double newHeadSecond = cSREV.second;
				pair<double,double> newHead(newHeadFirst, newHeadSecond);
				cFSmallFO.setHead(newHead);
				
			}
			
			
		}
	}
	
	/* retain historical head
	 bool cFSmallFOFinalHeadDirectionBool = cFSmallFO.getHeadIsInDirectionMAEV();
	 if (cFSmallFOFinalHeadDirectionBool == true) {
	 // record this vector for history
	 double newHeadFirst = 0.1*cSEV.first + 0.9*pSH.first;
	 double newHeadSecond = 0.1*cSEV.second + 0.9*pSH.second;
	 pair<double,double> newHead(newHeadFirst, newHeadSecond);
	 cFSmallFO.setHead(newHead);
	 
	 } else {
	 // record this vector for history
	 double newHeadFirst = 0.1*cSREV.first + 0.9*pSH.first;
	 double newHeadSecond = 0.1*cSREV.second + 0.9*pSH.second;
	 pair<double,double> newHead(newHeadFirst, newHeadSecond);
	 cFSmallFO.setHead(newHead);
	 
	 }
	 */
	
	double smallDotProd = calculateDotProduct(cSVV, cSEV);
	
	if (smallDotProd >= 0) {
		cout << "Smaller dot product is positive"<<endl;
		//		cFSmallFO.setHeadIsInDirectionMAEV(true);
	} else {
		cout << "Smaller dot product is negative"<<endl;
		//		cFSmallFO.setHeadIsInDirectionMAEV(false);
	}
	
	
	// update the flyobject vector
	vector<FlyObject > updatedFOVector;
	updatedFOVector.push_back(cFLargeFO);
	updatedFOVector.push_back(cFSmallFO);
	
	currentFI.setFOVector(updatedFOVector);
	
	fIVector[fileCounter] = currentFI;
	
	//	cout << "checking the update"<<endl;
	//	FrameInfo tempFI = fIVector[fileCounter];
	//	
	//	vector<FlyObject > tempFIFOVector = tempFI.getFOVector();
	//	FlyObject tempFILFO = tempFIFOVector[0];
	//	FlyObject tempFISFO = tempFIFOVector[1];
	//	pair<double, double> tempFILFOVV = tempFILFO.getVelocityV();
	//	cout << "Large object velocity vector "<<tempFILFOVV.first<<","<<tempFILFOVV.second<<endl;
	//	pair<double,double > tempFISFOVV = tempFISFO.getVelocityV();
	//	cout << "Small object velocity vector "<<tempFISFOVV.first<<","<<tempFISFOVV.second<<endl;
	
	
}


void objectHeadDirection(FlyObject prevFO, FlyObject &currentFO) {
	
	// take the head direction from the previous frame
	pair<double, double> pFHH = prevFO.getHead();
	
	pair<double, double> cFOEV = currentFO.getMajorAxisEV();
	normalizeVector(cFOEV);
	pair<double, double> cFOREV(-cFOEV.first, -cFOEV.second);
	
	double dotProdEVAndHH = calculateDotProduct(cFOEV, pFHH);
	if (dotProdEVAndHH > 0 ) {
		cout << "Current head is in direction with the Previous frame Head(which is the historical head from the maxDistIndex)"<<endl;
		double newHeadX = 0.1*cFOEV.first+0.9*pFHH.first;
		double newHeadY = 0.1*cFOEV.second+0.9*pFHH.second;
		pair<double, double> newHead(newHeadX, newHeadY);
		currentFO.setHead(newHead);
		currentFO.setHeadIsInDirectionMAEV(true);
		
	} else {
		cout << "Current head is in direction with the previous frame head"<<endl;
		double newHeadX = 0.1*cFOREV.first + 0.9*pFHH.first;
		double newHeadY = 0.1*cFOREV.second + 0.9*pFHH.second;
		pair<double, double> newHead(newHeadX, newHeadY);
		currentFO.setHead(newHead);
		currentFO.setHeadIsInDirectionMAEV(false);
		
	}
	
	// copy the velocity vector from the previous frame
	pair<double, double> pFVV = prevFO.getVelocityV();
	currentFO.setVelocityV(pFVV);

}

void objectHeadDirection(FlyObject prevFO, FlyObject &currentFO, bool prevFFOHD) {
	
	// take the head direction from the previous frame
	//pair<double, double> pFHH = prevFO.getHead();
	
	pair<double,double> pFHeadDir;
	calculateHeadVector(prevFO, pFHeadDir);

	
	pair<double, double> cFOEV = currentFO.getMajorAxisEV();
	normalizeVector(cFOEV);
	pair<double, double> cFOREV(-cFOEV.first, -cFOEV.second);
	
	double dotProdEVAndHD = calculateDotProduct(cFOEV, pFHeadDir);
	if (dotProdEVAndHD > 0 ) {
		cout << "Current head is in direction with the Previous frame Head direction"<<endl;
		/*double newHeadX = 0.1*cFOEV.first+0.9*pFHH.first;
		double newHeadY = 0.1*cFOEV.second+0.9*pFHH.second;
		pair<double, double> newHead(newHeadX, newHeadY);
		currentFO.setHead(newHead);
		currentFO.setHeadIsInDirectionMAEV(true);
		 */
		currentFO.setHead(cFOEV);
		currentFO.setHeadIsInDirectionMAEV(true);
		
	} else {
		cout << "Current head is in reverse direction with the previous frame head direction"<<endl;
		/*double newHeadX = 0.1*cFOREV.first + 0.9*pFHH.first;
		double newHeadY = 0.1*cFOREV.second + 0.9*pFHH.second;
		pair<double, double> newHead(newHeadX, newHeadY);
		currentFO.setHead(newHead);
		currentFO.setHeadIsInDirectionMAEV(false);
		 */
		currentFO.setHead(cFOREV);
		currentFO.setHeadIsInDirectionMAEV(false);

	}
	
	// copy the velocity vector from the previous frame
	if (prevFFOHD == true) {
		pair<double, double> pFVV = prevFO.getVelocityV();
		currentFO.setVelocityV(pFVV);
	}
	
}

void objectHeadDirection(FlyObject &currentFO, int saveEV=0) {

	// get the velocity vector
	pair<double, double> cFOVV = currentFO.getVelocityV();
	pair<double, double> cFOEV = currentFO.getMajorAxisEV();
	normalizeVector(cFOEV);
	pair<double, double> cFOREV(-cFOEV.first, -cFOEV.second);
	
	double dotProdVVAndEV = calculateDotProduct(cFOEV, cFOVV);
	if (dotProdVVAndEV > 0 ) {
		cout << "Current head is in direction with the velocity vector\n";
		// set the head to the eigen vector
		currentFO.setHead(cFOEV);
		currentFO.setHeadIsInDirectionMAEV(true);
		
		if (saveEV == 1) {
			cout << "saving the eigen vector for the first object"<<endl;
			evDirectionF.push_back(cFOEV);
		} else if (saveEV == 2){
			cout << "saving the eigen vector for the second object"<<endl;
			evDirectionS.push_back(cFOEV);			
		}
		
	} else if (dotProdVVAndEV < 0 ){
		cout << "Current head is in reverse direction of the velocity vector"<<endl;
		currentFO.setHead(cFOREV);
		currentFO.setHeadIsInDirectionMAEV(false);
		
		if (saveEV == 1) {
			cout << "saving the eigen vector for the first object"<<endl;
			evDirectionF.push_back(cFOREV);
		} else if (saveEV == 2) {
			cout << "saving the eigen vector for the second object"<<endl;
			evDirectionS.push_back(cFOREV);			
		}
		
	} else {

		pair<double, double> zero(0.0,0.0);
		
		if (saveEV == 1) {
			cout << "saving the zero eigen vector for the first object"<<endl;
			evDirectionF.push_back(zero);
		} else if (saveEV == 2) {
			cout << "saving the zero eigen vector for the second object"<<endl;
			evDirectionS.push_back(zero);			
		}
		
	}

	
}

void objectHeadDirection(FlyObject &currentFO, pair<double, double> cFV) {
	
//	// get the velocity vector
//	pair<double, double> cFOVV = currentFO.getVelocityV();
	pair<double, double> cFOEV = currentFO.getMajorAxisEV();
	normalizeVector(cFOEV);
	pair<double, double> cFOREV(-cFOEV.first, -cFOEV.second);
	
	double dotProdVVAndEV = calculateDotProduct(cFOEV, cFV);
	if (dotProdVVAndEV > 0 ) {
		cout << "Current head is in direction with the vector used\n";
		// set the head to the eigen vector
		currentFO.setHead(cFOEV);
		currentFO.setHeadIsInDirectionMAEV(true);
	} else {
		cout << "Current head is in reverse direction with the vector used"<<endl;
		currentFO.setHead(cFOREV);
		currentFO.setHeadIsInDirectionMAEV(false);
	}
	
	
}


void outputInfo(int midIndex) {

	FrameInfo midFI = fIVector[midIndex];
	vector<FlyObject > mFOVector = midFI.getFOVector();
	FlyObject mFFirstFO = mFOVector[0];
	FlyObject mFSecondFO = mFOVector[1];
	
	pair<int, int> midFCentroid = mFFirstFO.getCentroid();
	pair<int, int> midSCentroid = mFSecondFO.getCentroid();
	
	cout << "mid index is "<<midIndex<<endl;
	cout << "file name is "<<fnVector[midIndex]<<endl;
	cout << "Mid Large Centroid is "<<midFCentroid.first<<" "<<midFCentroid.second<<endl;
	cout << "Mid Small Centroid is "<<midSCentroid.first<<" "<<midSCentroid.second<<endl;
	
}

void velocityDirection(int st, int end, pair<double, double > &velDirectionF, pair<double, double>&velDirectionS) {

	// find the average velocity vector
	cout << "Finding average velocity vector from "<<fnVector[st]<<" to "<<fnVector[end]<<endl;
	FrameInfo prevFI = fIVector[st];
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	FlyObject pFFirstFO = pFOVector[0];
	FlyObject pFSecondFO = pFOVector[1];
	
		
	FrameInfo currentFI = fIVector[end];
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	FlyObject cFFirstFO = cFOVector[0];
	FlyObject cFSecondFO = cFOVector[1];
		
	// get the summation of (velocity direction from start frame to half of the frames in this interval)
	pair<int, int> cFFCentroid = cFFirstFO.getCentroid();
	pair<int, int> cFSCentroid = cFSecondFO.getCentroid();
		
		
	pair<int, int> pFFCentroid = pFFirstFO.getCentroid();
	pair<int, int> pFSCentroid = pFSecondFO.getCentroid();
		
		
	int velXFirst = cFFCentroid.first - pFFCentroid.first;
	int velYFirst = cFFCentroid.second - pFFCentroid.second;
		
	int velXSecond = cFSCentroid.first - pFSCentroid.first;
	int velYSecond = cFSCentroid.second - pFSCentroid.second;
		
	cout << "Velocity vector"<<endl;
	cout << "First = "<<velXFirst<<","<<velYFirst<<endl;
	cout << "Second = "<<velXSecond<<","<<velYSecond<<endl;
	
	pair<double, double> cFVV(static_cast<double> (velXFirst), static_cast<double> (velYFirst));
	pair<double, double> cSVV(static_cast<double> (velXSecond), static_cast<double> (velYSecond));
	
	velDirectionF = cFVV;
	velDirectionS = cSVV;
	
	
}
double getSpeed(pair<double, double> vector) {
	double value = vector.first*vector.first + vector.second*vector.second;
	value = sqrt(value);
	
	return value;
	
	
}

void velocityDirections(int stIndex, int endIndex) {
	
	
	velocityDirectionsF.clear();
	velocityDirectionsS.clear();
	
	//int intervalLength = 5;
	cout << "Initial velocity direction calculation"<<endl;
	cout << "From index "<<stIndex<<" to "<<endIndex<<endl;
	cout << "From "<<fnVector[stIndex]<<" to "<<fnVector[endIndex]<<endl;
	
	/*overAllVelocityF.first = 0;
	overAllVelocityF.second = 0;
	overAllVelocityS.first = 0;
	overAllVelocityS.second = 0;
	*/
	
	for (int i=stIndex; i<endIndex; i=i++) {
	
		pair<double, double > velDirectionF;
		pair<double, double > velDirectionS;
		
		// get the velocity
		velocityDirection(i,i+1, velDirectionF, velDirectionS);
		
		// extract the fly object for update
		FrameInfo currentFI = fIVector[i];
		vector<FlyObject > cFOVector = currentFI.getFOVector();
		FlyObject cFFirstFO = cFOVector[0];
		FlyObject cFSecondFO = cFOVector[1];
		// get the speed of the velocity
		double speedF = getSpeed(velDirectionF);
		cFFirstFO.setSpeed(speedF);
		double speedS = getSpeed(velDirectionS);
		cFSecondFO.setSpeed(speedS);
		
		// save the velocity direction
		normalizeVector(velDirectionF);
		velocityDirectionsF.push_back(velDirectionF);
		normalizeVector(velDirectionS);
		velocityDirectionsS.push_back(velDirectionS);
		
		cout << "Normalized velocity"<<endl;
		cout << "First = " <<velDirectionF.first<<","<<velDirectionF.second<<endl;
		cout << "Second = " <<velDirectionS.first<<","<<velDirectionS.second<<endl;
		
		// set the velocity vector in the objects
		cFFirstFO.setVelocityV(velDirectionF);
		cFSecondFO.setVelocityV(velDirectionS);
		
		// find the first object head
		cout<<fnVector[i]<<endl;
		cout << "Calculating initial head direction from velocity direction for the first object and storing the ev in direction to the vv"<<endl;
		objectHeadDirection(cFFirstFO,1);
		
		cout << "Calculating initial head direction from the velocity direction for the second object and storing the ev in direction to the vv"<<endl;
		objectHeadDirection(cFSecondFO,2);
		
		
		// update the flyobject vector
		vector<FlyObject > updatedFOVector;
		updatedFOVector.push_back(cFFirstFO);
		updatedFOVector.push_back(cFSecondFO);
		currentFI.setFOVector(updatedFOVector);
		cout << "Updating the frame "<<fnVector[i]<<" after calculating its direction from velocity vector\n";
		fIVector[i] = currentFI;
		
		/*//cout << fIVector[i];
		// first object overall velocity
		overAllVelocityF.first += velDirectionF.first;
		overAllVelocityF.second += velDirectionF.second;
		// second object overall velocity
		overAllVelocityS.first += velDirectionS.first;
		overAllVelocityS.second += velDirectionS.second;
		*/
		
	}
	
	
	
}


ofstream fout("LongestPositive.txt");
void largestIncreasingPositiveDotProductSeq(vector<pair<double, double> > velocityDirs, int &startIndex, int &endIndex) {
	
	int positiveVelSeqSize = 0;
	int flag = false;
	int maxSeqSize = 0;
	int st = 0;
	for (int j=0; j<velocityDirs.size()-1; j++) {
		pair<double,double> prevVel = velocityDirs[j];
		pair<double, double> currVel  = velocityDirs[j+1];
		
		double dotProd = calculateDotProduct(prevVel, currVel);
		
		if( dotProd > 0 && flag == false) {
			st = j;
			positiveVelSeqSize++;
			flag = true;
			//cout << "In first if positiveSize "<<positiveVelSeqSize<<endl;
			
		} else if (dotProd > 0 && flag == true) {
			positiveVelSeqSize++;
			//cout << "In second if positive "<<positiveVelSeqSize<<endl;
		} else {
			positiveVelSeqSize = 0;
			flag = false;
			//cout << "Else\n";			
			
		}
		
		if (positiveVelSeqSize > maxSeqSize) {
			maxSeqSize = positiveVelSeqSize;
			startIndex = st;
			endIndex = st+positiveVelSeqSize;
			//cout << "maxseq updated \npositiveSize "<<positiveVelSeqSize<<endl;
			//cout << "st "<<startIndex<<endl;
			//cout << "end "<<endIndex<<endl;
		}
		
		
		
	}
	
	// if dot product is alternately 0 and nonzero then nothing will be updated. In that case take the first nonzero velocity index
	if (maxSeqSize == 0) {
		bool zero = false;
		for (int j=0; j<velocityDirs.size(); j++) {
			pair<double, double > prevVel = velocityDirs[j];
			if (prevVel.first != 0 || prevVel.second != 0) {
				startIndex = j;
				endIndex = j;
				zero = true;
				break;
			}
		}
	
		if (zero != true) {
			cout << "All directions zero"<<endl;
			startIndex = 0;
			endIndex = 0;
		}
	
	}
	
}


void propagateDirections(int object, int s, int e, int origStart, int origEnd) {

	if (object == 1) {
		cout << "For first"<<endl;
	} else {
		cout << "For second"<<endl;
	}

	
	/*double maxDotProduct = -1;
	int maxDotProductIndex = -1;
	 */
	double maxVelValue = -1;
	int maxVelValIndex = -1;
	
	// find the representative frame
	for (int i=s; i<=e; i++) {
		FrameInfo currentFI = fIVector[i];
		vector<FlyObject > cFOVector = currentFI.getFOVector();
		FlyObject cFFO = cFOVector[object-1];
		pair<double , double> cFVV = cFFO.getVelocityV();
		//cout << "Velocity before normalization "<<cFVV.first<<","<<cFVV.second<<endl;
		//normalizeVector(cFVV);
		/*cout << "Velocity after normalization "<<cFVV.first<<","<<cFVV.second<<endl;
		pair<double, double> cFEV = cFFO.getMajorAxisEV();
		cout << "Eigen vector before normalization "<<cFEV.first<<","<<cFEV.second<<endl;
		normalizeVector(cFEV);
		cout << "Eigen vector after normalization "<<cFEV.first<<","<<cFEV.second<<endl;
		
		double dotProd = calculateDotProduct(cFEV, cFVV);
		cout << "Dot product absolute value for frame "<<fnVector[i]<<" : "<<abs(dotProd)<<" real value was "<<dotProd<<endl;
		
		if (maxDotProduct < abs(dotProd) ) {
			maxDotProduct = abs(dotProd);
			maxDotProductIndex = i;
		}*/
		
		double velValue = cFFO.getSpeed();
		cout << "speed at "<<fnVector[i]<<" "<<velValue<<endl;
		if (velValue > maxVelValue) {
			maxVelValue = velValue;
			maxVelValIndex = i;
		}
		
		
	}
	
	cout << "Maximum speed is chosen for for frame "<<fnVector[maxVelValIndex]<<" : "<<maxVelValue<<endl;

	// set the head direction according to the represntative velocity
	int t = maxVelValIndex;
	FrameInfo currentFI = fIVector[t];
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	FlyObject cFFirstFO = cFOVector[0];
	FlyObject cFSecondFO = cFOVector[1];
	cout << "Setting representative head direction of frame "<<fnVector[t]<<endl;
	fout << "Setting representative head direction of frame "<<fnVector[t]<<endl;
	
	if (object == 1) {
		cout << "Setting the head dir according to the representative velocity in the longest positive sequence at "<<fnVector[t]<<endl;
		bool evDirFlag = cFFirstFO.getHeadIsInDirectionMAEV();
		cout << "Before for the first object setting ev in velocity direction it is "<<evDirFlag<<endl;
		objectHeadDirection(cFFirstFO);
		evDirFlag = cFFirstFO.getHeadIsInDirectionMAEV();
		cout << "After setting ev in velocity direction it is "<<evDirFlag<<endl;
		cFOVector[0] = cFFirstFO;
		currentFI.setFOVector(cFOVector);
		fIVector[t] = currentFI;
	}
	else {
		pair<double, double> tempVelocity = cFSecondFO.getVelocityV();
		//cout << "Velocity was "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
		cout << "Setting the head direction according to the representative velocity in the longest positive sequence"<<endl;
		
		bool evDirFlag = cFSecondFO.getHeadIsInDirectionMAEV();
		cout << "Before for the second object setting ev in velocity direction it is "<<evDirFlag<<endl;
		objectHeadDirection(cFSecondFO);
		evDirFlag = cFSecondFO.getHeadIsInDirectionMAEV();
		cout << "After setting ev in velocity direction it is "<<evDirFlag<<endl;
		
		//tempVelocity = cFSecondFO.getVelocityV();
		//cout << "Velocity became "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
		cFOVector[1] = cFSecondFO;
		currentFI.setFOVector(cFOVector);
		fIVector[t] = currentFI;
	}
	
	
	
	int intervalLength = 1;
	if ((e-s)> intervalLength) {
		
		cout << "Propagating in the longest positive frame interval starting the direction from the frame "<<fnVector[t];
		cout << " to "<<fnVector[e]<<endl;
		FrameInfo prevFI = fIVector[t];
		vector<FlyObject > pFOVector = prevFI.getFOVector();
		FlyObject pFFirstFO = pFOVector[0];
		FlyObject pFSecondFO = pFOVector[1];
		
		for (int i=t+1; i<=e; i++) {
			FrameInfo currentFI = fIVector[i];
			vector<FlyObject > cFOVector = currentFI.getFOVector();
			FlyObject cFFirstFO = cFOVector[0];
			FlyObject cFSecondFO = cFOVector[1];
			
			if (object == 1) {
				//cout << "First object extract"<<endl;
				objectHeadDirection(pFFirstFO, cFFirstFO, false);
				//cout << "First object update"<<endl;
				cFOVector[0] = cFFirstFO;
				
				pFFirstFO = cFFirstFO;
				
			} else {
				//cout << "Second object extract"<<endl;
				pair<double, double> tempVelocity = cFSecondFO.getVelocityV();
				//cout << "Velocity was "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
				objectHeadDirection(pFSecondFO, cFSecondFO, false);
				//cout << "Second object update"<<endl;
				tempVelocity = cFSecondFO.getVelocityV();
				//cout << "Velocity became "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
				cFOVector[1] = cFSecondFO;
				
				pFSecondFO = cFSecondFO;
				
			}
			
			currentFI.setFOVector(cFOVector);
			
			fIVector[i] = currentFI;
			
		}
		
		cout << "propagating from "<<fnVector[t]<<" to "<<fnVector[s]<<endl;
		prevFI = fIVector[t];
		pFOVector = prevFI.getFOVector();
		pFFirstFO = pFOVector[0];
		pFSecondFO = pFOVector[1];
		
		for (int i=t-1; i>=s; i--) {
			FrameInfo currentFI = fIVector[i];
			vector<FlyObject > cFOVector = currentFI.getFOVector();
			FlyObject cFFirstFO = cFOVector[0];
			FlyObject cFSecondFO = cFOVector[1];
			
			if (object == 1) {
				//cout << "First object extract"<<endl;
				objectHeadDirection(pFFirstFO, cFFirstFO, false);
				//cout << "First object update"<<endl;
				cFOVector[0] = cFFirstFO;
				
				pFFirstFO = cFFirstFO;
				
			} else {
				//cout << "Second object extract"<<endl;
				objectHeadDirection(pFSecondFO, cFSecondFO, false);
				//cout << "Second object update"<<endl;
				cFOVector[1] = cFSecondFO;
				
				pFSecondFO = cFSecondFO;
				
			}
			
			currentFI.setFOVector(cFOVector);
			
			fIVector[i] = currentFI;
			
		}
		
		
		
	}
	// propagate upwards
	FrameInfo prevFI = fIVector[e];
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	FlyObject pFFirstFO = pFOVector[0];
	FlyObject pFSecondFO = pFOVector[1];
	
	if (e < origEnd) {
		cout << "Propagating front from "<<fnVector[e]<<" to "<<fnVector[origEnd]<<endl;		
		
		for (int i=e+1; i<=origEnd; i++) {
			FrameInfo currentFI = fIVector[i];
			vector<FlyObject > cFOVector = currentFI.getFOVector();
			FlyObject cFFirstFO = cFOVector[0];
			FlyObject cFSecondFO = cFOVector[1];
			
			if (object == 1) {
				//cout << "First object extract"<<endl;
				objectHeadDirection(pFFirstFO, cFFirstFO, false);
				//cout << "First object update"<<endl;
				cFOVector[0] = cFFirstFO;
				
				pFFirstFO = cFFirstFO;
				
			} else {
				//cout << "Second object extract"<<endl;
				objectHeadDirection(pFSecondFO, cFSecondFO, false);
				//cout << "Second object update"<<endl;
				cFOVector[1] = cFSecondFO;
				
				pFSecondFO = cFSecondFO;
				
			}
			
			currentFI.setFOVector(cFOVector);
			
			fIVector[i] = currentFI;
			
		}
		
	}
	
	
	
	// propagate downwards
	prevFI = fIVector[s];
	pFOVector = prevFI.getFOVector();
	pFFirstFO = pFOVector[0];
	pFSecondFO = pFOVector[1];
	
	if (s > origStart ) {
		cout << "Propagating down wards from "<<fnVector[s]<<" to "<<fnVector[origStart]<<endl;
		for (int i=s-1; i>=origStart; i--) {
			FrameInfo currentFI = fIVector[i];
			vector<FlyObject > cFOVector = currentFI.getFOVector();
			FlyObject cFFirstFO = cFOVector[0];
			FlyObject cFSecondFO = cFOVector[1];
			
			if (object == 1) {
				//cout << "First object extract"<<endl;
				objectHeadDirection(pFFirstFO, cFFirstFO, false);
				//cout << "First object update"<<endl;
				cFOVector[0] = cFFirstFO;
				
				pFFirstFO = cFFirstFO;
				
			} else {
				//cout << "Second object extract"<<endl;
				objectHeadDirection(pFSecondFO, cFSecondFO, false);
				//cout << "Second object update"<<endl;
				cFOVector[1] = cFSecondFO;
				
				pFSecondFO = cFSecondFO;
				
			}
			
			currentFI.setFOVector(cFOVector);
			
			fIVector[i] = currentFI;
		}
		
	}
	
}


// find the head direction from the collsion point to backward
void calculateHeadDirection(int st, int end, int maxDistIndex) {

	cout << "From file "<<fnVector[st]<<" to "<<fnVector[end]<<endl;
	//cout << "Assigning object head direction between Index "<<mid<<" to startIndex "<<st<<endl;
	
	// calculate the velocity directions first
	// clear evDirectionF and evDirectionS to store the new evs for the current sequence
	evDirectionF.clear();
	evDirectionS.clear();
	
	velocityDirections(st, end);

	cout << "Size of evDirectionF "<<evDirectionF.size()<<endl;
	cout << "Size of evDirectionS "<<evDirectionS.size()<<endl;
	
	
	// debug
	cout << "------------ALL VELOCITY AND CORRESPONDING EV-------------\n";
	int a;
	for (a=0; a < velocityDirectionsF.size(); a++) {

		cout << "For frame "<<fnVector[a+st]<<endl;
		
		pair<double, double> z = velocityDirectionsF[a];
		pair<double, double> zEV = evDirectionF[a];
		cout <<" First object velocity = "<<z.first<<","<<z.second<<endl;
		cout <<" First object ev       = "<<zEV.first<<","<<zEV.second<<endl;
		
		pair<double, double> w = velocityDirectionsS[a];
		pair<double, double> wEV = evDirectionS[a];
		
		cout << "Second object velocity = "<<w.first<<","<<w.second<<endl;
		cout << "Second object ev       = "<<wEV.first<<","<<wEV.second<<endl;
		
		
	}
	cout << "Last frame index wont have velocity a+st("<<(a+st)<<") = end("<<end<<") "<<fnVector[a+st]<<endl;
	cout << "------------END-------------\n";
	
	int s;
	int e;
	int fs,fe;
	fout<<"------------------------------------------------------------------"<<endl;
	largestIncreasingPositiveDotProductSeq(evDirectionF, s, e);
	cout << "Positive indexes are "<<fnVector[st+s]<<" to "<<fnVector[st+e]<<endl;
	int si = s + st;
	int ei = e + st;
	fout << "For first object max positive directions from "<<fnVector[si]<<" to "<<fnVector[ei]<<endl;
	propagateDirections(1, si, ei, st, end);
	
	s = 0;
	e = 0;
	
	largestIncreasingPositiveDotProductSeq(evDirectionS, s, e);
	cout << "Positive indexes are "<<fnVector[st+s]<<" to "<<fnVector[st+e]<<endl;
	si = s + st;
	ei = e + st;
	fout << "For second object max positive directions from "<<fnVector[si]<<" to "<<fnVector[ei]<<endl;
	propagateDirections(2, si, ei, st, end);
	
}
// min dist from prev frame's 0th index object
void objectCorrespondence(FrameInfo &prevFI, FrameInfo &currentFI) {
	
	// simplest assumption that the larger object will remain close to larger one
	// and the smaller one will remain close to smaller
	// just find the min distance from any one of them
	// initially just find the min distance from the first object to the other two
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	
	//	bool currentlySingleBlob = currentFI.getIsSingleBlob();
	
	//	if (currentlySingleBlob == false) {
	
	FlyObject pFLargeFO = pFOVector[0];
	FlyObject pFSmallFO = pFOVector[1];
	
	FlyObject cFLargeFO = cFOVector[0];
	FlyObject cFSmallFO = cFOVector[1];
	
	double distLTL = euclideanDist(pFLargeFO, cFLargeFO);
	cout << "previousFirst to currentFirst "<<distLTL<<endl;
	
	double distLTS = euclideanDist(pFLargeFO, cFSmallFO);
	cout << "prevFirst to currentSecond "<<distLTS<<endl;
	
	double min1 = min(distLTL, distLTS);
	cout<<"min of FTF and FTS is "<<min1<<endl;
	
	double distSTL = euclideanDist(pFSmallFO, cFLargeFO);
	cout << "previousSecond to currentLarge "<<distSTL<<endl;
	
	double distSTS =euclideanDist(pFSmallFO, cFSmallFO);
	cout << "prevSecond to currentSecond "<<distSTS<<endl;
	
	double min2 = min(distSTL, distSTS);
	
	// if prev frame larger object is closer to current frame smaller object
	// this might happen for two cases i) the smaller object is closer in the current frame (so this situation should be handled with some other heuristics)
	//								   ii)the segmentation process toggles the size (error in the segmentation process)
	
	if (min1 < min2) {
		if (distLTS == min1) {
			cout<<"Shortest distance is from the previous frame first object. And shortest distance is from previos first object to current second object so we need to swap the objects in current frame"<<endl;
			currentFI.swapTheFlyObject();
		}
	} else {
		if (distSTL == min2) {
			cout << "Shortest distance is from the previous frame second object. And shortest distance is from previous second object to current first object so we need to swap the objects in the current frame"<<endl;
			currentFI.swapTheFlyObject();
		}
	}

	
}

double euclideanDist(FlyObject a, FlyObject b) {
	
	pair<int, int> aCentroid = a.getCentroid();
	pair<int,int> bCentroid = b.getCentroid();
	
	//cout << "Distance from "<<aCentroid.first<<","<<aCentroid.second<<" to "<<bCentroid.first<<","<<bCentroid.second<<endl;
	double x2 = pow((static_cast<double> (aCentroid.first)-static_cast<double> (bCentroid.first)), 2.0);
	double y2 = pow((static_cast<double> (aCentroid.second)-static_cast<double> (bCentroid.second)), 2.0);
	double dist = sqrt((x2+y2));
	cout << dist<<endl;
	
	return dist;
}

void processASequence(int startOfATrackSequence, int endOfATrackSequence) {

	// find the frame that gives us the furthest distance between the two objects
	double maxDist = -1;
	int maxDistIndex = -1;
	for (int i=startOfATrackSequence; i<=(endOfATrackSequence); i++) {
		FrameInfo currentFI = fIVector[i];

		vector<FlyObject > cFOVector = currentFI.getFOVector();
		FlyObject cFFirstFO = cFOVector[0];
		FlyObject cFSecondFO = cFOVector[1];
		
		double currDist = euclideanDist(cFFirstFO, cFSecondFO);
		
		if (currDist > maxDist) {
			maxDist = currDist;
			maxDistIndex = i;
			cout << "New max distance" << maxDist << " at the frame number "<<i;
			cout << endl;
		}
		
		
		
	}

	cout << "Maximum distance between the frame found at the index "<<maxDistIndex<<endl;
	cout << "Assigning object correspondance between maxDistIndex "<<maxDistIndex<<" to startIndex "<<startOfATrackSequence<<endl;
	// assign closest distance object association before the between frames startOfASeq to maxDistIndex
	FrameInfo prevFI = fIVector[maxDistIndex];
	for (int i=maxDistIndex-1; i>=startOfATrackSequence; i--) {
		FrameInfo currentFI = fIVector[i];
		objectCorrespondence(prevFI, currentFI);
		
		// update the fIVector
		fIVector[i] = currentFI;
		prevFI = currentFI;
		
	}
	cout << "Assigning object correspondance between maxDistIndex+1 "<<(maxDistIndex+1)<<" to endIndex "<<endOfATrackSequence<<endl;
	// assign closest distance object association after the maxDistIndex to endOfSeq
	prevFI = fIVector[maxDistIndex];
	for (int i=maxDistIndex+1; i<=(endOfATrackSequence); i++) {
		FrameInfo currentFI = fIVector[i];

		cout << "Object correspondance frame number "<<i<<endl;
		
		objectCorrespondence(prevFI, currentFI);
		
		// update the fIVector
		fIVector[i] = currentFI;
		prevFI = currentFI;
		
		
	}
	
	double sequenceFirstAverage = 0;
	double sequenceSecondAverage = 0;
	// find the average area of the two sequences and choose the larger
	for (int i=startOfATrackSequence; i<=(endOfATrackSequence); i++) {
		FrameInfo currentFI = fIVector[i];
		vector<FlyObject > fOVector = currentFI.getFOVector();
		FlyObject cFFirstFO = fOVector[0];
		FlyObject cFSecondFO = fOVector[1];
		
		sequenceFirstAverage += cFFirstFO.getArea();
		sequenceSecondAverage += cFSecondFO.getArea();
		
		cout << "For frame number "<<i<<"\n";
		cout << "SequenceFirst area sum = "<<sequenceFirstAverage<<endl;
		cout << "SequenceSecond area sum = "<<sequenceSecondAverage<<endl;
	}
	
	sequenceFirstAverage /= (endOfATrackSequence-startOfATrackSequence + 1);
	sequenceSecondAverage /= (endOfATrackSequence-startOfATrackSequence + 1);
	
	cout << "----------------------------------------------------\n";
	cout << "SequenceFirst average area = "<<sequenceFirstAverage<<endl;
	cout << "SequenceSecond average area = "<<sequenceSecondAverage<<endl;
	cout << "----------------------------------------------------\n";
	
	if (sequenceFirstAverage > sequenceSecondAverage) {
		fout << "First object is the female"<<endl;
	} else {
		fout<<"Second object is female"<<endl;
	}
	
	// calculating the head direction from the collision time to backward
	cout << "calculating the head direction "<<startOfATrackSequence<<" to "<<endOfATrackSequence<<endl;
	fout<<"calculating the head direction "<<startOfATrackSequence<<" to "<<endOfATrackSequence<<endl;
	calculateHeadDirection(startOfATrackSequence, endOfATrackSequence, maxDistIndex);
	
	if (sequenceFirstAverage > sequenceSecondAverage) {
		cout << "First sequence is the object A"<<endl;
		drawTheSequence(startOfATrackSequence, endOfATrackSequence, 1);
	}
	else {
		cout << "Second sequence is the object A"<<endl;
		drawTheSequence(startOfATrackSequence, endOfATrackSequence, 0);
	}

	
}

int diagLength;

int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		cerr << "Usage: executablename <inputFile.txt> <originalImagePath> <finalOutputPath> <maskImagePath>" << endl; // input file contains name of the
		// input image files
		return -1;
	}
	
	MagickCore::SetMagickResourceLimit(MagickCore::MemoryResource, 1536);
	MagickCore::SetMagickResourceLimit(MagickCore::MapResource, 4092);
	
	string fileName;
	ifstream inputFile(argv[1]);
	
	// save the input file name
	string ifns(argv[1]);
	inputFileName = ifns;

	if (inputFile.fail() ) {
		cout << "cannot open the input file that contains name of the input images\n";
		exit(1);
	}
	
	// set the global paths
	string tempOIP(argv[2]);
	origImagePath = tempOIP;
	
	string tempFOP(argv[3]);
	finalOutputPath = tempFOP;
	
	string tempMIP(argv[4]);
	maskImagePath = tempMIP;
	
	unsigned int objCount = 0;
	int i;
	
	int frameCounter = 0;
	int fileCounter=0;

	char buffer[100];
	string imgSize;
//	FlyObject a,b;
	vector<pair<int,int> > shape;
	vector<FlyObject > tempFOV;
	
	// to find the new head direction
	bool currentlyCalculatingHead = true;
	
	
	
	while (inputFile>>fileName) {
		
//	Image* img = new Image(argv[1]);// = new Image(argv[1]);

		int fi = fileName.find("Test");
		// current sequence numbers spans from 0 - 18019, so 5 digits are needed
		int span = 5;
		string tempString = fileName.substr(fi+4,span);
		int frameCounter = atoi(tempString.c_str());
		cout << frameCounter<<endl;
		
		string fileNameForSave = fileName;
		
		// save the name in the vector
		fnVector.push_back(fileName);
	
		fileName = "output/filtered/final/"+fileName;
		Image* img = new Image(fileName.c_str());
		int width = img->columns(),height = img->rows();
		diagLength= static_cast<int> ( sqrt( (height*height) + (width*width) ) );
		cout << "Diagonal length is "<<diagLength<<endl;
//		Image* imgWithInfo;
//		imgWithInfo = new Image(fileName.c_str());
		sprintf(buffer,"%ix%i",width,height);
		string imsize(buffer);
		imgSize = imsize;
		// residual image is initialized with black representing not visited.
		residual = new Image(buffer, "black");
		
		cout<<"reading file "<<fileName<<endl;
		
		tempFOV.clear();
		
		for (int x = 0; x<width; x++) {
			for (int y = 0; y<height; y++) {
			
				//cout<<"comes here"<<endl;
				shape.clear();
				findObj(img, x, y, shape, true, true);
				unsigned int s = shape.size();
			
				if ( s > 0 )
				{
				
					cout << "size of the object is: " << s <<endl;
					vector<double> eigenVal = covariantDecomposition(shape);

					{					
//						objCount++;
										
						double velocity_x=0.0, velocity_y=0.0;
						// save the object information
						FlyObject tempFO(s, 
										 pair<int, int> (eigenVal[6], eigenVal[7]), 
										 pair<double, double> (eigenVal[4], eigenVal[5]), 
										 pair<double,double> (velocity_x, velocity_y),
										 false,
										 pair<double, double> (eigenVal[4], eigenVal[5]),
										 0.0);
						tempFOV.push_back(tempFO);
						
						
						
											
					}
				}
				
			}
		}
		
//		cout<<"Sorting the objects according to size"<<endl;
//		bubbleSort(tempFOV);
		
		fOVector.clear();
		
		for (int ti=0; ti<tempFOV.size(); ti++){
			
			FlyObject a = tempFOV[ti];
			fOVector.push_back(a);
			
		}
		
		bool currentFrameIsSingleBlob = false;
		// if there is only one object then state of the system is single blob
		if (fOVector.size() == 1 and currentFrameIsSingleBlob == false) {
			currentFrameIsSingleBlob = true;
			
			// if start as a single blob
			if (fileCounter == 0) {
				cout << "Start as a single blob"<<endl;
				startOfAOneObject = fileCounter;
			}

		}
		
		FrameInfo tempFI(frameCounter, fOVector, currentFrameIsSingleBlob);
		fIVector.push_back(tempFI);
		bool flag;
		FrameInfo currentFI = fIVector[fileCounter];
		
		sequenceSize++;

		// increase the frame Counter
		fileCounter++;
		
		delete img;
	
		delete residual;
	
		
	}
	
	inputFile.close();
	
	if (startOfATrackSequence!=-1 && endOfATrackSequence == -1) {
		cout << "Last sequence that does not stick to a single blob status startIndex "<<startOfATrackSequence<<" endIndex "<<(sequenceSize+startOfATrackSequence-2)<<endl;
		processASequence(startOfATrackSequence, (sequenceSize+startOfATrackSequence-2));
	}
	else if (startOfAOneObject != -1 && endOfAOneObject == -1) {
		cout << "Last sequence that does not separate from one object state\n";
		drawTheSequence(startOfAOneObject, (sequenceSize+startOfAOneObject - 2), 0, true);
	}
	
	
	// reopen the file name list to read the name of the file. This should be changed to rename the filtered file
	// and establish a link between the file name and the framenumber so that this file need not be read again.
	
	
	return 0;
}

// min dist from prev frame's 0th index object
bool identifyFlyObjectFromFrameToFrame(FrameInfo prevFI, FrameInfo &currentFI,  bool gotRidOfSingleBlob) {
	
	// simplest assumption that the larger object will remain close to larger one
	// and the smaller one will remain close to smaller
	// just find the min distance from any one of them
	// initially just find the min distance from the first object to the other two
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	
	bool currentlySingleBlob = currentFI.getIsSingleBlob();
	
	if (currentlySingleBlob == false) {
		
		FlyObject pFLargeFO = pFOVector[0];
		//FlyObject pFSmallFO = pFOVector[1];
		
		FlyObject cFLargeFO = cFOVector[0];
		FlyObject cFSmallFO = cFOVector[1];
		
		// if just got rid off the single blob then the distance from the centroid of the large object before collision
		// which is saved to the new two centroids needs to be calculated. This will give two direction
		// dot product with the saved large object direction and directions are calculated the positive direction
		// gives the new large object
		/// ASSUMPTION is that the large object tends to go toward the direction before collision
		//  the smaller object will go either opposite or will be behind the object
		// find the minimum distance among the centroids and chose the close one
		// find minimum distance from the larger object = min1
		double distLTL = euclideanDist(pFLargeFO, cFLargeFO);
		cout << "previousL to currentL "<<distLTL<<endl;
		
		double distLTS = euclideanDist(pFLargeFO, cFSmallFO);
		cout << "prevL to currentS "<<distLTS<<endl;
		
		double min1 = min(distLTL, distLTS);
		cout<<"min of LTL and LTS is "<<min1<<endl;
		// if prev frame larger object is closer to current frame smaller object
		// this might happen for two cases i) the smaller object is closer in the current frame (so this situation should be handled with some other heuristics)
		//								   ii)the segmentation process toggles the size (error in the segmentation process)
		if (distLTS == min1) {
			cout<<"The object should be swapped"<<endl;
			currentFI.swapTheFlyObject();
		}
	}
	// return value ignorable from where it is called
	return false;
}

// with area and min dist when objects area are almost same
/*
bool identifyFlyObjectFromFrameToFrame(FrameInfo prevFI, FrameInfo &currentFI) {
	
	// simplest assumption that the larger object will remain close to larger one
	// and the smaller one will remain close to smaller
	// just find the min distance from any one of them
	// initially just find the min distance from the first object to the other two
	vector<FlyObject > pFOVector = prevFI.getFOVector();
	vector<FlyObject > cFOVector = currentFI.getFOVector();
	
	FlyObject pFLargeFO = pFOVector[0];
	FlyObject pFSmallFO = pFOVector[1];
	
	FlyObject cFLargeFO = cFOVector[0];
	FlyObject cFSmallFO = cFOVector[1];
	
	// find the area ratio of the current two objects.
	// if they are within 5 percent, calculate the minimum distance from previous larger to current larger and smaller
	double curr_larger = cFLargeFO.getArea();
	double curr_smaller = cFSmallFO.getArea();
	double ratio = (curr_smaller/curr_larger) * 100.0;
	if (ratio >= 95.0) {
		cout <<"Current ratio greater than equal 95.0 percent. Areas are : "<<curr_larger<<" and "<<curr_smaller<<endl;
		// find minimum distance from the larger object = min1
		double distLTL = euclideanDist(pFLargeFO, cFLargeFO);
		cout << "L to L "<<distLTL<<endl;
	
		double distLTS = euclideanDist(pFLargeFO, cFSmallFO);
		cout << "L to S "<<distLTS<<endl;
	
		double min1 = min(distLTL, distLTS);
		cout<<"min of LTL and LTS is "<<min1<<endl;
		if (distLTS == min1) {
			cout<<"The object should be swapped"<<endl;
			currentFI.swapTheFlyObject();
		}
	}
	return false;
}
*/

void drawTheFlyObject(FrameInfo currentFI, string fileName, int isFirst, bool singleBlob) {
	
	cout << "isFirst is "<<isFirst<<endl;
	
	string inputFileName = origImagePath + fileName;
	//	string inputFileName = "output/identified/"+fileName;
	
	// when do not want to identify on the original comment the line below and uncomment the above line
	// debugging for drawing the circle
	string outputFileName = finalOutputPath + fileName;
	//	string outputFileName = "output/identified_with_cropped/" + fileName;
	
	cout<<"file name "<<inputFileName<<endl;
	Image* img = new Image(inputFileName.c_str());
	
	//	cout<<"img width "<<img->columns()<<" and height "<<img->rows()<<endl;
	string object1Color("blue");
	string object2Color("red");
	vector<FlyObject > fOVector = currentFI.getFOVector();
	cout << "While drawing it found objects = "<<fOVector.size()<<endl;

	int x0, x1, y0, y1;
	string maskFile = maskImagePath + fileName;
	Image* maskImage = new Image(maskFile.c_str());
	
	if (singleBlob == false) {
		
		
		FlyObject currentFO = fOVector[isFirst];
		
		pair<int, int> centroid = currentFO.getCentroid();
		pair<double, double> majorAxisEV = currentFO.getMajorAxisEV();
		
		bool eVDirection = currentFO.getHeadIsInDirectionMAEV();
		
		double ev_x, ev_y;

		if (eVDirection == true) {
			
			x0 = centroid.first;
			y0 = centroid.second;
			ev_x = static_cast<double>(centroid.first) + static_cast<double>(diagLength)* majorAxisEV.first;
			ev_y = static_cast<double>(centroid.second) + static_cast<double>(diagLength)* majorAxisEV.second;

			x1 = static_cast<int> (ev_x);
			y1 = static_cast<int> (ev_y);
			
		} else {
			
			x0 = centroid.first;
			y0 = centroid.second;
			ev_x = static_cast<double>(centroid.first) - static_cast<double>(diagLength)* majorAxisEV.first;
			ev_y = static_cast<double>(centroid.second) - static_cast<double>(diagLength)* majorAxisEV.second;
			
			x1 = static_cast<int> (ev_x);
			y1 = static_cast<int> (ev_y);
			
			
		}
		
		int isHitting = draw_line_bm(maskImage, x0, y0, x1, y1 );
		
		
		
		
		
		for (int n=0; n<fOVector.size(); n++) {
		
			FlyObject currentFO = fOVector[n];
		
			pair<int, int> centroid = currentFO.getCentroid();
			pair<double, double> majorAxisEV = currentFO.getMajorAxisEV();
		
			bool eVDirection = currentFO.getHeadIsInDirectionMAEV();
		
			double ev_x, ev_y;
			//draw the object tracking circle
			if (n == isFirst and n==0) {
				cout << "Tracking the n = "<<n<<endl;
				img->strokeColor("yellow");
				img->draw(DrawableCircle(centroid.first, centroid.second, centroid.first+5, centroid.second));
	
			} else if ( n == isFirst and n==1) {
				
				cout << "Tracking the "<<n<<endl;
				img->strokeColor("yellow");
				//img->fillColor("none");
				img->draw(DrawableCircle(centroid.first, centroid.second, centroid.first+5, centroid.second));
				
				
			}
			
			
			// draw the female when tracked by the male fly
			if (isHitting == 1) {
				if (n != isFirst and n == 0) {
					img->strokeColor("red");
					
					/*img->draw(DrawableLine(centroid.first - 5, centroid.second - 5, centroid.first + 5, centroid.second - 5));
					img->draw(DrawableLine(centroid.first + 5, centroid.second - 5, centroid.first + 5, centroid.second + 5));
					img->draw(DrawableLine(centroid.first + 5, centroid.second + 5, centroid.first - 5, centroid.second + 5));
					img->draw(DrawableLine(centroid.first - 5, centroid.second + 5, centroid.first - 5, centroid.second - 5));
					 */
					img->draw(DrawableRectangle(centroid.first - 5, centroid.second - 5, centroid.first + 5, centroid.second + 5));
				} else if (n != isFirst and n==1) {
					
					img->strokeColor("red");
					
					/*img->draw(DrawableLine(centroid.first - 5, centroid.second - 5, centroid.first + 5, centroid.second - 5));
					img->draw(DrawableLine(centroid.first + 5, centroid.second - 5, centroid.first + 5, centroid.second + 5));
					img->draw(DrawableLine(centroid.first + 5, centroid.second + 5, centroid.first - 5, centroid.second + 5));
					img->draw(DrawableLine(centroid.first - 5, centroid.second + 5, centroid.first - 5, centroid.second - 5));*/
					
					img->draw(DrawableRectangle(centroid.first - 5, centroid.second - 5, centroid.first + 5, centroid.second + 5));
				}
			}
			
			// draw the Axis direction
			if (eVDirection == true) {
				ev_x = static_cast<double>(centroid.first) + 50.0 * majorAxisEV.first;
				ev_y = static_cast<double>(centroid.second) + 50.0 * majorAxisEV.second;
				img->strokeColor("green");
				img->draw( DrawableLine( centroid.first, centroid.second, static_cast<int>(ev_x), static_cast<int>(ev_y) ));
			} else {
				ev_x = static_cast<double>(centroid.first) - 50.0 * majorAxisEV.first;
				ev_y = static_cast<double>(centroid.second) - 50.0 * majorAxisEV.second;
				img->strokeColor("green");
				img->draw( DrawableLine( centroid.first, centroid.second, static_cast<int>(ev_x), static_cast<int>(ev_y) ));
			}
			
			// draw the velocity vector
			
			/*img->strokeColor("blue");
			 pair<double, double> velocityV = currentFO.getVelocityV();
			 ev_x = static_cast<double>(centroid.first) + 30.0 * velocityV.first;
			 ev_y = static_cast<double>(centroid.second) + 30.0 * velocityV.second;
			 img->draw(DrawableLine( centroid.first, centroid.second, static_cast<int>(ev_x), static_cast<int>(ev_y) ));
			 */
			
			/*
			 // draw the historical head vector
			 img->strokeColor("white");
			 pair<double, double> headV = currentFO.getHead();
			 ev_x = static_cast<double> (centroid.first) + 25.0*headV.first;
			 ev_y = static_cast<double> (centroid.second) + 25.0*headV.second;
			 img->draw( DrawableLine(centroid.first, centroid.second, static_cast<int> (ev_x), static_cast<int> (ev_y)) );
			 */
			
			/*
			 // overall velocity direction
			 if (n == 0) {
			 ev_x = static_cast<double> (centroid.first) + 10*overAllVelocityF.first;
			 ev_y = static_cast<double> (centroid.second)+ 10*overAllVelocityF.second;
			 img->strokeColor("cyan");
			 img->draw(DrawableLine(centroid.first, centroid.second, static_cast<int> (ev_x), static_cast<int> (ev_y)  ) );
			 } else {
			 ev_x = static_cast<double> (centroid.first) + 10*overAllVelocityS.first;
			 ev_y = static_cast<double> (centroid.second)+ 10*overAllVelocityS.second;
			 img->strokeColor("cyan");
			 img->draw(DrawableLine(centroid.first, centroid.second, static_cast<int> (ev_x), static_cast<int> (ev_y)  ) );
			 
			 }
			 
			 */
			// average velocity direction
			/*if (n == 0) {
			 ev_x = static_cast<double> (centroid.first) + 10*avgVelocityF.first;
			 ev_y = static_cast<double> (centroid.second)+ 10*avgVelocityF.second;
			 img->strokeColor("cyan");
			 img->draw(DrawableLine(centroid.first, centroid.second, static_cast<int> (ev_x), static_cast<int> (ev_y)  ) );
			 } else {
			 ev_x = static_cast<double> (centroid.first) + 10*avgVelocityS.first;
			 ev_y = static_cast<double> (centroid.second)+ 10*avgVelocityS.second;
			 img->strokeColor("cyan");
			 img->draw(DrawableLine(centroid.first, centroid.second, static_cast<int> (ev_x), static_cast<int> (ev_y)  ) );
			 
			 }*/
			
					
		}
	}
	// overwrite the file now with axis
	//	img->write(inputFileName.c_str());
	
	// when do not want to identify on the original comment below line and uncomment the above one
	img->write(outputFileName.c_str());
	delete img;
	delete maskImage;
	
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
//			//cout<<obj.size()<<endl;
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
