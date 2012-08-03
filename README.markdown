Automated Categorization of Drosophila Learning and Memory Behaviors using Video Analysis
=========================================================================================

For more information about this project, goto:

https://www.cs.drexel.edu/~david/Papers/Md_Reza_MS_Thesis.pdf_

This repository contains all of the source codes used for the Fly Tracking project.

Scripts
-------

You will find a few scripts to automate the processing of the data:

* ./process-video-beta
* ./batch-process


Tools
-----

You will find the utilities in fly-tools. You will need to 'make' them which is
easy to do. If you have trouble building, please make sure that you have gsl
and ImageMagick++, MagickWand, OpenCV and CvBlob libraries and headers
installed.  

    $ cd fly-data
    $ make

The programs that will be built are located here:

* ./fly-tools/FilterFlyMask
* ./fly-tools/FlyTracking
* ./fly-tools/mask-generator
* ./fly-tools/standard-deviation
* ./fly-tools/derive-background
* ./fly-tools/filter-mask

Dependencies
------------

Make sure you have these libraries and headers installed:

* OpenCv
* cvBlob
* MagickWand
* ImageMagick++  

You will also need MPlayer to extract the PNGs.

Usage
-----

Basically the fly tracking process takes several steps to complete. All of these
steps can be automated with the process-video-beta script available in the
scripts directory.

1. Cropping and Extracting Cell Images
2. Deriving Cell Backgrounds
3. Generating Cell Masks
4. Filtering Masks
5. Fly Tracking

### Step 1: Cropping and Extracting Cell Images ###

Since each video has several fly cells at one time, we need to crop each cell
into it's own set. To do so, extract one frame of the video and then use gimp to
determine the x, y, height and width of each cell (the select tool is very handy
for this). You should write each cell into a file in this format: 

	height:width:x:y

You should save this in a file (make sure to name it properly. For the purpose
of this explaination I am going to be working on video "Group1"

Group1First10MinCropFile will look like this:

	180:172:119:116
	181:173:195:120
	179:170:370:114


