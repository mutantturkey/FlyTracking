fly-tools
=========

FlyTracking
------------

The FlyTracking application does the bulk of the work.

Usage:

	FlyTracking -i <inputFile.txt> -o <originalImagePath> -f <finalOutputPath> -m <maskImagePath> -O <outputFilePrefix>

FilterFlyMask
------------

FilterFlyMask runs some filtering operations on masks to ensure that the FlyTracking tool processes the information correctly.

Usage:

	 FilterFlyMask -f <image filename> -r <ratio> -m <mask image> -o <outputFolderName>

mask-generator
-------------

This tool creates masks from cropped video frames, which the need to be filtered by the FilterFlyMask tool.

Usage:

	mask-generator -b <derived-background> -i <input-list> -o <output-folder>

Todo: 

* automatically detect CPU count and apply threads accordingly 

derive-background
-----------------

This tool will generate a common background image of a set of video frames (PNGs), based on the statistical mode of each pixel. 

Warning: 

  This will eat a huge chunk of memory. with 2,500 images at 173x174 pixels, it took almost 3Gb of ram.

Usage:

  derive-background -i <input-list> -s <sample-file> -o <output-filename> 

Todo:

* reduce memory - zlib? less pointers?

