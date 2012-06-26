#include <wand/MagickWand.h>
#include <stdio.h>
#include <string.h>

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(-1); }

int main(int argc, char **argv) {

  MagickWand *magick_wand;
  MagickWand *first;

  double green = 0;
  double red = 0;
  double blue = 0;

  int nImages = 0;
  int image = 0;

  int i = 0;
  int j = 0;
  int k = 0;
  int l = 0;

  MagickWandGenesis();
  first = NewMagickWand();
  // Check the height and width of the first image
  MagickReadImage(first, "Background.png");

  const int height =  MagickGetImageHeight(first);
  const int width =  MagickGetImageWidth(first);

  printf("%d %d \n", height, width);


  // Count how many images there are in the input file

  char fname[256];
  FILE *count = fopen( argv[1], "r");
  if(count != NULL) {  
    while((fgets(fname, sizeof(fname), count)) != NULL) {
      nImages++;
    }
    fclose(count);
  }

  printf("%d \n", nImages);
  //this will store the number of pixels in each row
  unsigned long number_wands; 

  char filename[256];
  char *temp;
  FILE *file = fopen ( argv[1], "r" );


  // initialize this crazy array.

  long int array[nImages][height][width][3];
  //long int array[nImages][height][width][3];

  printf("%d", nImages);
  if ( file != NULL ) {
    while ((fgets(filename, sizeof(filename), file)) != NULL ) {
      temp = strchr(filename, '\n');
      if (temp != NULL) *temp = '\0';
      char *filename_r = malloc(256);
      strncpy(filename_r, filename, sizeof(filename));
      magick_wand = NewMagickWand();
      MagickReadImage(magick_wand, filename);


      PixelIterator* iterator = NewPixelIterator(magick_wand);

      PixelWand **pixels = PixelGetNextIteratorRow(iterator,&number_wands);

      for (i=0; pixels != (PixelWand **) NULL; i++) {
        for (j=0; j<number_wands; j++) {
          green = PixelGetGreen(*pixels);
          blue = PixelGetBlue(*pixels);
          red = PixelGetRed(*pixels);

          printf("r: Image: %d Height: %d Width: %d Red: %ld Green: %ld Blue: %ld \n", image,  i, j, round(red*255), round(green*255), round(blue*255));
          array[image][i][j][0] = round(red*255);
          array[image][i][j][1] = round(green*255);
          array[image][i][j][2] = round(blue*255); 
        }
        PixelSyncIterator(iterator);
        pixels=PixelGetNextIteratorRow(iterator,&number_wands);
      }

      PixelSyncIterator(iterator);
      iterator=DestroyPixelIterator(iterator);
      ClearMagickWand(magick_wand);

      image++;
    }
    fclose ( file );
  }
  else {
    exit(0);
  }

  for (i = 0; i < nImages; i++) {
    for (j = 0; j < height; j++) {
      for (k = 0; k < width; k++) {
        for (l = 0; l < 3; l++) {
          printf("w: Image: %d Height: %d Width: %d Color: %d Value: %ld \n", i, j, k, l, array[i][j][k][l]);
        }
      }
    }
  }



  //write back out to png

  //if (status == MagickFalse) {
  //   printf("Error writing to png\n");
  //}
  //else {
  //   printf("still w00tness!!\n");
  // }

  // for (j=0; j<361; j++) {
  //  printf("%lf \n", imagepixels[j]);
  //if (j+1 % 19 == 0)
  //   printf("\n");
  // }

  MagickWandTerminus();

  return 0;
}
