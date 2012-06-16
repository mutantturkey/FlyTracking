#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#include <list>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <queue>

#include <ImageMagick/Magick++.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_eigen.h>

#include "FrameInfo.h"
#include "MyPair.h"

using namespace Magick;
using namespace std;
void findObj(Image* img, int x, int y, vector<pair<int,int> > & shape ,bool eightCon=true, bool colorLookingFor=true);
void eightConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color=true);
void findObjIterative(Image* img, int x, int y, vector<pair<int, int> > & shape, bool eightCon, double colorLookingFor);
void fourConnObjIterative(Image* img, int x, int y, vector<pair<int, int> > & obj, double colorLookingFor);
vector<double> covariantDecomposition(vector<pair<int,int> > & points);
pair<int,int> getCentroid(vector<pair<int,int> > & points);
bool isInterface(Image* orig, unsigned int x, unsigned int y);
void writeFrameImage(int fn, string imS);
int roundT(double v) {return int(v+0.5);}


const double PI = atan(1.0)*4.0;
const double FACTOR_EIGEN = 100;

Image* residual;
Image* imgForFilter;

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
    string usage = "Usage: FilterFlyMask -f <image filename> -r <ratio> -m <mask image> -o <outputFolderName>";
		string fileName;
		string inputMaskFileLocation;
		string outputFileLocation;
		double ratioSecondLargestToLargest;
		int c;
    opterr = 0;

    while ((c = getopt (argc, argv, "m:f:r:o:hv")) != -1)
        switch (c)
        {
        case 'f':
            fileName = optarg;
            break;
        case 'm':
            inputMaskFileLocation = optarg;
            break;
        case 'r':
            ratioSecondLargestToLargest = atof(optarg);
            break;
        case 'o':
            outputFileLocationi = optarg;
            break;
        case 'v':
            break;
        case 'h':
						cout << usage << endl;
        		exit(1);
						break;
				defaut:
						break;
				}
	
	if( fileName.empty() || inputMaskFileLocation.empty() || ratioSecondLargestToLargest == NULL || outputFileLocation.empty()) {
		cout << usage << endl;
		exit(1);
	}
  //MagickCore::SetMagickResourceLimit(MagickCore::MemoryResource, 1536);
  //MagickCore::SetMagickResourceLimit(MagickCore::MapResource, 2048);

  ratioSecondLargestToLargest = 1/ratioSecondLargestToLargest;

  char buffer[100];

  string savedFileName = fileName;
  string finalImageName = outputFileLocation + "final/"+ savedFileName;

    // get the input mask file
    fileName =  inputMaskFileLocation + fileName;

    Image* original = new Image(fileName.c_str());
    int width = original->columns();
    int height = original->rows();
    sprintf(buffer,"%ix%i",width,height);

    imgForFilter = new Image(buffer, "white");
    shape.clear();

    // find the black background from location (0,0)
    ColorMono topLeftColor = ColorMono(original->pixelColor(0,0));
    ColorMono topRightColor = ColorMono(original->pixelColor(width-1,0));
    ColorMono bottomRightColor = ColorMono(original->pixelColor(width-1,height-1));
    ColorMono bottomLeftColor = ColorMono(original->pixelColor(0,height-1));

    if (topLeftColor.mono() == false) {

      findObjIterative(original, 0, 0, shape, false, 0.0);

    } else if (topRightColor.mono() == false) {

      cout << "Top left is not black pixel for FLOOD FILLING so flood filling from top right\n";
      findObjIterative(original, width-1, 0, shape, false, 0.0);

    } else if (bottomRightColor.mono() == false) {

      cout << "Top left/Top right are not black pixel for FLOOD FILLING so flood filling from bottom right\n";
      findObjIterative(original, width-1, height-1, shape, false, 0.0);

    } else {

      cout << "Top left/top right/bottom right are not black pixel for FLOOD FILLING so flood filling from the bottomleft\n";
      findObjIterative(original, 0, height-1, shape, false, 0.0);

    }

    string inputFileName =  outputFileLocation + "temp/" + savedFileName;

    Image* final_image = imgForFilter;


    sprintf(buffer,"%ix%i",width,height);

    // residual image is initialized with black representing not visited.
    residual = new Image(buffer, "black");


    shapeVectors.clear();
    sizeNIndexVector.clear();

    // find the objects and sort according to size
    int objectCounter = 0;
    for (int x=0; x<width; x++) {
      for (int y=0; y<height; y++) {
        // find the white object using eight connected
        shape.clear();
        findObj(final_image, x, y, shape, true, true);
        int s = shape.size();

        if (s > 0) {
          shapeVectors.push_back(shape);
          pair<int, int> si(s, objectCounter);
          sizeNIndexVector.push_back(si);
          objectCounter++;
        }
      }
    }

    bubbleSort();

    // take the largest object
    double currentLargestSize = static_cast<double>(sizeNIndexVector[0].first);
    double secondLargest = 0;
    double ratio = 0;

    if (sizeNIndexVector.size() > 1) {

      secondLargest = static_cast<double>(sizeNIndexVector[1].first);
      ratio = secondLargest/currentLargestSize;

    }

    // find the largest to second largest ratio if it is less than the defined ratio then
    // the objects are single object

    int numberOfObjects = 0;

    if (sizeNIndexVector.size() == 1) {
      numberOfObjects = 1;
    }
    else if (ratio <= ratioSecondLargestToLargest ) {
      numberOfObjects = 1;
    } else {
      numberOfObjects = 2;
    }

    Image* imgFinal = new Image(buffer, "black");

    for (int n=0; n<numberOfObjects; n++) {

      int totalPoints = sizeNIndexVector[n].first;

      for (int i=0; i<totalPoints; i++) {

        imgFinal->pixelColor(shapeVectors[ sizeNIndexVector[n].second ][i].first, shapeVectors[ sizeNIndexVector[n].second ][i].second, "white");
      }

    }

    // write final image
    cout << finalImageName << " \r";
    imgFinal->write( finalImageName.c_str() );

