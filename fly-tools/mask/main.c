// Calvin Morrison Copyright 2012 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <libgen.h>

#include <wand/MagickWand.h> 
#include <wand/composite.h> 
#include <wand/convert.h> 

#include "thpool.h"

#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(EXIT_FAILURE); }

MagickWand *background;
char *output_folder = NULL;

void convert_image(char *file) {

  MagickWand *mask = NewMagickWand();
  char *output_name = malloc(256);
  if(MagickReadImage(mask, file) == MagickFalse) {
    ThrowWandException(mask);
    return;
  }

  MagickCompositeImage(mask, background, DifferenceCompositeOp, 0, 0);
  MagickAutoLevelImage(mask); 
  MagickThresholdImage(mask, 30000);

  sprintf(output_name, "%s%s", output_folder, basename(file));
  if(MagickWriteImages(mask, output_name, MagickTrue) == MagickFalse) {
    ThrowWandException(mask);
  }

  mask = DestroyMagickWand(mask);

  printf("output written to: %s \n", output_name);
  free(output_name);
  free(file);
}

int main( int argc, char **argv){

  char *usage = "Usage: generate-mask -b <Background filename> -i <image list > -o <outputFolderName>";
  char *background_file = NULL;
  char *image_list = NULL;
  int c;

  while ((c = getopt (argc, argv, "b:i:o:hv")) != -1)
    switch (c) {
      case 'b':
        background_file = optarg;
        break;
      case 'i':
        image_list = optarg;
        break;
      case 'o':
        output_folder = optarg;
        break;
      case 'v':
        break;
      case 'h':
        puts(usage);
        exit(EXIT_SUCCESS);
        break;
      default:
        break;
    }

  if( background_file == NULL || image_list == NULL || output_folder == NULL ) {
    puts(usage);
    exit(EXIT_FAILURE);
  }
  MagickWandGenesis();

  background = NewMagickWand();

  if(MagickReadImage(background, background_file) == MagickFalse) {
    puts("background could not load error");
    exit(EXIT_FAILURE);
  }

  thpool_t* threadpool;        
  threadpool=thpool_init(4);

  char filename[256];	
  char *temp;
  FILE *f = fopen ( image_list, "r" );
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
    exit(EXIT_FAILURE);
  }

  puts("Will kill threadpool");
  thpool_destroy(threadpool);
  MagickWandTerminus();
  return 0;
}
