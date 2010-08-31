/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef HAVE_WINDOWS_H
#include <process.h> /* getpid() */
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#endif
#include "mpi.h"

/* definitions */

#define PROMPT 1
#define USE_PPM /* define to output a color image, otherwise output is a grayscale pgm image */

#ifndef MIN
#define MIN(x, y)	(((x) < (y))?(x):(y))
#endif
#ifndef MAX
#define MAX(x, y)	(((x) > (y))?(x):(y))
#endif

#define DEFAULT_PORT 7470   /* default port to listen on */
#define NOVALUE 99999       /* indicates a parameter is as of yet unspecified */
#define MAX_ITERATIONS 10000 /* maximum 'depth' to look for mandelbrot value */
#define INFINITE_LIMIT 4.0  /* evalue when fractal is considered diverging */
#define MAX_X_VALUE 2.0     /* the highest x can go for the mandelbrot, usually 1.0 */
#define MIN_X_VALUE -2.0    /* the lowest x can go for the mandelbrot, usually -1.0 */
#define MAX_Y_VALUE 2.0     /* the highest y can go for the mandelbrot, usually 1.0 */
#define MIN_Y_VALUE -2.0    /* the lowest y can go for the mandelbrot, usually -1.0 */
#define IDEAL_ITERATIONS 100 /* my favorite 'depth' to default to */
/* the ideal default x and y are currently the same as the max area */
#define IDEAL_MAX_X_VALUE 1.0
#define IDEAL_MIN_X_VALUE -1.0
#define IDEAL_MAX_Y_VALUE 1.0
#define IDEAL_MIN_Y_VALUE -1.0
#define MAX_WIDTH 2048       /* maximum size of image, across, in pixels */
#define MAX_HEIGHT 2048      /* maximum size of image, down, in pixels */
#define IDEAL_WIDTH 768       /* my favorite size of image, across, in pixels */
#define IDEAL_HEIGHT 768      /* my favorite size of image, down, in pixels */

#define RGBtocolor_t(r,g,b) ((color_t)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define getR(r) ((int)((r) & 0xFF))
#define getG(g) ((int)((g>>8) & 0xFF))
#define getB(b) ((int)((b>>16) & 0xFF))

typedef int color_t;

typedef struct complex_t
{
  /* real + imaginary * sqrt(-1) i.e.   x + iy */
  double real, imaginary;
} complex_t;

/* prototypes */

void read_mand_args(int argc, char *argv[], int *o_max_iterations,
		    int *o_pixels_across, int *o_pixels_down,
		    double *o_x_min, double *o_x_max,
		    double *o_y_min, double *o_y_max, int *o_julia,
		    double *o_julia_real_x, double *o_julia_imaginary_y,
		    double *o_divergent_limit, int *o_alternate,
		    char *filename, int *num_colors, int *use_stdin,
		    int *save_image, int *use_datatypes);
void check_mand_params(int *m_max_iterations,
		       int *m_pixels_across, int *m_pixels_down,
		       double *m_x_min, double *m_x_max,
		       double *m_y_min, double *m_y_max,
		       double *m_divergent_limit);
void check_julia_params(double *julia_x_constant, double *julia_y_constant);
int additive_mandelbrot_point(complex_t coord_point, 
			    complex_t c_constant,
			    int Max_iterations, double divergent_limit);
int additive_mandelbrot_point(complex_t coord_point, 
			    complex_t c_constant,
			    int Max_iterations, double divergent_limit);
int subtractive_mandelbrot_point(complex_t coord_point, 
			    complex_t c_constant,
			    int Max_iterations, double divergent_limit);
complex_t exponential_complex(complex_t in_complex);
complex_t multiply_complex(complex_t in_one_complex, complex_t in_two_complex);
complex_t divide_complex(complex_t in_one_complex, complex_t in_two_complex);
complex_t inverse_complex(complex_t in_complex);
complex_t add_complex(complex_t in_one_complex, complex_t in_two_complex);
complex_t subtract_complex(complex_t in_one_complex, complex_t in_two_complex);
double absolute_complex(complex_t in_complex);
void dumpimage(char *filename, int in_grid_array[], int in_pixels_across, int in_pixels_down,
	  int in_max_pixel_value, char input_string[], int num_colors, color_t colors[]);
int single_mandelbrot_point(complex_t coord_point, 
			    complex_t c_constant,
			    int Max_iterations, double divergent_limit);
color_t getColor(double fraction, double intensity);
int Make_color_array(int num_colors, color_t colors[]);
void PrintUsage();
static int sock_write(int sock, void *buffer, int length);
static int sock_read(int sock, void *buffer, int length);

#ifdef USE_PPM
const char *default_filename = "pmandel.ppm";
#else
const char *default_filename = "pmandel.pgm";
#endif

/* Solving the Mandelbrot set
 
   Set a maximum number of iterations to calculate before forcing a terminate
   in the Mandelbrot loop.
*/

/* Command-line parameters are all optional, and include:
   -xmin # -xmax #      Limits for the x-axis for calculating and plotting
   -ymin # -ymax #      Limits for the y-axis for calculating and plotting
   -xscale # -yscale #  output pixel width and height
   -depth #             Number of iterations before we give up on a given
                        point and move on to calculate the next
   -diverge #           Criteria for assuming the calculation has been
                        "solved"-- for each point z, when
                             abs(z=z^2+c) > diverge_value
   -julia        Plot a Julia set instead of a Mandelbrot set
   -jx # -jy #   The Julia point that defines the set
   -alternate #    Plot an alternate Mandelbrot equation (varient 1 or 2 so far)
*/

int myid;
int use_stdin = 0;
int sock;

void swap(int *i, int *j)
{
    int x;
    x = *i;
    *i = *j;
    *j = x;
}

