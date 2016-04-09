all: 
	mpic++ -O3 main.cpp -lm
	mpirun -np 2 ./a.out 4