#include <wand/MagickWand.h>
#include <gsl/gsl_histogram.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define round(x)((int)((x)+0.5))
#define ThrowWandException(wand)  {  char *description; ExceptionType severity; description=MagickGetException(wand,&severity);  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);  description=(char *) MagickRelinquishMemory(description);  exit(-1); }
#define array(i,j,k,l) (array[height*width*i + width*j + k + l])

int nImages;
unsigned long height;
unsigned long width;

int findmax (uint8_t *p, int n) {
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

  char *usage = "derive-background -i <input-list> -s <sample-file> -o <output-filename>";
  char *sample_file = NULL;
  char *output_file = NULL;
  char *image_list = NULL;
  int c;

  while ((c = getopt (argc, argv, "s:i:o:h")) != -1)
    switch (c) {
      case 's':
        sample_file = optarg;
        break;
      case 'i':
        image_list = optarg;
        break;
      case 'o':
        output_file = optarg;
        break;
      case 'h':
        puts(usage);
        exit(1);
        break;
      default:
        break;
    }

  if( sample_file == NULL || image_list == NULL || output_file == NULL ) {
    puts(usage);
    exit(1);
  }

  MagickWand *input_wand, *first_wand, *output_wand;
  PixelIterator *input_iterator, *output_iterator;
  PixelWand **pixels, *p_out;

  double green, red, blue;

  int i,j, k, x, y;
  int image = 0;

  char filename[256];
  char rgb[128];
  char *temp;

  FILE *input_file;

  uint8_t ***output;

  MagickWandGenesis();

  // open the first image and get the height and width
  first_wand = NewMagickWand();
  if(MagickReadImage(first_wand, sample_file) == MagickFalse) {
    ThrowWandException(first_wand);
  }

  int  height =  MagickGetImageHeight(first_wand);
  int width =  MagickGetImageWidth(first_wand);

  first_wand =  DestroyMagickWand(first_wand);

  printf("height: %d width:%d \n", height, width);

  // count how many images there are in the input file
  FILE *count = fopen( image_list, "r");
  if(count != NULL) {  
    while((fgets(filename, sizeof(filename), count)) != NULL) {
      nImages++;
    }
    fclose(count);
  }

  printf("number of images: %d \n", nImages);

  uint8_t * array = (uint8_t *)malloc(nImages*height*width*3*sizeof(uint8_t));

  // initialize the storage arrays
  output = calloc(height, sizeof(output[0]));
  for(i = 0; i < height; i++) {
    output[i] = calloc(width, sizeof(output[0][0]));
    for(j = 0; j < width; j++) {
      output[i][j] = calloc(3, sizeof(output[0][0][0]));
    }
  }

  // store each pixel in the storage array. array[nImage][height][width][ { R, G, B} ] 
  input_file = fopen ( image_list, "r" );
  if ( input_file != NULL ) {
    while ((fgets(filename, sizeof(filename), input_file)) != NULL ) {
      temp = strchr(filename, '\n');
      if (temp != NULL) *temp = '\0';

      input_wand = NewMagickWand();
      MagickReadImage(input_wand, filename);

      input_iterator = NewPixelIterator(input_wand);

      printf("Image number:%d Filename: %s \n", image, filename);
      for (i=0; i<height; i++) {
        pixels = PixelGetNextIteratorRow(input_iterator, &width);
        for (j=0; j<width ; j++) {

          green = PixelGetGreen(pixels[j]);
          blue = PixelGetBlue(pixels[j]);
          red = PixelGetRed(pixels[j]);

          array(image,i,j,0) = round(red*255);
          array(image,i,j,1) = round(green*255);
          array(image,i,j,2) = round(blue*255); 
          // printf("array: (%d, %d,%d,%d,%d,%d) \n", image, i, j, array[image][i][j][0], array[image][i][j][1], array[image][i][j][2]);
        }
        PixelSyncIterator(input_iterator);
      }
      PixelSyncIterator(input_iterator);
      input_iterator=DestroyPixelIterator(input_iterator);
      ClearMagickWand(input_wand);

      image++;
    }
    fclose ( input_file );
  }
  else {
    printf("could not open file \n");
    exit(1);
  }

  // calculate histograms

  for (j = 0; j < height; j++) {
    for (k = 0; k < width; k++) {

      uint8_t red_histogram[255] = { 0 };
      uint8_t blue_histogram[255] = { 0 };
      uint8_t green_histogram[255] = { 0 };

      for (i = 0; i < nImages; i++) {
        red_histogram[array(i,j,k,0)] += 1;
        blue_histogram[array(i,j,k,1)] += 1;
        green_histogram[array(i,j,k,2)] += 1;
      }

      int red_val = findmax(red_histogram, 255);
      int blue_val = findmax(blue_histogram, 255);
      int green_val = findmax(green_histogram, 255);

      output[j][k][0] = (int)red_val;
      output[j][k][1] = (int)blue_val;
      output[j][k][2] = (int)green_val;

    }
  }

  pixels = NewPixelWand();
  p_out = NewPixelWand();
  PixelSetColor(p_out,"white");

  output_wand = NewMagickWand();
  MagickNewImage(output_wand,width,height,p_out);

  output_iterator=NewPixelIterator(output_wand);
  for(x=0;x<height;x++) {

    pixels=PixelGetNextIteratorRow(output_iterator,&y);

    for(y=0;y<width;y++) {
      sprintf(rgb, "rgb(%d,%d,%d)", output[x][y][0], output[x][y][1], output[x][y][2]);
      printf("write (%d,%d), %s \n", x ,y, rgb);
      PixelSetColor(pixels[y],rgb);
    }
    PixelSyncIterator(output_iterator);
  }

  output_iterator=DestroyPixelIterator(output_iterator);
  MagickWriteImage(output_wand, output_file);
  DestroyMagickWand(output_wand);

  MagickWandTerminus();
  return 0;
}
