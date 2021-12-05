#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#define THREADS 4

#define filterWidth 3
#define filterHeight 3

#define RGB_MAX 255
typedef struct {
	 unsigned char r, g, b;
} PPMPixel;

struct parameter {
	PPMPixel *image;         //original image
	PPMPixel *result;        //filtered image
	unsigned long int w;     //width of image
	unsigned long int h;     //height of image
	unsigned long int start; //starting point of work
	unsigned long int size;  //equal share of work (almost equal if odd)
};
FILE *file;


/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) using convolution.
    (1) For each pixel in the input image, the filter is conceptually placed on top of the image with its origin lying on that pixel.
    (2) The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding filter values.
    (3) The results are summed together to yield a single output value that is placed in the output image at the location of the pixel being processed on the input.
 
 */
void *threadfn(void *params)
{
	
	int laplacian[filterWidth][filterHeight] =
	{
	  -1, -1, -1,
	  -1,  8, -1,
	  -1, -1, -1,
	};
    PPMPixel *img = ((struct parameter*) params) -> image;
    unsigned long int start = ((struct parameter*)params) -> start;
    unsigned long int size = ((struct parameter*)params) -> size;
    unsigned long int w = ((struct parameter*)params) -> w;
    unsigned long int h = ((struct parameter*)params) -> h;

    int red, blue, green;
    int x_coordinate, y_coordinate;
    
    for (int i = 0; i < w; i++)
        { //i = iteratorImageWidth
            for (int j = start; j < start + size; j++)
            { // j = iteratorImageHeight
                for (int m = 0; m < filterHeight; m++) //this point on is getting all of the needed data from each surrounding pixel, compiling the data and then uding it for the one pixel in the second loop
                    { //m = iteratorFilterHeight 
                    for (int n = 0; n < filterWidth; n++)

                    { //l = iteratorFilterWidth
                                
                        x_coordinate = (i - filterWidth / 2 + n + w) % w;
                        y_coordinate = (j - filterHeight / 2 + m + h) % h;
                        red += (img[y_coordinate * w + x_coordinate].r * laplacian[n][m]);
                        green+= (img[y_coordinate * w + x_coordinate].g * laplacian[n][m]);
                        blue+= (img[y_coordinate * w + x_coordinate].b * laplacian[n][m]);
                    }
                }
                if(red > RGB_MAX)
                    red = RGB_MAX;
                if (red < 0)
                    red = 0;
                if(green > RGB_MAX)
                    green = RGB_MAX;
                if (green < 0)
                    green = 0;
                if(blue > RGB_MAX)
                    blue = RGB_MAX;
                if (blue < 0)
                    blue = 0;

                ((struct parameter*)params) -> result[j * w + i].r =red;
                ((struct parameter*)params) -> result[j * w + i].g = green;
                ((struct parameter*)params) -> result[j * w + i].b = blue;
                red = 0;
                green = 0;
                blue = 0;
            }
        }
    /*For all pixels in the work region of image (from start to start+size)
      Multiply every value of the filter with corresponding image pixel. Note: this is NOT matrix multiplication.
      Store the new values of r,g,b in p->result.
     */
		
	return NULL;
}


/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "name" (the second argument).
 */
void writeImage(PPMPixel *image, char *name, unsigned long int width, unsigned long int height)
{
    FILE * imageFile = fopen(name, "wb+");
    fprintf(imageFile, "P6\n%lu %lu\n%d\n", width, height, 255);
    size_t dimensions = width * height;

    //write the function
    int check = fwrite(image, sizeof(PPMPixel), dimensions, imageFile);

    if (check != dimensions)
    {
        printf("Error writing to the file");
        exit(1);
    }

    fclose(imageFile);
}
    


/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height
    255                 -- max color value
 
 Check if the image format is P6. If not, print invalid format error message.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename)
 */
PPMPixel *readImage(const char *filename, unsigned long int *width, unsigned long int *height)

