Automated Categorization of Drosophila Learning and Memory Behaviors using Video Analysis
=========================================================================================

For more information about this project, goto:

https://www.cs.drexel.edu/~david/Papers/Md_Reza_MS_Thesis.pdf

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
4. Filtering Cell Masks
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

After you have completed that, you will want to run the extraction step of the
process-video-beta script. To get a more detailed look at the arguments accepted
by the processing script, run it with the -h flag.

	process-video-beta -v First10MinGroup1.avi -l Group1First10MinCropFile -n 1 -t First10MinSet -d data/ -1

The -d flag indicates the folder you want to extract these files to, and will be
used as the directory structure for the rest of the steps. The -1 flag tells it
to run the first out of five steps.

This will take some time, and require a lot of CPU and disk space.

### Step 2: Deriving Cell Backgrounds ###

The second step of the process is deriving the background image from the video.
By doing this, we can subtract the background from the current frame to generate
the mask. When subtracted the parts left over are the fly, while things in the
background won't show up.

	process-video-beta -v First10MinGroup1.avi -l Group1First10MinCropFile -n 1 -t First10MinSet -d data/ -2

This will derive the backgrounds for all of the Cells in the video seperately.
After this tool is done running, it will output the Background.png into the mask
folder. $data-root/$settype$setnumber/Background.png, for example it might be in
data/First10MinSet2/Background.png.

Verify this image. Sometimes when the flies do not move the background might
include the fly! If there are any left of flie parts, use GIMP to airbrush over
that section with an appropriate color (guess what the background would be
"close" too).

### Step 3: Generating Cell Masks ###

The third step of the process is generating the Masks from the background and
the cropped images.

	process-video-beta -v First10MinGroup1.avi -l Group1First10MinCropFile -n 1 -t First10MinSet -d data/ -3

This step will output the masks in $data-root/$settype$setnumber/Masks/

### Step 4: Filtering Cell Masks ###

The fourth step is the filtering step. It's important because it makes sure the
masks do not have stray objects in them (which will mess up the final tracking
sequence) 


	process-video-beta -v First10MinGroup1.avi -l Group1First10MinCropFile -n 1 -t First10MinSet -d data/ -4

The filtered masks are in $data-root/$settype$setnumber/Filtered/final/

### Step 5: Fly Tracking ###

This is the final step of the image processing. It will generate results in the
$data-root/$settype$setnumber/Final/. There will be PNGs showing the tracking
results (refer to the paper to see what the polygons and lines refer too), a few
files that indicate the standard deviation and mean of certain fly behaviors,
and a "stat" file, which contains debug and statistical information as well.

This process willl take a long time beceause it iterates through each images in a
set (about 18,000 images).

	process-video-beta -v First10MinGroup1.avi -l Group1First10MinCropFile -n 1 -t First10MinSet -d data/ -4

After this is done running you should review the pngs to make sure that the fly
tracking was performed correctlyAfter this is done running you should review the
pngs to make sure that the fly tracking was performed correctly. 

### Into the Beyond ###

Now that we've done the hard work of image processing the rest is up to you! The
data is all there, now you need to do manually generate the feature vectors, and
you need to run our simple k-means clustering. 

The K-Means clustering requires Java to run and our Least Squares Solution CCI
calculator was written in Matlab and requires it to run.



