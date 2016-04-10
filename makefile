all: 
	mpic++  main.cpp -lm
	mpirun -np 4 ./a.out