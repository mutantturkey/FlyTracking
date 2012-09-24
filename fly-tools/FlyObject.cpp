/*
 *  FlyObject.cpp
 *  
 *
 *  Created by Md. Alimoor Reza on 6/26/10.
 *  Copyright 2010 Drexel University. All rights reserved.
 *
 */

#include "FlyObject.h"

FlyObject::FlyObject(int area, pair<int, int> centroid, pair<double,double> majorAxisEV, pair<double,double> velocityV,bool headIsInDirectionMAEV, pair<double,double> head, double speed) {
	this->area = area;
	this->centroid = centroid;
	this->majorAxisEV = majorAxisEV;
	this->velocityV = velocityV;
	this->headIsInDirectionMAEV = headIsInDirectionMAEV;
	this->head = head;
	this->speed = speed;
}

FlyObject::FlyObject(const FlyObject &f){
	this->area = f.getArea();
	this->centroid=f.getCentroid();
	this->majorAxisEV =f.getMajorAxisEV();
	this->velocityV = f.getVelocityV();
	this->headIsInDirectionMAEV = f.getHeadIsInDirectionMAEV();
	this->head = f.getHead();
	this->speed = f.getSpeed();
}

int FlyObject::getArea() const {
	return area;
}

pair<int, int> FlyObject::getCentroid() const {
	return this->centroid;
}

pair<double,double> FlyObject::getMajorAxisEV() const {
	return this->majorAxisEV;
}

pair<double,double> FlyObject::getVelocityV() const {
	return this->velocityV;
}
bool FlyObject::getHeadIsInDirectionMAEV() const {
	return this->headIsInDirectionMAEV;
}
pair<double,double> FlyObject::getHead() const {
	return this->head;
}
double FlyObject::getSpeed() const {
	return this->speed;
}
void FlyObject::setArea(int area) {
	this->area = area;	
}

void FlyObject::setCentroid(pair<int, int> centroid) {
	this->centroid = centroid;
}

void FlyObject::setMajorAxisEV(pair<double,double> majorAxisEV) {
	this->majorAxisEV = majorAxisEV;
}

void FlyObject::setVelocityV(pair<double,double> velocityV) {
	this->velocityV = velocityV;
}
void FlyObject::setHead(pair<double, double> head) {
	this->head = head;
	cout << "new head is set"<<endl;

}
void FlyObject::setSpeed(double speed) {
	this->speed = speed;
}

void FlyObject::normalizeVelocity() {
	double temp = velocityV.first*velocityV.first + velocityV.second*velocityV.second;
	temp = sqrt(temp);
	cout << "sum = "<<temp<<endl;
	cout << "unnormalized velocity "<<velocityV.first<<","<<velocityV.second<<endl;
	if (temp != 0) {
		velocityV.first = velocityV.first/temp;
		velocityV.second = velocityV.second/temp;
		cout << "unit velocity   "<<velocityV.first<<","<<velocityV.second<<endl;
	} else {
		cout <<"velocity zero"<<endl;
	}

}
void FlyObject::setHeadIsInDirectionMAEV(bool headIsInDirectionMAEV) {
	this->headIsInDirectionMAEV = headIsInDirectionMAEV;
	cout << "head flag set to "<<this->headIsInDirectionMAEV<<endl;
}


void FlyObject::output(ostream &out) {
	out<<"Area			: "<<area<<endl;
	out<<"Centroid		: ("<<centroid.first<<","<<centroid.second<<")"<<endl;
	out<<"MajorAxisEV	: ("<<majorAxisEV.first<<","<<majorAxisEV.second<<")"<<endl;
	out<<"VelocityV     : ("<<velocityV.first<<","<<velocityV.second<<")"<<endl;
	out<<"HeadIsInDir   : "<<this->headIsInDirectionMAEV<<endl;
	out<<"Head          : ("<<this->head.first<<","<<head.second<<endl;
	out<<"Speed         : "<<speed<<endl;
}
