#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#define round(x)((int)((x)+0.5))
#define array(i,j,k,l) (array[height*width*i + width*j + k + l])

int nImages;
unsigned long height = 0;
unsigned long width = 0;

int findmax(long *p, int n) {
  int mx = 0, v = p[0];
  for (int i = 1; i < n; i++) {
    if (p[i] > v) {
      v = p[i];
      mx = i;
    }
  }
  return mx;
}

using namespace std;
using namespace cv;

int main(int argc, char **argv ) {

  string usage = "derive-background -i <input-list> -o <output-filename>";
  string output_file;
  string inputFileName;
  int c;

  while ((c = getopt (argc, argv, "i:o:h")) != -1)
    switch (c) {
      case 'i':
        inputFileName = optarg;
        break;
      case 'o':
        output_file = optarg;
        break;
      case 'h':
        cout << usage << endl;
        exit(EXIT_FAILURE);
        break;
      default:
        break;
    }

  if( inputFileName.empty() || output_file.empty() ) {
    cout << usage << endl;
    exit(EXIT_FAILURE);
  }

  // Okay Declare all our working variables here.

  IplImage *first_image = NULL;
  IplImage *output_image = NULL;

  int i,j, k;
  int height = 0;
  int width = 0;

  string filename;

  // open first image in input_file to deterimine the height and width
  ifstream input_file(inputFileName.c_str());

  if (input_file.fail() ) {
    cerr << "cannot open the input file that contains name of the input images\n";
    exit(EXIT_FAILURE);
  }


  // Read first line for our first image and rewind
  input_file>>filename; 

  first_image = cvLoadImage(filename.c_str(), CV_LOAD_IMAGE_UNCHANGED);

  if(!first_image) {
    cerr << "couldn't read first image" << endl;
    exit(1);
  } 

  height = first_image->height;
  width = first_image->width;

  cvReleaseImage(&first_image);

  if (height == 0 || width == 0) {
    cerr << "height or width = 0!" << endl;
    exit(1);
  }

  cout << "height: " << height << " width: " << width << endl;

  // count number of images we have in the file

  input_file.clear();
  input_file.seekg(0);
  while(input_file>>filename) {
    nImages++;
  }

  cout << "number of images: " << nImages << endl;

  // initialize the storage arrays
  long *array = (long *)malloc(height*width*256*3*sizeof(long));
  if(array == NULL) {
    cerr << "could not allocate the proper memory, sorry!" << endl;
    exit(EXIT_FAILURE);
  }

  input_file.clear();
  input_file.seekg(0);

  while (input_file>>filename) {

    IplImage *input_image = NULL;
    input_image = cvLoadImage(filename.c_str(), CV_LOAD_IMAGE_UNCHANGED);

    cout << "reading: " << filename << endl;

    for (i=0; i<height; i++) {
      for (j=0; j<width; j++) {
        CvScalar s;
        s = cvGet2D(input_image, i, j);

        int r = s.val[2];
        int g = s.val[1];
        int b = s.val[0];

        array(r,i,j,0) += 1;
        array(g,i,j,1) += 1;
        array(b,i,j,2) += 1; 
      }
    }
    cvReleaseImage(&input_image);
  }

  // calculate histograms
  output_image = cvCreateImage(cvSize(width,height),IPL_DEPTH_32F,3);

  for (j = 0; j < height; j++) {
    for (k = 0; k < width; k++) {

      CvScalar s;

      long red_histogram[255] = { 0 };
      long blue_histogram[255] = { 0 };
      long green_histogram[255] = { 0 };

      for (i = 0; i < 256; i++) {
        red_histogram[i] += array(i,j,k,0);
        green_histogram[i] += array(i,j,k,1);
        blue_histogram[i] += array(i,j,k,2);
      }

      int red_val = findmax(red_histogram, 255);
      int blue_val = findmax(blue_histogram, 255);
      int green_val = findmax(green_histogram, 255);

      s.val[2] = red_val;
      s.val[1] = blue_val; 
      s.val[0] = green_val; 

      cvSet2D(output_image, j, k, s);
    }
  }

  if(!cvSaveImage(output_file.c_str(), output_image)) {
    cerr << "Could not save "<< output_file << endl;
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
