fly-tools
=========

These are the programs that do all the hard work!

FlyTracking
------------

The FlyTracking application does the bulk of the work. 

Usage:

	FlyTracking -i <inputFile.txt> -o <originalImagePath> -f <finalOutputPath> -m <maskImagePath> -O <outputFilePrefix>

FilterFlyMask
------------

FilterFlyMask runs some filtering operations on masks to ensure that the 
FlyTracking tool processes the information correctly.

Usage:

	FilterFlyMask -f <image-filename> -r <ratio> -m <mask-image> -o <outputFolderName>

filter-mask
-----------

filter-mask is an alternative fly filter that works much faster. It utilizes 
the OpenCV and CvBlob libraries.

The filter works simply, it counts the number of blobs and grabs the largest
two. Anything else is discarded and the two blobs are written to a new image.
        
Usage:

	filter-mask -i <input-file> -o <output-file> -r <ratio>

mask-generator
-------------

This tool creates binary masks from cropped video frames, which the need to be 
filtered by the FilterFlyMask tool.

The program works by doing 3 specific image manipulations to the input images.

1. subtract the background image from the input image
2. auto-level the image. This operation sets the lightest pixel to white and 
then normalizes the rest of the image accordingly. This step is important to 
ensure that the mask output is relatively similar when ligt conditions are 
changing during a sequence.
3. Threshold the image, anything over 30% is marked as white, the rest is black

Usage:

	mask-generator -b <derived-background> -i <input-list> -o <output-folder>

Todo: 

* automatically detect CPU count and apply threads accordingly 

derive-background
-----------------

This tool will generate a common background image of a set of video frames
(PNGs), based on the statistical mode of each pixel. 

The program works by iterating through each image, through each pixel, adding 
them to a histogram and then calculates each pixel's mode and spews out the 
resulting image.

Usage:

  derive-background -i <input-list> -o <output-filename> 
