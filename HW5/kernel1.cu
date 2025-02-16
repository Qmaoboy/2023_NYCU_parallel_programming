#include <cuda.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 8

__global__ void mandelKernel(
    int *d_out, 
    float lowerX, float lowerY,
    float stepX, float stepY,
    int resX,int maxIterations) {
    // To avoid error caused by the floating number, use the following pseudo code
    //
    // float x = lowerX + thisX * stepX;
    // float y = lowerY + thisY * stepY;

    int thisX = blockIdx.x * blockDim.x + threadIdx.x;
    int thisY = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = thisY*resX + thisX;

    float c_re = lowerX + thisX * stepX;
    float c_im = lowerY + thisY * stepY;
    float z_re = c_re;
    float z_im = c_im;
    
    int iter;
    for (iter = 0;iter < maxIterations;iter++) {
        if (z_re * z_re + z_im * z_im > 4.f)
            break;

        float new_re = z_re * z_re - z_im * z_im;
        float new_im = 2.f * z_re * z_im;
        z_re = c_re + new_re;
        z_im = c_im + new_im;
    }

    d_out[idx] = iter;
}

// Host front-end function that allocates the memory and launches the GPU kernel
void hostFE (float upperX, float upperY, float lowerX, float lowerY, int* img, int resX, int resY, int maxIterations)
{
    float stepX = (upperX - lowerX) / resX;
    float stepY = (upperY - lowerY) / resY;

    int *h_out, *d_out; // Result on host and device
    int size = resX * resY * sizeof(int);

    // Allocate memory on host and device
    // const unsigned int width = 1600;
    // const unsigned int height = 1200;
    h_out = (int *)malloc(size);
    cudaMalloc((void **)&d_out, size);

    // CUDA function
    dim3 threadsPerBlock(BLOCK_SIZE, BLOCK_SIZE);
    dim3 numBlocks(resX / threadsPerBlock.x, resY / threadsPerBlock.y);
    mandelKernel<<<numBlocks, threadsPerBlock>>>(d_out,
                                                 lowerX, lowerY,
                                                 stepX, stepY,
                                                 resX,maxIterations);
    //cuda copy from d_out to h_out
    cudaMemcpy(h_out, d_out, size, cudaMemcpyDeviceToHost);
    //save img in *img
    memcpy(img, h_out, size);

    // Free allocated memory
    free(h_out);
    cudaFree(d_out);
}

