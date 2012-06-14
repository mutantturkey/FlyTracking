/*
 *  FrameInfo.h
 *  
 *
 *  Created by Md. Alimoor Reza on 6/26/10.
 *  Copyright 2010 Drexel University. All rights reserved.
 *
 */

#include<iostream>
#include<vector>
#include<fstream>
#include "FlyObject.h"
using namespace std;

class FrameInfo {
public:
	FrameInfo(int frameNo, vector<FlyObject > fOVector, bool isSingleBlob);
	FrameInfo(const FrameInfo &f);
	int getFrameNo() const;
	bool getIsSingleBlob() const;
	vector<FlyObject > getFOVector() const;

	void setFrameNo(int fn);
	void setIsSingleBlob(bool isSingleBlob);
	void setFOVector(vector<FlyObject > fov);
	void swapTheFlyObject();
	void output(ostream &out);
	
	
private:
	bool isSingleBlob;
	int frameNo;
	vector<FlyObject > fOVector;
};
