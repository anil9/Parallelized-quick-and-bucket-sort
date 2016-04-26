#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include <math.h>

using namespace std;

int main(int argc, char **argv)
{
 // declare bounds
    #define LOWER_BOUND 0
    #define UPPER_BOUND 10000000

    if(argc < 2){
        fprintf(stderr,"No size N given");
        exit(EXIT_FAILURE);
    }
    // declare size of array to sort
    int ARRAY_SIZE = atoi(argv[1]);
    if(ARRAY_SIZE <= 0){
        fprintf(stderr,"Array size cannot be negative");
        exit(EXIT_FAILURE);   
    }
    vector<int> unsorted_vector(ARRAY_SIZE);

	    
	    // data generation
	    int seed = 42;
	    srand(seed);
	    for(int i=0; i<ARRAY_SIZE; i++){
	        unsorted_vector[i] = rand() %(UPPER_BOUND-LOWER_BOUND)+LOWER_BOUND;
	        
	    }
	
	MPI::Init(argc, argv);
	 // time the wall clock time of the execution
    double start_time,end_time;
    MPI::COMM_WORLD.Barrier();
    start_time = MPI::Wtime();
    // sort the bucket using sort included from algorithms library
    sort(unsorted_vector.begin(),unsorted_vector.end());
    end_time = MPI::Wtime();

    printf("That took %f seconds\n",end_time-start_time);
    /*for(int i = 0; i<ARRAY_SIZE; ++i){
            printf("%d\n", unsorted_vector[i]);
        }
        */

}