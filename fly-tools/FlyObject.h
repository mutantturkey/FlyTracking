/*
 *  FlyObject.h
 *
 *  Created by Md. Alimoor Reza on 6/26/10.
 *  Copyright 2010 Drexel University. All rights reserved.
 *
 */
#include<iostream>
#include<vector>
#include<list>
#include<cmath>
using namespace std;

class FlyObject {
public:
	FlyObject(int area, pair<int, int> centroid, pair<double,double> majorAxisEV, pair<double,double> velocityV,bool headIsInDirectionMAEV, pair<double, double> head, double speed);
	FlyObject(const FlyObject &f);
	int getArea() const;
	pair<int, int> getCentroid() const;
	pair<double,double> getMajorAxisEV() const;
	pair<double,double> getVelocityV() const;
	bool getHeadIsInDirectionMAEV() const;
	pair<double,double> getHead() const;
	double getSpeed() const;
	void setArea(int area);
	void setCentroid(pair<int, int>);
	void setMajorAxisEV(pair<double,double>);
	void setVelocityV(pair<double,double>);
	void normalizeVelocity();
	void setHeadIsInDirectionMAEV(bool);
	void setHead(pair<double, double> head);
	void setSpeed(double speed);
	void output(ostream &out);
	

private:
	int area;
	pair<int, int> centroid; 
	pair<double,double> majorAxisEV;
	pair<double,double> velocityV;
	bool headIsInDirectionMAEV;
	pair<double, double> head;
	double speed;
};
