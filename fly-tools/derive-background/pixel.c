#include <wand/MagickWand.h>
#include <gsl/gsl_histogram.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define round(x)((x)>=0?(int)((x)+0.5):(int)((x)-0.5))
#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(-1); }


int main(int argc, char **argv ) {

  MagickWand *magick_wand;
  MagickWand *first;
  double green = 0;
  double red = 0;
  double blue = 0;

  int image = 0;

  int i = 0;
  int j = 0;
  int k = 0;
  int l = 0;

  int height = 0;
  int width = 0;
  int nImages = 0;

  MagickWandGenesis();
  first = NewMagickWand();
  // Check the height and width of the first image
  MagickReadImage(first, "Background.png");

  height =  MagickGetImageHeight(first);
  width =  MagickGetImageWidth(first);

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
  uint8_t ****array;
  int dim1 = nImages;
  int dim2 = height;
  int dim3 = width;
  int dim4 = 3;

  array = calloc(dim1, sizeof(array[0]));
  for(i = 0; i < dim1; i++)
  {
    array[i] = calloc(dim2, sizeof(array[0][0]));
    for(j = 0; j < dim2; j++)
    {
      array[i][j] = calloc(dim3, sizeof(array[0][0][0]));
      for(k = 0; k < dim3; k++)
      {
        array[i][j][k] = calloc(dim4, sizeof(array[0][0][0][0]));
      }
    }
  }

  if ( file != NULL ) {
    while ((fgets(filename, sizeof(filename), file)) != NULL ) {
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

          //    printf("write: %d %d %d \n", round(red*255), round(green*255), round(blue*255));
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


  // initialize the output array
  int output[height][width][3];

  // calculate histograms

  for (j = 0; j < height; j++) {
    for (k = 0; k < width; k++) {

      gsl_histogram * r = gsl_histogram_alloc (nImages);
      gsl_histogram * g = gsl_histogram_alloc (nImages);
      gsl_histogram * b = gsl_histogram_alloc (nImages);

      gsl_histogram_set_ranges_uniform (r, 0, 255);
      gsl_histogram_set_ranges_uniform (g, 0, 255);
      gsl_histogram_set_ranges_uniform (b, 0, 255);

      for (i = 0; i < nImages; i++) {

        gsl_histogram_increment (r, array[i][j][k][0]);
        gsl_histogram_increment (g, array[i][j][k][1]);
        gsl_histogram_increment (b, array[i][j][k][2]);
      //  printf("height %d width %d image %d %hhu %hhu %hhu \n", j, k, i, array[i][j][k][0], array[i][j][k][1], array[i][j][k][2]);
      }


      size_t red = gsl_histogram_max_val(r);
      size_t green = gsl_histogram_max_val(r);
      size_t blue = gsl_histogram_max_val(r);
      output[j][k][0] = array[red][j][k][0];
      output[j][k][1] = array[green][j][k][1];
      output[j][k][2] = array[blue][j][k][2];

      printf("calculated histogram for (%d,%d) \n ", j, k); 
      /* int red_val = array[red - 1 ][j][k][0];
         int blue_val = array[blue - 1 ][j][k][1];
         int green_val = array[green - 1][j][k][2];
         printf("h:%d w:%d rm: %d gm: %d bm: %d rv: %lu bv: %lu gv:%lu\n",
         j, k, red_val, green_val, blue_val, red, blue, green);
         */
      gsl_histogram_free (r);
      gsl_histogram_free (g);
      gsl_histogram_free (b); 
    }
  }

  MagickWand *m_wand = NULL;
  PixelWand *p_wand = NULL;
  PixelIterator *iterator = NULL;
  PixelWand **pixels = NULL;

  int x,y;
  int grey;
  MagickWandGenesis();

  p_wand = NewPixelWand();
  PixelSetColor(p_wand,"white");
  m_wand = NewMagickWand();
  
  MagickNewImage(m_wand,height,width,p_wand);

  // Get a new pixel iterator
  iterator=NewPixelIterator(m_wand);

  for(y=0; y < height;y++) {
    // Get the next row of the image as an array of PixelWands

    pixels=PixelGetNextIteratorRow(iterator,&x);
    for(x=0; x < width; x++) {
      char rgb[17];
      sprintf(rgb, "rgb(%d,%d,%d)", output[x][y][0], output[x][y][1], output[x][y][2]);
      printf("(%d,%d) %s \n", y, x, rgb);
      PixelSetColor(pixels[x], rgb);
    }
    // Sync writes the pixels back to the m_wand
    PixelSyncIterator(iterator);
  }
  // Clean up
  iterator=DestroyPixelIterator(iterator);
  MagickWriteImage(m_wand,"output.png");
  DestroyMagickWand(m_wand);
  MagickWandTerminus();

  return 0;
}
