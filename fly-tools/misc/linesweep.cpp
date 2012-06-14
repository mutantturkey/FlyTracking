#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <cassert>
#include <cmath>
#include <Magick++.h>
#include "jzImage.h"

using namespace Magick;
using namespace std;

unsigned int jzImage::_h;
unsigned int jzImage::_w;
char* jzImage::data = NULL;

// colors in debugging
const int C = 10;

static map<unsigned int, long unsigned int> len;
bool inWhite;
bool startedWhite;
int x_init, y_init;

inline int roundT(double v) { return int(v+0.5); }
inline int dist(int x0, int y0, int x1, int y1) {return roundT(sqrt(pow(double(x1)-x0, 2.0)+pow(double(y1)-y0, 2.0)));}

inline void drawLine(int x0, int y0, int x1, int y1, jzImage& img);
inline void lookAt(int x, int y, jzImage& img);


void writeHist(const char* filename)
{
	map<unsigned int,long unsigned int>::iterator front = len.begin(),
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
	vector<long unsigned int> hist(last+1, 0);

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
	//len.clear();
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

void writeHistLog( const char* filename,map<unsigned int, double> len)
{
	map<unsigned int, double >::iterator front = len.begin(),
	back = len.end();
	back--;


	unsigned int first = front->first, last = back->first;
	/*if (cutoff != -1 && cutoff < int(last))
		last = cutoff;
		*/
	cout<< "Min: " << first << endl
		<< "Max: " << last << endl
		<< "Count: " << last-first << endl;
	//vector<unsigned int> hist(last-first, 0);
	vector<double> hist(last+1, 0);

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
	if (argc < 3)
	{
		cerr << "Usage: lineSweep <inputFile> <outputFile>" << endl;
		return -1;
	}

	int i, j;

	inWhite = false;
	len.clear();

	MagickCore::SetMagickResourceLimit(MagickCore::MemoryResource, 1536);
	MagickCore::SetMagickResourceLimit(MagickCore::MapResource, 4096);
	Image* img = new Image(argv[1]);

	int width = img->columns(),
		height = img->rows();

	jzImage &myImg = jzImage::Instance(img);

	delete img;

	/*
	// doesn't take pixel(0,0) and also takes pixel(width-1, height-1) twice
	//map<pair<pair<int,int>, pair<int,int> >, bool> hitMap;
	const int boundSize = 2*(width+height-2);
	static pair<int, int>* boundary = new pair<int,int>[boundSize];

	for (i = 0; i<width-1; i++)
	{
		boundary[i].first = i+1;
		boundary[i].second = 0;
		boundary[width+height+i-2].first = i+1;
		boundary[width+height+i-2].second = height-1;
	}

	for (i = 0; i<height-1; i++)
	{
		boundary[width+i-1].first = width-1;
		boundary[width+i-1].second = i+1;
		boundary[height+2*width+i-3].first = 0;
		boundary[height+2*width+i-3].second = i+1;
	}

	for (i = 0; i<boundSize-1; i++)
	{
		cerr << "At: " << i << " of " << boundSize << endl;
		// not considering boundary-1 th pixel ?????
		for (j = i+1; j<boundSize-1; j++)
			if ( !myImg.isOnSameBoundaryAs(
					myImg.indexOf(boundary[i].first,boundary[i].second),
					myImg.indexOf(boundary[j].first,boundary[j].second)) )
				drawLine(boundary[i].first,boundary[i].second,boundary[j].first,boundary[j].second,myImg);
	}

	*/

	//map<pair<pair<int,int>, pair<int,int> >, bool> hitMap;
	const int boundSize = 2*(width+height-2);
	static pair<int, int>* boundary = new pair<int,int>[boundSize];

	for (i = 0; i<=width-1; i++)
	{
		boundary[i].first = i;
		boundary[i].second = 0;
		boundary[width+(height-2)+i].first = i;
		boundary[width+(height-2)+i].second = height-1;
	}

	for (i = 0; i<height-2; i++)
	{
		boundary[width+i].first = width-1;
		boundary[width+i].second = i+1;
		boundary[width+(height-2)+width+i].first = 0;
		boundary[width+(height-2)+width+i].second = i+1;
	}


	for (i = 0; i<boundSize; i++)
		cerr<<"boundr["<<i<<"] == "<<boundary[i].first<<","<<boundary[i].second<<endl;


	//drawLine(7,1,4,7,myImg);
	// m >= 1
	//drawLine(3,2,5,5,myImg);
	// 0<=m<1
	//drawLine(5,5,1,4,myImg);
	// -1< m < 0
	//drawLine(7,0,2,3,myImg);
	// m<-1
	//drawLine(1,7,3,2,myImg);
	// dx = 0
	//drawLine(5,0,5,5,myImg);
	// dy = 0
	//drawLine(0,3,7,3,myImg);
	// multiple length
	//drawLine(1,7,7,1,myImg);



	for (i = 0; i<boundSize-1; i++)
	{
		//cerr << "At: " << i << " of " << boundSize << endl;
		for (j = i+1; j<boundSize; j++)
			if ( !myImg.isOnSameBoundaryAs(
					myImg.indexOf(boundary[i].first,boundary[i].second),
					myImg.indexOf(boundary[j].first,boundary[j].second)) ) {
				drawLine(boundary[i].first,boundary[i].second,boundary[j].first,boundary[j].second,myImg);
			} /*else
				cerr << i <<" and "<< j << " are on the same boundary"<<endl;
				*/
	}

	writeHist(argv[2]);

	/*ofstream newFile("output.txt");
	newFile<<"size == " << len.size()<<endl;
	for ( unsigned int i=0; i<len.size(); i++) {
		newFile<<"len["<<i<<"] == " <<len[i]<<endl;

	}
	newFile.close();
	*/
	// compute the logarithmic distribution.

	map< unsigned int, double> lineSweepValLog;
	map<unsigned int,long unsigned int>::iterator front = len.begin(), back = len.end();

	for( front; front != back; front++ ) {
		double logvalue = log(front->second);
		lineSweepValLog[front->first] = logvalue;
	}

	string fileNameLog(argv[2]);
	fileNameLog = fileNameLog + "_log_hist.txt";
	writeHistLog(fileNameLog.c_str(), lineSweepValLog);


	delete[] boundary;

	return 0;
}

/*
void lookAt(int x, int y, jzImage& img)
{
	if (img.pixelColor(x,y) == 1)
		// if it was the first white pixel then
		if (!inWhite)
		{
			inWhite = true;
			x_init = x;
			y_init = y;
		}

	if (img.pixelColor(x,y) == 0)
		if (inWhite)
		{
			int val = roundT(dist(x_init, y_init, x, y));
			len[val]++;
			inWhite = false;
		}
}
*/

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
		/*else {

		}
		*/
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

/*
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
}
*/

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
}
