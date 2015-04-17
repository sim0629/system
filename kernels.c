/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/*
 * Please fill in the following student struct
 */
student_t student = {
    "2009-11744",              /* Team name */
    "심규민",     /* First member full name */
    "sim0629@snu.ac.kr",  /* First member email address */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/*
 * naive_rotate - The naive baseline version of rotate
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst)
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/*
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 16, m = 8;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_8_8_descr[] = "rotate: 8 x 8";
void rotate_8_8(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 8, m = 8;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_8_16_descr[] = "rotate: 8 x 16";
void rotate_8_16(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 8, m = 16;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_8_32_descr[] = "rotate: 8 x 32";
void rotate_8_32(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 8, m = 32;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_16_8_descr[] = "rotate: 16 x 8";
void rotate_16_8(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 16, m = 8;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_16_16_descr[] = "rotate: 16 x 16";
void rotate_16_16(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 16, m = 16;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_16_32_descr[] = "rotate: 16 x 32";
void rotate_16_32(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 16, m = 32;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_32_8_descr[] = "rotate: 32 x 8";
void rotate_32_8(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 32, m = 8;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_32_16_descr[] = "rotate: 32 x 16";
void rotate_32_16(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 32, m = 16;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char rotate_32_32_descr[] = "rotate: 32 x 32";
void rotate_32_32(int dim, pixel *src, pixel *dst)
{
    int ii, jj;
    int i, j;
    int n = 32, m = 32;
    for (ii = 0; ii < dim; ii += n)
    for (jj = 0; jj < dim; jj += m)
        for (i = ii; i < ii + n; i++)
        for (j = jj; j < jj + m; j++)
            dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_rotate_functions()
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);
    add_rotate_function(&rotate, rotate_descr);
    add_rotate_function(&rotate_8_8, rotate_8_8_descr);
    add_rotate_function(&rotate_8_16, rotate_8_16_descr);
    add_rotate_function(&rotate_8_32, rotate_8_32_descr);
    add_rotate_function(&rotate_16_8, rotate_16_8_descr);
    add_rotate_function(&rotate_16_16, rotate_16_16_descr);
    add_rotate_function(&rotate_16_32, rotate_16_32_descr);
    add_rotate_function(&rotate_32_8, rotate_32_8_descr);
    add_rotate_function(&rotate_32_16, rotate_32_16_descr);
    add_rotate_function(&rotate_32_32, rotate_32_32_descr);
    /* ... Register additional test functions here */
}
