#include <wand/MagickWand.h>
#include <gsl/gsl_histogram.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define round(x)((x)>=0?(int)((x)+0.5):(int)((x)-0.5))
#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(-1); }
int
findmax (uint8_t *p, int n)
{
   int mx = 0, v = p[0];
    for (int i = 1; i < n; i++) {
         if (p[i] > v) {
                v = p[i];
                     mx = i;
                        }
          }
     return mx;
}

int main(int argc, char **argv ) {

  MagickWand *magick_wand;
  MagickWand *first_wand;

  double green, red, blue;
  int i,j, k;
  int x,y;

  int image = 0;
  int height = 0;
  int width = 0;
  int nImages = 0;

  unsigned long number_wands; 

  char filename[256];
  char rgb[128];
  char *temp;

  uint8_t ****array;

  // open the first image and get the height and width
  first_wand = NewMagickWand();
  if(MagickReadImage(first_wand, argv[2]) == MagickFalse) {
    ThrowWandException(first_wand);
  }

  height =  MagickGetImageHeight(first_wand);
  width =  MagickGetImageWidth(first_wand);

  first_wand =  DestroyMagickWand(first_wand);

  printf("height: %d width:%d \n", height, width);


  // count how many images there are in the input file
  FILE *count = fopen( argv[1], "r");
  if(count != NULL) {  
    while((fgets(filename, sizeof(filename), count)) != NULL) {
      nImages++;
    }
    fclose(count);
  }

  printf("number of imagges: %d \n", nImages);


  // initialize the storage array.
  array = calloc(nImages, sizeof(array[0]));
  for(i = 0; i < nImages; i++)
  {
    array[i] = calloc(height, sizeof(array[0][0]));
    for(j = 0; j < height; j++)
    {
      array[i][j] = calloc(width, sizeof(array[0][0][0]));
      for(k = 0; k < width; k++)
      {
        array[i][j][k] = calloc(3, sizeof(array[0][0][0][0]));
      }
    }
  }

  MagickWandGenesis();

  // store each pixel in the storage array. array[nImage][height][width][ { R, G, B} ] 
  FILE *input_file = fopen ( argv[1], "r" );
  if ( input_file != NULL ) {
    while ((fgets(filename, sizeof(filename), input_file)) != NULL ) {
      temp = strchr(filename, '\n');
      if (temp != NULL) *temp = '\0';

      magick_wand = NewMagickWand();
      MagickReadImage(magick_wand, filename);

      PixelIterator* iterator = NewPixelIterator(magick_wand);

      PixelWand **pixels = PixelGetNextIteratorRow(iterator,&number_wands);
      printf("Image number:%d Filename: %s \n", image, filename);
      for (i=0; pixels != (PixelWand **) NULL; i++) {
        for (j=0; j<number_wands; j++) {

          green = PixelGetGreen(*pixels);
          blue = PixelGetBlue(*pixels);
          red = PixelGetRed(*pixels);

          array[image][i][j][0] = round(red*255);
          array[image][i][j][1] = round(green*255);
          array[image][i][j][2] = round(blue*255); 
          // printf("array: (%d, %d,%d,%d,%d,%d) \n", image, i, j, array[image][i][j][0], array[image][i][j][1], array[image][i][j][2]);
        }
        PixelSyncIterator(iterator);
        pixels=PixelGetNextIteratorRow(iterator,&number_wands);
      }

      PixelSyncIterator(iterator);
      iterator=DestroyPixelIterator(iterator);
      ClearMagickWand(magick_wand);

      image++;
    }
    fclose ( input_file );
  }
  else {
    printf("could not open file \n");
    exit(1);
  }


  // initialize the output array
  uint8_t output[200][200][3];

  // calculate histograms

  for (j = 0; j < height; j++) {
    for (k = 0; k < width; k++) {


      uint8_t histr[256] = { 0 }, histg[256] = { 0 }, histb[256] = { 0 };

        //printf("height %d width %d image %d %hhu %hhu %hhu \n", j, k, i, array[i][j][k][0], array[i][j][k][1], array[i][j][k][2]);
      for (i = 0; i < nImages; i++) {
        histr[array[i][j][k][0]] += 1;
        histg[array[i][j][k][1]] += 1;
        histb[array[i][j][k][2]] += 1;
      }

      output[j][k][0] = findmax(histr, 256);
      output[j][k][1] = findmax(histg, 256);
      output[j][k][2] = findmax(histb, 256);

      printf("out (%d,%d,%d,%d,%d) \n ", j, k, output[j][k][0], output[j][k][1], output[j][k][2]); 

    }
  }

  MagickWand *m_wand = NULL;
  PixelWand *p_wand = NULL;
  PixelIterator *iterator = NULL;
  PixelWand **pixels = NULL;

  p_wand = NewPixelWand();
  PixelSetColor(p_wand,"white");
  m_wand = NewMagickWand();

  MagickNewImage(m_wand,width,height,p_wand);

  iterator=NewPixelIterator(m_wand);

  for(x=0;x<height;x++) {

    pixels=PixelGetNextIteratorRow(iterator,&y);

    for(y=0;y<width;y++) {

      sprintf(rgb, "rgb(%d,%d,%d)", output[x][y][0], output[x][y][1], output[x][y][2]);
      printf("write (%d,%d), %s \n", x ,y, rgb);
      PixelSetColor(pixels[y],rgb); // this segfaults
    }

    PixelSyncIterator(iterator);
  }

  iterator=DestroyPixelIterator(iterator);
  MagickWriteImage(m_wand,"output.png");
  DestroyMagickWand(m_wand);

  MagickWandTerminus();

  return 0;
}
