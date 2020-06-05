// Name: Avijeet Adhikari
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>


void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
    printf("-n <thread_number> Number of threads you want to use for your process\n");
    printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000 -n 2\n\n");
}

// Creating a struct because the value will be changed for each number of thread that we create.
struct My_struct
{
    const char *outfile;
    double xcenter;
    double ycenter;
    double scale;
    int    image_width ;
    int    image_height ;
    int    max ;
    int    begin; //it is for tracking start work point for each thread
    int    end; //it is for tracking end work point for each thread
    int thread_number; //thread number entered in command line argument
    struct bitmap *bm;
    
}; struct My_struct my_struct;

//Creating a void to use for pthread_create, this function accept pointer as an argument.
void * fn( void * ptr) 
{
        struct My_struct *S = (struct My_struct *) ptr;

        int i,j;
    
        for(j=S->begin; j<S->end; j++)
        {
            for(i=0; i<S->image_width; i++)
            {
                // Determine the point in x,y space for that pixel.
                double x = (S->xcenter - S->scale) + i*((S->xcenter+S->scale)-(S->xcenter-S->scale))/S->image_width;
                double y = (S->ycenter - S->scale) + j*((S->ycenter+S->scale)-(S->ycenter-S->scale))/S->image_height;
                
                //From this point I just merged the iteration_at_point and iteration_to_color on this function
                //It computes the number of iterations at point x, y
                //in the Mandelbrot space, up to a maximum of max.

                double x0 = x;
                double y0 = y;
                
                int iter = 0;
                
                while( (x*x + y*y <= 4) && iter < S->max )
                {
                    
                    double xt = x*x - y*y + x0;
                    double yt = 2*x*y + y0;
                    
                    x = xt;
                    y = yt;
                    
                    iter++;
                }
                
                /*
                 Convert a iteration number to an RGBA color.
                 Here, we just scale to gray with a maximum of imax.
                 Modify this function to make more interesting colors.
                 */
                int gray = 255*iter/S->max;
                int color = MAKE_RGBA(gray,gray,gray,0);
                
                // Set the pixel in the bitmap.
                bitmap_set(S->bm,i,j,color);
            }
            
    
        }
    return 0;
}

int main( int argc, char *argv[] )
{
	char c;
	// These are the default configuration values used
	// if no command line arguments are given.
    my_struct.outfile = "mandel.bmp";
	my_struct.xcenter = 0;
	my_struct.ycenter = 0;
	my_struct.scale = 4;
	my_struct.image_width = 500;
	my_struct.image_height = 500;
	my_struct.max = 1000;
    my_struct.thread_number = 1;
    
	// For each command line argument given,
	// override the appropriate configuration value.

    while((c = getopt(argc,argv,"x:y:s:W:H:m:o:n:h"))!=-1) {
		switch(c) {
			case 'x':
				my_struct.xcenter = atof(optarg);
				break;
			case 'y':
				my_struct.ycenter = atof(optarg);
				break;
			case 's':
				my_struct.scale = atof(optarg);
				break;
			case 'W':
				my_struct.image_width = atoi(optarg);
				break;
			case 'H':
				my_struct.image_height = atoi(optarg);
				break;
			case 'm':
				my_struct.max = atoi(optarg);
				break;
			case 'o':
				my_struct.outfile = optarg;
				break;
            case 'n':
                my_struct.thread_number=atoi(optarg);
                break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}
        // Display the configuration of the image
   
    printf("mandel: x=%lf y=%lf scale=%lf max=%d thread_number=%d outfile=%s\n",my_struct.xcenter,my_struct.ycenter,my_struct.scale,my_struct.max,my_struct.thread_number,my_struct.outfile);
    
    // Create a bitmap of the appropriate size.
    struct bitmap *bm = bitmap_create(my_struct.image_width,my_struct.image_height);
    
    // Fill it with a dark blue, for debugging
    bitmap_reset(bm,MAKE_RGBA(0,0,255,0));
    //Creating an array of pthread_t type to identify the each thread and the array size
    //should be number of thread that is passed.
    pthread_t id[my_struct.thread_number];
    //ccreating array of struct of size thread number,it will be used in pthreead_create function
    //as each different thread will have different begin and end point.
    struct My_struct arr_struct[my_struct.thread_number];
    
    int image_range;// this will be the height that each individual thread needs to compute
    image_range = my_struct.image_height/my_struct.thread_number;
    int i;
    //In this loop we will perform work division to each individual thread.
    for( i=0; i<my_struct.thread_number; i++)
    {
        arr_struct[i].xcenter=my_struct.xcenter;
        arr_struct[i].ycenter=my_struct.ycenter;
        arr_struct[i].scale=my_struct.scale;
        arr_struct[i].image_width= my_struct.image_width;
        arr_struct[i].image_height= my_struct.image_height;
        arr_struct[i].max=my_struct.max;
        arr_struct[i].bm = bm;
        
        //assigning begin point to the number of threads that will be created
        arr_struct[i].begin = i * image_range;
        
        if(i== my_struct.thread_number-1)// condition to find out the last thread so that last thread cover rest of total height.
        {
            arr_struct[i].end = my_struct.image_height; //end point for last thread will be the total height of the image
        }
        //if the thread is not last the work will be evenly distributed to all the beginner threads.
        else
        {
            arr_struct[i].end = image_range + (i * image_range);
        }
        //creating thread as per the number of thread id.
        pthread_create(&id[i], NULL, fn, (void*)&arr_struct[i]);
    }
    
    int j;
    //joining with the terminated thread
    for ( j=0; j< my_struct.thread_number; j++)
    {
        pthread_join(id[j],NULL);
    }
    // Save the image in the stated file.
    if(!bitmap_save(bm,my_struct.outfile))
    {
        fprintf(stderr,"mandel: couldn't write to %s: %s\n",my_struct.outfile,strerror(errno));
        return 1;
    }

	return 0;
}
