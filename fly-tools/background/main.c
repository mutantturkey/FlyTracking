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

int findmax (uint8_t *p, int n)
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

  char *usage = "derive-background -i <input-list> -s <sample-file> -o <output-filename>";
  char *sample_file = NULL;
  char *output_file = NULL;
  char *image_list = NULL;
  bool histogram = false;
  int c;
  int opterr = 0;

  while ((c = getopt (argc, argv, "s:i:o:mh")) != -1)
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
      case 'm':
        histogram = true;
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

  MagickWand *magick_wand, *first_wand, *output_wand;
  PixelIterator *input_iterator, *output_iterator;
  PixelWand **pixels, *p_out;

  double green, red, blue;

  int i,j, k, x, y;
  int image = 0;
  int nImages = 0;

  unsigned long height, width;


  char filename[256];
  char rgb[128];
  char *temp;

  FILE *input_file;

  uint8_t ****array;
  uint8_t ***output;

  // open the first image and get the height and width
  first_wand = NewMagickWand();
  if(MagickReadImage(first_wand, sample_file) == MagickFalse) {
    ThrowWandException(first_wand);
  }

  height =  MagickGetImageHeight(first_wand);
  width =  MagickGetImageWidth(first_wand);

  first_wand =  DestroyMagickWand(first_wand);

  printf("height: %ld width:%ld \n", height, width);


  // count how many images there are in the input file
  FILE *count = fopen( image_list, "r");
  if(count != NULL) {  
    while((fgets(filename, sizeof(filename), count)) != NULL) {
      nImages++;
    }
    fclose(count);
  }

  printf("number of images: %d \n", nImages);


  // initialize the storage arrays
  output = calloc(height, sizeof(output[0]));
  for(i = 0; i < height; i++) {
    output[i] = calloc(width, sizeof(output[0][0]));
    for(j = 0; j < width; j++) {
      output[i][j] = calloc(3, sizeof(output[0][0][0]));
    }
  }

  array = calloc(nImages, sizeof(array[0]));
  for(i = 0; i < nImages; i++) {
    array[i] = calloc(height, sizeof(array[0][0]));
    for(j = 0; j < (long) height; j++) {
      array[i][j] = calloc(width, sizeof(array[0][0][0]));
      for(k = 0; k < width; k++){
        array[i][j][k] = calloc(3, sizeof(array[0][0][0][0]));
      }
    }
  }

  MagickWandGenesis();

  // store each pixel in the storage array. array[nImage][height][width][ { R, G, B} ] 
  input_file = fopen ( image_list, "r" );
  if ( input_file != NULL ) {
    while ((fgets(filename, sizeof(filename), input_file)) != NULL ) {
      temp = strchr(filename, '\n');
      if (temp != NULL) *temp = '\0';

      magick_wand = NewMagickWand();
      MagickReadImage(magick_wand, filename);

      input_iterator = NewPixelIterator(magick_wand);

      printf("Image number:%d Filename: %s \n", image, filename);
      for (i=0; i<height; i++) {
        pixels = PixelGetNextIteratorRow(input_iterator, &width);
        for (j=0; j<width ; j++) {

          green = PixelGetGreen(pixels[j]);
          blue = PixelGetBlue(pixels[j]);
          red = PixelGetRed(pixels[j]);

          array[image][i][j][0] = round(red*255);
          array[image][i][j][1] = round(green*255);
          array[image][i][j][2] = round(blue*255); 
          // printf("array: (%d, %d,%d,%d,%d,%d) \n", image, i, j, array[image][i][j][0], array[image][i][j][1], array[image][i][j][2]);
        }
        PixelSyncIterator(input_iterator);
      }
      PixelSyncIterator(input_iterator);
      input_iterator=DestroyPixelIterator(input_iterator);
      ClearMagickWand(magick_wand);

      image++;
    }
    fclose ( input_file );
  }
  else {
    printf("could not open file \n");
    exit(1);
  }

  // calculate histograms

  if (!histogram) { 
    for (j = 0; j < height; j++) {
      for (k = 0; k < width; k++) {

        uint8_t histr[256] = { 0 }, histg[256] = { 0 }, histb[256] = { 0 };
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
  } else {
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
        }


        size_t red = gsl_histogram_max_val(r);
        size_t green = gsl_histogram_max_val(r);
        size_t blue = gsl_histogram_max_val(r);
        output[j][k][0] = array[red][j][k][0];
        output[j][k][1] = array[blue][j][k][1];
        output[j][k][2] = array[green][j][k][2];

        gsl_histogram_free (r);
        gsl_histogram_free (g);
        gsl_histogram_free (b);
        printf("out (%d,%d,%d,%d,%d) \n ", j, k, output[j][k][0], output[j][k][1], output[j][k][2]); 

      }
    }
  }
  pixels = NewPixelWand();
  p_out = NewPixelWand();
  PixelSetColor(p_out,"white");

  output_wand = NewMagickWand();
  MagickNewImage(output_wand,width,height,p_out);

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
