/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    int i, j;
    int ii, jj;
    if (M == 32 && N == 32)
    {
        for (j = 0; j < M; j += 8)
        {
            for (i = 0; i < N; i++)
            {
                a0 = A[i][j + 0];
                a1 = A[i][j + 1];
                a2 = A[i][j + 2];
                a3 = A[i][j + 3];
                a4 = A[i][j + 4];
                a5 = A[i][j + 5];
                a6 = A[i][j + 6];
                a7 = A[i][j + 7];
                B[j + 0][i] = a0;
                B[j + 1][i] = a1;
                B[j + 2][i] = a2;
                B[j + 3][i] = a3;
                B[j + 4][i] = a4;
                B[j + 5][i] = a5;
                B[j + 6][i] = a6;
                B[j + 7][i] = a7;
            }
        }
    }
    else if (M == 64 && N == 64)
    {
        for (jj = 0; jj < M; jj += 8)
        {
            for (ii = 0; ii < N; ii += 8)
            {
                if (ii == jj)
                {
                    j = jj;
                    for (i = ii; i < ii + 4; i++)
                    {
                        a0 = A[i][j + 0];
                        a1 = A[i][j + 1];
                        a2 = A[i][j + 2];
                        a3 = A[i][j + 3];
                        a4 = A[i][j + 4];
                        a5 = A[i][j + 5];
                        a6 = A[i][j + 6];
                        a7 = A[i][j + 7];
                        B[j + 0][i] = a0;
                        B[j + 1][i] = a1;
                        B[j + 2][i] = a2;
                        B[j + 3][i] = a3;
                        B[j + 3][i + 4] = a4;
                        B[j + 2][i + 4] = a5;
                        B[j + 1][i + 4] = a6;
                        B[j + 0][i + 4] = a7;
                    }
                    for (i = ii + 4; i < ii + 8; i++)
                    {
                        a0 = A[i][j + 0];
                        a1 = A[i][j + 1];
                        a2 = A[i][j + 2];
                        a3 = A[i][j + 3];
                        a4 = A[i][j + 4];
                        a5 = A[i][j + 5];
                        a6 = A[i][j + 6];
                        a7 = A[i][j + 7];
                        B[j + 7][i - 4] = a0;
                        B[j + 6][i - 4] = a1;
                        B[j + 5][i - 4] = a2;
                        B[j + 4][i - 4] = a3;
                        B[j + 4][i] = a4;
                        B[j + 5][i] = a5;
                        B[j + 6][i] = a6;
                        B[j + 7][i] = a7;
                    }
                    for (j = 0; j < 4; j++)
                    {
                        for (i = 0; i < 4; i++)
                        {
                            a0 = B[jj + j][ii + 4 + i];
                            a1 = B[jj + 4 + (3 - j)][ii + i];
                            B[jj + j][ii + 4 + i] = a1;
                            B[jj + 4 + (3 - j)][ii + i] = a0;
                        }
                    }
                }
                else
                {
                    j = jj;
                    for (i = ii; i < ii + 4; i++)
                    {
                        B[j + 0][i] = A[i][j + 0];
                        B[j + 1][i] = A[i][j + 1];
                        B[j + 2][i] = A[i][j + 2];
                        B[j + 3][i] = A[i][j + 3];
                        B[j + 3][i + 4] = A[i][j + 4];
                        B[j + 2][i + 4] = A[i][j + 5];
                        B[j + 1][i + 4] = A[i][j + 6];
                        B[j + 0][i + 4] = A[i][j + 7];
                        if (i == ii + 0)
                        {
                            a0 = B[j + 0][i + 4];
                            a4 = B[j + 1][i + 4];
                        }
                        else if (i == ii + 1)
                        {
                            a1 = B[j + 0][i + 4];
                            a5 = B[j + 1][i + 4];
                        }
                        else if (i == ii + 2)
                        {
                            a2 = B[j + 0][i + 4];
                            a6 = B[j + 1][i + 4];
                        }
                        else if (i == ii + 3)
                        {
                            a3 = B[j + 0][i + 4];
                            a7 = B[j + 1][i + 4];
                        }
                    }
                    for (i = ii + 4; i < ii + 8; i++)
                    {
                        B[j + 7][i - 4] = A[i][j + 0];
                        B[j + 6][i - 4] = A[i][j + 1];
                        B[j + 5][i - 4] = A[i][j + 2];
                        B[j + 4][i - 4] = A[i][j + 3];
                        B[j + 4][i] = A[i][j + 4];
                        B[j + 5][i] = A[i][j + 5];
                        B[j + 6][i] = A[i][j + 6];
                        B[j + 7][i] = A[i][j + 7];
                    }

                    B[jj + 7][ii + 0] ^= a0; a0 ^= B[jj + 7][ii + 0]; B[jj + 7][ii + 0] ^= a0;
                    B[jj + 7][ii + 1] ^= a1; a1 ^= B[jj + 7][ii + 1]; B[jj + 7][ii + 1] ^= a1;
                    B[jj + 7][ii + 2] ^= a2; a2 ^= B[jj + 7][ii + 2]; B[jj + 7][ii + 2] ^= a2;
                    B[jj + 7][ii + 3] ^= a3; a3 ^= B[jj + 7][ii + 3]; B[jj + 7][ii + 3] ^= a3;

                    B[jj + 3][ii + 4] ^= a0; a0 ^= B[jj + 3][ii + 4]; B[jj + 3][ii + 4] ^= a0;
                    B[jj + 3][ii + 5] ^= a1; a1 ^= B[jj + 3][ii + 5]; B[jj + 3][ii + 5] ^= a1;
                    B[jj + 3][ii + 6] ^= a2; a2 ^= B[jj + 3][ii + 6]; B[jj + 3][ii + 6] ^= a2;
                    B[jj + 3][ii + 7] ^= a3; a3 ^= B[jj + 3][ii + 7]; B[jj + 3][ii + 7] ^= a3;

                    B[jj + 4][ii + 0] ^= a0; a0 ^= B[jj + 4][ii + 0]; B[jj + 4][ii + 0] ^= a0;
                    B[jj + 4][ii + 1] ^= a1; a1 ^= B[jj + 4][ii + 1]; B[jj + 4][ii + 1] ^= a1;
                    B[jj + 4][ii + 2] ^= a2; a2 ^= B[jj + 4][ii + 2]; B[jj + 4][ii + 2] ^= a2;
                    B[jj + 4][ii + 3] ^= a3; a3 ^= B[jj + 4][ii + 3]; B[jj + 4][ii + 3] ^= a3;

                    B[jj + 0][ii + 4] ^= a0; a0 ^= B[jj + 0][ii + 4]; B[jj + 0][ii + 4] ^= a0;
                    B[jj + 0][ii + 5] ^= a1; a1 ^= B[jj + 0][ii + 5]; B[jj + 0][ii + 5] ^= a1;
                    B[jj + 0][ii + 6] ^= a2; a2 ^= B[jj + 0][ii + 6]; B[jj + 0][ii + 6] ^= a2;
                    B[jj + 0][ii + 7] ^= a3; a3 ^= B[jj + 0][ii + 7]; B[jj + 0][ii + 7] ^= a3;

                    B[jj + 0][ii + 4] ^= B[jj + 3][ii + 4];
                    B[jj + 3][ii + 4] ^= B[jj + 0][ii + 4];
                    B[jj + 0][ii + 4] ^= B[jj + 3][ii + 4];

                    B[jj + 0][ii + 5] ^= B[jj + 3][ii + 5];
                    B[jj + 3][ii + 5] ^= B[jj + 0][ii + 5];
                    B[jj + 0][ii + 5] ^= B[jj + 3][ii + 5];

                    B[jj + 0][ii + 6] ^= B[jj + 3][ii + 6];
                    B[jj + 3][ii + 6] ^= B[jj + 0][ii + 6];
                    B[jj + 0][ii + 6] ^= B[jj + 3][ii + 6];

                    B[jj + 0][ii + 7] ^= B[jj + 3][ii + 7];
                    B[jj + 3][ii + 7] ^= B[jj + 0][ii + 7];
                    B[jj + 0][ii + 7] ^= B[jj + 3][ii + 7];

                    B[jj + 6][ii + 0] ^= a4; a4 ^= B[jj + 6][ii + 0]; B[jj + 6][ii + 0] ^= a4;
                    B[jj + 6][ii + 1] ^= a5; a5 ^= B[jj + 6][ii + 1]; B[jj + 6][ii + 1] ^= a5;
                    B[jj + 6][ii + 2] ^= a6; a6 ^= B[jj + 6][ii + 2]; B[jj + 6][ii + 2] ^= a6;
                    B[jj + 6][ii + 3] ^= a7; a7 ^= B[jj + 6][ii + 3]; B[jj + 6][ii + 3] ^= a7;

                    B[jj + 2][ii + 4] ^= a4; a4 ^= B[jj + 2][ii + 4]; B[jj + 2][ii + 4] ^= a4;
                    B[jj + 2][ii + 5] ^= a5; a5 ^= B[jj + 2][ii + 5]; B[jj + 2][ii + 5] ^= a5;
                    B[jj + 2][ii + 6] ^= a6; a6 ^= B[jj + 2][ii + 6]; B[jj + 2][ii + 6] ^= a6;
                    B[jj + 2][ii + 7] ^= a7; a7 ^= B[jj + 2][ii + 7]; B[jj + 2][ii + 7] ^= a7;

                    B[jj + 5][ii + 0] ^= a4; a4 ^= B[jj + 5][ii + 0]; B[jj + 5][ii + 0] ^= a4;
                    B[jj + 5][ii + 1] ^= a5; a5 ^= B[jj + 5][ii + 1]; B[jj + 5][ii + 1] ^= a5;
                    B[jj + 5][ii + 2] ^= a6; a6 ^= B[jj + 5][ii + 2]; B[jj + 5][ii + 2] ^= a6;
                    B[jj + 5][ii + 3] ^= a7; a7 ^= B[jj + 5][ii + 3]; B[jj + 5][ii + 3] ^= a7;

                    B[jj + 1][ii + 4] ^= a4; a4 ^= B[jj + 1][ii + 4]; B[jj + 1][ii + 4] ^= a4;
                    B[jj + 1][ii + 5] ^= a5; a5 ^= B[jj + 1][ii + 5]; B[jj + 1][ii + 5] ^= a5;
                    B[jj + 1][ii + 6] ^= a6; a6 ^= B[jj + 1][ii + 6]; B[jj + 1][ii + 6] ^= a6;
                    B[jj + 1][ii + 7] ^= a7; a7 ^= B[jj + 1][ii + 7]; B[jj + 1][ii + 7] ^= a7;

                    B[jj + 1][ii + 4] ^= B[jj + 2][ii + 4];
                    B[jj + 2][ii + 4] ^= B[jj + 1][ii + 4];
                    B[jj + 1][ii + 4] ^= B[jj + 2][ii + 4];

                    B[jj + 1][ii + 5] ^= B[jj + 2][ii + 5];
                    B[jj + 2][ii + 5] ^= B[jj + 1][ii + 5];
                    B[jj + 1][ii + 5] ^= B[jj + 2][ii + 5];

                    B[jj + 1][ii + 6] ^= B[jj + 2][ii + 6];
                    B[jj + 2][ii + 6] ^= B[jj + 1][ii + 6];
                    B[jj + 1][ii + 6] ^= B[jj + 2][ii + 6];

                    B[jj + 1][ii + 7] ^= B[jj + 2][ii + 7];
                    B[jj + 2][ii + 7] ^= B[jj + 1][ii + 7];
                    B[jj + 1][ii + 7] ^= B[jj + 2][ii + 7];

                }
            }
        }
    }
    else
    {
        for (ii = 0; ii < N; ii += 16)
        {
            for (jj = 0; jj < M; jj += 8)
            {
                for (i = ii; i < ii + 16 && i < N; i++)
                {
                    for (j = jj; j < jj + 8 && j < M; j++)
                    {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

