#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <mpi.h>

void construct_matrices(int *n_ptr, int *m_ptr, int *l_ptr,int **a_mat_ptr, int **b_mat_ptr)
{
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // Process 0 reads the matrix dimensions
        // "/home/.grade/HW4/data-set/data1_1"
        FILE *file=fopen(argv[1],"r");
        if (file == NULL) {
        fprintf(stderr, "Failed to open the file for reading.\n");
        return 1;
        }
        printf("Enter the dimensions (n m l): ");
        fscanf(file,"%d %d %d", n_ptr, m_ptr, l_ptr);
        
        // Broadcast matrix dimensions to all processes
        MPI_Bcast(n_ptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(m_ptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(l_ptr, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Calculate the number of elements each process will handle
        int local_n = *n_ptr / size;
        int local_elements_a = local_n * *m_ptr;
        int local_elements_b = *m_ptr * *l_ptr;

        // Allocate memory for the local portion of matrices A and B
        *a_mat_ptr = (int *)malloc(*n_ptr * *m_ptr * sizeof(int));
        *b_mat_ptr = (int *)malloc(*m_ptr * *l_ptr * sizeof(int));

        //scan data
        for (int i=0;i<*n_ptr;i++){
            for (int j=0;j<*m_ptr;j++){
            fscanf(file,a_mat_ptr[i * *n_ptr+j]);
            }
        }
        for (int i=0;i<*m_ptr;i++){
            for (int j=0;j<*l_ptr;j++){
            fscanf(file,b_mat_ptr[i * *m_ptr+j]);
            }
        }


        // Scatter matrix A data to all processes
        MPI_Scatter(a_mat_ptr, local_elements_a, MPI_INT, *a_mat_ptr, local_elements_a, MPI_INT, 0, MPI_COMM_WORLD);

        // Scatter matrix B data to all processes
        MPI_Scatter(a_mat_ptr, local_elements_b, MPI_INT, *b_mat_ptr, local_elements_b, MPI_INT, 0, MPI_COMM_WORLD);

        // Now each process has its portion of matrix A and B
        fclose(file)
    }


}
    
    
    

void matrix_multiply(const int n, const int m, const int l,const int *a_mat, const int *b_mat)
{
  int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Calculate the number of elements each process will handle
    int local_n = n / size;
    int local_elements_a = local_n * m;
    int local_elements_b = m * l;

    // Allocate memory for the local portion of matrices A, B, and the result
    int *local_a = (int *)malloc(local_elements_a * sizeof(int));
    int *local_b = (int *)malloc(local_elements_b * sizeof(int));
    int *local_result = (int *)malloc(local_n * l * sizeof(int));

    // Scatter matrix A and matrix B data to all processes
    MPI_Scatter(a_mat, local_elements_a, MPI_INT, local_a, local_elements_a, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(b_mat, local_elements_b, MPI_INT, local_b, local_elements_b, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform local matrix multiplication
    for (int i = 0; i < local_n; i++) {
        for (int j = 0; j < l; j++) {
            local_result[i * l + j] = 0;
            for (int k = 0; k < m; k++) {
                local_result[i * l + j] += local_a[i * m + k] * local_b[k * l + j];
            }
        }
    }

    // Gather the results from all processes to process 0
    MPI_Gather(local_result, local_n * l, MPI_INT, &a_mat, local_n * l, MPI_INT, 0, MPI_COMM_WORLD);
    // Free the allocated memory for local matrices
    free(local_a);
    free(local_b);
    free(local_result);
}



void destruct_matrices(int *a_mat, int *b_mat)
{
    // Free the memory for matrix A
    free(a_mat);

    // Free the memory for matrix B
    free(b_mat);
}


