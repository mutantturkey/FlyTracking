CPPC = g++
CC = gcc
CPPFLAGS = -O2 -Wall -s $(shell pkg-config --cflags --libs ImageMagick++ gsl)
CFLAGS = -O2 -Wall -s $(shell pkg-config --cflags --libs MagickWand )

all: FilterFlyMask FlyTrackingMain mask-generator derive-background

derive-background:
	$(CC) background/main.c -std=c99 -o derive-background $(CFLAGS)
mask-generator:
	$(CC) mask/main.c mask/thpool.c -I./mask/ -o mask-generator $(CFLAGS) -pthread
FlyTrackingMain:
	$(CPPC) FlyTrackingMain.cpp FlyObject.cpp FrameInfo.cpp -o FlyTracking $(CPPFLAGS)

FilterFlyMask:
	$(CPPC) FilterFlyMask.cpp FlyObject.cpp FrameInfo.cpp -o FilterFlyMask $(CPPFLAGS)

clean:
	rm -vf FilterFlyMask FlyTracking mask-generator
