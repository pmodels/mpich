/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/* This program is a slightly updated version of the report_colors program found on the MPICH-G2 home page. */

#include <mpi.h>
#include <stdio.h>

void print_topology(int me, int size, int *depths, int **colors)
{
    int i, j, max = 0;
    FILE *fp;
    char fname[100];

    sprintf(fname, "colors.%d", me);
    if (!(fp = fopen(fname, "w")))
    {
	fprintf(stderr, "ERROR: could not open fname >%s<\n", fname);
	MPI_Abort(MPI_COMM_WORLD, 1);
    } /* endif */

    fprintf(fp, "proc\t");
    for (i = 0; i < size; i++)
	fprintf(fp, "% 3d", i);
    fprintf(fp, "\nDepths\t");

    for (i = 0; i < size; i++)
    {
	fprintf(fp, "% 3d", depths[i]);
	if ( max < depths[i] )
	    max = depths[i];
    } /* endfor */

    for (j = 0; j < max; j++)
    {
	fprintf(fp, "\nlvl %d\t", j);
	for (i = 0; i < size; i++)
	    if ( j < depths[i] )
		fprintf(fp, "% 3d", colors[i][j]);
	    else
		fprintf(fp, "   ");
    } /* endfor */
    fprintf(fp, "\n");

    fclose(fp);

    return;

} /* end print_topology() */

int main (int argc, char *argv[])
{
    int me, nprocs, flag, rv;
    int *depths;
    int **colors;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    rv = MPI_Attr_get(MPI_COMM_WORLD, MPIG_TOPOLOGY_DEPTHS_KEYVAL, &depths, &flag);
    if ( rv != MPI_SUCCESS )
    {
	printf("MPI_Attr_get(depths) failed, aborting\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    } /* endif */
    if ( flag == 0 )
    {
	printf("MPI_Attr_get(depths): depths not available...\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    } /* endif */
    rv = MPI_Attr_get(MPI_COMM_WORLD, MPIG_TOPOLOGY_COLORS_KEYVAL, &colors, &flag);
    if ( rv != MPI_SUCCESS )
    {
	printf("MPI_Attr_get(colors) failed, aborting\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    } /* endif */
    if ( flag == 0 )
    {
	printf("MPI_Attr_get(colors): depths not available...\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    } /* endif */

    print_topology(me, nprocs, depths, colors);

    MPI_Finalize();

    return 0;

} /* end main() */
