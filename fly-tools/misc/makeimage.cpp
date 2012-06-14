#include <Magick++.h>
#include <iostream>
#include<fstream>
#include<string>
using namespace std;
using namespace Magick;

int main(int argc,char **argv)

{
	// Construct the image object. Seperating image construction from the
	// the read operation ensures that a failure to read the image file
	// doesn't render the image object useless.
	
	Image* img;
	char buffer[100];
	sprintf(buffer,"%ix%i",7,7);
	
	// residual image is initialized with black representing not visited.
	//residual = new Image(buffer, "black");
	
	img = new Image(buffer, "white");
	
	for (int j=0; j<=3;j++) {
		for (int i=0; i<=3-j; i++) {
			img->pixelColor(i,j, "black");
			img->pixelColor(6-i,j, "black");
		}
	}

	int k;
	for (int j=4; j<=6;j++) {
		for (int i=0; i<=j-3; i++) {
			img->pixelColor(i,j, "black");
			img->pixelColor(6-i,j, "black");
		}
	}
	
	
	//img->pixelColor(0,3, "red");

	
	string namei = "7x7.png";
	img->write(namei.c_str());
	
	delete img;
		    
	return 0;
    
}

