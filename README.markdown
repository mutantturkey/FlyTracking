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

You will find a few programs, FilterFlyMask, FlyTracking, and mask-generator
in fly-tools. You will need to 'make' them which is easy to do. If you have
trouble building, please make sure that you have gsl and ImageMagick++ and
the MagickWand libraries and headers installed.

	$ cd fly-data
	$ make

The programs that will be built are located here:

* ./fly-tools/FilterFlyMask
* ./fly-tools/FlyTracking
* ./fly-tools/mask-generator
