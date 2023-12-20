SHELL=/bin/sh
cc:=mpicxx
gcc:=gcc
spread:=parallel-scp -h host.txt -r ~/HW4 ~


%.o:%.cc
	${cc} -c $*.cc -o $*.o

matmul:main.o matmul.o
	${cc} matmul.o main.o -o matmul
	${spread}

# hello: hello.cc
# 	${cc} hello.cc -o mpi_hello
# 	${spread}
# 	mpirun -np 8 --hostfile hosts mpi_hello

# pi_bl:pi_block_linear.cc
# 	${cc} pi_block_linear.cc -o pi_block_linear
# 	${spread}
# 	mpirun -np 4 --hostfile hosts pi_block_linear 1000000000

# pi_bt:pi_block_tree.cc
# 	${cc} pi_block_tree.cc -o pi_block_tree
# 	${spread}
# 	mpirun -np 4 --hostfile hosts pi_block_tree 1000000000

# pi_nbl:pi_nonblock_linear.cc 
# 	${cc} pi_nonblock_linear.cc -o pi_nonblock_linear
# 	${spread}
# 	mpirun -np 4 --hostfile hosts pi_nonblock_linear 1000000000

# pi_gather:pi_gather.cc
# 	${cc} pi_gather.cc -o pi_gather
# 	${spread}
# 	mpirun -np 4 --hostfile hosts pi_gather 1000000000

# pi_reduce:pi_reduce.cc
# 	${cc} pi_reduce.cc -o pi_reduce
# 	${spread}
# 	mpirun -np 4 --hostfile hosts pi_reduce 1000000000

# pi_oneside:
# 	${cc} pi_one_side.cc -o pi_one_side
# 	${spread}
# 	mpirun -np 4 --hostfile hosts pi_one_side 1000000000


.PHONY:clean
clean:
	rm -rf *.o