#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <cvblob.h>

using namespace std;
using namespace cvb;

bool cmpArea(const pair<CvLabel, CvBlob*>  &p1, const pair<CvLabel, CvBlob*> &p2)
{
  return p1.second->area < p2.second->area;
}

int main(int argc, char* argv[]) {

  int c;
  int drawNumber = 2;

  double ratio;

  unsigned int result;

  bool ratioSet;

  string usage = "filter-mask -i <input-file> -o <output-file> -r <ratio>";
  string inputFileName;
  string outputFileName;

  CvBlobs blobs;
  CvBlobs largeBlobs;

  IplImage *img;
  IplImage *imgOut;
  IplImage *grey;
  IplImage *labelImg;

  vector< pair<CvLabel, CvBlob*> > blobList;

  while ((c = getopt (argc, argv, "i:o:r:h")) != -1)
    switch (c) {
      case 'i':
        inputFileName = optarg;
        break;
      case 'o':
        outputFileName = optarg;
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

  img = cvLoadImage(inputFileName.c_str(), 1);
  imgOut = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1); cvZero(imgOut);

  grey = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);

  cvCvtColor(img, grey, CV_BGR2GRAY);

  labelImg = cvCreateImage(cvGetSize(grey),IPL_DEPTH_LABEL,1);
  result = cvLabel(grey, labelImg, blobs);

  // copy and sort blobs
  copy(blobs.begin(), blobs.end(), back_inserter(blobList));
  sort(blobList.begin(), blobList.end(), cmpArea);

  // if no blobs.
  if(blobList.size() == 0) { 
    cerr << "No blobs found." << endl;
    cvSaveImage(outputFileName.c_str(), imgOut);
    exit(1);
  } 

  // if only one blob is detected.
  if(blobList.size() == 1 ) {
    cout << "Only one blob found!" << endl;
    drawNumber = 1;
  }

  // if the ratio of the of the two blobs is smaller than the input ratio.
  if( ((double)blobList[blobList.size()-drawNumber].second->area / (double)blobList[blobList.size()-1].second->area) < (1/ratio) ) {
    cout << "the second largest blob is smaller than the ratio. only drawing largest blob!" << endl;
    drawNumber = 1; 
  }

  for (int i=blobList.size()-drawNumber; i<blobList.size(); i++) {
    largeBlobs.insert( CvLabelBlob(blobList[i].first, blobList[i].second) );
    cout << "Blob #" << blobList[i].first << " -> " << (*blobList[i].second) << endl;
  }

  // draw the selected blobsto imgOut and write the image.
  cvFilterLabels(labelImg, imgOut, largeBlobs);
  cvSaveImage(outputFileName.c_str(), imgOut);

  // Release all the memory
  cvReleaseImage(&imgOut);
  cvReleaseImage(&grey);
  cvReleaseImage(&labelImg);
  cvReleaseImage(&img);
  cvReleaseBlobs(blobs);

  return 0;
}
