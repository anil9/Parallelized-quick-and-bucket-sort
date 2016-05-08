#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include <math.h>


using namespace std;


void merge (vector<int>& sub_array, int* received_array, int received_size);

int main(int argc, char **argv)
{
    unsigned int P;
    unsigned int p;
    int * unsorted_array;
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
    
    MPI::Init(argc, argv);
    // time the wall clock time of the execution
    double start_time,end_time;
    MPI::COMM_WORLD.Barrier();
    start_time = MPI::Wtime();

    P = MPI::COMM_WORLD.Get_size();
    p = MPI::COMM_WORLD.Get_rank();

    // Create array of unsorted elements in root 
    if(p == 0){
	    unsorted_array = new int[ARRAY_SIZE];
	    int seed = 42;
	    // data generation
	    srand(seed);
	    for(int i=0; i<ARRAY_SIZE; i++){
	        unsorted_array[i] = rand() %(UPPER_BOUND-LOWER_BOUND)+LOWER_BOUND;
	    }
	}

	// Portion of array for eacy processor
    int portion = ceil((1.0*ARRAY_SIZE)/P);
    
    int start_index = p*portion;
    int stop_index;
    if(p==P-1){
        stop_index = ARRAY_SIZE;
    }else{
        stop_index = start_index+portion;
    }

    int	sub_array_size = stop_index - start_index;
    
    // Setup for distribution of subarrays to all processors
    int * send_counts;
    if(p == 0){
    	send_counts = new int[P];
	    for(int i = 0; i < P-1; ++i){
	    	send_counts[i] = sub_array_size;
	    }
	    send_counts[P-1] = ARRAY_SIZE - (P-1)*portion;
	}

    int * displs;
    if(p == 0){
    	displs = new int[P];
	    for(int i = 0; i < P; ++i){
	    	displs[i] = i*sub_array_size;
	    }
	}


    // we use Scatterv to distribute parts of the array at the root node. Scatterv is used to send varying size of data from root node.
    // we use send_counts to know the amount of elements we shall send to each processor.
    // we use displs to specify the index of the first element of each processor in the send buffer.
    int * tmp_sub_array = new int[sub_array_size];
    MPI::COMM_WORLD.Scatterv(&unsorted_array[0], &send_counts[0], &displs[0], MPI::INT, &tmp_sub_array[0], sub_array_size, MPI::INT, 0);
   
    vector<int> sub_array;
    sub_array.reserve(sub_array_size);	

    for(int i = 0; i < sub_array_size; ++i){
    	sub_array.push_back(tmp_sub_array[i]);
    }

 	// Local sort
    sort(sub_array.begin(), sub_array.end());
    
    // Sending data back and merging 
    int step = 1;
    while(step < P){
    	// Processor is in use next step (parent)
        if(p % (2*step) == 0){
            int right_child = p+step;
            if(right_child < P){
                int receiving_size;
                MPI::COMM_WORLD.Recv(&receiving_size, 1, MPI::INT, right_child, 0);
                int * receiving_array = new int[receiving_size];
                MPI::COMM_WORLD.Recv(&receiving_array[0], receiving_size, MPI::INT, right_child, 1);
                merge(sub_array, receiving_array, receiving_size);
                delete[] receiving_array;
            }
        } else {

            int parent = p-step;
            int send_size = sub_array.size();
            MPI::COMM_WORLD.Isend(&send_size, 1, MPI::INT, parent, 0);
            MPI::COMM_WORLD.Isend(&sub_array[0], send_size, MPI::INT, parent, 1);
            // child done after sending
            break;
        }
        step *=2;

    }

    // end of execution. We stop and print the timer here. (Use Barrier so all processes are finished at this point.)
    MPI::COMM_WORLD.Barrier();
    end_time = MPI::Wtime();

    if(p==0){   
        printf("That took %f seconds\n",end_time-start_time);       
    }
    
    delete[] tmp_sub_array;
    if(p == 0) {
    	delete[] send_counts;
    	delete[] displs;
    	delete[] unsorted_array;	
    } 
    MPI::Finalize();


}

void merge (vector<int>& sub_array, int* received_array, int received_size){
    
    int c1, c2; 
    c1=c2=0;
    vector<int> resulting_array;
    resulting_array.reserve(sub_array.size()+received_size);

    while(c1 < sub_array.size() && c2 < received_size){
        if(sub_array[c1] <= received_array[c2]){
            resulting_array.push_back(sub_array[c1]);
            
            c1++;
        } else {
            resulting_array.push_back(received_array[c2]);
            
            c2++;
        }
    }
    while(c1 < sub_array.size()){
        resulting_array.push_back(sub_array[c1]);
        
        c1++;
    }

    while(c2 < received_size){
        resulting_array.push_back(received_array[c2]);
        c2++;
    }
    sub_array = resulting_array;
}


