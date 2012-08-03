#include <iostream>
#include <sstream>
#include <fstream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <cvblob.h>

using namespace std;
using namespace cvb;

ofstream nullLog;
ostream* output;

bool cmpArea(const pair<CvLabel, CvBlob*>  &p1, const pair<CvLabel, CvBlob*> &p2)
{
  return p1.second->area < p2.second->area;
}

int main(int argc, char* argv[]) {

  int c;
  int drawNumber = 2;

  double ratio = 0;

  bool verbose = false;
  bool ratioSet = false;

  string usage = "filter-mask -i <input-file> -o <output-file> -r <ratio>";

  string inputFileName;
  string outputFileName;

  CvBlobs blobs;
  CvBlobs largeBlobs;

  IplImage *inputImg;
  IplImage *outputImg;
  IplImage *labelImg;

  vector< pair<CvLabel, CvBlob*> > blobList;

  while ((c = getopt (argc, argv, "i:o:r:hv")) != -1)
    switch (c) {
      case 'i':
        inputFileName = optarg;
        break;
      case 'o':
        outputFileName = optarg;
        break;
      case 'v':
        verbose = true;
        break;
      case 'r':
        ratioSet = true;
        ratio = atoi(optarg);
        break;
      case 'h':
        cout << usage << endl;
        exit(0);
        break;
      default:
        break;
    }

  if ( inputFileName.empty() || outputFileName.empty() || !ratioSet ) {
    cerr << usage << endl;
    exit(1);
  }	

  if(verbose) {
    output = &cout;
  }	else {
    output = &nullLog; 
  }

  *output << "Verbose logging enabled" << endl;

  // read input file
  inputImg = cvLoadImage(inputFileName.c_str(), CV_LOAD_IMAGE_GRAYSCALE);

  outputImg = cvCreateImage(cvGetSize(inputImg), IPL_DEPTH_8U, 1); cvZero(outputImg);

  // label blobs
  labelImg = cvCreateImage(cvGetSize(inputImg),IPL_DEPTH_LABEL,1);
  cvLabel(inputImg, labelImg, blobs);

  // copy and sort blobs
  copy(blobs.begin(), blobs.end(), back_inserter(blobList));
  sort(blobList.begin(), blobList.end(), cmpArea);

  // if no blobs.
  if(blobList.size() == 0) { 
    cerr << "No blobs found" << endl;
    cvSaveImage(outputFileName.c_str(), outputImg);
    exit(1);
  } 

  // if only one blob is detected.
  if(blobList.size() == 1 ) {
    *output << "Only one blob found" << endl;
    drawNumber = 1;
  }

  // if the ratio of the of the two blobs is smaller than the input ratio.
  if( ((double)blobList[blobList.size()-drawNumber].second->area / (double)blobList[blobList.size()-1].second->area) < (1/ratio) ) {
    *output << "the second largest blob is smaller than the ratio. only drawing largest blob" << endl;
    drawNumber = 1; 
  }

  for (int i=blobList.size()-drawNumber; i<(int)blobList.size(); i++) {
    largeBlobs.insert( CvLabelBlob(blobList[i].first, blobList[i].second) );
    *output << "Blob #" << blobList[i].first << " -> " << (*blobList[i].second) << endl;
  }

  // draw the selected blobsto outputImg and write the image.
  cvFilterLabels(labelImg, outputImg, largeBlobs);
  cvSaveImage(outputFileName.c_str(), outputImg);

  // Release all the memory
  cvReleaseImage(&outputImg);
  cvReleaseImage(&inputImg);
  cvReleaseImage(&labelImg);
  cvReleaseImage(&inputImg);
  cvReleaseBlobs(blobs);

  return 0;
}
