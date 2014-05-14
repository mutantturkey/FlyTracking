#include <iomanip>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#include <list>
#include <fstream>
#include <cassert>
#include <cstdlib>

#include <Magick++.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_eigen.h>
#include <unistd.h>

#include "FrameInfo.h"

using namespace Magick;
using namespace std;

// One of these output streams will be used. If directed towards null, nothing happens, towards output, it will be printed
ofstream nullLog;
ostream* output;

// Our output files for debugging and information
ofstream foutLPS;
ofstream foutSt;
ofstream foutDebugCen;
ofstream foutDebugSpeed;


const double PI = atan(1.0)*4.0;
const double FACTOR_EIGEN = 100;
const int STUCKING_TO_A_SINGLE_BLOB = 1;
const int SEPARATING_FROM_SINGLE_BLOB = 2;

bool isInFemaleBlob;
int maskImageHeight;
int maskImageWidth;
int diagLength;

vector<pair<int,int> > bresenhamLine;
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

// This decides if we want to output an overlay. You'll get the same results
// either way but you won't be able to verify it. taken as an argument.
bool writeFinalImages = false;

// GLOBAL PATHS
string inputFileName;
string maskImagePath;
string origImagePath;
string finalOutputPath;
string outputFilePrefix;

vector<pair<double, double> > velocityDirectionsF;
vector<pair<double, double> > velocityDirectionsS;
vector<string> fnVector;

pair<double, double> avgVelocityF;
pair<double, double> avgVelocityS;

pair<double, double> overAllVelocityF;
pair<double, double> overAllVelocityS;

vector<pair<double, double> > evDirectionF;
vector<pair<double, double> > evDirectionS;

// Information about frames that will be written out at the end of proessing.
int totalMaleLookingAtFemale = 0;
int totalFemaleLookingAtMale = 0;
int totalSingleBlob = 0;
int totalUnprocessedFrame = 0;
int totalSeparated = 0;

map<unsigned int, unsigned int> centroidDistanceMap;
map<unsigned int, unsigned int> headDirAngleMap;
map<unsigned int, unsigned int> speedMap;

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
  for(unsigned int i=1; i<fov.size(); i++) {
    for(unsigned int j=0; j<fov.size()-i; j++) {
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


void getMeanStdDev(map<unsigned int, unsigned int> dataMap, double *mean, double *standardDev) {
    // mean = 1/N*(sum(i*H_i)) for i = 0 to M-1
    double sumOfValues = 0.0;
    double i = 0.0;
    double N = 0.0;
    double M = 0.0;

		vector<double> currentHistogramValues;

		for(i = 0; i < dataMap.size(); i++) {
			sumOfValues = sumOfValues + i*dataMap[i];
			N += dataMap[i];

			cout << "sum:" << sumOfValues << endl;
			cout << "N:" << N << endl;
			cout << "i:" << i << endl;
		}

    // mean
    *mean = sumOfValues/N;
		double lmean = 0;
    lmean = sumOfValues/N;
		cout << "mean:" << *mean << endl;
		cout << "mean:" << lmean << " = " << sumOfValues << "/" << N << endl;

    // sigma^2 = (sum( (i-mean)^2*H_i ) )/(N-1) for i = 0 to M-1
    *standardDev = 0.0;
    double sumSquaredResults = 0.0;
    int j = 0;
    for ( i = 0.0; i < dataMap.size(); i++) {
      sumSquaredResults += pow((i-*mean), 2.0)*dataMap[j];
			*output << "sumsqres:" << sumSquaredResults << endl;
      j++;
    }

    // standard deviation
    *standardDev = sumSquaredResults/(N-1);
    *standardDev = sqrt(*standardDev);
}

double writeHist(const char* filename, map<unsigned int, unsigned int> dataMap) {
  *output << "In the beginning of the write hist" << endl;
  *output << "dataMap size " << dataMap.size() << endl;

  if (dataMap.size() == 0) {
    *output << "Empty histogram" << endl;
    ofstream fout(filename);
    fout <<"No entry in the histogram and size is " << dataMap.size() << endl;
    fout.close();
    return 0;

  }

  map<unsigned int, unsigned int>::iterator front = dataMap.begin(),
    back = dataMap.end();
  back--;

  unsigned int first = front->first, last = back->first;
  *output << "Min: " << first << " " << "Max: " << last << " " << "Count: " << last-first << endl;
  vector<unsigned int> hist(last+1, 0);

  try {
    for (unsigned int j = 0; j<first; j++) {
      hist[j] = 0;
    }
    for (unsigned int j = first; j<=last; j++) {
      hist[j] = dataMap[j];
    }
  }
  catch (...) {
    cerr << "Bad histogram bucketing" << endl;
  }

  try {
    ofstream fout(filename);
    for (unsigned int i = 0; i<hist.size(); i++) {
      fout << hist[i] << endl;
    }
    fout << first << " " << last << " " << hist.size() << endl;
    fout.close();
  }
  catch (...) {
    cerr << "Bad memory loc for opening file" << endl;
  }
	
	return 0;
}

double calculateDotProduct(pair<double, double> v, pair<double, double> eV);
void calculateHeadVector(FlyObject fO, pair<double,double> &headDirection);
vector<double> covariantDecomposition(vector<pair<int,int> > & points);
void determineHeadDirection(int fileCounter);
void drawTheFlyObject(FrameInfo currentFI, string fileName, int isFirst, bool singleBlob=false,bool unprocessed = false);
void drawTheSequence(int startIndex, int endIndex, int isFirst, bool singleBlob = false, bool unprocessed = false);
void eightConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
double euclideanDist(FlyObject a, FlyObject b);
void findObj(Image* img, int x, int y, vector<pair<int,int> > & shape ,bool eightCon=true, bool colorLookingFor=true);
void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
pair<int,int> getCentroid(vector<pair<int,int> > & points);
bool isInterface(Image* orig, unsigned int x, unsigned int y);
bool identifyFlyObjectFromFrameToFrame(FrameInfo prevFI, FrameInfo& currentFI, bool gotRidOfSingleBlob=false) ;
int roundT(double v) {return int(v+0.5);}
void normalizeVector(pair<double,double> &a);
void writeFrameImage(int fn, string imS);

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
    *output << "Current eigen is in direction with the historical head"<<endl;
    // record this vector for history
    double newHeadFirst = 0.1*cLEV.first+0.9*pLH.first;
    double newHeadSecond = 0.1*cLEV.second+0.9*pLH.second;
    pair<double, double> newHead(newHeadFirst,newHeadSecond);
    cFLargeFO.setHead(newHead);
    cFLargeFO.setHeadIsInDirectionMAEV(true);
    // set the identified EV
    identifiedCLEV = cLEV;

  } else {
    *output << "Current eigen is in opposite direction of the historical head\n";
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
  *output << "largerDotProd "<<largeDotProd<<endl;
  if (largeDotProd >= 0) {
    *output<<"Larger Dot prod is positive"<<endl;
    //		cFLargeFO.setHeadIsInDirectionMAEV(true);
  } else {
    *output<<"Larger Dot prod is negative"<<endl;
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
    *output << "Current eigen is in direction with the historical head for the smaller fly object"<<endl;
    // record this vector for history
    double newHeadFirst = 0.1*cSEV.first + 0.9*pSH.first;
    double newHeadSecond = 0.1*cSEV.second + 0.9*pSH.second;
    pair<double,double> newHead(newHeadFirst, newHeadSecond);
    cFSmallFO.setHead(newHead);
    cFSmallFO.setHeadIsInDirectionMAEV(true);
    // set the identified EV to current EV
    identifiedCSEV = cSEV;
  } else {
    *output << "Current eigen is in direction with the historical head for the smaller fly object"<<endl;
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
  *output << "Smaller dot product is positive"<<endl;
  //		cFSmallFO.setHeadIsInDirectionMAEV(true);
} else {
  *output << "Smaller dot product is negative"<<endl;
  //		cFSmallFO.setHeadIsInDirectionMAEV(false);
}


// update the flyobject vector
vector<FlyObject > updatedFOVector;
updatedFOVector.push_back(cFLargeFO);
updatedFOVector.push_back(cFSmallFO);

currentFI.setFOVector(updatedFOVector);

fIVector[fileCounter] = currentFI;

//	*output << "checking the update"<<endl;
//	FrameInfo tempFI = fIVector[fileCounter];
//	
//	vector<FlyObject > tempFIFOVector = tempFI.getFOVector();
//	FlyObject tempFILFO = tempFIFOVector[0];
//	FlyObject tempFISFO = tempFIFOVector[1];
//	pair<double, double> tempFILFOVV = tempFILFO.getVelocityV();
//	*output << "Large object velocity vector "<<tempFILFOVV.first<<","<<tempFILFOVV.second<<endl;
//	pair<double,double > tempFISFOVV = tempFISFO.getVelocityV();
//	*output << "Small object velocity vector "<<tempFISFOVV.first<<","<<tempFISFOVV.second<<endl;


}

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
// *output<<" val is = " << val << " at (x0,y0) = " << x_init << "," << y_init << " to (x1,y1) = " << x << "," << y <<endl;
len[val]++;
} else
startedWhite = false;

inWhite = false;
}
}
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



