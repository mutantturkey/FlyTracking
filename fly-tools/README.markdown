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
