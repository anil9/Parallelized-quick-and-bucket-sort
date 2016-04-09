all: 
	mpic++  main.cpp -lm
	mpirun -np 19 ./a.out