void putPixel(Image* maskImage, int x, int y) {

  // ignore the pixels outside of the image boundary
  // if outside maximum y
  if (y >= maskImageHeight) return;
  if (y < 0) return;
  if (x >= maskImageWidth) return;
  if (x < 0) return;

  ColorMono currPixelColor = ColorMono(maskImage->pixelColor(x,y));

  if (currPixelColor.mono() == true) {
    isInFemaleBlob = true;
    //isInBlackZone = false;
    *output << "Hit the male object at "<<x<<","<<y<<endl;

  } else {
    *output << "Going through" << x << "," << y << endl;
    pair<int,int> t(x,y);
    bresenhamLine.push_back(t);
  }
}

int draw_line_bm(Image* maskImage, int x0, int y0, int x1, int y1) {

  isInFemaleBlob = false;
  bresenhamLine.clear();

  maskImageHeight = maskImage->rows();
  maskImageWidth = maskImage->columns();

  int x, y;
  int dx, dy;
  dx = x1 - x0;
  dy = y1 - y0;
  double m = static_cast<double> (dy)/static_cast<double> (dx);

  int octant = -1;
  if (( m >= 0 and m <= 1) and x0 < x1 ) { 
    *output << "Octant 1"<<endl;
    octant = 1;
  } else if ((m > 1) and (y0 < y1)) {
    *output << "Octant 2"<<endl;
    octant = 2;
  } else if ((m < -1) and (y0 < y1)) {
    *output << "Octant 3"<<endl;
    octant = 3;
  } else if ((m <=0 and m >= -1) and (x0 > x1)) {
    *output << "Octant 4"<<endl;
    octant = 4;
  } else if ((m > 0 and m <=1) and (x0 > x1) ) {
    *output << "Octant 5"<<endl;
    octant = 5;
  }else if ((m > 1) and (y0 > y1) ) {
    *output << "Octant 6"<<endl;
    octant = 6;
  }else if ((m < -1) and (y0 > y1) ) {
    *output << "Octant 7"<<endl;
    octant = 7;
  } else if ((m <=0 and m >= -1) and (x0 < x1) ) {
    *output << "Octant 8"<<endl;
    octant = 8;
  }

  int d;
  int delE, delN, delW, delNE, delNW, delSW, delS, delSE;

  dx = abs(dx);
  dy = abs(dy);

  switch (octant) {
    case 1:
      d = 2*dy - dx;
      delE = 2*dy;
      delNE = 2*(dy-dx);

      x = x0;
      y = y0;

      putPixel(maskImage,x, y);

      if (isInFemaleBlob == true)
        return 1;

      while (x < x1) {
        // if we choose E because midpoint is above the line
        if (d <= 0) {
          d = d + delE;
          x = x+1;
        }
        else {
          d = d + delNE;
          x = x + 1;
          y = y + 1;
        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;
      }
      break;

    case 2:

      d = 2*dx - dy;
      delN = 2*dx;
      delNE = 2*(dx-dy);
      x = x0;
      y = y0;

      putPixel(maskImage,x, y);
      if (isInFemaleBlob == true)
        return 1;

      while (y < y1) {
        // if we choose N because midpoint is above the line
        if (d<=0) {
          d = d + delN;
          y = y + 1;
        } 
        else {
          d = d + delNE;
          x = x + 1;
          y = y + 1;

        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;
      }
      break;

    case 3:

      d = dy - 2*dx;
      delN = 2*dx;
      delNW = 2*(dx-dy);
      x = x0;
      y = y0;

      putPixel(maskImage,x, y);
      if (isInFemaleBlob == true)
        return 1;


      while (y < y1) {

        if (d <= 0) {
          d = d + delN;
          y = y + 1;
        } 
        else {
          d = d + delNW;
          y = y + 1;
          x = x - 1;
        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;
      }
      break;

    case 4:

      d = -dx + 2*dy;
      delW = 2*dy;
      delNW = 2*(-dx + dy);

      x = x0;
      y = y0;

      putPixel(maskImage,x, y);
      if (isInFemaleBlob == true)
        return 1;


      while ( x > x1 ) {

        if (d <= 0) {
          d = d + delW;
          x = x - 1;
        }
        else {
          d = d + delNW;
          x = x - 1;
          y = y + 1;
        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;
      }

      break;

    case 5:

      d = -dx + 2*dy;
      delW = 2*dy;
      delSW = 2*(-dx+dy);
      x = x0;
      y = y0;

      putPixel(maskImage,x, y);
      if (isInFemaleBlob == true)
        return 1;


      while (x > x1) {

        if (d<=0) {
          d = d + delW;
          x = x - 1;
        }
        else {
          d = d + delSW;
          x = x - 1;
          y = y - 1;
        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;
      }

      break;

    case 6:

      d = 2*dx - dy;
      delS = 2*dx;
      delSW = 2*(dx-dy);

      x = x0;
      y = y0;

      putPixel(maskImage,x,y);
      if (isInFemaleBlob == true)
        return 1;

      while (y > y1) {

        if (d<=0) {

          d = d + delS;
          y = y -1;
        }
        else {
          d = d + delSW;
          y = y -1;
          x = x -1;
        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;

      }

      break;

    case 7:

      d = 2*dx - dy;
      delS = 2*dx;
      delSE = 2*(dx-dy);

      x = x0;
      y = y0;

      putPixel(maskImage,x,y);
      if (isInFemaleBlob == true)
        return 1;

      while (y > y1) {

        if (d<=0) {
          d = d + delS;
          y = y -1;
        }
        else {
          d = d + delSE;
          y = y - 1;
          x = x + 1;
        }

        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;

      }

      break;

    case 8:

      d = 2*dy - dx;
      delE = 2*dy;
      delSE = 2*(dy - dx);

      x = x0;
      y = y0;

      putPixel(maskImage,x,y);
      if (isInFemaleBlob == true)
        return 1;


      while (x < x1) {

        if (d<=0) {
          d = d + delE;
          x = x + 1;
        }
        else {
          d = d + delSE;
          y = y - 1;
          x = x + 1;
        }

       // *output << "putpixel "<<x<<","<<y<<endl;
        putPixel(maskImage,x, y);
        if (isInFemaleBlob == true)
          return 1;
      }

      break;

    default:
      *output << "No octant which should be a bug\n";
      exit(EXIT_FAILURE);
      break;
  }

  return 0;
}


inline double euclideanDist(pair<int, int > newLocation, pair<int, int> initLocation) {
  double temp = pow((newLocation.first - initLocation.first), 2.0) + pow((newLocation.second - initLocation.second), 2.0);
  temp = sqrt(temp);
  return temp;
}


inline double getSpeed(pair<double, double> vector) {
  double value = vector.first*vector.first + vector.second*vector.second;
  value = sqrt(value);
  return value;
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

void drawTheSequence(int startIndex, int endIndex, int isFirst, bool singleBlob, bool unprocessed) {

  *output << "Should draw "<<isFirst<<endl;

  string fileName = fnVector[startIndex];
  FrameInfo prevFI = fIVector[startIndex];

  *output << "Extracting information for image "<< fnVector[startIndex] << endl;

  *output<<prevFI;

  drawTheFlyObject(prevFI, fileName, isFirst, singleBlob, unprocessed);

  for (int i=startIndex+1; i<=endIndex; i++) {
    FrameInfo nextFI = fIVector[i];
    cout << "Extracting information for image "<< fnVector[i] << endl;
    *output<<nextFI;
    fileName = fnVector[i];
    drawTheFlyObject(nextFI, fileName,  isFirst, singleBlob, unprocessed);
  }

}


void objectHeadDirection(FlyObject prevFO, FlyObject &currentFO) {

  // take the head direction from the previous frame
  pair<double, double> pFHH = prevFO.getHead();

  pair<double, double> cFOEV = currentFO.getMajorAxisEV();
  normalizeVector(cFOEV);
  pair<double, double> cFOREV(-cFOEV.first, -cFOEV.second);

  double dotProdEVAndHH = calculateDotProduct(cFOEV, pFHH);
  if (dotProdEVAndHH > 0 ) {
    *output << "Current head is in direction with the Previous frame Head(which is the historical head from the maxDistIndex)"<<endl;
    double newHeadX = 0.1*cFOEV.first+0.9*pFHH.first;
    double newHeadY = 0.1*cFOEV.second+0.9*pFHH.second;
    pair<double, double> newHead(newHeadX, newHeadY);
    currentFO.setHead(newHead);
    currentFO.setHeadIsInDirectionMAEV(true);

  } else {
    *output << "Current head is in direction with the previous frame head"<<endl;
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
    *output << "Current head is in direction with the Previous frame Head direction"<<endl;
    /*double newHeadX = 0.1*cFOEV.first+0.9*pFHH.first;
      double newHeadY = 0.1*cFOEV.second+0.9*pFHH.second;
      pair<double, double> newHead(newHeadX, newHeadY);
      currentFO.setHead(newHead);
      currentFO.setHeadIsInDirectionMAEV(true);
      */
    currentFO.setHead(cFOEV);
    currentFO.setHeadIsInDirectionMAEV(true);

  } else {
    *output << "Current head is in reverse direction with the previous frame head direction"<<endl;
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
    *output << "Current head is in direction with the velocity vector\n";
    // set the head to the eigen vector
    currentFO.setHead(cFOEV);
    currentFO.setHeadIsInDirectionMAEV(true);

    if (saveEV == 1) {
      *output << "saving the eigen vector for the first object"<<endl;
      evDirectionF.push_back(cFOEV);
    } else if (saveEV == 2){
      *output << "saving the eigen vector for the second object"<<endl;
      evDirectionS.push_back(cFOEV);			
    }

  } else if (dotProdVVAndEV < 0 ){
    *output << "Current head is in reverse direction of the velocity vector"<<endl;
    currentFO.setHead(cFOREV);
    currentFO.setHeadIsInDirectionMAEV(false);

    if (saveEV == 1) {
      *output << "saving the eigen vector for the first object"<<endl;
      evDirectionF.push_back(cFOREV);
    } else if (saveEV == 2) {
      *output << "saving the eigen vector for the second object"<<endl;
      evDirectionS.push_back(cFOREV);			
    }

  } else {

    pair<double, double> zero(0.0,0.0);

    if (saveEV == 1) {
      *output << "saving the zero eigen vector for the first object"<<endl;
      evDirectionF.push_back(zero);
    } else if (saveEV == 2) {
      *output << "saving the zero eigen vector for the second object"<<endl;
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
    *output << "Current head is in direction with the vector used\n";
    // set the head to the eigen vector
    currentFO.setHead(cFOEV);
    currentFO.setHeadIsInDirectionMAEV(true);
  } else {
    *output << "Current head is in reverse direction with the vector used"<<endl;
    currentFO.setHead(cFOREV);
    currentFO.setHeadIsInDirectionMAEV(false);
  }

}


void velocityDirection(int st, int end, pair<double, double > &velDirectionF, pair<double, double>&velDirectionS) {

  // find the average velocity vector
  *output << "Finding average velocity vector from "<<fnVector[st]<<" to "<<fnVector[end]<<endl;
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

  *output << "Velocity vector"<<endl;
  *output << "First = "<<velXFirst<<","<<velYFirst<<endl;
  *output << "Second = "<<velXSecond<<","<<velYSecond<<endl;

  pair<double, double> cFVV(static_cast<double> (velXFirst), static_cast<double> (velYFirst));
  pair<double, double> cSVV(static_cast<double> (velXSecond), static_cast<double> (velYSecond));

  velDirectionF = cFVV;
  velDirectionS = cSVV;
}




void velocityDirections(int stIndex, int endIndex) {

  velocityDirectionsF.clear();
  velocityDirectionsS.clear();
  int i = 0;
  //int intervalLength = 5;
  *output << "Initial velocity direction calculation"<<endl;
  *output << "From index "<<stIndex<<" to "<<endIndex<<endl;
  *output << "From "<<fnVector[stIndex]<<" to "<<fnVector[endIndex]<<endl;

  for (i=stIndex; i<endIndex; i=i++) {

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

    *output << "Normalized velocity"<<endl;
    *output << "First = " <<velDirectionF.first<<","<<velDirectionF.second<<endl;
    *output << "Second = " <<velDirectionS.first<<","<<velDirectionS.second<<endl;

    // set the velocity vector in the objects
    cFFirstFO.setVelocityV(velDirectionF);
    cFSecondFO.setVelocityV(velDirectionS);

    // find the first object head
    *output<<fnVector[i]<<endl;
    *output << "Calculating initial head direction from velocity direction for the first object and storing the ev in direction to the vv"<<endl;
    objectHeadDirection(cFFirstFO,1);

    *output << "Calculating initial head direction from the velocity direction for the second object and storing the ev in direction to the vv"<<endl;
    objectHeadDirection(cFSecondFO,2);


    // update the flyobject vector
    vector<FlyObject > updatedFOVector;
    updatedFOVector.push_back(cFFirstFO);
    updatedFOVector.push_back(cFSecondFO);
    currentFI.setFOVector(updatedFOVector);
    *output << "Updating the frame "<<fnVector[i]<<" after calculating its direction from velocity vector\n";
    fIVector[i] = currentFI;

    /*//*output << fIVector[i];
    // first object overall velocity
    overAllVelocityF.first += velDirectionF.first;
    overAllVelocityF.second += velDirectionF.second;
    // second object overall velocity
    overAllVelocityS.first += velDirectionS.first;
    overAllVelocityS.second += velDirectionS.second;
    */

  }

}


void largestIncreasingPositiveDotProductSeq(vector<pair<double, double> > velocityDirs, int &startIndex, int &endIndex) {

  int positiveVelSeqSize = 0;
  int flag = false;
  int maxSeqSize = 0;
  int st = 0;
  for (unsigned int j=0; j<velocityDirs.size()-1; j++) {
    pair<double,double> prevVel = velocityDirs[j];
    pair<double, double> currVel  = velocityDirs[j+1];

    double dotProd = calculateDotProduct(prevVel, currVel);

    if( dotProd > 0 && flag == false) {
      st = j;
      positiveVelSeqSize++;
      flag = true;
      //*output << "In first if positiveSize "<<positiveVelSeqSize<<endl;

    } else if (dotProd > 0 && flag == true) {
      positiveVelSeqSize++;
      //*output << "In second if positive "<<positiveVelSeqSize<<endl;
    } else {
      positiveVelSeqSize = 0;
      flag = false;
      //*output << "Else\n";			

    }

    if (positiveVelSeqSize > maxSeqSize) {
      maxSeqSize = positiveVelSeqSize;
      startIndex = st;
      endIndex = st+positiveVelSeqSize;
      //*output << "maxseq updated \npositiveSize "<<positiveVelSeqSize<<endl;
      //*output << "st "<<startIndex<<endl;
      //*output << "end "<<endIndex<<endl;
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
      *output << "All directions zero"<<endl;
      startIndex = 0;
      endIndex = 0;
    }
  }
}


void propagateDirections(int object, int s, int e, int origStart, int origEnd) {

  if (object == 1) {
    *output << "For first"<<endl;
  } else {
    *output << "For second"<<endl;
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
    //pair<double , double> cFVV = cFFO.getVelocityV();
    //*output << "Velocity before normalization "<<cFVV.first<<","<<cFVV.second<<endl;
    //normalizeVector(cFVV);
    /**output << "Velocity after normalization "<<cFVV.first<<","<<cFVV.second<<endl;
      pair<double, double> cFEV = cFFO.getMajorAxisEV();
      *output << "Eigen vector before normalization "<<cFEV.first<<","<<cFEV.second<<endl;
      normalizeVector(cFEV);
      *output << "Eigen vector after normalization "<<cFEV.first<<","<<cFEV.second<<endl;

      double dotProd = calculateDotProduct(cFEV, cFVV);
      *output << "Dot product absolute value for frame "<<fnVector[i]<<" : "<<abs(dotProd)<<" real value was "<<dotProd<<endl;

      if (maxDotProduct < abs(dotProd) ) {
      maxDotProduct = abs(dotProd);
      maxDotProductIndex = i;
      }*/

    double velValue = cFFO.getSpeed();
    *output << "speed at "<<fnVector[i]<<" "<<velValue<<endl;
    if (velValue > maxVelValue) {
      maxVelValue = velValue;
      maxVelValIndex = i;
    }

  }

  *output << "Maximum speed is chosen for for frame "<<fnVector[maxVelValIndex]<<" : "<<maxVelValue<<endl;

  // set the head direction according to the represntative velocity
  int t = maxVelValIndex;
  FrameInfo currentFI = fIVector[t];
  vector<FlyObject > cFOVector = currentFI.getFOVector();
  FlyObject cFFirstFO = cFOVector[0];
  FlyObject cFSecondFO = cFOVector[1];
  *output << "Setting representative head direction of frame "<<fnVector[t]<<endl;
  foutLPS << "Setting representative head direction of frame "<<fnVector[t]<<endl;

  if (object == 1) {
    *output << "Setting the head dir according to the representative velocity in the longest positive sequence at "<<fnVector[t]<<endl;
    bool evDirFlag = cFFirstFO.getHeadIsInDirectionMAEV();
    *output << "Before for the first object setting ev in velocity direction it is "<<evDirFlag<<endl;
    objectHeadDirection(cFFirstFO);
    evDirFlag = cFFirstFO.getHeadIsInDirectionMAEV();
    *output << "After setting ev in velocity direction it is "<<evDirFlag<<endl;
    cFOVector[0] = cFFirstFO;
    currentFI.setFOVector(cFOVector);
    fIVector[t] = currentFI;
  }
  else {
    //pair<double, double> tempVelocity = cFSecondFO.getVelocityV();
    //*output << "Velocity was "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
    *output << "Setting the head direction according to the representative velocity in the longest positive sequence"<<endl;

    bool evDirFlag = cFSecondFO.getHeadIsInDirectionMAEV();
    *output << "Before for the second object setting ev in velocity direction it is "<<evDirFlag<<endl;
    objectHeadDirection(cFSecondFO);
    evDirFlag = cFSecondFO.getHeadIsInDirectionMAEV();
    *output << "After setting ev in velocity direction it is "<<evDirFlag<<endl;

    //tempVelocity = cFSecondFO.getVelocityV();
    //*output << "Velocity became "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
    cFOVector[1] = cFSecondFO;
    currentFI.setFOVector(cFOVector);
    fIVector[t] = currentFI;
  }

  int intervalLength = 1;
  if ((e-s)> intervalLength) {

    *output << "Propagating in the longest positive frame interval starting the direction from the frame "<<fnVector[t];
    *output << " to "<<fnVector[e]<<endl;
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
        //*output << "First object extract"<<endl;
        objectHeadDirection(pFFirstFO, cFFirstFO, false);
        //*output << "First object update"<<endl;
        cFOVector[0] = cFFirstFO;

        pFFirstFO = cFFirstFO;

      } else {
        //*output << "Second object extract"<<endl;
        pair<double, double> tempVelocity = cFSecondFO.getVelocityV();
        //*output << "Velocity was "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
        objectHeadDirection(pFSecondFO, cFSecondFO, false);
        //*output << "Second object update"<<endl;
        tempVelocity = cFSecondFO.getVelocityV();
        //*output << "Velocity became "<<tempVelocity.first<<","<<tempVelocity.second<<endl;
        cFOVector[1] = cFSecondFO;

        pFSecondFO = cFSecondFO;

      }

      currentFI.setFOVector(cFOVector);

      fIVector[i] = currentFI;

    }

    *output << "propagating from "<<fnVector[t]<<" to "<<fnVector[s]<<endl;
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
        //*output << "First object extract"<<endl;
        objectHeadDirection(pFFirstFO, cFFirstFO, false);
        //*output << "First object update"<<endl;
        cFOVector[0] = cFFirstFO;

        pFFirstFO = cFFirstFO;

      } else {
        //*output << "Second object extract"<<endl;
        objectHeadDirection(pFSecondFO, cFSecondFO, false);
        //*output << "Second object update"<<endl;
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
    *output << "Propagating front from "<<fnVector[e]<<" to "<<fnVector[origEnd]<<endl;		

    for (int i=e+1; i<=origEnd; i++) {
      FrameInfo currentFI = fIVector[i];
      vector<FlyObject > cFOVector = currentFI.getFOVector();
      FlyObject cFFirstFO = cFOVector[0];
      FlyObject cFSecondFO = cFOVector[1];

      if (object == 1) {
        objectHeadDirection(pFFirstFO, cFFirstFO, false);
        cFOVector[0] = cFFirstFO;
        pFFirstFO = cFFirstFO;

      } else {
        objectHeadDirection(pFSecondFO, cFSecondFO, false);
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
    *output << "Propagating down wards from "<<fnVector[s]<<" to "<<fnVector[origStart]<<endl;
    for (int i=s-1; i>=origStart; i--) {
      FrameInfo currentFI = fIVector[i];
      vector<FlyObject > cFOVector = currentFI.getFOVector();
      FlyObject cFFirstFO = cFOVector[0];
      FlyObject cFSecondFO = cFOVector[1];

      if (object == 1) {
        //*output << "First object extract"<<endl;
        objectHeadDirection(pFFirstFO, cFFirstFO, false);
        //*output << "First object update"<<endl;
        cFOVector[0] = cFFirstFO;

        pFFirstFO = cFFirstFO;

      } else {
        //*output << "Second object extract"<<endl;
        objectHeadDirection(pFSecondFO, cFSecondFO, false);
        //*output << "Second object update"<<endl;
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

  *output << "From file "<<fnVector[st]<<" to "<<fnVector[end]<<endl;
  //*output << "Assigning object head direction between Index "<<mid<<" to startIndex "<<st<<endl;

  // calculate the velocity directions first
  // clear evDirectionF and evDirectionS to store the new evs for the current sequence
  evDirectionF.clear();
  evDirectionS.clear();

  velocityDirections(st, end);

  *output << "Size of evDirectionF "<<evDirectionF.size()<<endl;
  *output << "Size of evDirectionS "<<evDirectionS.size()<<endl;

  // debug
  *output << "------------ALL VELOCITY AND CORRESPONDING EV-------------\n";
  int a;
  for (a=0; a < velocityDirectionsF.size(); a++) {

    *output << "For frame "<<fnVector[a+st]<<endl;

    pair<double, double> z = velocityDirectionsF[a];
    pair<double, double> zEV = evDirectionF[a];
    *output <<" First object velocity = "<<z.first<<","<<z.second<<endl;
    *output <<" First object ev       = "<<zEV.first<<","<<zEV.second<<endl;

    pair<double, double> w = velocityDirectionsS[a];
    pair<double, double> wEV = evDirectionS[a];

    *output << "Second object velocity = "<<w.first<<","<<w.second<<endl;
    *output << "Second object ev       = "<<wEV.first<<","<<wEV.second<<endl;
  }

  *output << "Last frame index wont have velocity a+st("<<(a+st)<<") = end("<<end<<") "<<fnVector[a+st]<<endl;
  *output << "------------END-------------\n";

  int s;
  int e;

  foutLPS<<"------------------------------------------------------------------"<<endl;
  largestIncreasingPositiveDotProductSeq(evDirectionF, s, e);
  *output << "Positive indexes are "<<fnVector[st+s]<<" to "<<fnVector[st+e]<<endl;
  int si = s + st;
  int ei = e + st;
  foutLPS << "For first object max positive directions from "<<fnVector[si]<<" to "<<fnVector[ei]<<endl;
  propagateDirections(1, si, ei, st, end);

  s = 0;
  e = 0;

  largestIncreasingPositiveDotProductSeq(evDirectionS, s, e);
  *output << "Positive indexes are "<<fnVector[st+s]<<" to "<<fnVector[st+e]<<endl;
  si = s + st;
  ei = e + st;
  foutLPS << "For second object max positive directions from "<<fnVector[si]<<" to "<<fnVector[ei]<<endl;
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
  *output << "previousFirst to currentFirst "<<distLTL<<endl;

  double distLTS = euclideanDist(pFLargeFO, cFSmallFO);
  *output << "prevFirst to currentSecond "<<distLTS<<endl;

  double min1 = min(distLTL, distLTS);
  *output<<"min of FTF and FTS is "<<min1<<endl;

  double distSTL = euclideanDist(pFSmallFO, cFLargeFO);
  *output << "previousSecond to currentLarge "<<distSTL<<endl;

  double distSTS =euclideanDist(pFSmallFO, cFSmallFO);
  *output << "prevSecond to currentSecond "<<distSTS<<endl;

  double min2 = min(distSTL, distSTS);

  // if prev frame larger object is closer to current frame smaller object
  // this might happen for two cases i) the smaller object is closer in the current frame (so this situation should be handled with some other heuristics)
  //								   ii)the segmentation process toggles the size (error in the segmentation process)

  if (min1 < min2) {
    if (distLTS == min1) {
      *output<<"Shortest distance is from the previous frame first object. And shortest distance is from previos first object to current second object so we need to swap the objects in current frame"<<endl;
      currentFI.swapTheFlyObject();
    }
  } else {
    if (distSTL == min2) {
      *output << "Shortest distance is from the previous frame second object. And shortest distance is from previous second object to current first object so we need to swap the objects in the current frame"<<endl;
      currentFI.swapTheFlyObject();
    }
  }

}

double euclideanDist(FlyObject a, FlyObject b) {

  pair<int, int> aCentroid = a.getCentroid();
  pair<int,int> bCentroid = b.getCentroid();

  //*output << "Distance from "<<aCentroid.first<<","<<aCentroid.second<<" to "<<bCentroid.first<<","<<bCentroid.second<<endl;
  double x2 = pow((static_cast<double> (aCentroid.first)-static_cast<double> (bCentroid.first)), 2.0);
  double y2 = pow((static_cast<double> (aCentroid.second)-static_cast<double> (bCentroid.second)), 2.0);
  double dist = sqrt((x2+y2));
  *output << dist<<endl;

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
      *output << "New max distance" << maxDist << " at the frame number "<<i;
      *output << endl;
    }

  }

  *output << "Maximum distance between the frame found at the index "<<maxDistIndex<<endl;
  *output << "Assigning object correspondance between maxDistIndex "<<maxDistIndex<<" to startIndex "<<startOfATrackSequence<<endl;
  // assign closest distance object association before the between frames startOfASeq to maxDistIndex
  FrameInfo prevFI = fIVector[maxDistIndex];
  for (int i=maxDistIndex-1; i>=startOfATrackSequence; i--) {
    FrameInfo currentFI = fIVector[i];
    objectCorrespondence(prevFI, currentFI);

    // update the fIVector
    fIVector[i] = currentFI;
    prevFI = currentFI;

  }
  *output << "Assigning object correspondance between maxDistIndex+1 "<<(maxDistIndex+1)<<" to endIndex "<<endOfATrackSequence<<endl;
  // assign closest distance object association after the maxDistIndex to endOfSeq
  prevFI = fIVector[maxDistIndex];
  for (int i=maxDistIndex+1; i<=(endOfATrackSequence); i++) {
    FrameInfo currentFI = fIVector[i];

    *output << "Object correspondance frame number "<<i<<endl;

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

    *output << "For frame number "<<i<<"\n";
    *output << "SequenceFirst area sum = "<<sequenceFirstAverage<<endl;
    *output << "SequenceSecond area sum = "<<sequenceSecondAverage<<endl;
  }

  sequenceFirstAverage /= (endOfATrackSequence-startOfATrackSequence + 1);
  sequenceSecondAverage /= (endOfATrackSequence-startOfATrackSequence + 1);

  *output << "----------------------------------------------------\n";
  *output << "SequenceFirst average area = "<<sequenceFirstAverage<<endl;
  *output << "SequenceSecond average area = "<<sequenceSecondAverage<<endl;
  *output << "----------------------------------------------------\n";

  if (sequenceFirstAverage > sequenceSecondAverage) {
    foutLPS << "First object is the female and avg size is "<<sequenceFirstAverage<<" and "<<sequenceSecondAverage<<endl;
  } else {
    foutLPS<<"Second object is femaleand avg size is "<<sequenceFirstAverage<<" and "<<sequenceSecondAverage<<endl;
  }

  // calculating the head direction from the collision time to backward
  *output << "calculating the head direction "<<startOfATrackSequence<<" to "<<endOfATrackSequence<<endl;
  foutLPS<<"calculating the head direction "<<startOfATrackSequence<<" to "<<endOfATrackSequence<<endl;
  calculateHeadDirection(startOfATrackSequence, endOfATrackSequence, maxDistIndex);

  if (sequenceFirstAverage > sequenceSecondAverage) {
    *output << "First sequence is the object A"<<endl;
    drawTheSequence(startOfATrackSequence, endOfATrackSequence, 1, false, false);
  }
  else {
    *output << "Second sequence is the object A"<<endl;
    drawTheSequence(startOfATrackSequence, endOfATrackSequence, 0, false, false);
  }

}



// is set each time the start is found
// useful when one blob is detected on the border of the mask image
bool isFoundStartPoint = false;
int hitTheFly(Image* maskImage, int &intersectX, int &intersectY) {

  //Image* maskImage = new Image(fileName.c_str());

  for (int i=bresenhamLine.size()-1; i>=0; i--) {

    pair<int,int> tempP = bresenhamLine[i];

    int x = tempP.first;

    int y = tempP.second;

    ColorMono currPixelColor = ColorMono(maskImage->pixelColor(x,y));

    if (currPixelColor.mono() == true) {
      *output << "Hit the target fly"<<" "<<x<<","<<y<<endl;
      intersectX = x;
      intersectY = y;
      return 1;
    } else if (currPixelColor.mono() == false and ( x == 0 || x == (maskImageWidth-1) || y == 0 || y == (maskImageHeight-1))) {
      *output << "Is at the boundary"<<endl;
      return 2;
    }
    else {
      *output << "Going through" << x << "," << y << endl;
    }


  }
  return -1;

}

void findTheStartPoint(string fileName, int desiredSize, int otherSize, int cen_x, int cen_y,  bool eVDirection) {

  string segmImageFileName = maskImagePath + fileName;

  int width = 0;
  int height = 0;

  int x0, y0, x1, y1;

  x0 = cen_x;
  y0 = cen_y;

  double ev_x;
  double ev_y;

  char buffer[100];

  bool found = false;	

  vector<pair<int, int> > foundShape;
  vector<pair<int,int> > shape;

  Image *image, *mask;

  cout << "Segmented image "<<segmImageFileName<<"\n";

  if (desiredSize == otherSize) {
    foutDebugCen<<"File name "<<segmImageFileName<<endl;
    foutDebugCen<<"MaleSize == FemaleSize\n";
    foutDebugCen<<"DesiredCentroid.first, DesiredCentroid.second = ("<<cen_x<<","<<cen_y<<")"<<endl;
  }

  image = new Image(segmImageFileName.c_str());

  width = image->columns();
  height = image->rows();

  sprintf(buffer,"%ix%i",width,height);

  // the residual image should be newed
  residual = new Image(buffer, "black");

  *output<<"Detecting the male object for finding the start point"<<endl;
  for (int x = 0; x<width and found == false; x++) {
    for (int y = 0; y<height and found == false; y++) {

      //*output<<"comes here"<<endl;
      shape.clear();
      findObj(image, x, y, shape, true, true);
      unsigned int s = shape.size();

      if ( s > 0 )
      {

        *output << "size of the object is: " << s <<endl;

        if (desiredSize == otherSize) {

          // debug:
          *output << "Inside the maleSize == femaleSize where (cen_x, cen_y) = "<<cen_x<<","<<cen_y<<endl;
          pair<int, int> tmpCentroid = getCentroid(shape);
          foutDebugCen<<"tmpCentroid.first, tmpCentroid.second = ("<<tmpCentroid.first<<","<<tmpCentroid.second<<")"<<endl;
          if (tmpCentroid.first == cen_x and tmpCentroid.second == cen_y) {

            found = true;
            *output << "Detected the desired object when the sizes are equal"<<endl;
            foundShape = shape;
            foutDebugCen<<"Found by desired object after recomputing the centroid"<<endl;
          }

          // not correct logic when the centroid can be black pixel
          /*for (int l=0; l<s and found == false; l++) {

            pair<int, int> point = shape[l];
            *output << "point.first, point.second "<<point.first<<","<<point.second<<endl;

          // this should give most of the time when the centroid is not a black pixel
          // a centroid can be black pixel when the shape of the fly is distorted inside
          if (point.first == cen_x and point.second == cen_y) {
          found = true;
          *output << "Detected the male object when the sizes of male/female are equal"<<endl;
          foundShape = shape;
          }
          }*/


        } else {

          *output<<"Elseblock: Inside the desiredSize is not equal the otherSize"<<endl;
          if (s == desiredSize) {
            found = true;
            *output << "Detected the desired object just comparing the sizes"<<endl;
            foundShape = shape;
            *output<<"foundshape size "<<foundShape.size()<<endl;
          }
        }
      }
    }
  }

  if (found == true) {
    *output<<"foundshape is assigned a value"<<endl;
  } else {
    *output<<"ERROR: foundshape is not assigned a value so the next step would draw a line over an empty image"<<endl;
    exit(EXIT_FAILURE);
  }

  vector<double> eigenVal = covariantDecomposition(foundShape);

  if (eVDirection == true) {
    ev_x = static_cast<double> (x0) + static_cast<double> (diagLength)*eigenVal[4];
    ev_y = static_cast<double> (y0) + static_cast<double> (diagLength)*eigenVal[5];
  }
  else {
    ev_x = static_cast<double> (x0) - static_cast<double> (diagLength)*eigenVal[4];
    ev_y = static_cast<double> (y0) - static_cast<double> (diagLength)*eigenVal[5];

  }

  x1 = static_cast<int> (ev_x);
  y1 = static_cast<int> (ev_y);

  *output<<"Endpoint: centroid (x0,y0)==("<<x0<<","<<y0<<")"<<endl;
  *output<<"Startpoint: OutsidePointInEVDirection (x1,y1)==("<<x1<<","<<y1<<")"<<endl;

  mask = new Image(buffer, "black");

  for (int i=0; i<foundShape.size(); i++) {
    pair<int,int > point = foundShape[i];
    mask->pixelColor(point.first, point.second,"white");
  }

  /*int hits= */ draw_line_bm(mask, x1, y1, x0, y0);


  //mask->strokeColor("red");
  //mask->draw(DrawableLine(x1, y1, x0, y0));
  //mask->write("test.png");

  *output<<"BresenhamLine size is "<<bresenhamLine.size()<<endl;
  /*	if (hits == 1) {

  // if the object is at the border then bresenhamLine should be empty; because we start saving the coordinate
  // when the line enters into the mask image
  if (bresenhamLine.size() > 0) {
  pair<int, int> temp = bresenhamLine[bresenhamLine.size()-1];
  *output << "Finding the starting point: Hits source at "<<temp.first<<","<<temp.second<<endl;
  ColorMono c = mask->pixelColor(temp.first, temp.second);
  //maleSP_x = prev_x;
  //maleSP_y = prev_y;

  if (c.mono() == true) {
  *output << "start point from the source object should be black"<<endl;
  exit(0);
  }
  isFoundStartPoint = true;
  // reset it after its corresponding hitTheFly() function call

  } 
  else {
  isFoundStartPoint = false;
  *output<<"The object is at the border in the ask image. BresenhamLine vector is empty. Size : "<<bresenhamLine.size()<<endl;
  }

  } else {
  *output << "Error: The brsenham line must intersect to the source object."<<endl;
  exit(0);
  }
  */
  // if the object is at the border then bresenhamLine should be empty; because we start saving the coordinate
  // when the line enters into the mask image
  if (bresenhamLine.size() > 0) {
    pair<int, int> temp = bresenhamLine[bresenhamLine.size()-1];
    *output << "Finding the starting point: Hits source at "<<temp.first<<","<<temp.second<<endl;
    ColorMono c = mask->pixelColor(temp.first, temp.second);
    //maleSP_x = prev_x;
    //maleSP_y = prev_y;

    if (c.mono() == true) {
      *output << "start point from the source object should be black"<<endl;
      exit(EXIT_FAILURE);
    }
    isFoundStartPoint = true;
    // reset it after its corresponding hitTheFly() function call

  } 
  else {
    isFoundStartPoint = false;
    *output<<"The object is at the border in the mask image. BresenhamLine vector is empty. Size : "<<bresenhamLine.size()<<endl;
  }

  delete residual;
  delete image;
  delete mask;
}


void calculateStatistics(FrameInfo currentFI, string fileName, int isFirst, bool singleBlob, bool isHitting, bool isHittingFemaleToMale, bool unprocessed) {

  foutSt<< "Statistics generation for "<<fileName<<endl;
  foutSt<< "----------------------------------------------\n";
  vector<FlyObject > fOVector = currentFI.getFOVector();

  if (singleBlob == false and unprocessed == false)  {

    FlyObject maleFO = fOVector[isFirst];
    FlyObject femaleFO = fOVector[1-isFirst]; 

    pair<int, int> maleCentroid = maleFO.getCentroid();
    pair<int, int> femaleCentroid = femaleFO.getCentroid();

    pair<double, double> maleMajorAxisEV = maleFO.getMajorAxisEV();
    pair<double, double > femaleMajorAxisEV = femaleFO.getMajorAxisEV();

    bool maleEVDir = maleFO.getHeadIsInDirectionMAEV();
    bool femaleEVDir = femaleFO.getHeadIsInDirectionMAEV();


    // 1. finding the distance between the centroids
    double tempDist = pow(static_cast<double>(maleCentroid.first - femaleCentroid.first),2) + pow(static_cast<double>(maleCentroid.second-femaleCentroid.second),2);

    tempDist = sqrt(tempDist);

    // round the function
    unsigned int dist = roundT(tempDist);

    centroidDistanceMap[dist] = centroidDistanceMap[dist] + 1;

    foutSt << "Centroid distance "<<dist<<endl;

    // 2. finding the angle between the head direction
    normalizeVector(maleMajorAxisEV);
    normalizeVector(femaleMajorAxisEV);
    pair<double, double> maleHeadDir;
    pair<double, double> femaleHeadDir;

    if (maleEVDir == true) {
      maleHeadDir = maleMajorAxisEV;
      foutSt<<"Male Head In direction of ev"<<endl;

    } else {
      foutSt<<"Male Head is in opposite of ev"<<endl;
      maleHeadDir.first = -maleMajorAxisEV.first;
      maleHeadDir.second = -maleMajorAxisEV.second;
    }

    if (femaleEVDir == true) {
      femaleHeadDir = femaleMajorAxisEV;
      foutSt<<"Female Head In direction of ev"<<endl;
    } else {
      femaleHeadDir.first = -femaleMajorAxisEV.first;
      femaleHeadDir.second = -femaleMajorAxisEV.second;
      foutSt<<"Female Head is in opposite of ev"<<endl;
    }

    double dp = calculateDotProduct(femaleHeadDir, maleHeadDir);

    *output<<"Dot product "<<fixed<<setprecision(8)<<dp<<endl;

    float rad = acos(static_cast<float>(dp));
    *output<<"Angle in radian "<<rad<<endl;

    float deg = rad*180.0/static_cast<float>(PI);
    *output<<"Angle in deg "<<deg<<endl;

    unsigned int a = static_cast<unsigned int>(roundT(static_cast<double>(deg)));
    *output<<"Angle after rounding "<<a<<endl;

    headDirAngleMap[a]++;

    // TODO: what is going on with the constant comparison?
    foutSt<<"Dot product was "<<dp<<endl;
    foutSt<<"Angle between ("<<maleHeadDir.first<<","<<maleHeadDir.second<<") and ("<<femaleHeadDir.first<<","<<femaleHeadDir.second<<") : "<<a<<endl;
    if(a == -2147483648 || a == 2147483648) {
      *output<<"Angle between ("<<maleHeadDir.first<<","<<maleHeadDir.second<<") and ("<<femaleHeadDir.first<<","<<femaleHeadDir.second<<") : "<<a<<endl;
      *output<<"Incorrect angle calculation :"<<a<<endl;
      exit(EXIT_FAILURE);
    }
    // 3. generate number of times male is looking at the female
    if (isHitting == true) {
      totalMaleLookingAtFemale = totalMaleLookingAtFemale + 1;
      foutSt << "Male is looking at female"<<endl;
    }
    // generate number of times female is looking at the male
    if (isHittingFemaleToMale == true) {
      totalFemaleLookingAtMale = totalFemaleLookingAtMale + 1;
      foutSt <<"Female is looking at male"<<endl;
    }


    // 4. generate speed distribution consider speeds of both
    double speedMale = maleFO.getSpeed();
    double speedFemale = femaleFO.getSpeed();
    foutSt<<"Male speed is "<<speedMale<<" and Female speed is "<<speedFemale<<endl;
    int spM = roundT(speedMale);
    int spF = roundT(speedFemale);
    foutDebugSpeed<<fileName<<"\t"<<"Male "<<speedMale<<"\t"<<"Female "<<speedFemale<<endl;
    foutDebugSpeed<<fileName<<"\t"<<"Male "<<spM<<"\t"<<"Female "<<spF<<endl;
    foutDebugSpeed<<"---------------------------------------"<<endl;
    speedMap[spM] = speedMap[spM] + 1;
    speedMap[spF] = speedMap[spF] + 1;


    totalSeparated = totalSeparated + 1;
    foutSt<<"Frame contains separated flies\n";
    //foutSt<<"Male speed "<<speedMale<<endl;
    //foutSt<<"Female speed "<<speedFemale<<endl;

  } else if (singleBlob == true and unprocessed == false) {
    // 4. generate the number of times they are as single blob
    totalSingleBlob++;
    *output << "Current frame is single blob\n";
    foutSt << "Frame is a single blob\n";
  } // singleBlob == false and unprocessed == true 
  else if (unprocessed == true) {
    totalUnprocessedFrame = totalUnprocessedFrame + 1;
    *output <<"Current frame is unprocessed"<<endl; 
    foutSt <<"Current frame is unprocessed"<<endl;
  } // else condition would never be generated singleBlob == true and unprocessed == true.
  // so it is not checked.

}


void drawTheFlyObject(FrameInfo currentFI, string fileName, int isFirst, bool singleBlob, bool unprocessed) {

  *output << "isFirst is "<<isFirst<<endl;
  Image* img = NULL;
  string inputFileName;
  string outputFileName;
  if(writeFinalImages) {	
    inputFileName = origImagePath + fileName;
    outputFileName = finalOutputPath + fileName;
    img = new Image(inputFileName.c_str());
  }
  vector<FlyObject > fOVector = currentFI.getFOVector();

  *output << "While drawing it found objects = "<<fOVector.size()<<endl;

  string maskFile = maskImagePath + fileName;
  Image* maskImage = new Image(maskFile.c_str());
  int intersectX=-1, intersectY=-1;
  int isHitting=-1;
  int isHittingFemaleToMale = -1;

  // the unprocessed flag is used to handle those sequence that are less than 15 frames long and flies are separated there.
  if (singleBlob == false and unprocessed == false) {

    // find the sizes for finding the start point
    FlyObject fO = fOVector[isFirst];
    int maleSize = fO.getArea();
    fO = fOVector[1-isFirst];
    int femaleSize = fO.getArea();
    pair<int, int> femaleCentroid = fO.getCentroid();
    bool eVDirectionFemale = fO.getHeadIsInDirectionMAEV();

    FlyObject currentFO = fOVector[isFirst];

    pair<int, int> centroid = currentFO.getCentroid();

    bool eVDirection = currentFO.getHeadIsInDirectionMAEV();

    // debug:
    //*output<<"Calling the findTheStartPoint() function"<<endl;
    //*output<<"Female size "<<femaleSize<<" maleSize "<<maleSize<<" MaleCentroid = "<<centroid.first<<", "<<centroid.second<<endl;

    // initialize the flag with false for the next found of findTheStartPoint()
    isFoundStartPoint = false;

    // finding male hitting the female
    findTheStartPoint(fileName, maleSize, femaleSize, centroid.first, centroid.second, eVDirection);
    if (isFoundStartPoint == true) {
      isHitting = hitTheFly(maskImage, intersectX, intersectY);
      *output<<"Male intersects the female at "<<intersectX<<","<<intersectY<<endl;
      foutSt<<"Male intersects the female at "<<intersectX<<","<<intersectY<<endl;
    } else {
      isHitting = -1;
      *output<<"Male head direction doesn't intersect with the female"<<endl;
      foutSt<<"Male head direction doesn't interesect with the female"<<endl;
    }

    intersectX = -1;
    intersectY = -1;
    // initialize the flag with false for the next found of findTheStartPoint
    isFoundStartPoint = false;

    // female hitting the male
    findTheStartPoint(fileName, femaleSize, maleSize, femaleCentroid.first, femaleCentroid.second, eVDirectionFemale);
    if ( isFoundStartPoint == true ) {
      isHittingFemaleToMale = hitTheFly(maskImage, intersectX, intersectY);
      *output<<"Female intersects the male at "<<intersectX<<","<<intersectY<<endl;
      foutSt<<"Female intersects the male at "<<intersectX<<","<<intersectY<<endl;
    } else {
      isHittingFemaleToMale = -1;
      *output<<"Female head direction doesn't intersect with the male"<<endl;
      foutSt<<"Female head direction doesn't intersect with the male"<<endl;
    }

    for (int n=0; n<fOVector.size(); n++) {

      FlyObject currentFO = fOVector[n];

      pair<int, int> centroid = currentFO.getCentroid();
      pair<double, double> majorAxisEV = currentFO.getMajorAxisEV();

      bool eVDirection = currentFO.getHeadIsInDirectionMAEV();

      double ev_x, ev_y;
      double prev_x, prev_y;

      if(writeFinalImages) {
        // draw the female when tracked by the male fly
        if (isHitting == 1) {
          if (n != isFirst and n == 0) {
            img->strokeColor("red");

            img->draw(DrawableRectangle(centroid.first - 6, centroid.second - 6, centroid.first + 6, centroid.second + 6));
          } else if (n != isFirst and n==1) {

            img->strokeColor("red");

            img->draw(DrawableRectangle(centroid.first - 6, centroid.second - 6, centroid.first + 6, centroid.second + 6));
          }
        }


        // draw the male when tracked by the female fly
        // this situation will draw the YELLOW circle as well as the ORANGE rectangle around the
        // male object. This double color ensure that this situation is incorrectly detects the male
        // as female. Because our assumption is that female never tracks the male. So the actual female
        // tracking male will occur very insignificant times.
        if (isHittingFemaleToMale == 1) {
          if (n == isFirst and n == 0) {
            img->strokeColor("Red");

            img->draw(DrawableRectangle(centroid.first - 6, centroid.second - 6, centroid.first + 6, centroid.second + 6));
          } else if (n == isFirst and n==1) {

            img->strokeColor("Red");

            img->draw(DrawableRectangle(centroid.first - 6, centroid.second - 6, centroid.first + 6, centroid.second + 6));
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
        img->strokeColor("blue");
        pair<double, double> velocityV = currentFO.getVelocityV();
        ev_x = static_cast<double>(centroid.first) + 30.0 * velocityV.first;
        ev_y = static_cast<double>(centroid.second) + 30.0 * velocityV.second;
        img->draw(DrawableLine( centroid.first, centroid.second, static_cast<int>(ev_x), static_cast<int>(ev_y) ));

        // draw the historical head vector
        img->strokeColor("white");
        pair<double, double> headV = currentFO.getHead();
        ev_x = static_cast<double> (centroid.first) + 25.0*headV.first;
        ev_y = static_cast<double> (centroid.second) + 25.0*headV.second;
        img->draw( DrawableLine(centroid.first, centroid.second, static_cast<int> (ev_x), static_cast<int> (ev_y)) );

        //draw the object tracking circle
        if (n == isFirst and n==0) {
          *output << "Tracking the n = "<<n<<endl;
          img->strokeColor("yellow");
          img->draw(DrawableCircle(centroid.first, centroid.second, centroid.first+5, centroid.second));
          img->pixelColor(prev_x, prev_y, "red");
        } else if ( n == isFirst and n==1) {
          *output << "Tracking the "<<n<<endl;
          img->strokeColor("yellow");
          img->fillColor("none");
          img->pixelColor(prev_x, prev_y, "red");
          img->draw(DrawableCircle(centroid.first, centroid.second, centroid.first+5, centroid.second));
        }

      }
    }
  }

  if(writeFinalImages) {
    img->write(outputFileName.c_str());
    delete img;
  }
  delete maskImage;

  // TODO: Wtf is going on here?
  if (isHitting == 1 || isHittingFemaleToMale == 0)
    calculateStatistics(currentFI, fileName, isFirst, singleBlob, true, false, unprocessed);
  else if (isHitting == 0 || isHittingFemaleToMale == 1)
    calculateStatistics(currentFI, fileName, isFirst, singleBlob, false, true, unprocessed);
  else if (isHitting == 1 || isHittingFemaleToMale == 1)
    calculateStatistics(currentFI, fileName, isFirst, singleBlob, true, true, unprocessed);
  else
    calculateStatistics(currentFI, fileName, isFirst, singleBlob, false, false, unprocessed);

}


void findObj(Image* img, int x, int y, vector<pair<int,int> > & shape ,bool eightCon, bool colorLookingFor) {
  assert(residual != NULL);
  if (eightCon == true)
    eightConnObj(img, x, y, shape, colorLookingFor);
  else {
    fourConnObj(img, x, y, shape, colorLookingFor);
  }
}


void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color) {
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

    // setting the residual image at pixel(x,y) to white.
    residual->pixelColor(x,y, ColorMono(true));

    // Recursively call all of it's eight neighbours.
    fourConnObj(img, x+1, y, obj, color);
    fourConnObj(img, x, y-1, obj, color);

    fourConnObj(img, x-1, y, obj, color);
    fourConnObj(img, x, y+1, obj, color);
  }

}

void eightConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color) {
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
pair<int,int> getCentroid(vector<pair<int,int> > & points) {
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


vector<double> covariantDecomposition(vector<pair<int,int> > & points) {
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
  //		*output<<"eigenvector = \n";
  //		gsl_vector_fprintf (stdout, &evec_i.vector, "%g");
  //	}

  gsl_vector_free(eigenVal);
  gsl_matrix_free(matrice);
  gsl_matrix_free(eigenVec);

  return retval;
}

// isInterface for binary image
bool isInterface(Image* orig, unsigned int x, unsigned int y) {
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

int main(int argc, char **argv) {

  int c;	
  bool verbose = false;
  string usage = "Usage: FlyTracking -i <inputFile.txt> -o <originalImagePath> -f <finalOutputPath> -m <maskImagePath> -p <outputFilePrefix>";
  int opterr = 0;

  while ((c = getopt (argc, argv, "i:f:m:p:o:hxv")) != -1)
    switch (c) {
      case 'i':
        inputFileName = optarg;
        break;
      case 'o':
        origImagePath = optarg;
        break;
      case 'f':
        finalOutputPath = optarg;
        break;
      case 'm':
        maskImagePath = optarg;
        break;
      case 'p':
        outputFilePrefix = optarg;
        break;
      case 'h':
        cout << usage << endl;
        exit(EXIT_FAILURE);
        break;
      case 'x':
        writeFinalImages = true;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        break;
    }

  (verbose) ? output = &cout : output = &nullLog; 

  *output << "verbose logging out" << endl;

  if( inputFileName.empty() || origImagePath.empty() || finalOutputPath.empty() || maskImagePath.empty() || outputFilePrefix.empty() ) {
    cerr <<  usage << endl;
    cerr << "input name: " << inputFileName << endl;
    cerr << "original path: " << origImagePath << endl;
    cerr << "output path: " << finalOutputPath << endl;
    cerr << "mask path: " << maskImagePath << endl;
    cerr << "output prefix: " << outputFilePrefix << endl;
    exit(EXIT_FAILURE);
  }	

  string fileName;	
  ifstream inputFile(inputFileName.c_str());

  if (inputFile.fail() ) {
    cerr << "cannot open the input file that contains name of the input images\n";
    exit(EXIT_FAILURE);
  }

  string statFileName = finalOutputPath + outputFilePrefix + "_statFile.txt";
  foutSt.open(statFileName.c_str());

  if (foutSt.fail()) {
    cerr<<"cannot open the statfile"<<endl;
    exit(EXIT_FAILURE);
  }

  // debug file
  string foutDebugCenFN = finalOutputPath + outputFilePrefix + "_statFileDebug.txt";
  foutDebugCen.open(foutDebugCenFN.c_str());

  if (foutDebugCen.fail()) {
    cerr << "cannot open the statDebug file"<<endl;
    exit(EXIT_FAILURE);
  }

  // debug file speed distribution
  string foutDebugSpeedFN = finalOutputPath + outputFilePrefix + "_speedDebug.txt";
  foutDebugSpeed.open(foutDebugSpeedFN.c_str());
  if (foutDebugSpeed.fail()) {
    cerr << "cannot open the speedDebug file"<<endl;
    exit(EXIT_FAILURE);
  }

  // open the file for statistics
  string lPSFileName("LongestPositive.txt");
  lPSFileName = finalOutputPath + outputFilePrefix + "_" + lPSFileName;
  *output << "LongestPositive.txt file name is "<<lPSFileName<<endl;
  foutLPS.open(lPSFileName.c_str());

  // unsigned int objCount = 0;
  //int frameCounter = 0;
  int fileCounter=0;

  char buffer[100];
  string imgSize;
  //	FlyObject a,b;
  vector<pair<int,int> > shape;
  vector<FlyObject > tempFOV;

  // to find the new head direction
  //bool currentlyCalculatingHead = true;

  while (inputFile>>fileName) {

    int fi = fileName.find("_");
    // Be aware that this limits us to sample size of 99,999 (55.55 minutes)
    // current sequence numbers spans from 0 - 18019, so 5 digits are needed
    int span = 5;
    string tempString = fileName.substr(fi+1,span);
    int frameCounter = atoi(tempString.c_str());
    //*output << frameCounter<<endl;

    string fileNameForSave = fileName;

    // save the name in the vector
    fnVector.push_back(fileName);

    fileName = maskImagePath + fileName;
    cout << "Reading file "<<fileName<<endl;
    Image* img = new Image(fileName.c_str());
    int width = img->columns(),height = img->rows();
    diagLength= static_cast<int> ( sqrt( (height*height) + (width*width) ) );

    //*output << "Diagonal length is "<<diagLength<<endl;
    //		Image* imgWithInfo;
    //		imgWithInfo = new Image(fileName.c_str());
    sprintf(buffer,"%ix%i",width,height);
    string imsize(buffer);
    imgSize = imsize;
    // residual image is initialized with black representing not visited.
    residual = new Image(buffer, "black");

    *output<<"reading file "<<fileName<<endl;

    tempFOV.clear();

    for (int x = 0; x<width; x++) {
      for (int y = 0; y<height; y++) {

        //*output<<"comes here"<<endl;
        shape.clear();
        findObj(img, x, y, shape, true, true);
        unsigned int s = shape.size();

        if ( s > 0 )
        {
          //		*output << "size of the object is: " << s <<endl;
          vector<double> eigenVal = covariantDecomposition(shape);
          {					
            //objCount++;

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

    delete img;
    delete residual;

    //		*output<<"Sorting the objects according to size"<<endl;
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
        //	*output << "Start as a single blob"<<endl;
        startOfAOneObject = fileCounter;
      }

    }

    FrameInfo tempFI(frameCounter, fOVector, currentFrameIsSingleBlob);
    fIVector.push_back(tempFI);
    FrameInfo currentFI = fIVector[fileCounter];

    // start processing the sequence if applicable
    if (sequenceSize > 1) {

      FrameInfo prevFI = fIVector[fileCounter-1];

      int seqCond = sequenceCondition(prevFI, currentFI);

      if (seqCond == STUCKING_TO_A_SINGLE_BLOB) {

        endOfATrackSequence = fileCounter-1;

        // save the index for printing the one object later on
        startOfAOneObject = fileCounter;

      } else if (seqCond == SEPARATING_FROM_SINGLE_BLOB) {

        startOfATrackSequence = fileCounter;

        // draw the single blob sequence
        endOfAOneObject = fileCounter - 1;
        *output << "Only one object StartIndex "<<startOfAOneObject<<" endIndex "<<endOfAOneObject<<" and seqSize "<<sequenceSize<<endl;
        // use the two variables (startOfAOneObject, endOfAOneObject) pair to draw the single-blob objects.
        // third parameter is used to indicate whether the current sequence is SINGLE_BLOB. So true is passed.
        // fourth parameter is used to indicate whether the current sequence is processed(to separate it from the actual single blob state).
        // since we are considering the sequence length >=  15 to be a single blob state, so pass false parameter.
        drawTheSequence(startOfAOneObject, endOfAOneObject,0, true, false);
        startOfAOneObject = -1;
        endOfAOneObject = -1;
        //startOfATrackSequence = 1;
        sequenceSize = 1;
      }


      if(seqCond == STUCKING_TO_A_SINGLE_BLOB) {
        *output << "StartIndex "<<startOfATrackSequence<<" endIndex "<<endOfATrackSequence<<" and seqSize "<<sequenceSize<<endl;	
        // if a sequence size is greater than 15 then it is processed. endOfATrackSequence - startOfATrackSequence == 15 when the size of the sequence is 16.
        // endOfATrackSequence - startOfATrackSequence > 15 when the size of the sequence is > 16.
        if ((endOfATrackSequence - startOfATrackSequence) >=15 ) {
          processASequence(startOfATrackSequence, endOfATrackSequence);
          *output << "Done processing"<<endl;
        } else {
          // use the two variables (startOfATrackSequence, endOfATrackSequence) pair to draw the unprocessed frames.
          // third parameter is used to indicate whether the current sequence is not actual SINGLE_BLOB. So false is passed.
          // fourth parameter is used to indicate whether the current sequence is processed(to separate it from the actual single blob state).
          // since we are considering the sequence length <  15 to be a single blob state, so pass false parameter.
          *output << "Sequence is only "<<(endOfATrackSequence-startOfATrackSequence+1)<<" images long so assumed as a single blob"<<endl;
          drawTheSequence(startOfATrackSequence, endOfATrackSequence, 0, false, true);

          // increase the unprocessed frame counter
          //totalUnprocessedFrame = totalUnprocessedFrame + (endOfATrackSequence - startOfATrackSequence + 1);
          foutSt<<"-------------------------------"<<endl;
          foutSt<<"Unprocessed size "<<(endOfATrackSequence-startOfATrackSequence+1)<<endl;
          foutSt<<"Total unprocessed size "<<totalUnprocessedFrame<<endl;
          foutSt<<"-------------------------------"<<endl;
        }

        initSequence();
        //*output << "Start of a single blob "<<startOfAOneObject<<" and end of a single blob "<<endOfAOneObject<<endl;

      }
      //*output << "Done for "<<fnVector[fileCounter-1]<<endl;
    }

    // increase the frame Counter
    fileCounter++;

    // increase the sequence size
    sequenceSize++;
  }

  //*output << "No more files "<<startOfAOneObject<<" "<<endOfAOneObject<<endl;
  inputFile.close();

  //open this after debug
  /**/
  if (startOfATrackSequence!=-1 && endOfATrackSequence == -1) {
    if ((sequenceSize-1) > 15 ) {

      *output << "Last sequence that does not stick to a single blob status startIndex "<<startOfATrackSequence<<" endIndex "<<(sequenceSize+startOfATrackSequence-2)<<endl;
      processASequence(startOfATrackSequence, (sequenceSize+startOfATrackSequence-2));
      *output << "Done processing"<<endl;

    } else {
      // this case was not handled earlier. It can happen that the flies are separated during the last sequences and less than 15 frames long.
      *output << "Sequence is only "<<(sequenceSize-1)<<" images long so assumed as a single blob"<<endl;
      drawTheSequence(startOfATrackSequence, (sequenceSize+startOfATrackSequence-2), 0, false, true);
      foutSt<<"-------------------------------"<<endl;
      foutSt<<"Unprocessed size "<<(sequenceSize-1)<<endl;
      foutSt<<"Total unprocessed size "<<totalUnprocessedFrame<<endl;
      foutSt<<"-------------------------------"<<endl;

    }
    initSequence();

  } else if (startOfAOneObject != -1 && endOfAOneObject == -1) {
    *output << "Last sequence that does not separate from one object state\n";
    drawTheSequence(startOfAOneObject, (sequenceSize+startOfAOneObject - 2), 0, true, false);
    endOfAOneObject = -1;
    startOfAOneObject = -1;
    sequenceSize = 1;
  }


  string cDDistFileName = finalOutputPath + outputFilePrefix + "_centroidDist.txt";
  string hDAngleDistFileName = finalOutputPath + outputFilePrefix + "_headDist.txt";
  string speedDistFileName = finalOutputPath + outputFilePrefix + "_speedDist.txt";

	//TODO: rewrite to just generate the percentages
	double centroidmean, centroidstd, headdirmean, headdirstd, speedmean, speedstd;
	getMeanStdDev(centroidDistanceMap, &centroidmean, &centroidstd);
	*output  << "centroid M :" <<  centroidmean << " S:" << centroidstd << endl;
	getMeanStdDev(headDirAngleMap, &headdirmean, &headdirstd);
	getMeanStdDev(speedMap, &speedmean, &speedstd);

  double centroid = writeHist(cDDistFileName.c_str(), centroidDistanceMap);
  double headDirAngle = writeHist(hDAngleDistFileName.c_str(), headDirAngleMap);
  double speed  = writeHist(speedDistFileName.c_str(), speedMap);

  // new calculation of percentage look at should consider only those frames where the flies are separated
  double percentageLookingAt = static_cast<double>(totalMaleLookingAtFemale+totalFemaleLookingAtMale)/static_cast<double>(totalSeparated);
  percentageLookingAt *= 100.0;

  double percentageSingleBlob = static_cast<double>(totalSingleBlob)/static_cast<double>(fileCounter);
  percentageSingleBlob *= 100.0;

  foutSt<<"Total number of together "<<totalSingleBlob<<endl;
  foutSt<<"Total number of male looking at "<<totalMaleLookingAtFemale<<endl;
  foutSt<<"Total number of female looking at "<<totalFemaleLookingAtMale<<endl;
  foutSt<<"Total number of looking at "<<(totalMaleLookingAtFemale+totalFemaleLookingAtMale)<<endl;
  foutSt<<"Total number of unprocessed frame "<<totalUnprocessedFrame<<endl;
  foutSt<<"Total number of frame where flies are separated (from the counter totalSeparated) "<<totalSeparated<<endl;
  foutSt<<"Total number of frame where flies are separated "<<(fileCounter-totalUnprocessedFrame-totalSingleBlob)<<endl;
  foutSt<<"Total number of frame "<<fileCounter<<endl;
  foutSt<<"Percentage of frame in looking at mode "<<percentageLookingAt<<endl;
  foutSt<<"Percentage of frame single blob "<<percentageSingleBlob<<endl;

	foutSt<<"looking at\ttogether\tCentroid M\tCentroid Dev\tHeadDir M\tHeadDir Dev\tSpeed M\tSpeed Dev"<<endl;
	foutSt<<percentageLookingAt << "\t";
	foutSt<<percentageSingleBlob<< "\t";
	foutSt<< centroidmean<<"\t";
	foutSt<< centroidstd<<"\t";
	foutSt<< headdirmean<<"\t";
	foutSt<< headdirstd<<"\t";
	foutSt<< speedmean <<"\t";
	foutSt<< speedstd <<"\t" << endl;

  foutSt.close();
  foutDebugCen.close();
  foutLPS.close();
  foutDebugSpeed.close();

  return 0;
}

