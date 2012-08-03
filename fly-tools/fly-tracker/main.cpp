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

  bool verbose = false;

  string usage = "fly-tracker -i <input-list> -o <output-folder> -t <type> -n <number>";

  string inputFileName;
  string outputFolderName;

  int setNumber;
  bool setNumberSet;
  string setType;

  vector< pair<CvLabel, CvBlob*> > blobList;

  while ((c = getopt (argc, argv, "i:o:t:n:hv")) != -1)
    switch (c) {
      case 'i':
        inputFileName = optarg;
        break;
      case 'o':
        outputFolderName = optarg;
        break;
      case 'v':
        verbose = true;
        break;
      case 't':
        setType = optarg;
        break;
      case 'n':
        setNumberSet = true;
        setNumber = atoi(optarg);
        break;
      case 'h':
        cout << usage << endl;
        exit(EXIT_SUCCESS);
        break;
      default:
        break;
    }

  if ( inputFileName.empty() || outputFolderName.empty() || !setNumberSet || setType.empty() ) {
    cerr << usage << endl;
    exit(EXIT_FAILURE);
  }	

  (verbose) ? output = &cout : output = &nullLog; 

  *output << "Verbose logging enabled" << endl;

  // read input file

  ifstream inputFile(inputFileName.c_str());

  if (inputFile.fail() ) {
    cerr << "cannot open the input file that contains name of the input images\n";
    exit(EXIT_FAILURE);
  }

  string fileName;
  long int fileNumber = 0;
  long int totalFrames = 0;
  long int togetherFrames = 0;

  bool togetherState = false;

  while (inputFile>>fileName) {

    totalFrames += 1;

    IplImage *inputImg, *outputImg, *labelImg;
    inputImg = cvLoadImage(fileName.c_str(), CV_LOAD_IMAGE_GRAYSCALE);

    CvBlobs blobs;

    vector< pair<CvLabel, CvBlob*> > blobList;

    // label blobs
    labelImg = cvCreateImage(cvGetSize(inputImg),IPL_DEPTH_LABEL,1);

    cvLabel(inputImg, labelImg, blobs);

    // copy and sort blobs
    copy(blobs.begin(), blobs.end(), back_inserter(blobList));
    sort(blobList.begin(), blobList.end(), cmpArea);

    if (blobList.size() == 1) {
      togetherState = true;
      togetherFrames += 1;
    } else {  
      togetherState = false;
    }

    // List detected blob in each file
    *output << "File: " << fileName << endl;
    *output << "\t Together:" << togetherState << endl;
    for (int i=0; i<(int)blobList.size(); i++) {
    *output << "\t Blob #" << blobList[i].first << " -> " << (*blobList[i].second) << endl;
    }

    // Release all the memory
    cvReleaseImage(&labelImg);
    cvReleaseImage(&inputImg);
    cvReleaseBlobs(blobs);

  }

    cout << "Final Information" << endl;
    cout << "\t Total Number of frames in video: " << totalFrames << endl; 
    cout << "\t Number of frames where there flies are together: " << togetherFrames << endl; 
    cout << "\t Percent of video where there flies are together: " << (long double)togetherFrames / totalFrames << endl; 
  return 0;
}
