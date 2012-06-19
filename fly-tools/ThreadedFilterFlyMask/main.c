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
#include "thpool.h"

#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(-1); }

MagickWand *background;
char **global_argv;



																																										
void convert_image(char *file) {

	printf("filename: %s", file);
	MagickWand *mask = NewMagickWand();
	MagickBooleanType status;
	char output_name[256];
	
		
	if(MagickReadImage(mask, file) == MagickFalse) {
		ThrowWandException(mask);
		printf("could not read file: %s", file );
		return;
	}

	/* image ops */
	MagickResizeImage(mask,10,10,LanczosFilter,1.0);
	
  sprintf(output_name, "%s%s", global_argv[3], basename(file));

	
	status=MagickWriteImages(mask, output_name , MagickTrue);
	if ( status == MagickFalse ) {
			puts("write error");
 }

	mask = DestroyMagickWand(mask);
}

int main( int argc, char **argv){

	// argv 1 = Background
	// argv 2 = input list
	// argv 3 = output folder
	
	char *stream;

	stream = "hey \n";

	printf("%s", stream);
  if (stream[strlen(stream)] == '\n') {
				  stream[strlen(stream) - 1] == '\0';
}
	printf("%s", stream);
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
	threadpool=thpool_init(1);        /* initialise it to 4 number of threads */
	
	char line[256];
	FILE *f = fopen ( argv[2], "r" );
	if ( f != NULL ) {
		while ( fgets ( line, sizeof line, f ) != NULL ) {
			thpool_add_work(threadpool, (void*)convert_image, (void*)line);
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
