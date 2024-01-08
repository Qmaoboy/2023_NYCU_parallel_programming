__kernel void convolution(const __global float *inputImage, __global float *outputImage, __constant float *filter,
                          const int imageHeight, const int imageWidth, const int filterWidth) 
{
   int idx = get_global_id(0);
   int row_idx = idx / imageWidth;
   int col_idx = idx % imageWidth;
   int halfFSize = filterWidth / 2;
   int k, l;
   float sumup = 0.0f;

   for (k = -halfFSize;k <= halfFSize;k++) {
       for (l = -halfFSize; l <= halfFSize; l++)
        {
            if(filter[(k + halfFSize) * filterWidth + l + halfFSize] != 0)
            {
                if (row_idx + k >= 0 && row_idx + k < imageHeight &&
                    col_idx + l >= 0 && col_idx + l < imageWidth)
                {
                    sumup += inputImage[(row_idx + k) * imageWidth + col_idx + l] *
                            filter[(k + halfFSize) * filterWidth +l + halfFSize];
                }
            }
        }
   }
   
   outputImage[row_idx * imageWidth + col_idx] = sumup;
}
