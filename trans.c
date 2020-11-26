/* 
 * Name : Enze Li
 * Andrew id : enzel
 *
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
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int blocksize;
    int row, col, i, j;
    int a, b, x, y, z; 

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    /* save diagnal to last, avoid cache confliction of Aii and Bii */
    if (M == 32 && N == 32) {
        blocksize = 8;
        for (row = 0; row < N; row += blocksize){
            for (col = 0; col < M; col += blocksize){
                for (i = row; i < row + blocksize && i < N; ++i){
                    for (j = col; j < col + blocksize && j < M; ++j){
                        if (i != j) {
                            a = A[i][j];
                            B[j][i] = a;
                        } 
                    }
                    if (row == col) {
                        a = A[i][i];
                        B[i][i] = a;
                    }
                }
            }
        }
    }

    /* 
     *  4X4 blocks 
     *  if block is near diagnal, A[i][j] and B[j][i] cached in same set 
     *  try to use every item before one is evicted by another
     */

    if (M == 64 && N == 64) {
        blocksize = 4;
        for (i = 0; i < N; i += blocksize){
            for (j = 0; j < M; j += blocksize){
                /* load A[i][] A[i+1][] A[i+2][]*/
                a = A[i][j];
                b = A[i+1][j];
                x = A[i+2][j];
                y = A[i+2][j+1];
                z = A[i+2][j+2];

                /* load B[j+3][] */
                B[j+3][i] = A[i][j+3];
                B[j+3][i+1] = A[i+1][j+3];
                B[j+3][i+2] = A[i+2][j+3];

                /* load B[j+2][], may evict A[i+2][] */
                B[j+2][i] = A[i][j+2];
                B[j+2][i+1] = A[i+1][j+2];
                B[j+2][i+2] = z;

                z = A[i+1][j+1];

                /* load B[j+1][], may evict A[i+1][] */
                B[j+1][i] = A[i][j+1];
                B[j+1][i+1] = z;
                B[j+1][i+2] = y;

                /* load B[j][], may evict A[i][] */
                B[j][i] = a;
                B[j][i+1] = b;
                B[j][i+2] = x;

                /* load A[i+3][], may evict B[j+3][] */
                B[j][i+3] = A[i+3][j];
                B[j+1][i+3] = A[i+3][j+1];
                B[j+2][i+3] = A[i+3][j+2];

                a = A[i+3][j+3];

                /* load B[j+3][], may evict A[i+3][] */
                B[j+3][i+3] = a;
            }
        }
    }

    /* a proper blocksize will work */
    if (M == 61 && N == 67) {
        blocksize = 18;
        for (row = 0; row < N; row += blocksize){
            for (col = 0; col < M; col += blocksize){
                for (i = row; i < row + blocksize && i < N; ++i){
                    for (j = col; j < col + blocksize && j < M; ++j){
                        a = A[i][j];
                        B[j][i] = a;
                    }
                }
            }
        }
    }

    ENSURES(is_transpose(M, N, A, B));
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

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
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