int main(int argc, char *argv[])
{
    complex_t coord_point, julia_constant;
    double x_max, x_min, y_max, y_min, x_resolution, y_resolution;
    double divergent_limit;
    char file_message[160];
    char filename[100];
    int icount, imax_iterations;
    int ipixels_across, ipixels_down;
    int i, j, k, julia, alternate_equation;
    int imin, imax, jmin, jmax;
    int *work;
    /* make an integer array of size [N x M] to hold answers. */
    int *grid_array = NULL;
    int numprocs;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int num_colors;
    color_t *colors = NULL;
    int listener;
    int save_image = 0;
    int optval;
    int big_size;
    MPI_Win win;
    int error;
    int done;
    int use_datatypes = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Get_processor_name(processor_name, &namelen);

    if (numprocs == 1)
    {
	PrintUsage();
	MPI_Finalize();
	exit(0);
    }

    if (myid == 0)
    {
	printf("Welcome to the Mandelbrot/Julia set explorer.\n");

	/* Get inputs-- region to view (must be within x/ymin to x/ymax, make sure
	xmax>xmin and ymax>ymin) and resolution (number of pixels along an edge,
	N x M, i.e. 256x256)
	*/

	read_mand_args(argc, argv, &imax_iterations, &ipixels_across, &ipixels_down,
	    &x_min, &x_max, &y_min, &y_max, &julia, &julia_constant.real,
	    &julia_constant.imaginary, &divergent_limit,
	    &alternate_equation, filename, &num_colors, &use_stdin, &save_image, &use_datatypes);
	check_mand_params(&imax_iterations, &ipixels_across, &ipixels_down,
	    &x_min, &x_max, &y_min, &y_max, &divergent_limit);

	if (julia == 1) /* we're doing a julia figure */
	    check_julia_params(&julia_constant.real, &julia_constant.imaginary);

	MPI_Bcast(&num_colors, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&imax_iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ipixels_across, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ipixels_down, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&divergent_limit, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&julia, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&julia_constant.real, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&julia_constant.imaginary, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&alternate_equation, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&use_datatypes, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else
    {
	MPI_Bcast(&num_colors, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&imax_iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ipixels_across, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ipixels_down, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&divergent_limit, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&julia, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&julia_constant.real, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&julia_constant.imaginary, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&alternate_equation, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&use_datatypes, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    if (myid == 0)
    {
	colors = malloc((num_colors+1)* sizeof(color_t));
	if (colors == NULL)
	{
	    MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(-1);
	}
	Make_color_array(num_colors, colors);
	colors[num_colors] = 0; /* add one on the top to avoid edge errors */
    }

    /* allocate memory */
    big_size = ipixels_across * ipixels_down * sizeof(int);
    if (myid == 0)
    {
	/* window memory should be allocated by MPI in case the provider requires it */
	error = MPI_Alloc_mem(big_size, MPI_INFO_NULL, &grid_array);
	if (error != MPI_SUCCESS)
	{
	    printf("Memory allocation failed for data array, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(-1);
	}
	/* allocate an array to put the workers tasks in */
	work = (int*)malloc(numprocs * sizeof(int) * 5);
	if (work == NULL)
	{
	    printf("Memory allocation failed for work array, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(-1);
	}
    }
    else
    {
	/* the non-root processes just need scratch space to store data in */
	if ( (grid_array = (int *)calloc(big_size, 1)) == NULL)
	{
	    printf("Memory allocation failed for data array, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(-1);
	}
	/* window memory should be allocated by MPI in case the provider requires it */
	error = MPI_Alloc_mem(5 * sizeof(int), MPI_INFO_NULL, &work);
	if (error != MPI_SUCCESS)
	{
	    printf("Memory allocation failed for work array, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(-1);
	}
    }

    if (myid == 0)
    {
	int istep, jstep;
	int i1[400], i2[400], j1[400], j2[400];
	int ii, jj;
	struct sockaddr_in addr;
	int len;
	char line[1024], *token;

	srand(getpid());

	if (!use_stdin)
	{
	    addr.sin_family = AF_INET;
	    addr.sin_addr.s_addr = INADDR_ANY;
	    addr.sin_port = htons(DEFAULT_PORT);

	    listener = socket(AF_INET, SOCK_STREAM, 0);
	    if (listener == -1)
	    {
		printf("unable to create a listener socket.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(-1);
	    }
	    if (bind(listener, &addr, sizeof(addr)) == -1)
	    {
		addr.sin_port = 0;
		if (bind(listener, &addr, sizeof(addr)) == -1)
		{
		    printf("unable to create a listener socket.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		    exit(-1);
		}
	    }
	    if (listen(listener, 1) == -1)
	    {
		printf("unable to listen.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(-1);
	    }
	    len = sizeof(addr);
	    getsockname(listener, &addr, &len);
	    
	    printf("%s listening on port %d\n", processor_name, ntohs(addr.sin_port));
	    fflush(stdout);

	    sock = accept(listener, NULL, NULL);
	    if (sock == -1)
	    {
		printf("unable to accept a socket connection.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(-1);
	    }
	    printf("accepted connection from visualization program.\n");
	    fflush(stdout);

#ifdef HAVE_WINDOWS_H
	    optval = 1;
	    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));
#endif

	    printf("sending image size to visualizer.\n");
	    sock_write(sock, &ipixels_across, sizeof(int));
	    sock_write(sock, &ipixels_down, sizeof(int));
	    sock_write(sock, &num_colors, sizeof(int));
	    sock_write(sock, &imax_iterations, sizeof(int));
	}

	error = MPI_Win_create(grid_array, big_size, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
	if (error != MPI_SUCCESS)
	{
	    printf("MPI_Win_create failed, error 0x%x\n", error);
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}

	for (;;)
	{
	    /* get x_min, x_max, y_min, and y_max */
	    if (use_stdin)
	    {
		printf("input xmin ymin xmax ymax max_iter, (0 0 0 0 0 to quit):\n");fflush(stdout);
		fgets(line, 1024, stdin);
		printf("read <%s> from stdin\n", line);fflush(stdout);
		token = strtok(line, " \n");
		x_min = atof(token);
		token = strtok(NULL, " \n");
		y_min = atof(token);
		token = strtok(NULL, " \n");
		x_max = atof(token);
		token = strtok(NULL, " \n");
		y_max = atof(token);
		token = strtok(NULL, " \n");
		imax_iterations = atoi(token);
	    }
	    else
	    {
		printf("reading xmin,ymin,xmax,ymax.\n");fflush(stdout);
		sock_read(sock, &x_min, sizeof(double));
		sock_read(sock, &y_min, sizeof(double));
		sock_read(sock, &x_max, sizeof(double));
		sock_read(sock, &y_max, sizeof(double));
		sock_read(sock, &imax_iterations, sizeof(int));
	    }
	    printf("x0,y0 = (%f, %f) x1,y1 = (%f,%f) max_iter = %d\n", x_min, y_min, x_max, y_max, imax_iterations);fflush(stdout);

	    /* break the work up into 400 pieces */
	    istep = ipixels_across / 20;
	    jstep = ipixels_down / 20;
	    if (istep < 1)
		istep = 1;
	    if (jstep < 1)
		jstep = 1;
	    k = 0;
	    for (i=0; i<20; i++)
	    {
		for (j=0; j<20; j++)
		{
		    i1[k] = MIN(istep * i, ipixels_across - 1);
		    i2[k] = MIN((istep * (i+1)) - 1, ipixels_across - 1);
		    j1[k] = MIN(jstep * j, ipixels_down - 1);
		    j2[k] = MIN((jstep * (j+1)) - 1, ipixels_down - 1);
		    k++;
		}
	    }

	    /* shuffle the work */
	    for (i=0; i<500; i++)
	    {
		ii = rand() % 400;
		jj = rand() % 400;
		swap(&i1[ii], &i1[jj]);
		swap(&i2[ii], &i2[jj]);
		swap(&j1[ii], &j1[jj]);
		swap(&j2[ii], &j2[jj]);
	    }

	    /*printf("bcasting the limits: (%f,%f)(%f,%f)\n", x_min, y_min, x_max, y_max);fflush(stdout);*/
	    /* let everyone know the limits */
	    MPI_Bcast(&x_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&x_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&y_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&y_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&imax_iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);

	    /* check for the end condition */
	    if (x_min == x_max && y_min == y_max)
	    {
		break;
	    }

	    /* put one piece of work to each worker for each epoch until the work is exhausted */
	    k = 0;
	    done = 0;
	    while (!done)
	    {
		error = MPI_Win_fence(0, win);
		if (error != MPI_SUCCESS)
		{
		    printf("'handout work' fence failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		/* hand out work */
		for (i=1; i<numprocs; i++)
		{
		    if (!done)
		    {
			work[(i*5)+0] = k+1;
			work[(i*5)+1] = i1[k]; /* imin */
			work[(i*5)+2] = i2[k]; /* imax */
			work[(i*5)+3] = j1[k]; /* jmin */
			work[(i*5)+4] = j2[k]; /* jmax */
		    }
		    else
		    {
			work[(i*5)+0] = -1;
			work[(i*5)+1] = -1;
			work[(i*5)+2] = -1;
			work[(i*5)+3] = -1;
			work[(i*5)+4] = -1;
		    }
		    /*printf("sending work(%d) to %d\n", k+1, cur_proc);fflush(stdout);*/
		    error = MPI_Put(&work[i*5], 5, MPI_INT, i, 0, 5, MPI_INT, win);
		    if (error != MPI_SUCCESS)
		    {
			printf("put failed, error 0x%x\n", error);
			MPI_Abort(MPI_COMM_WORLD, -1);
		    }
		    if (k<399)
			k++;
		    else
			done = 1;
		}
		error = MPI_Win_fence(0, win);
		if (error != MPI_SUCCESS)
		{
		    printf("'handout work' -> 'do work' fence failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		/* do work */
		error = MPI_Win_fence(0, win);
		if (error != MPI_SUCCESS)
		{
		    printf("'do work' -> 'collect results' fence failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		/* send the results to the visualizer */
		for (i=1; i<numprocs; i++)
		{
		    if (work[i*5] != -1)
		    {
			sock_write(sock, &work[i*5 + 1], 4*sizeof(int));
			for (j=work[i*5+3]; j<=work[i*5+4]; j++)
			{
			    sock_write(sock,
				&grid_array[(j*ipixels_across)+work[i*5+1]],
				(work[i*5+2]+1-work[i*5+1])*sizeof(int));
			}
		    }
		}
	    }
	    error = MPI_Win_fence(0, win);
	    if (error != MPI_SUCCESS)
	    {
		printf("'collect results' -> 'done work' fence failed, error 0x%x\n", error);
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    /* hand out "done" work */
	    for (i=1; i<numprocs; i++)
	    {
		work[(i*5)+0] = 0;
		work[(i*5)+1] = 0;
		work[(i*5)+2] = 0;
		work[(i*5)+3] = 0;
		work[(i*5)+4] = 0;

		error = MPI_Put(&work[i*5], 5, MPI_INT, i, 0, 5, MPI_INT, win);
		if (error != MPI_SUCCESS)
		{
		    printf("put failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
	    }
	    error = MPI_Win_fence(0, win);
	    if (error != MPI_SUCCESS)
	    {
		printf("'done work' -> 'done' fence failed, error 0x%x\n", error);
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }

	    /* tell the visualizer the image is done */
	    if (!use_stdin)
	    {
		work[0] = 0;
		work[1] = 0;
		work[2] = 0;
		work[3] = 0;
		sock_write(sock, work, 4 * sizeof(int));
	    }
	}
    }
    else
    {
	MPI_Datatype dtype;

	error = MPI_Win_create(work, 5*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
	if (error != MPI_SUCCESS)
	{
	    printf("MPI_Win_create failed, error 0x%x\n", error);
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
	for (;;)
	{
	    MPI_Bcast(&x_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&x_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&y_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&y_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&imax_iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);

	    /* check for the end condition */
	    if (x_min == x_max && y_min == y_max)
	    {
		break;
	    }

	    x_resolution = (x_max-x_min)/ ((double)ipixels_across);
	    y_resolution = (y_max-y_min)/ ((double)ipixels_down);

	    error = MPI_Win_fence(0, win);
	    if (error != MPI_SUCCESS)
	    {
		printf("'receive work' fence failed, error 0x%x\n", error);
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    /* receive work from the root */
	    error = MPI_Win_fence(0, win);
	    if (error != MPI_SUCCESS)
	    {
		printf("'receive work' -> 'do work' fence failed, error 0x%x\n", error);
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    while (work[0] != 0)
	    {
		imin = work[1];
		imax = work[2];
		jmin = work[3];
		jmax = work[4];

		if (use_datatypes)
		{
		    MPI_Type_vector(jmax - jmin + 1, /* rows */
			imax - imin + 1, /* column width */
			ipixels_across, /* stride, distance between rows */
			MPI_INT,
			&dtype);
		    MPI_Type_commit(&dtype);
		    k = 0;
		}

		for (j=jmin; j<=jmax; ++j)
		{
		    coord_point.imaginary = y_max - j*y_resolution; /* go top to bottom */

		    for (i=imin; i<=imax; ++i)
		    {
			/* Call Mandelbrot routine for each code, fill array with number of iterations. */

			coord_point.real = x_min + i*x_resolution; /* go left to right */
			if (julia == 1)
			{
			    /* doing Julia set */
			    /* julia eq:  z = z^2 + c, z_0 = grid coordinate, c = constant */
			    icount = single_mandelbrot_point(coord_point, julia_constant, imax_iterations, divergent_limit);
			}
			else if (alternate_equation == 1)
			{
			    /* doing experimental form 1 */
			    icount = subtractive_mandelbrot_point(coord_point, julia_constant, imax_iterations, divergent_limit);
			}
			else if (alternate_equation == 2)
			{
			    /* doing experimental form 2 */
			    icount = additive_mandelbrot_point(coord_point, julia_constant, imax_iterations, divergent_limit);
			}
			else
			{
			    /* default to doing Mandelbrot set */
			    /* mandelbrot eq: z = z^2 + c, z_0 = c, c = grid coordinate */
			    icount = single_mandelbrot_point(coord_point, coord_point, imax_iterations, divergent_limit);
			}
			if (use_datatypes)
			{
			    grid_array[k++] = icount;
			}
			else
			{
			    grid_array[(j*ipixels_across) + i] = icount;
			    error = MPI_Put(&grid_array[(j*ipixels_across) + i], 1, MPI_INT, 0, (j * ipixels_across) + i, 1, MPI_INT, win);
			    if (error != MPI_SUCCESS)
			    {
				printf("put failed, error 0x%x\n", error);
				MPI_Abort(MPI_COMM_WORLD, -1);
			    }
			}
		    }
		}
		if (use_datatypes)
		{
		    MPI_Put(grid_array, k, MPI_INT, 0, (jmin * ipixels_across) + imin, 1, dtype, win);
		}
		/* synch with the root */
		error = MPI_Win_fence(0, win);
		if (error != MPI_SUCCESS)
		{
		    printf("'do work' -> 'wait for work to be collected' fence failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		if (use_datatypes)
		{
		    MPI_Type_free(&dtype);
		}
		/* fence while the root writes to the visualizer. */
		error = MPI_Win_fence(0, win);
		if (error != MPI_SUCCESS)
		{
		    printf("'wait for work to be collected' -> 'receive work' fence failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		/* fence to allow the root to put the next piece of work */
		error = MPI_Win_fence(0, win);
		if (error != MPI_SUCCESS)
		{
		    printf("'receive work' -> 'do work' fence failed, error 0x%x\n", error);
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
	    }
	}
    }

    if (myid == 0 && save_image)
    {
	imax_iterations = 0;
	for (i=0; i<ipixels_across * ipixels_down; ++i)
	{
	    /* look for "brightest" pixel value, for image use */
	    if (grid_array[i] > imax_iterations)
		imax_iterations = grid_array[i];
	}

	if (julia == 0)
	    printf("Done calculating mandelbrot, now creating file\n");
	else
	    printf("Done calculating julia, now creating file\n");
	fflush(stdout);

	/* Print out the array in some appropriate form. */
	if (julia == 0)
	{
	    /* it's a mandelbrot */
	    sprintf(file_message, "Mandelbrot over (%lf-%lf,%lf-%lf), size %d x %d",
		x_min, x_max, y_min, y_max, ipixels_across, ipixels_down);
	}
	else
	{
	    /* it's a julia */
	    sprintf(file_message, "Julia over (%lf-%lf,%lf-%lf), size %d x %d, center (%lf, %lf)",
		x_min, x_max, y_min, y_max, ipixels_across, ipixels_down,
		julia_constant.real, julia_constant.imaginary);
	}

	dumpimage(filename, grid_array, ipixels_across, ipixels_down, imax_iterations, file_message, num_colors, colors);
    }

    MPI_Finalize();
    if (colors)
	free(colors);
    return 0;
} /* end of main */

void PrintUsage()
{
    printf("usage: mpiexec -n x pmandel [options]\n");
    printf("options:\n -xmin # -xmax #\n -ymin # -ymax #\n -depth #\n -xscale # -yscale #\n -out filename\n -i\n -nodtypes or -many_puts\n");
    printf("All options are optional.\n");
    printf("-i will allow you to input the min/max parameters from stdin and output the resulting image to a ppm file.");
    printf("  Otherwise the root process will listen for a separate visualizer program to connect to it.\n");
    printf("The defaults are:\n xmin %f, xmax %f\n ymin %f, ymax %f\n depth %d\n xscale %d, yscale %d\n %s\n",
	IDEAL_MIN_X_VALUE, IDEAL_MAX_X_VALUE, IDEAL_MIN_Y_VALUE, IDEAL_MAX_Y_VALUE, IDEAL_ITERATIONS,
	IDEAL_WIDTH, IDEAL_HEIGHT, default_filename);
    fflush(stdout);
}

int sock_write(int sock, void *buffer, int length)
{
    int result;
    int num_bytes;
    int total = 0;
    struct timeval t;
    fd_set set;

    while (length)
    {
	num_bytes = send(sock, buffer, length, 0);
	if (num_bytes == -1)
	{
#ifdef HAVE_WINDOWS_H
	    result = WSAGetLastError();
	    if (result == WSAEWOULDBLOCK)
	    {
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, &set, NULL, NULL, &t);
		continue;
	    }
#else
	    if (errno == EWOULDBLOCK)
	    {
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, &set, NULL, NULL, &t);
		continue;
	    }
#endif
	    return total;
	}
	length -= num_bytes;
	buffer = (char*)buffer + num_bytes;
	total += num_bytes;
    }
    return total;
}

int sock_read(int sock, void *buffer, int length)
{
    int result;
    int num_bytes;
    int total = 0;
    struct timeval t;
    fd_set set;

    while (length)
    {
	num_bytes = recv(sock, buffer, length, 0);
	if (num_bytes == -1)
	{
#ifdef HAVE_WINDOWS_H
	    result = WSAGetLastError();
	    if (result == WSAEWOULDBLOCK)
	    {
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, &set, NULL, NULL, &t);
		continue;
	    }
#else
	    if (errno == EWOULDBLOCK)
	    {
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, &set, NULL, NULL, &t);
		continue;
	    }
#endif
	    return total;
	}
	length -= num_bytes;
	buffer = (char*)buffer + num_bytes;
	total += num_bytes;
    }
    return total;
}

color_t getColor(double fraction, double intensity)
{
    /* fraction is a part of the rainbow (0.0 - 1.0) = (Red-Yellow-Green-Cyan-Blue-Magenta-Red)
    intensity (0.0 - 1.0) 0 = black, 1 = full color, 2 = white
    */
    double red, green, blue;
    int r,g,b;
    double dtemp;

    fraction = fabs(modf(fraction, &dtemp));

    if (intensity > 2.0)
	intensity = 2.0;
    if (intensity < 0.0)
	intensity = 0.0;

    dtemp = 1.0/6.0;

    if (fraction < 1.0/6.0)
    {
	red = 1.0;
	green = fraction / dtemp;
	blue = 0.0;
    }
    else
    {
	if (fraction < 1.0/3.0)
	{
	    red = 1.0 - ((fraction - dtemp) / dtemp);
	    green = 1.0;
	    blue = 0.0;
	}
	else
	{
	    if (fraction < 0.5)
	    {
		red = 0.0;
		green = 1.0;
		blue = (fraction - (dtemp*2.0)) / dtemp;
	    }
	    else
	    {
		if (fraction < 2.0/3.0)
		{
		    red = 0.0;
		    green = 1.0 - ((fraction - (dtemp*3.0)) / dtemp);
		    blue = 1.0;
		}
		else
		{
		    if (fraction < 5.0/6.0)
		    {
			red = (fraction - (dtemp*4.0)) / dtemp;
			green = 0.0;
			blue = 1.0;
		    }
		    else
		    {
			red = 1.0;
			green = 0.0;
			blue = 1.0 - ((fraction - (dtemp*5.0)) / dtemp);
		    }
		}
	    }
	}
    }

    if (intensity > 1)
    {
	intensity = intensity - 1.0;
	red = red + ((1.0 - red) * intensity);
	green = green + ((1.0 - green) * intensity);
	blue = blue + ((1.0 - blue) * intensity);
    }
    else
    {
	red = red * intensity;
	green = green * intensity;
	blue = blue * intensity;
    }

    r = (int)(red * 255.0);
    g = (int)(green * 255.0);
    b = (int)(blue * 255.0);

    return RGBtocolor_t(r,g,b);
}

int Make_color_array(int num_colors, color_t colors[])
{
    double fraction, intensity;
    int i;

    intensity = 1.0;
    for (i=0; i<num_colors; i++)
    {
	fraction = (double)i / (double)num_colors;
	colors[i] = getColor(fraction, intensity);
    }
    return 0;
}

void getRGB(color_t color, int *r, int *g, int *b)
{
    *r = getR(color);
    *g = getG(color);
    *b = getB(color);
}

void read_mand_args(int argc, char *argv[], int *o_max_iterations,
		    int *o_pixels_across, int *o_pixels_down,
		    double *o_x_min, double *o_x_max,
		    double *o_y_min, double *o_y_max, int *o_julia,
		    double *o_julia_real_x, double *o_julia_imaginary_y,
		    double *o_divergent_limit, int *o_alternate,
		    char *filename, int *o_num_colors, int *use_stdin,
		    int *save_image, int *use_datatypes)
{	
    int i;

    *o_julia_real_x = NOVALUE;
    *o_julia_imaginary_y = NOVALUE;

    /* set defaults */
    *o_max_iterations = IDEAL_ITERATIONS;
    *o_pixels_across = IDEAL_WIDTH;
    *o_pixels_down = IDEAL_HEIGHT;
    *o_x_min = IDEAL_MIN_X_VALUE;
    *o_x_max = IDEAL_MAX_X_VALUE;
    *o_y_min = IDEAL_MIN_Y_VALUE;
    *o_y_max = IDEAL_MAX_Y_VALUE;
    *o_divergent_limit = INFINITE_LIMIT;
    strcpy(filename, default_filename);
    *o_num_colors = IDEAL_ITERATIONS;
    *use_stdin = 0; /* default is to listen for a controller */
    *save_image = NOVALUE;
    *use_datatypes = 1;

    *o_julia = 0; /* default is "generate Mandelbrot" */
    *o_alternate = 0; /* default is still "generate Mandelbrot" */
    *o_divergent_limit = INFINITE_LIMIT; /* default total range is assumed
					 if not explicitly overwritten */

    /* We just cycle through all given parameters, matching what we can.
    Note that we force casting, because we expect that a nonsensical
    value is better than a poorly formatted one (fewer crashes), and
    we'll later sort out the good from the bad
    */

    for (i = 0; i < argc; ++i) /* grab command line arguments */
	if (strcmp(argv[i], "-xmin\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_x_min);
	else if (strcmp(argv[i], "-xmax\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_x_max);
	else if (strcmp(argv[i], "-ymin\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_y_min);
	else if (strcmp(argv[i], "-ymax\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_y_max);
	else if (strcmp(argv[i], "-depth\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%d", &*o_max_iterations);
	else if (strcmp(argv[i], "-xscale\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%d", &*o_pixels_across);
	else if (strcmp(argv[i], "-yscale\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%d", &*o_pixels_down);
	else if (strcmp(argv[i], "-diverge\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_divergent_limit);
	else if (strcmp(argv[i], "-jx\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_julia_real_x);
	else if (strcmp(argv[i], "-jy\0") == 0 && argv[i+1] != NULL) 
	    sscanf(argv[i+1], "%lf", &*o_julia_imaginary_y);
	else if (strcmp(argv[i], "-alternate\0") == 0 && argv[i+1] != NULL)
	    sscanf(argv[i+1], "%d", &*o_alternate);
	else if (strcmp(argv[i], "-julia\0") == 0)
	    *o_julia = 1;
	else if (strcmp(argv[i], "-out\0") == 0 && argv[i+1] != NULL)
	    strcpy(filename, argv[i+1]);
	else if (strcmp(argv[i], "-colors\0") == 0 && argv[i+1] != NULL)
	    sscanf(argv[i+1], "%d", &*o_num_colors);
	else if (strcmp(argv[i], "-i\0") == 0)
	    *use_stdin = 1;
	else if (strcmp(argv[i], "-nodtypes\0") == 0)
	    *use_datatypes = 0;
	else if (strcmp(argv[i], "-many_puts\0") == 0)
	    *use_datatypes = 0;
	else if (strcmp(argv[i], "-save\0") == 0)
	    *save_image = 1;
	else if (strcmp(argv[i], "-nosave\0") == 0)
	    *save_image = 0;
	else if (strcmp(argv[i], "-help\0") == 0)
	{
	    PrintUsage();
	    MPI_Finalize();
	    exit(0);
	}

	if (*save_image == NOVALUE)
	{
	    if (*use_stdin == 1)
		*save_image = 1;
	    else
		*save_image = 0;
	}
#if DEBUG2
	if (myid == 0)
	{
	    printf("Doing %d iterations over (%.2lf:%.2lf,%.2lf:%.2lf) on a %d x %d grid\n",
		*o_max_iterations, *o_x_min,*o_x_max,*o_y_min,*o_y_max,
		*o_pixels_across, *o_pixels_down);
	}
#endif
}

void check_mand_params(int *m_max_iterations,
		       int *m_pixels_across, int *m_pixels_down,
		       double *m_x_min, double *m_x_max,
		       double *m_y_min, double *m_y_max,
		       double *m_divergent_limit)
{
    /* Get first batch of limits */
#if PROMPT
    if (*m_x_min == NOVALUE || *m_x_max == NOVALUE)
    {
	/* grab unspecified limits */
	printf("Enter lower and higher limits on x (within -1 to 1): ");
	scanf("%lf %lf", &*m_x_min, &*m_x_max);
    }

    if (*m_y_min == NOVALUE || *m_y_max == NOVALUE)
    {
	/* grab unspecified limits */
	printf("Enter lower and higher limits on y (within -1 to 1): ");
	scanf("%lf %lf", &*m_y_min, &*m_y_max);
    }
#endif

    /* Verify limits are reasonable */

    if (*m_x_min < MIN_X_VALUE || *m_x_min > *m_x_max)
    {
	printf("Chosen lower x limit is too low, resetting to %lf\n",MIN_X_VALUE);
	*m_x_min = MIN_X_VALUE;
    }
    if (*m_x_max > MAX_X_VALUE || *m_x_max <= *m_x_min)
    {
	printf("Chosen upper x limit is too high, resetting to %lf\n",MAX_X_VALUE);
	*m_x_max = MAX_X_VALUE;
    }
    if (*m_y_min < MIN_Y_VALUE || *m_y_min > *m_y_max)
    {
	printf("Chosen lower y limit is too low, resetting to %lf\n",MIN_Y_VALUE);
	*m_y_min = MIN_Y_VALUE;
    }
    if (*m_y_max > MAX_Y_VALUE || *m_y_max <= *m_y_min)
    {
	printf("Chosen upper y limit is too high, resetting to %lf\n",MAX_Y_VALUE);
	*m_y_max = MAX_Y_VALUE;
    }

    /* Get second set of limits */
#if PROMPT

    if (*m_max_iterations == NOVALUE)
    {
	/* grab unspecified limit */
	printf("Enter max interations to do: ");
	scanf("%d",&*m_max_iterations);
    }
#endif

    /* Verify second set of limits */

    if (*m_max_iterations > MAX_ITERATIONS || *m_max_iterations < 0)
    {
	printf("Warning, unreasonable number of iterations, setting to %d\n", MAX_ITERATIONS);
	*m_max_iterations = MAX_ITERATIONS;
    }

    /* Get third set of limits */
#if PROMPT
    if (*m_pixels_across == NOVALUE || *m_pixels_down == NOVALUE)
    {
	/* grab unspecified limits */
	printf("Enter pixel size (horizonal by vertical, i.e. 256 256): ");
	scanf(" %d %d", &*m_pixels_across, &*m_pixels_down);
    }
#endif

    /* Verify third set of limits */

    if (*m_pixels_across > MAX_WIDTH)
    {
	printf("Warning, image requested is too wide, setting to %d pixel width\n", MAX_WIDTH);
	*m_pixels_across = MAX_WIDTH;
    }
    if (*m_pixels_down > MAX_HEIGHT)
    {
	printf("Warning, image requested is too tall, setting to %d pixel height\n", MAX_HEIGHT);
	*m_pixels_down = MAX_HEIGHT;
    }

#if DEBUG2
    printf("%d iterations over (%.2lf:%.2lf,%.2lf:%.2lf), %d x %d grid, diverge value %lf\n",
	*m_max_iterations, *m_x_min,*m_x_max,*m_y_min,*m_y_max,
	*m_pixels_across, *m_pixels_down, *m_divergent_limit);
#endif
}

void check_julia_params(double *julia_x_constant, double *julia_y_constant)
{	

    /* Hey, we're not stupid-- if they must Prompt, we will also be Noisy */
#if PROMPT
    if (*julia_x_constant == NOVALUE || *julia_y_constant == NOVALUE)
    {
	/* grab unspecified limits */
	printf("Enter Coordinates for Julia expansion: ");
	scanf("%lf %lf", &*julia_x_constant, &*julia_y_constant);
    }
#endif

#if DEBUG2
    /* In theory, any point can be investigated, although it's not much point
    if it's outside of the area viewed.  But, hey, that's not our problem */
    printf("Exploring julia set around (%lf, %lf)\n", *julia_x_constant, *julia_y_constant);
#endif
}

/* This is a Mandelbrot variant, solving the code for the equation:
z = z(1-z)
*/

/* This routine takes a complex coordinate point (x+iy) and a value stating
what the upper limit to the number of iterations is.  It eventually
returns an integer of how many counts the code iterated for within
the given point/region until the exit condition ( abs(x+iy) > limit) was met.
This value is returned as an integer.
*/

int subtractive_mandelbrot_point(complex_t coord_point, 
				 complex_t c_constant,
				 int Max_iterations, double divergent_limit)
{
    complex_t z_point, a_point; /* we need 2 pts to use in our calculation */
    int num_iterations;  /* a counter to track the number of iterations done */

    num_iterations = 0; /* zero our counter */
    z_point = coord_point; /* initialize to the given start coordinate */

    /* loop while the absolute value of the complex coordinate is < our limit
    (for a mandelbrot) or until we've done our specified maximum number of
    iterations (both julia and mandelbrot) */

    while (absolute_complex(z_point) < divergent_limit &&
	num_iterations < Max_iterations)
    {
	/* z = z(1-z) */
	a_point.real = 1.0; a_point.imaginary = 0.0; /* make "1" */
	a_point = subtract_complex(a_point,z_point);
	z_point = multiply_complex(z_point,a_point);

	++num_iterations;
    } /* done iterating for one point */

    return num_iterations;
}

/* This is a Mandelbrot variant, solving the code for the equation:
z = z(z+c)
*/

/* This routine takes a complex coordinate point (x+iy) and a value stating
what the upper limit to the number of iterations is.  It eventually
returns an integer of how many counts the code iterated for within
the given point/region until the exit condition ( abs(x+iy) > limit) was met.
This value is returned as an integer.
*/

int additive_mandelbrot_point(complex_t coord_point, 
			      complex_t c_constant,
			      int Max_iterations, double divergent_limit)
{
    complex_t z_point, a_point; /* we need 2 pts to use in our calculation */
    int num_iterations;  /* a counter to track the number of iterations done */

    num_iterations = 0; /* zero our counter */
    z_point = coord_point; /* initialize to the given start coordinate */

    /* loop while the absolute value of the complex coordinate is < our limit
    (for a mandelbrot) or until we've done our specified maximum number of
    iterations (both julia and mandelbrot) */

    while (absolute_complex(z_point) < divergent_limit &&
	num_iterations < Max_iterations)
    {
	/* z = z(z+C) */
	a_point = add_complex(z_point,coord_point);
	z_point = multiply_complex(z_point,a_point);

	++num_iterations;
    } /* done iterating for one point */

    return num_iterations;
}

/* This is a Mandelbrot variant, solving the code for the equation:
z = z(z+1)
*/

/* This routine takes a complex coordinate point (x+iy) and a value stating
what the upper limit to the number of iterations is.  It eventually
returns an integer of how many counts the code iterated for within
the given point/region until the exit condition ( abs(x+iy) > limit) was met.
This value is returned as an integer.
*/

int exponential_mandelbrot_point(complex_t coord_point, 
				 complex_t c_constant,
				 int Max_iterations, double divergent_limit)
{
    complex_t z_point, a_point; /* we need 2 pts to use in our calculation */
    int num_iterations;  /* a counter to track the number of iterations done */

    num_iterations = 0; /* zero our counter */
    z_point = coord_point; /* initialize to the given start coordinate */

    /* loop while the absolute value of the complex coordinate is < our limit
    (for a mandelbrot) or until we've done our specified maximum number of
    iterations (both julia and mandelbrot) */

    while (absolute_complex(z_point) < divergent_limit &&
	num_iterations < Max_iterations)
    {
	/* z = z(1-z) */
	a_point.real = 1.0; a_point.imaginary = 0.0; /* make "1" */
	a_point = subtract_complex(a_point,z_point);
	z_point = multiply_complex(z_point,a_point);

	++num_iterations;
    } /* done iterating for one point */

    return num_iterations;
}

void complex_points_to_image(complex_t in_julia_coord_set[],
			     int in_julia_set_size,
			     int i_pixels_across,int i_pixels_down,
			     int *out_image_final, int *out_max_colors)
{
    int i, i_temp_quantize;
    double x_resolution_element, y_resolution_element, temp_quantize;
    double x_max, x_min, y_max, y_min;

    /* Convert the complex points into an image--

    first, find the min and max for each axis. */

    x_min = in_julia_coord_set[0].real;
    x_max = x_min;
    y_min = in_julia_coord_set[0].imaginary;
    y_max = y_min;

    for (i=0;i<in_julia_set_size;++i)
    {
	if (in_julia_coord_set[i].real < x_min)
	    x_min = in_julia_coord_set[i].real;
	if (in_julia_coord_set[i].real > x_max)
	    x_max = in_julia_coord_set[i].real;
	if (in_julia_coord_set[i].imaginary < y_min)
	    y_min = in_julia_coord_set[i].imaginary;
	if (in_julia_coord_set[i].imaginary > y_max)
	    y_max = in_julia_coord_set[i].imaginary;
    }

    /* convert to fit image grid size: */

    x_resolution_element =  (x_max - x_min)/(double)i_pixels_across;
    y_resolution_element =  (y_max - y_min)/(double)i_pixels_down;

#if DEBUG
    printf("%lf %lf\n",x_resolution_element, y_resolution_element);
#endif


    /* now we can put each point into a grid space, and up the count of
    said grid.  Since calloc initialized it to zero, this is safe */
    /* A point x,y goes to grid region i,j, where
    xi =  (x-xmin)/x_resolution   (position relative to far left)
    yj =  (ymax-y)/y_resolution   (position relative to top)
    This gets mapped to our 1-d array as  xi + yj*i_pixels_across,
    taken as an integer (rounding down) and checking array limits
    */

    for (i=0; i<in_julia_set_size; ++i)
    {
	temp_quantize =
	    (in_julia_coord_set[i].real - x_min)/x_resolution_element +
	    (y_max -  in_julia_coord_set[i].imaginary)/y_resolution_element *
	    (double)i_pixels_across;
	i_temp_quantize = (int)temp_quantize;
	if (i_temp_quantize > i_pixels_across*i_pixels_down)
	    i_temp_quantize = i_pixels_across*i_pixels_down;

#if DEBUG
	printf("%d %lf %lf %lf %lf %lf %lf %lf\n",
	    i_temp_quantize, temp_quantize, x_min, x_resolution_element,
	    in_julia_coord_set[i].real, y_max, y_resolution_element,
	    in_julia_coord_set[i].imaginary);
#endif

	++out_image_final[i_temp_quantize];
    }

    /* find the highest value (most occupied bin) */
    *out_max_colors = 0;
    for (i=0;i<i_pixels_across*i_pixels_down;++i)
    {
	if (out_image_final[i] > *out_max_colors)
	{
	    *out_max_colors = out_image_final[i];
	}
    }
}

complex_t exponential_complex(complex_t in_complex)
{
    complex_t out_complex;
    double e_to_x;
    /* taking the exponential,   e^(x+iy) = e^xe^iy = e^x(cos(y)+isin(y) */
    e_to_x = exp(in_complex.real);
    out_complex.real = e_to_x * cos(in_complex.imaginary);
    out_complex.imaginary = e_to_x * sin(in_complex.imaginary);
    return out_complex;
}

complex_t multiply_complex(complex_t in_one_complex, complex_t in_two_complex)
{
    complex_t out_complex;
    /* multiplying complex variables-- (x+iy) * (a+ib) = xa - yb + i(xb + ya) */
    out_complex.real = in_one_complex.real * in_two_complex.real -
	in_one_complex.imaginary * in_two_complex.imaginary;
    out_complex.imaginary = in_one_complex.real * in_two_complex.imaginary +
	in_one_complex.imaginary * in_two_complex.real;
    return out_complex;
}

complex_t divide_complex(complex_t in_one_complex, complex_t in_two_complex)
{
    complex_t out_complex;
    double divider;
    /* dividing complex variables-- (x+iy)/(a+ib) = (xa - yb)/(a^2+b^2)
    + i (y*a-x*b)/(a^2+b^2) */
    divider = (in_two_complex.real*in_two_complex.real -
	in_two_complex.imaginary*in_two_complex.imaginary);
    out_complex.real = (in_one_complex.real * in_two_complex.real -
	in_one_complex.imaginary * in_two_complex.imaginary) / divider;
    out_complex.imaginary = (in_one_complex.imaginary * in_two_complex.real -
	in_one_complex.real * in_two_complex.imaginary) / divider;
    return out_complex;
}

complex_t inverse_complex(complex_t in_complex)
{
    complex_t out_complex;
    double divider;
    /* inverting a complex variable: 1/(x+iy) */
    divider = (in_complex.real*in_complex.real -
	in_complex.imaginary*in_complex.imaginary);
    out_complex.real = (in_complex.real - in_complex.imaginary) / divider;
    out_complex.imaginary = (in_complex.real - in_complex.imaginary) / divider;
    return out_complex;
}

complex_t add_complex(complex_t in_one_complex, complex_t in_two_complex)
{
    complex_t out_complex;
    /* adding complex variables-- do by component */
    out_complex.real = in_one_complex.real + in_two_complex.real;
    out_complex.imaginary = in_one_complex.imaginary + in_two_complex.imaginary;
    return out_complex;
}

complex_t subtract_complex(complex_t in_one_complex, complex_t in_two_complex)
{
    complex_t out_complex;
    /* subtracting complex variables-- do by component */
    out_complex.real = in_one_complex.real - in_two_complex.real;
    out_complex.imaginary = in_one_complex.imaginary - in_two_complex.imaginary;
    return out_complex;
}

double absolute_complex(complex_t in_complex)
{
    double out_double;
    /* absolute value is (for x+yi)  sqrt( x^2 + y^2) */
    out_double = sqrt(in_complex.real*in_complex.real + in_complex.imaginary*in_complex.imaginary);
    return out_double;
}

/* This routine takes a complex coordinate point (x+iy) and a value stating
what the upper limit to the number of iterations is.  It eventually
returns an integer of how many counts the code iterated for within
the given point/region until the exit condition ( abs(x+iy) > limit) was met.
This value is returned as an integer.
*/

int single_mandelbrot_point(complex_t coord_point, 
			    complex_t c_constant,
			    int Max_iterations, double divergent_limit)
{
    complex_t z_point;  /* we need a point to use in our calculation */
    int num_iterations;  /* a counter to track the number of iterations done */

    num_iterations = 0; /* zero our counter */
    z_point = coord_point; /* initialize to the given start coordinate */

    /* loop while the absolute value of the complex coordinate is < our limit
    (for a mandelbrot) or until we've done our specified maximum number of
    iterations (both julia and mandelbrot) */

    while (absolute_complex(z_point) < divergent_limit &&
	num_iterations < Max_iterations)
    {
	/* z = z*z + c */
	z_point = multiply_complex(z_point,z_point);
	z_point = add_complex(z_point,c_constant);

	++num_iterations;
    } /* done iterating for one point */

    return num_iterations;
}

/* Writes out a linear integer array of grayscale pixel values to a
pgm-format file (with header).  The exact format details are given
at the end of this routine in case you don't have the man pages
installed locally.  Essentially, it's a 2-dimensional integer array
of pixel grayscale values.  Very easy to manipulate externally.
*/

/* You need the following inputs:
A linear integer array with the actual pixel values (read in as
consecutive rows),
The width and height of the grid, and
The maximum pixel value (to set greyscale range).  We are assuming
that the lowest value is "0".
*/

void dumpimage(char *filename, int in_grid_array[], int in_pixels_across, int in_pixels_down,
	       int in_max_pixel_value, char input_string[], int num_colors, color_t colors[])
{
    FILE *ifp;
    int i, j, k;
#ifdef USE_PPM
    int r, g, b;
#endif

    printf("%s\nwidth: %d\nheight: %d\ncolors: %d\nstr: %s\n",
	filename, in_pixels_across, in_pixels_down, num_colors, input_string);
    fflush(stdout);

    if ( (ifp=fopen(filename, "w")) == NULL)
    {
	printf("Error, could not open output file\n");
	MPI_Abort(MPI_COMM_WORLD, -1);
	exit(-1);
    }

#ifdef USE_PPM
    fprintf(ifp, "P3\n");  /* specifies type of file, in this case ppm */
    fprintf(ifp, "# %s\n", input_string);  /* an arbitrary file identifier */
    /* now give the file size in pixels by pixels */
    fprintf(ifp, "%d %d\n", in_pixels_across, in_pixels_down);
    /* give the max r,g,b level */
    fprintf(ifp, "255\n");

    k=0; /* counter for the linear array of the final image */
    /* assumes first point is upper left corner (element 0 of array) */

    if (in_max_pixel_value < 1)
    {
	for (j=0; j<in_pixels_down; ++j) /* start at the top row and work down */
	{
	    for (i=0; i<in_pixels_across; ++i) /* go along the row */
	    {
		fprintf(ifp, "0 0 0 ");
	    }
	    fprintf(ifp, "\n"); /* done writing one row, begin next line */
	}
    }
    else
    {
	for (j=0; j<in_pixels_down; ++j) /* start at the top row and work down */
	{
	    for (i=0; i<in_pixels_across; ++i) /* go along the row */
	    {
		getRGB(colors[(in_grid_array[k] * num_colors) / in_max_pixel_value], &r, &g, &b);
		fprintf(ifp, "%d %d %d ", r, g, b); /* +1 since 0 = first color */
		++k;
	    }
	    fprintf(ifp, "\n"); /* done writing one row, begin next line */
	}
    }
#else
    fprintf(ifp, "P2\n");  /* specifies type of file, in this case pgm */
    fprintf(ifp, "# %s\n", input_string);  /* an arbitrary file identifier */
    /* now give the file size in pixels by pixels */
    fprintf(ifp, "%d %d\n", in_pixels_across, in_pixels_down);
    /* gives max number of grayscale levels */
    fprintf(ifp, "%d\n", in_max_pixel_value+1); /* plus 1 because 0=first color */

    k=0; /* counter for the linear array of the final image */
    /* assumes first point is upper left corner (element 0 of array) */

    for (j=0;j<in_pixels_down;++j) /* start at the top row and work down */
    {
	for (i=0;i<in_pixels_across;++i) /* go along the row */
	{
	    fprintf(ifp, "%d ", in_grid_array[k]+1); /* +1 since 0 = first color */
	    ++k;
	}
	fprintf(ifp, "\n"); /* done writing one row, begin next line */
    }
#endif

    fclose(ifp);
}