{
//     // writing the single in red
//     if (numberOfObjects == 1) {
// 
//       Image* singleObjectFinal = new Image(buffer, "black");
// 
//       int totalPoints = sizeNIndexVector[0].first;
// 
//       cout << "Output the single object of size = "<<totalPoints<<endl;
// 
//       for (int i=0; i<totalPoints; i++) {
// 
//         singleObjectFinal->pixelColor(shapeVectors[ sizeNIndexVector[0].second ][i].first, shapeVectors[ sizeNIndexVector[0].second ][i].second, "red");
//       }
// 
//       //string singleImageName = "output/filtered/single/"+fileName;
//       string singleImageName = outputFileLocation + "single/"+ savedFileName;
// 
//       singleObjectFinal->write(singleImageName.c_str());
// 
//     }
// 
}

  return 0;
}

void findObjIterative(Image* img, int x, int y, vector<pair<int, int> > & shape, bool eightCon, double colorLookingFor) {

  assert(imgForFilter != NULL);
  fourConnObjIterative(img, x, y, shape, colorLookingFor);

}


void fourConnObjIterative(Image* img, int x, int y, vector<pair<int, int> > & obj, double colorLookingFor) {

  /*
  Flood-fill (node, target-color, replacement-color):
  1. Set Q to the empty queue.
  2. If the color of node is not equal to target-color, return.
  3. Add node to Q.
  4. For each element n of Q:
  5.  If the color of n is equal to target-color:
  6.   Set w and e equal to n.
  7.   Move w to the west until the color of the node to the west of w no longer matches target-color.
  8.   Move e to the east until the color of the node to the east of e no longer matches target-color.
  9.   Set the color of nodes between w and e to replacement-color.
  10.   For each node n between w and e:
  11.    If the color of the node to the north of n is target-color, add that node to Q.
  If the color of the node to the south of n is target-color, add that node to Q.
  12. Continue looping until Q is exhausted.
  13. Return.

  */


  queue< MyPair > Q;

  ColorRGB imgpixel = ColorRGB(img->pixelColor(x,y));

  if ( (imgpixel.red() != colorLookingFor) and (imgpixel.green() != colorLookingFor) and (imgpixel.blue() != colorLookingFor)) {

    cout << "Returning without floodfilling because the first pixel is not the colorLookingFor"<<endl;
    return;

  }

  Q.push( MyPair(x,y));

  int width = img->columns(),height = img->rows();

  while (Q.empty() != true) {

    MyPair n = Q.front();
    Q.pop();

    ColorRGB westColor;
    ColorRGB eastColor;
    ColorRGB nColor;
    MyPair i,j;

    nColor = ColorRGB(img->pixelColor(n.first, n.second));

    if ( (nColor.red() == colorLookingFor) and (nColor.green() == colorLookingFor) and (nColor.blue() == colorLookingFor)) {

      //cout << "Current pixel is of the black color ("<<n.first<<","<<n.second<<") and Q size is = "<<Q.size()<<endl;
      MyPair w, e;
      w = n;
      e = n;

      // move w to the west until the color of the node to the west of w no longer matches target-color.
      do {
        // move to west
        i = w;
        w.first = w.first - 1;
        // color at w
        if ( w.first >=0 )
          westColor = ColorRGB(img->pixelColor(w.first, w.second));
        else {
          //cout << "outside of the image boundary in x direction so break the while loop for west"<<endl;
          break;
        }


      } while ( (westColor.red() == colorLookingFor) and (westColor.green() == colorLookingFor) and (westColor.blue() == colorLookingFor) );



      // move e to the east until the color of the node to the east of e no longer matches target-color.
      do {

        j = e;
        // move to east
        e.first = e.first + 1;
        // color of e
        if ( e.first < width )
          eastColor = ColorRGB(img->pixelColor(e.first, e.second));
        else {
          //cout << "outside of the image boundary in x direction so break the while loop for east"<<endl;
          break;
        }

      } while ((eastColor.red() == colorLookingFor) and (eastColor.green() == colorLookingFor) and (eastColor.blue() == colorLookingFor));


      //cout << "Current pixel west to east span is from ("<<i.first<<","<<i.second<<") to "<<j.first<<","<<j.second<<")"<<endl;
      // Set the color of nodes between w and e to replacement-color
      while ( i.first <= j.first ) {

        // set the color to black which is the replacement color
        // for our algorithm it is the colorLookingFor
        imgForFilter->pixelColor(i.first, i.second, ColorRGB(colorLookingFor, colorLookingFor, colorLookingFor) );

        // change the color to green to indicate that it is visited
        img->pixelColor(i.first, i.second, ColorRGB(0.0, 1.0, 0.0));

        //cout << "Current pixel visited "<<i.first<<","<<i.second<<endl;

        //	If the color of the node to the north of n is target-color, add that node to Q.
        if ( i.second-1 >=0 ) {

          MyPair n(i.first, i.second-1);
          ColorRGB northColor = ColorRGB(img->pixelColor(n.first, n.second));
          if ((northColor.red() == colorLookingFor) and (northColor.green() == colorLookingFor) and (northColor.blue() == colorLookingFor)) {
            Q.push(n);
            //cout << "North pixel not visited so pushed "<<n.first<<","<<n.second<<endl;
          }// else {
          //cout << "North pixel visited so not pushed"<<endl;
          //}

        }


        //  If the color of the node to the south of n is target-color, add that node to Q.
        if (i.second+1 < height) {
          MyPair s(i.first, i.second+1);
          ColorRGB southColor = ColorRGB(img->pixelColor(s.first, s.second));
          //cout << "South color "<<southColor.red() << " "<< southColor.green() <<" "<< southColor.blue()<< endl;
          if ((southColor.red() == colorLookingFor) and (southColor.green() == colorLookingFor) and (southColor.blue() == colorLookingFor)) {
            Q.push(s);
            //cout<<"South pixel not visited so pushed "<<s.first<<","<<s.second<<endl;
          } //else {
          //cout<<"South pixel visited so not pushed"<<endl;
          //}

        }

        i.first = i.first + 1;

      }

      // next step of the main while loop
      //cout << "Processed "<<n.first<<","<<n.second<<endl;

    } //else {
    //cout << "Current pixel is not of the black color so just discarded ("<<n.first<<","<<n.second<<") and Q size is = "<<Q.size()<<endl;
    //}


  }

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

void fourConnObj(Image* img, int x, int y, vector<pair<int, int> > & obj, bool color)
{
  int width = img->columns(),height = img->rows();

  // boundary violation check
  if ( (x >= (width)) || (x < 0) || (y >= (height) ) || (y < 0) )
    return;

  // residualpixel.mono() == true implies it is visited. Otherwise not visited.
  ColorMono residualpixel = ColorMono(residual->pixelColor(x,y));
  // imgpixel.mono() == true implies it is an object pixel. Otherwise it is blank region pixel.
  ColorMono imgpixel = ColorMono(img->pixelColor(x,y));

  // If the current pixel is already visited then return
  if (residualpixel.mono() == true)
    return;

  // Else if current pixel is not visited and it is black, which means it is not an object pixel; so return
  else if (residualpixel.mono() == false && imgpixel.mono() != color)
    return;
  // If current pixel is not visited and its value is white, which means a new object is found.
  else if (residualpixel.mono() == false && imgpixel.mono() == color) {
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
  // imgpixel.mono() == true implies it is an object pixel. Otherwise it is blank region pixel.
  ColorMono imgpixel = ColorMono(img->pixelColor(x,y));

  // If the current pixel is already visited then return
  if (residualpixel.mono() == true)
    return;

  // Else if current pixel is not visited and it is black, which means it is not an object pixel; so return
  else if (residualpixel.mono() == false && imgpixel.mono() != color)
    return;
  // If current pixel is not visited and its value is white, which means a new object is found.
  else if (residualpixel.mono() == false && imgpixel.mono() == color) {
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
