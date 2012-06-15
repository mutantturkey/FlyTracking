CC = g++
FLAGS = -O2 -s $(shell pkg-config --cflags --libs ImageMagick++ gsl)


all: FilterFlyMask FlyTrackingMain

FlyTrackingMain:
	$(CC) FlyTrackingMain.cpp FlyObject.cpp FrameInfo.cpp -o FlyTracking $(FLAGS)

FilterFlyMask:
	$(CC) FilterFlyMask.cpp FlyObject.cpp FrameInfo.cpp -o FilterFlyMask $(FLAGS)

clean:
	rm -vf FilterFlyMask FlyTracking