/* 
 * This is just an example on how to use the thpool library 
 * 
 * We create a pool of 4 threads and then add 20 tasks to the pool(10 task1 
 * functions and 10 task2 functions).
 * 
 * Task1 doesn't take any arguments. Task2 takes an integer. Task2 is used to show
 * how to add work to the thread pool with an argument.
 * 
 * As soon as we add the tasks to the pool, the threads will run them. One thread
 * may run x tasks in a row so if you see as output the same thread running several
 * tasks, it's not an error.
 * 
 * All jobs will not be completed and in fact maybe even none will. You can add a sleep()
 * function if you want to complete all tasks in this test file to be able and see clearer
 * what is going on.
 * 
 * */

#include <stdio.h>
#include <stdlib.h> 
#include <libgen.h>
#include <wand/MagickWand.h> 

#include <wand/composite.h> 
#include <wand/convert.h> 
#include "thpool.h"

#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(-1); }

MagickWand *background;
char **global_argv;

void convert_image(char *file) {

	MagickWand *mask = NewMagickWand();
	MagickBooleanType status;
	char *output_name = malloc(256);
	if(MagickReadImage(mask, file) == MagickFalse) {
  ThrowWandException(mask);
	 return;
  }

  // convert \\\( -composite -compose Difference $output_dir/Masks/$setname/Background.png {} \\\) \\\( -contrast-stretch 90%x0% \\\) \\\( -threshold 30% \\\) $output_dir/Masks/$setname/Masks/{/}

  MagickCompositeImage(mask, background, DifferenceCompositeOp, 0, 0);
  MagickAutoLevelImage(mask); 
	MagickThresholdImage(mask, 0.30);

	sprintf(output_name, "%s%s", global_argv[3], basename(file));
  if(MagickWriteImages(mask, output_name, MagickTrue) == MagickFalse) {
		ThrowWandException(mask);
	}

	mask = DestroyMagickWand(mask);

	printf("output written to: %s \n", output_name);
	free(output_name);
	free(file);
}

int main( int argc, char **argv){

	// argv 1 = Background
	// argv 2 = input list
	// argv 3 = output folder
	
	MagickBooleanType   status;
	global_argv = argv;
  MagickWandGenesis();

  background = NewMagickWand();

  status=MagickReadImage(background,argv[1]);
  if (status == MagickFalse) {
		puts("background could not load error");
		exit(0);
	}

	thpool_t* threadpool;             /* make a new thread pool structure     */
	threadpool=thpool_init(8);        /* initialise it to 4 number of threads */

	char filename[256];	
	char *temp;
	FILE *f = fopen ( argv[2], "r" );
	if ( f != NULL ) {
		while ( fgets ( filename, sizeof(filename), f ) != NULL ) {
			temp = strchr(filename, '\n');
      if (temp != NULL) *temp = '\0'; 
			char *filename_r = malloc(256);
			strncpy(filename_r, filename, sizeof(filename));
			printf("add work: %s \n", filename);

			thpool_add_work(threadpool, (void*)convert_image, (void*)filename_r);
		}
	fclose ( f );
	}
	else {
		exit(0);
	}
	
  puts("Will kill threadpool");
	thpool_destroy(threadpool);
  MagickWandTerminus();
	return 0;
}
