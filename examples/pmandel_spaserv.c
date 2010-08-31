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

#define DEFAULT_NUM_SLAVES 3 /* default number of children to spawn */
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
		    int *save_image, int *num_children);
void check_mand_params(int *m_max_iterations,
		       int *m_pixels_across, int *m_pixels_down,
		       double *m_x_min, double *m_x_max,
		       double *m_y_min, double *m_y_max,
		       double *m_divergent_limit,
		       int *m_num_children);
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
void output_data(int *in_grid_array, int coord[4], int *out_grid_array, int width, int height);
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
MPI_Comm comm;

void swap(int *i, int *j)
{
    int x;
    x = *i;
    *i = *j;
    *j = x;
}

typedef struct header_t
{
    int data[5];
} header_t;

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
    int work[5];
    header_t *result = NULL;
    /* make an integer array of size [N x M] to hold answers. */
    int *in_grid_array, *out_grid_array = NULL;
    int numprocs;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int num_colors;
    color_t *colors = NULL;
    MPI_Status status;
    int save_image = 0;
    int num_children = DEFAULT_NUM_SLAVES;
    int master = 1;
    MPI_Comm parent, *child_comm = NULL;
    MPI_Request *child_request = NULL;
    int error_code, error;
    char error_str[MPI_MAX_ERROR_STRING];
    int length;
    int index;
    char mpi_port[MPI_MAX_PORT_NAME];
    MPI_Info info;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_get_parent(&parent);
    MPI_Get_processor_name(processor_name, &namelen);

    if (parent == MPI_COMM_NULL)
    {
	if (numprocs > 1)
	{
	    printf("Error: only one process allowed for the master.\n");
	    PrintUsage();
	    error_code = MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(error_code);
	}

	printf("Welcome to the Mandelbrot/Julia set explorer.\n");

	master = 1;

	/* Get inputs-- region to view (must be within x/ymin to x/ymax, make sure
	xmax>xmin and ymax>ymin) and resolution (number of pixels along an edge,
	N x M, i.e. 256x256)
	*/

	read_mand_args(argc, argv, &imax_iterations, &ipixels_across, &ipixels_down,
	    &x_min, &x_max, &y_min, &y_max, &julia, &julia_constant.real,
	    &julia_constant.imaginary, &divergent_limit,
	    &alternate_equation, filename, &num_colors, &use_stdin, &save_image,
	    &num_children);
	check_mand_params(&imax_iterations, &ipixels_across, &ipixels_down,
	    &x_min, &x_max, &y_min, &y_max, &divergent_limit, &num_children);

	if (julia == 1) /* we're doing a julia figure */
	    check_julia_params(&julia_constant.real, &julia_constant.imaginary);

	/* spawn slaves */
	child_comm = (MPI_Comm*)malloc(num_children * sizeof(MPI_Comm));
	child_request = (MPI_Request*)malloc(num_children * sizeof(MPI_Request));
	result = (header_t*)malloc(num_children * sizeof(header_t));
	if (child_comm == NULL || child_request == NULL || result == NULL)
	{
	    printf("Error: unable to allocate an array of %d communicators, requests and work objects for the slaves.\n", num_children);
	    error_code = MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(error_code);
	}
	printf("Spawning %d slaves.\n", num_children);
	for (i=0; i<num_children; i++)
	{
	    error = MPI_Comm_spawn(argv[0], MPI_ARGV_NULL, 1,
		MPI_INFO_NULL, 0, MPI_COMM_WORLD, &child_comm[i], &error_code);
	    if (error != MPI_SUCCESS)
	    {
		error_str[0] = '\0';
		length = MPI_MAX_ERROR_STRING;
		MPI_Error_string(error, error_str, &length);
		printf("Error: MPI_Comm_spawn failed: %s\n", error_str);
		error_code = MPI_Abort(MPI_COMM_WORLD, -1);
		exit(error_code);
	    }
	}

	/* send out parameters */
	for (i=0; i<num_children; i++)
	{
	    MPI_Send(&num_colors, 1, MPI_INT, 0, 0, child_comm[i]);
	    MPI_Send(&imax_iterations, 1, MPI_INT, 0, 0, child_comm[i]);
	    MPI_Send(&ipixels_across, 1, MPI_INT, 0, 0, child_comm[i]);
	    MPI_Send(&ipixels_down, 1, MPI_INT, 0, 0, child_comm[i]);
	    MPI_Send(&divergent_limit, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
	    MPI_Send(&julia, 1, MPI_INT, 0, 0, child_comm[i]);
	    MPI_Send(&julia_constant.real, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
	    MPI_Send(&julia_constant.imaginary, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
	    MPI_Send(&alternate_equation, 1, MPI_INT, 0, 0, child_comm[i]);
	}
    }
    else
    {
	master = 0;
	MPI_Recv(&num_colors, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&imax_iterations, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&ipixels_across, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&ipixels_down, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&divergent_limit, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&julia, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&julia_constant.real, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&julia_constant.imaginary, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	MPI_Recv(&alternate_equation, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
    }

    if (master)
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
    if ( (in_grid_array = (int *)calloc(ipixels_across * ipixels_down, sizeof(int))) == NULL)
    {
	printf("Memory allocation failed for data array, aborting.\n");
	MPI_Abort(MPI_COMM_WORLD, -1);
	exit(-1);
    }

    if (master)
    {
	int istep, jstep;
	int i1[400], i2[400], j1[400], j2[400];
	int ii, jj;
	char line[1024], *token;

	if ( (out_grid_array = (int *)calloc(ipixels_across * ipixels_down, sizeof(int))) == NULL)
	{
	    printf("Memory allocation failed for data array, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	    exit(-1);
	}

	srand(getpid());

	if (!use_stdin)
	{
	    error = MPI_Info_create(&info);
	    if (error != MPI_SUCCESS)
	    {
		printf("Unable to create an MPI_Info to be used in MPI_Open_port.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    error = MPI_Open_port(info, mpi_port);
	    if (error != MPI_SUCCESS)
	    {
		printf("MPI_Open_port failed, aborting.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(-1);
	    }
	    printf("%s listening on port:\n%s\n", processor_name, mpi_port);
	    fflush(stdout);

	    error = MPI_Comm_accept(mpi_port, info, 0, MPI_COMM_WORLD, &comm);
	    if (error != MPI_SUCCESS)
	    {
		printf("MPI_Comm_accept failed, aborting.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(-1);
	    }
	    printf("accepted connection from visualization program.\n");
	    fflush(stdout);

	    printf("sending image size to visualizer.\n");
	    error = MPI_Send(&ipixels_across, 1, MPI_INT, 0, 0, comm);
	    if (error != MPI_SUCCESS)
	    {
		printf("Unable to send pixel width to visualizer, aborting.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    error = MPI_Send(&ipixels_down, 1, MPI_INT, 0, 0, comm);
	    if (error != MPI_SUCCESS)
	    {
		printf("Unable to send pixel height to visualizer, aborting.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    error = MPI_Send(&num_colors, 1, MPI_INT, 0, 0, comm);
	    if (error != MPI_SUCCESS)
	    {
		printf("Unable to send num_colors to visualizer, aborting.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	    error = MPI_Send(&imax_iterations, 1, MPI_INT, 0, 0, comm);
	    if (error != MPI_SUCCESS)
	    {
		printf("Unable to send imax_iterations to visualizer, aborting.\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	    }
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
		/*sscanf(line, "%g %g %g %g", &x_min, &y_min, &x_max, &y_max);*/
		/*scanf("%g %g %g %g", &x_min, &y_min, &x_max, &y_max);*/
	    }
	    else
	    {
		printf("reading xmin,ymin,xmax,ymax.\n");fflush(stdout);
		error = MPI_Recv(&x_min, 1, MPI_DOUBLE, 0, 0, comm, &status);
		if (error != MPI_SUCCESS)
		{
		    printf("Unable to receive x_min from the visualizer, aborting.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		error = MPI_Recv(&y_min, 1, MPI_DOUBLE, 0, 0, comm, &status);
		if (error != MPI_SUCCESS)
		{
		    printf("Unable to receive y_min from the visualizer, aborting.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		error = MPI_Recv(&x_max, 1, MPI_DOUBLE, 0, 0, comm, &status);
		if (error != MPI_SUCCESS)
		{
		    printf("Unable to receive x_max from the visualizer, aborting.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		error = MPI_Recv(&y_max, 1, MPI_DOUBLE, 0, 0, comm, &status);
		if (error != MPI_SUCCESS)
		{
		    printf("Unable to receive y_max from the visualizer, aborting.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
		error = MPI_Recv(&imax_iterations, 1, MPI_INT, 0, 0, comm, &status);
		if (error != MPI_SUCCESS)
		{
		    printf("Unable to receive imax_iterations from the visualizer, aborting.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
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

	    /*printf("sending the limits: (%f,%f)(%f,%f)\n", x_min, y_min, x_max, y_max);fflush(stdout);*/
	    /* let everyone know the limits */
	    for (i=0; i<num_children; i++)
	    {
		MPI_Send(&x_min, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
		MPI_Send(&x_max, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
		MPI_Send(&y_min, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
		MPI_Send(&y_max, 1, MPI_DOUBLE, 0, 0, child_comm[i]);
		MPI_Send(&imax_iterations, 1, MPI_INT, 0, 0, child_comm[i]);
	    }

	    /* check for the end condition */
	    if (x_min == x_max && y_min == y_max)
	    {
		break;
	    }

	    /* send a piece of work to each worker (there must be more work than workers) */
	    k = 0;
	    for (i=0; i<num_children; i++)
	    {
		work[0] = k+1;
		work[1] = i1[k]; /* imin */
		work[2] = i2[k]; /* imax */
		work[3] = j1[k]; /* jmin */
		work[4] = j2[k]; /* jmax */

		/*printf("sending work(%d) to %d\n", k+1, i);fflush(stdout);*/
		MPI_Send(work, 5, MPI_INT, 0, 100, child_comm[i]);
		MPI_Irecv(&result[i], 5, MPI_INT, 0, 200, child_comm[i], &child_request[i]);
		k++;
	    }
	    /* receive the results and hand out more work until the image is complete */
	    while (k<400)
	    {
		MPI_Waitany(num_children, child_request, &index, &status);
		memcpy(work, &result[index], 5 * sizeof(int));
		MPI_Recv(in_grid_array, (work[2] + 1 - work[1]) * (work[4] + 1 - work[3]), MPI_INT, 0, 201, child_comm[index], &status);
		/* draw data */
		output_data(in_grid_array, &work[1], out_grid_array, ipixels_across, ipixels_down);
		work[0] = k+1;
		work[1] = i1[k];
		work[2] = i2[k];
		work[3] = j1[k];
		work[4] = j2[k];
		/*printf("sending work(%d) to %d\n", k+1, index);fflush(stdout);*/
		MPI_Send(work, 5, MPI_INT, 0, 100, child_comm[index]);
		MPI_Irecv(&result[index], 5, MPI_INT, 0, 200, child_comm[index], &child_request[index]);
		k++;
	    }
	    /* receive the last pieces of work */
	    /* and tell everyone to stop */
	    for (i=0; i<num_children; i++)
	    {
		MPI_Wait(&child_request[i], &status);
		memcpy(work, &result[i], 5 * sizeof(int));
		MPI_Recv(in_grid_array, (work[2] + 1 - work[1]) * (work[4] + 1 - work[3]), MPI_INT, 0, 201, child_comm[i], &status);
		/* draw data */
		output_data(in_grid_array, &work[1], out_grid_array, ipixels_across, ipixels_down);
		work[0] = 0;
		work[1] = 0;
		work[2] = 0;
		work[3] = 0;
		work[4] = 0;
		/*printf("sending %d to tell %d to stop\n", work[0], i);fflush(stdout);*/
		MPI_Send(work, 5, MPI_INT, 0, 100, child_comm[i]);
	    }

	    /* tell the visualizer the image is done */
	    if (!use_stdin)
	    {
		work[0] = 0;
		work[1] = 0;
		work[2] = 0;
		work[3] = 0;
		error = MPI_Send(work, 4, MPI_INT, 0, 0, comm);
		if (error != MPI_SUCCESS)
		{
		    printf("Unable to send a done command to the visualizer, aborting.\n");
		    MPI_Abort(MPI_COMM_WORLD, -1);
		}
	    }
	}
    }
    else
    {
	for (;;)
	{
	    MPI_Recv(&x_min, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	    MPI_Recv(&x_max, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	    MPI_Recv(&y_min, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	    MPI_Recv(&y_max, 1, MPI_DOUBLE, 0, 0, parent, MPI_STATUS_IGNORE);
	    MPI_Recv(&imax_iterations, 1, MPI_INT, 0, 0, parent, MPI_STATUS_IGNORE);
	    /*printf("received bounding box: (%f,%f)(%f,%f)\n", x_min, y_min, x_max, y_max);fflush(stdout);*/

	    /* check for the end condition */
	    if (x_min == x_max && y_min == y_max)
	    {
		/*printf("slave done.\n");fflush(stdout);*/
		break;
	    }

	    x_resolution = (x_max-x_min)/ ((double)ipixels_across);
	    y_resolution = (y_max-y_min)/ ((double)ipixels_down);

	    MPI_Recv(work, 5, MPI_INT, 0, 100, parent, &status);
	    /*printf("received work: %d, (%d,%d)(%d,%d)\n", work[0], work[1], work[2], work[3], work[4]);fflush(stdout);*/
	    while (work[0] != 0)
	    {
		imin = work[1];
		imax = work[2];
		jmin = work[3];
		jmax = work[4];

		k = 0;
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
			in_grid_array[k] = icount;
			++k;
		    }
		}
		/* send the result to the root */
		work[0] = myid; /* useless in the spawn version - everyone is rank 0 */
		MPI_Send(work, 5, MPI_INT, 0, 200, parent);
		MPI_Send(in_grid_array, (work[2] + 1 - work[1]) * (work[4] + 1 - work[3]), MPI_INT, 0, 201, parent);
		/* get the next piece of work */
		MPI_Recv(work, 5, MPI_INT, 0, 100, parent, &status);
		/*printf("received work: %d, (%d,%d)(%d,%d)\n", work[0], work[1], work[2], work[3], work[4]);fflush(stdout);*/
	    }
	}
    }

    if (master && save_image)
    {
	imax_iterations = 0;
	for (i=0; i<ipixels_across * ipixels_down; ++i)
	{
	    /* look for "brightest" pixel value, for image use */
	    if (out_grid_array[i] > imax_iterations)
		imax_iterations = out_grid_array[i];
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

	dumpimage(filename, out_grid_array, ipixels_across, ipixels_down, imax_iterations, file_message, num_colors, colors);
    }

    if (master)
    {
	for (i=0; i<num_children; i++)
	{
	    MPI_Comm_disconnect(&child_comm[i]);
	}
	MPI_Comm_disconnect(&comm);
	free(child_comm);
	free(child_request);
	free(colors);
    }

    MPI_Finalize();
    return 0;
}

void PrintUsage()
{
    printf("usage: mpiexec -n 1 pmandel [options]\n");
    printf("options:\n -n # slaves\n -xmin # -xmax #\n -ymin # -ymax #\n -depth #\n -xscale # -yscale #\n -out filename\n -i\n");
    printf("All options are optional.\n");
    printf("-i will allow you to input the min/max parameters from stdin and output the resulting image to a ppm file.");
    printf("  Otherwise the root process will listen for a separate visualizer program to connect to it.\n");
    printf("The defaults are:\n xmin %f, xmax %f\n ymin %f, ymax %f\n depth %d\n xscale %d, yscale %d\n %s\n",
	IDEAL_MIN_X_VALUE, IDEAL_MAX_X_VALUE, IDEAL_MIN_Y_VALUE, IDEAL_MAX_Y_VALUE, IDEAL_ITERATIONS,
	IDEAL_WIDTH, IDEAL_HEIGHT, default_filename);
    fflush(stdout);
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
		    int *save_image, int *num_children)
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

    *o_julia = 0; /* default is "generate Mandelbrot" */
    *o_alternate = 0; /* default is still "generate Mandelbrot" */
    *o_divergent_limit = INFINITE_LIMIT; /* default total range is assumed
					 if not explicitly overwritten */

    *num_children = DEFAULT_NUM_SLAVES;

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
	else if (strcmp(argv[i], "-save\0") == 0)
	    *save_image = 1;
	else if (strcmp(argv[i], "-nosave\0") == 0)
	    *save_image = 0;
	else if (strcmp(argv[i], "-n\0") == 0)
	    sscanf(argv[i+1], "%d", &*num_children);
	else if ((strcmp(argv[i], "-help\0") == 0) || (strcmp(argv[i], "-?") == 0))
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
		       double *m_divergent_limit,
		       int *m_num_children)
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

    if (*m_num_children < 1)
    {
	printf("Error, invalid number of slaves (%d), setting to %d\n", *m_num_children, DEFAULT_NUM_SLAVES);
	*m_num_children = DEFAULT_NUM_SLAVES;
    }
    if (*m_num_children > 400)
    {
	printf("Error, number of slaves (%d) exceeds the maximum, setting to 400.\n", *m_num_children);
	*m_num_children = 400;
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

void output_data(int *in_grid_array, int coord[4], int *out_grid_array, int width, int height)
{
    int error;
    int i, j, k;
    k = 0;
    for (j=coord[2]; j<=coord[3]; j++)
    {
	for (i=coord[0]; i<=coord[1]; i++)
	{
	    out_grid_array[(j * height) + i] = in_grid_array[k];
	    k++;
	}
    }
    if (!use_stdin)
    {
	error = MPI_Send(coord, 4, MPI_INT, 0, 0, comm);
	if (error != MPI_SUCCESS)
	{
	    printf("Unable to send coordinates to the visualizer, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
	error = MPI_Send(in_grid_array, (coord[1] + 1 - coord[0]) * (coord[3] + 1 - coord[2]), MPI_INT, 0, 0, comm);
	if (error != MPI_SUCCESS)
	{
	    printf("Unable to send coordinate data to the visualizer, aborting.\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
    }
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
