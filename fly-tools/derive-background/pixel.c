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
  MagickWand *first_wand;

  double green, red, blue;
  int i,j, k;

  int image = 0;
  int height = 0;
  int width = 0;
  int nImages = 0;

  unsigned long number_wands; 

  char filename[256];
  char *temp;

  uint8_t ****array;

  MagickWandGenesis();

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

  printf("%d \n", nImages);


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
    fclose ( input_file );
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
        //printf("height %d width %d image %d %hhu %hhu %hhu \n", j, k, i, array[i][j][k][0], array[i][j][k][1], array[i][j][k][2]);
      }


      size_t red = gsl_histogram_max_bin(r);
      size_t green = gsl_histogram_max_bin(g);
      size_t blue = gsl_histogram_max_bin(b);
      int red_index = gsl_histogram_get(r, red);
      int green_index = gsl_histogram_get(g, green);
      int blue_index = gsl_histogram_get(b, blue);
      output[j][k][0] = array[red_index - 1 ][j][k][0];
      output[j][k][1] = array[green_index - 1 ][j][k][1];
      output[j][k][2] = array[blue_index - 1 ][j][k][2];

      printf("i (%d,%d) out(%d,%d,%d) arr(%d, %d, %d)\n ", j, k, output[j][k][0], output[j][k][1], output[j][k][2], array[red_index - 1][j][k][0], array[green_index - 1][j][k][1], array[blue_index - 1][j][k][2]); 

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
  char rgb[128];

  MagickWandGenesis();

  p_wand = NewPixelWand();
  PixelSetColor(p_wand,"white");
  m_wand = NewMagickWand();
  // Create a 100x100 image with a default of white
  MagickNewImage(m_wand,height,width,p_wand);
  // Get a new pixel iterator 
  iterator=NewPixelIterator(m_wand);
  for(y=0;y<150;y++) {
    // Get the next row of the image as an array of PixelWands
    pixels=PixelGetNextIteratorRow(iterator,&x);
    // Set the row of wands to a simple gray scale gradient
    for(x=0;x<150;x++) {
      sprintf(rgb, "rgb(%d,%d,%d)", output[i][j][0], output[i][j][1], output[i][j][2]);
      PixelSetColor(pixels[x],rgb);
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
