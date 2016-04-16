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
    unsigned int P;
    unsigned int p;
    // declare bounds
    #define LOWER_BOUND 0
    #define UPPER_BOUND 30000

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
    int * unsorted_array;

    if(p == 0){
	    unsorted_array = new int[ARRAY_SIZE];
	    // data generation
	    srand(time(NULL));
	    for(int i=0; i<ARRAY_SIZE; i++){
	        unsorted_array[i] = rand() %(UPPER_BOUND-LOWER_BOUND)+LOWER_BOUND;
	    }
	}

    MPI::Init(argc, argv);
    // time the wall clock time of the execution
    double start_time,end_time;
    MPI::COMM_WORLD.Barrier();
    start_time = MPI::Wtime();

    P = MPI::COMM_WORLD.Get_size();
    p = MPI::COMM_WORLD.Get_rank();

    int interval_size = ceil((1.0*UPPER_BOUND)/P);
    
    // Portion of array to look at
    int portion = ceil((1.0*ARRAY_SIZE)/P);
    int start_index = p*portion;
    int stop_index;
    if(p==P-1){
        stop_index = ARRAY_SIZE;
    }else{
        stop_index = start_index + portion;
    }
    int sub_array_size = stop_index - start_index;
    
    // distribute parts of array to all processes
    int * send_counts = new int[P];
    if(p == 0){
	    for(int i = 0; i < P-1; ++i){
	    	send_counts[i] = sub_array_size;
	    }
	    send_counts[P-1] = ARRAY_SIZE - (P-1)*portion;
	}

    int * displs = new int[P];
    if(p == 0){
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

    for(int i = 0; i < sub_array_size; ++i){
    	sub_array.push_back(tmp_sub_array[i]);
    }

    // small buckets will be communicated to their correct owner
    vector<vector<int> > small_buckets(P,vector<int>());
    // this processor's main bucket
    vector<int> large_bucket;
    
    // place each number in the correct bucket
    for(int i = 0; i<sub_array_size; ++i){
        int bucket = (sub_array[i]-LOWER_BOUND)/interval_size; 
        if(bucket != p){
            small_buckets[bucket].push_back(sub_array[i]);
        }else{
            large_bucket.push_back(sub_array[i]);
        }
    }

    // TODO: What is requests?
    MPI::Request * requests = new MPI::Request[P];
    // distribute the size of the buckets and then the buckets to their correct owner. Non-blocking send.
    for(int i = 0; i < P; ++i){
        if(i != p){
          int send_size = small_buckets[i].size();
          MPI::COMM_WORLD.Isend(&send_size, 1, MPI::INT, i, 0);  
          requests[i] = MPI::COMM_WORLD.Isend(&small_buckets[i][0], small_buckets[i].size(), MPI::INT, i, 1);
        }
    } 

    // Receive the size of the buckets and the buckets belonging to this process. Blocking receive (in order of ranks)
    for(int i = 0; i < P; ++i){
       if(i!= p){
            // receive size of bucket first. Need to know size to be able to receive the bucket.
            int size; 
            MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, i, 0);
            int * temp = new int[size];
            MPI::COMM_WORLD.Recv(&temp[0], size, MPI::INT, i, 1);
            for(int j = 0; j<size; ++j){
                large_bucket.push_back(temp[j]);
            }
            delete[] temp;
        }
    } 
    
    // sort the bucket using sort included from algorithms library
    sort(large_bucket.begin(),large_bucket.end());

    // all the processes send their bucket size to root node
    int * size_list = new int[P];
    if(p != 0){
        int send_size = large_bucket.size();
       MPI::Request request = MPI::COMM_WORLD.Isend(&send_size, 1, MPI::INT, 0, 0);
    }else{
        // root node keeps track of the size of every bucket (required for MPI::Gatherv argument recvcounts).
        size_list[0] = large_bucket.size();
        for(int i = 1; i<P; i++){
            MPI::COMM_WORLD.Recv(&size_list[i], 1, MPI::INT, i, 0);
        }
             
    }

    // which index to put incoming data from MPI::Gatherv. (for argument displs)
    int * index = new int[P];
    if(p == 0){
        index[0] = 0;
        for(int i = 1; i<P; ++i){
            index[i] = size_list[i-1] + index[i-1];
        }
    }
    // we use Gatherv to collect all the buckets at the root node. Gatherv is used to collect varying size of data from multiple sources.
    // we use size_list to know the size of every bucket we shall receive.
    // we use index to specify at which index the receiving buckets first element shall be positioned.
    MPI::COMM_WORLD.Gatherv(&large_bucket[0], large_bucket.size(), MPI::INT, &unsorted_array[0], &size_list[0], &index[0], MPI::INT, 0);
    // end of execution. We stop and print the timer here. (Use Barrier so all processes are finished at this point.)
    MPI::COMM_WORLD.Barrier();
    end_time = MPI::Wtime();
    if(p==0){
    	printf("That took %f seconds\n",end_time-start_time);
        //for(int i = 0; i<ARRAY_SIZE; ++i){
        //    printf("%d\n", unsorted_array[i]);
        //}
    }
    delete[] send_counts;
    delete[] displs;
    delete[] tmp_sub_array;
    delete[] unsorted_array;
    delete[] index;
    delete[] size_list;
    delete[] requests;

    MPI::Finalize();

    
}