/*
 *  FrameInfo.cpp
 *  
 *
 *  Created by Md. Alimoor Reza on 6/26/10.
 *  Copyright 2010 Drexel University. All rights reserved.
 *
 */

#include "FrameInfo.h"

FrameInfo::FrameInfo(int frameNo, vector<FlyObject > fOVector, bool isSingleBlob) {
	this->frameNo = frameNo;
	this->fOVector = fOVector;
	this->isSingleBlob = isSingleBlob;

}
FrameInfo::FrameInfo(const FrameInfo &f) {
	this->frameNo = f.getFrameNo();
	this->fOVector = f.getFOVector();
	this->isSingleBlob = f.getIsSingleBlob();
	
}
int FrameInfo::getFrameNo() const {
	return frameNo;
}
bool FrameInfo::getIsSingleBlob() const {
	return isSingleBlob;
}
vector<FlyObject > FrameInfo::getFOVector() const{
	return fOVector;
}

void FrameInfo::setFrameNo(int fn) {
	this->frameNo = fn;
}
void FrameInfo::setIsSingleBlob(bool isSingleBlob)  {
	this->isSingleBlob = isSingleBlob;
}
void FrameInfo::setFOVector(vector<FlyObject > fov)  {
	this->fOVector = fov;

//	cout << "setting fov \n";
//	FlyObject a = fOVector[0];
//	pair<double,double> av = a.getVelocityV();
//	cout << "velocity "<<av.first<<","<<av.second<<endl;
//
//	FlyObject b = fOVector[1];
//	pair<double,double> bv = b.getVelocityV();
//	cout << "velocity "<<bv.first<<","<<bv.second<<endl;
	
	
	
}
void FrameInfo ::swapTheFlyObject() {
	if (fOVector.size() > 1) {
		cout << "swapping\n";
		FlyObject a = fOVector[0];
		fOVector[0] = fOVector[1];
		fOVector[1] = a;
	}
}
void FrameInfo::output(ostream &out) {
	out<<"FrameNo			: "<<frameNo<<endl;
	out<<"IsSingleBlob		: "<<isSingleBlob<<endl;
//	out<<"fOVector size "<<fOVector.size()<<endl;
	for (int i=0; i<fOVector.size(); i++) {
		FlyObject a = fOVector[i];
//		out<<"FlyObject "<<i<<endl;
		a.output(out);
	}
}
