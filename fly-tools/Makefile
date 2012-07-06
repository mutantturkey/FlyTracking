CPPC = g++
CC = gcc
CFLAGS= -O2 -Wall -s 
CPPFLAGS= -O2 -Wall -s 
MAGICKCPPFLAGS = $(shell pkg-config --cflags --libs ImageMagick++ gsl)
MAGICKCFLAGS = $(shell pkg-config --cflags --libs MagickWand )

all: FilterFlyMask FlyTrackingMain mask-generator derive-background standard-deviation

derive-background:
	$(CC) background/main.c -std=c99 -o derive-background $(CFLAGS) $(MAGICKCFLAGS)
mask-generator:
	$(CC) mask/main.c mask/thpool.c -I./mask/ -o mask-generator $(CFLAGS) $(MAGICKCFLAGS) -pthread
standard-deviation:
	$(CPPC) standard-deviation/StandardDeviation.cpp -o standard-deviation $(CPPFLAGS)
FlyTrackingMain:
	$(CPPC) FlyTrackingMain.cpp FlyObject.cpp FrameInfo.cpp -o FlyTracking $(CPPFLAGS) $(MAGICKCPPFLAGS)
FilterFlyMask:
	$(CPPC) FilterFlyMask.cpp FlyObject.cpp FrameInfo.cpp -o FilterFlyMask $(CPPFLAGS) $(MAGICKCPPFLAGS)

clean:
	rm -vf FilterFlyMask FlyTracking mask-generator derive-background standard-deviation
