***
# Image Processing Program using MPI
***
Please change 'hosts' file before run
## HOW TO USE
1. type "make"
2. check following executables are made
    * 3
    * 3_mpi
3. program need 2 arguments  
	argument 1 : path of ppm image file  
	argument 2 : image processing mode [ 0, 1, 2 ]  
		0 : flip image horizontally  
		1 : grayscaling image  
		2 : mean filtering  
4. 3 executable can execute directly, other(3_mpi) can execute with mpiexec command  
    example :  
    ./3 ppm_example/Iggy.1024.ppm 0  
    mpiexec -np 9 -mca btl ^openib -hostfile hosts ./3_mpi ppm_example/Iggy.1024.ppm 1  

** ppm files can view in 꿀뷰 program in Windows, or Preview program in MacOS **