{
    FILE *filePointer;
    filePointer = fopen(filename, "rb");

    if (!filePointer)
    {
        printf("Couldnt open file\n");
        exit(1);
    }

    PPMPixel *img;
    char str[2];
    char commCheck;
    int colorCode;

    //check the image format by reading the first two characters in filename and compare them to P6.
    fscanf(filePointer, "%s\n", str);
    if (strcmp(str, "P6"))
    {
        printf("File is not a ppm image\n");
        exit(1);
    }
    commCheck = getc(filePointer);
    //If there are comments in the file, skip them. You may assume that comments exist only in the header block.
    while (!isdigit(commCheck))
    {   
        if(commCheck == '#') {
            do
            {
                commCheck = getc(filePointer);
            } while (commCheck != '\n');
        commCheck = getc(filePointer);
        }
    }
    ungetc(commCheck, filePointer);
    fscanf(filePointer, "%lu %lu\n", width, height);
    //Read rgb component. Check if it is equal to RGB_MAX. If  not, display error message.
    fscanf(filePointer, "%d\n", &colorCode);
    if (colorCode != RGB_MAX)
    {
        printf("File's RGB is not 255, try again!");
        exit(0);
    }
    //allocate memory for img. NOTE: A ppm image of w=200 and h=300 will contain 60000 triplets (i.e. for r,g,b), ---> 180000 bytes.
    img = malloc((sizeof(PPMPixel) * *width * *height));
    //read pixel data from filename into img. The pixel data is stored in scanline order from left to right (up to bottom) in 3-byte chunks (r g b values for each pixel) encoded as binary numbers.
    size_t dimensions = *height * *width;

    fread(img, sizeof(PPMPixel), dimensions, filePointer);
    fclose(filePointer);
    return img;
}

/* Create threads and apply filter to image.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads.
 Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image)
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {

    PPMPixel *result;
    printf("w = %ld, h = %ld\n", w, h);

    int numThreads = 10;
    //allocate memory for result
    result = malloc(w * h * sizeof(*result));
    int work = h/numThreads; 
    int size = work;
    //allocate memory for parameters (one for each thread)
    struct parameter *params;
    params = malloc(sizeof(*params) * numThreads);
    pthread_t threads[numThreads];
    int hold = 0;
    for (int i = 0; i < numThreads; i++){
        if(i == (numThreads - 1)){
            size = h%numThreads + work; 
        }
        params[i].size = size;
        params[i].start = i * work;
        params[i].w = w;
        params[i].h = h;
        params[i].image = image;
        params[i].result = result;
        //make sure thread is being created successfully
        
        if(pthread_create(&threads[i], NULL, threadfn, (void*)&params[i]) != 0){
            printf("Fail pthread_create");
            exit(1);
        }
        
    }
    //Let threads wait till they all finish their work.
    for (int j = 0; j < numThreads; j++){
        if (pthread_join(threads[j], (void *)result) != 0){
            printf("Fail pthread_Join");
            exit(1);
        }

    }
    free(params);
    return result;
}


/*The driver of the program. Check for the correct number of arguments. If wrong print the message: "Usage ./a.out filename"
    Read the image that is passed as an argument at runtime. Apply the filter. Print elapsed time in .3 precision (e.g. 0.006 s). Save the result image in a file called laplacian.ppm. Free allocated memory.
 */
int main(int argc, char *argv[])
{
	//load the image into the buffer
    char *name = "laplacian.ppm";
    unsigned long int w, h;
    double elapsedTime = 0.0;
    struct parameter pars;
    if (argc != 2)
    {
        printf("Usage ./a.out filename");
        return 0;
    }
    struct timeval start, stop;
        suseconds_t startTime;
        suseconds_t stopTime;

    pars.image = readImage(argv[1], &w, &h);
    gettimeofday(&start, NULL);
	pars.result = apply_filters(pars.image, w, h, 0);
    gettimeofday(&stop, NULL);
    writeImage(pars.result, name, w, h);
    double seconds = (stop.tv_usec - start.tv_usec);
    seconds = seconds / 1000000;
    printf("\nElapsed time: %.03f seconds\n", seconds);
    free(pars.image);
    free(pars.result);
	return 0;
}
