/* 2009-11744 심규민
 * 시스템 프로그래밍 cachelab part b
 *
 * 목표
 *  (s, E, b) = (5, 1, 5) 인 cache를 가지고
 *  32x32, 64x64, 61x67 matrix에 대하여
 *  out-place transpose를 miss가 적게 나도록
 *  효율적으로 하는 것이다.
 *
 * 구현
 *  기본적으로 blocking technique을 이용하였다.
 *  하나의 cache block은 32(=2^5)byte이므로 int(=4byte) 8개가 들어간다.
 *  이 특징을 이용하여 기본적으로 8개 단위로 끊어서 hit ratio를 올렸다.
 *  각 matrix 사이즈에 대한 상세 구현은 코드와 함께 주석으로 설명하였다.
 *
 * 결과
 *  32x32(MxN): 287 misses
 *  64x64(MxN): 1347 misses
 *  61x67(MxN): 1953 misses
 */

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

#define SWAP(a, b) (a ^= b, b ^= a, a ^= b)

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
    // local variables (total 12)
    int a0, a1, a2, a3, a4, a5, a6, a7;
    int i, j;
    int ii, jj;
    // 32x32
    if (M == 32 && N == 32)
    {
        // 원소를 8개씩 끊어서
        for (j = 0; j < M; j += 8)
        {
            for (i = 0; i < N; i++)
            {
                // A에 있는 하나의 block의 원소를
                // 모두 register로 읽어 들이고,
                a0 = A[i][j + 0];
                a1 = A[i][j + 1];
                a2 = A[i][j + 2];
                a3 = A[i][j + 3];
                a4 = A[i][j + 4];
                a5 = A[i][j + 5];
                a6 = A[i][j + 6];
                a7 = A[i][j + 7];
                // 읽어 들인 값으로 B를 채운다.
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
    // 64x64
    else if (M == 64 && N == 64)
    {
        // 일단 8x8 block으로 끊어서 처리한다.
        for (jj = 0; jj < M; jj += 8)
        {
            for (ii = 0; ii < N; ii += 8)
            {
                j = jj;
                // 대각선 상에 있는 block은
                if (ii == jj)
                {
                    // 4줄 단위로 같은 set에 들어가기 때문에 4줄씩 끊어서 처리한다.
                    // A의 4x8을 B의 8x4로 옮겨야 하지만 miss를 줄이기 위해 우선 B의 4x8에 옮긴다.
                    // 이 때 B에는 transpose가 맞는 4x4가 있고 틀린 4x4가 발생하는데
                    // 틀린 4x4는 줄을 역순으로 넣어놓는다.
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
                    // 나머지 4x8에 대해서도 같은 식으로 처리한다.
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
                    // B에 서로 뒤바뀐 두개의 4x4를 교환해준다.
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
                // 대각선 상에 있지 않은 block은
                else // ii != jj
                {
                    // 대각선 상에 있는 block과 마찬가지로 4줄씩 끊어서 처리한다.
                    // 틀린 4x4를 역순으로 넣어놓는 트릭은 마찬가지로 사용한다.
                    // 하지만 대각선 상에 있는 block과는 다르게 더 최적화 하였다.
                    // A의 4x8과 B의 4x8은 서로 같은 set에 들어가지 않기 때문에 register를 통해서 옮길 필요가 없다.
                    // 그래서 A의 전반부 4x8을 옮기면서 그 중에 8개의 값을 register에 불러다 놓을 수 있다.
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
                    // A의 후반부 4x8을 처리한다.
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
                    // 여기까지 일단 cold miss 16번만 발생한다.

                    // B에 서로 뒤바뀐 두개의 4x4를 교환해준다.
                    // 미리 불러들였던 register의 값과
                    // xor 방식의 in-place swap을 사용하면
                    // 4번의 miss로 해낼 수 있다.

                    SWAP(B[jj + 7][ii + 0], a0);
                    SWAP(B[jj + 7][ii + 1], a1);
                    SWAP(B[jj + 7][ii + 2], a2);
                    SWAP(B[jj + 7][ii + 3], a3);

                    SWAP(B[jj + 3][ii + 4], a0);
                    SWAP(B[jj + 3][ii + 5], a1);
                    SWAP(B[jj + 3][ii + 6], a2);
                    SWAP(B[jj + 3][ii + 7], a3);

                    SWAP(B[jj + 4][ii + 0], a0);
                    SWAP(B[jj + 4][ii + 1], a1);
                    SWAP(B[jj + 4][ii + 2], a2);
                    SWAP(B[jj + 4][ii + 3], a3);

                    SWAP(B[jj + 0][ii + 4], a0);
                    SWAP(B[jj + 0][ii + 5], a1);
                    SWAP(B[jj + 0][ii + 6], a2);
                    SWAP(B[jj + 0][ii + 7], a3);

                    SWAP(B[jj + 0][ii + 4], B[jj + 3][ii + 4]);
                    SWAP(B[jj + 0][ii + 5], B[jj + 3][ii + 5]);
                    SWAP(B[jj + 0][ii + 6], B[jj + 3][ii + 6]);
                    SWAP(B[jj + 0][ii + 7], B[jj + 3][ii + 7]);

                    SWAP(B[jj + 6][ii + 0], a4);
                    SWAP(B[jj + 6][ii + 1], a5);
                    SWAP(B[jj + 6][ii + 2], a6);
                    SWAP(B[jj + 6][ii + 3], a7);

                    SWAP(B[jj + 2][ii + 4], a4);
                    SWAP(B[jj + 2][ii + 5], a5);
                    SWAP(B[jj + 2][ii + 6], a6);
                    SWAP(B[jj + 2][ii + 7], a7);

                    SWAP(B[jj + 5][ii + 0], a4);
                    SWAP(B[jj + 5][ii + 1], a5);
                    SWAP(B[jj + 5][ii + 2], a6);
                    SWAP(B[jj + 5][ii + 3], a7);

                    SWAP(B[jj + 1][ii + 4], a4);
                    SWAP(B[jj + 1][ii + 5], a5);
                    SWAP(B[jj + 1][ii + 6], a6);
                    SWAP(B[jj + 1][ii + 7], a7);

                    SWAP(B[jj + 1][ii + 4], B[jj + 2][ii + 4]);
                    SWAP(B[jj + 1][ii + 5], B[jj + 2][ii + 5]);
                    SWAP(B[jj + 1][ii + 6], B[jj + 2][ii + 6]);
                    SWAP(B[jj + 1][ii + 7], B[jj + 2][ii + 7]);

                }
            }
        }
    }
    // 61x67 or MxN
    else
    {
        // 16x8 block으로 끊어서 처리한다.
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

