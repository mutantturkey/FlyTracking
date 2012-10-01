CPPC = g++
CC = gcc
CFLAGS= -O2 -Wall -s -Wextra -mtune=native
CPPFLAGS= -O2 -Wall -s -Wextra -mtune=native
MAGICKCPPFLAGS = $(shell pkg-config --cflags --libs ImageMagick++ gsl)
MAGICKCFLAGS = $(shell pkg-config --cflags --libs MagickWand )
CVBLOBCPPFLAGS = $(shell pkg-config --cflags --libs opencv cvblob )
CVCPPFLAGS = $(shell pkg-config --cflags --libs opencv)
all: FilterFlyMask FlyTrackingMain mask-generator derive-background standard-deviation filter-mask

derive-background:
	$(CC) background/main.cpp -o derive-background $(CPPFLAGS) $(CVCPPFLAGS)
filter-mask:
	$(CPPC) filter/main.cpp -o filter-mask $(CPPFLAGS) $(CVBLOBCPPFLAGS) $(CVCPPFLAGS)
mask-generator:
	$(CC) mask/main.c mask/thpool.c -I./mask/ -o mask-generator $(CFLAGS) $(MAGICKCFLAGS) -pthread
standard-deviation:
	$(CPPC) std-deviation/StandardDeviation.cpp -o standard-deviation $(CPPFLAGS)
FlyTrackingMain:
	$(CPPC) FlyTrackingMain.cpp FlyObject.cpp FrameInfo.cpp -o FlyTracking $(CPPFLAGS) $(MAGICKCPPFLAGS)
FilterFlyMask:
	$(CPPC) FilterFlyMask.cpp FlyObject.cpp FrameInfo.cpp -o FilterFlyMask $(CPPFLAGS) $(MAGICKCPPFLAGS)

clean:
	rm -vf FilterFlyMask FlyTracking mask-generator derive-background standard-deviation filter-mask
