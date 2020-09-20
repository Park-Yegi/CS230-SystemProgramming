/* Park Yegi, 20160253 */

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
	int i, j;
	int tmp;
	int row_block, col_block;
	int e0, e1, e2, e3;
	// int e4, e5, e6;


	/* 32 x 32 matrix */
	if (M==32){
		// make 8x8 block
		for (col_block=0; col_block<32; col_block+=8){
			for (row_block=0; row_block<32; row_block+=8){
				for (i=row_block; i<row_block+8; i++){
					for (j=col_block; j<col_block+8; j++){
						if (i != j)  // remove conflict miss
							B[j][i] = A[i][j];
						else
							tmp = A[i][j]; 
					}
					if (row_block == col_block)
						B[i][i] = tmp;
				}
			}
		}
	}
	

	/* 64 x 64 matrix */
	else if (M==64){
		for (col_block=0; col_block<64; col_block+=4){
			for (row_block=0; row_block<64; row_block+=4){
				for (i=row_block; i<row_block+4; i++){
					e0 = A[i][col_block];
					e1 = A[i][col_block+1];
					e2 = A[i][col_block+2];
					e3 = A[i][col_block+3];
	
					B[col_block][i] = e0;
					B[col_block+1][i] = e1;
					B[col_block+2][i] = e2;
					B[col_block+3][i] = e3;
				}
			}
		}
	}
		


	/* 61 x 67 matrix */
	else if (M==61){
		// make 8x8 block
		for (col_block=0; col_block<61; col_block+=8){
			for (row_block=0; row_block<67; row_block+=8){
				for (i=row_block; (i<row_block+8)&&(i<67); i++){
					for (j=col_block; (j<col_block+8)&&(j<61); j++){
						if (i != j)  // remove conflict miss
							B[j][i] = A[i][j];
						else
							tmp = A[i][j];
					}
					if (row_block == col_block)
						B[i][i] = tmp;
				}
			}
		}
	}

	else{
		printf("Wrong input!");
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

