MPI_EXEC = 3_mpi
SERIAL_EXEC = 3

all : $(MPI_EXEC) $(SERIAL_EXEC)

3_mpi : 3_mpi.c
	mpicc $^ -o $@

3 : 3.c
	gcc $^ -o $@

clean :
	rm $(MPI_EXEC) $(SERIAL_EXEC)
	rm *.ppm
