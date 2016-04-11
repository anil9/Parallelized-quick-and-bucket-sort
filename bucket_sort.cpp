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
    // declare size of array to sort
    #define ARRAY_SIZE 10000000

    int * sorted_array = new int[ARRAY_SIZE];
    int * unsorted_array = new int[ARRAY_SIZE];

    srand(time(NULL));
    for(int i=0; i<ARRAY_SIZE; i++){
        unsorted_array[i] = rand() %(UPPER_BOUND-LOWER_BOUND)+LOWER_BOUND;
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
    int subarray_size = ARRAY_SIZE/P;
    int start_index = p*subarray_size;
    int stop_index;
    if(p==P-1){
        stop_index = ARRAY_SIZE;
        //stop = p*subarray_size+subarray_size+rest;
    }else{
        stop_index = p*subarray_size+subarray_size;
    }
    // small buckets will be communicated to their correct owner
    vector<vector<int> > small_buckets(P,vector<int>());
    // this processor's main bucket
    vector<int> large_bucket;
    
    // place each number in the correct bucket
    for(int i = start_index; i<stop_index; ++i){
        int bucket = (unsorted_array[i]-LOWER_BOUND)/interval_size; 
        if(bucket != p){
            small_buckets[bucket].push_back(unsorted_array[i]);
        }else{
            large_bucket.push_back(unsorted_array[i]);
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
    delete[] requests;
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
    MPI::COMM_WORLD.Gatherv(&large_bucket[0], large_bucket.size(), MPI::INT, &sorted_array[0], &size_list[0], &index[0], MPI::INT, 0);
    // end of execution. We stop and print the timer here. (Use Barrier so all processes are finished at this point.)
    MPI::COMM_WORLD.Barrier();
    end_time = MPI::Wtime();
    if(p==0){
    	printf("That took %f seconds\n",end_time-start_time);
        //for(int i = 0; i<ARRAY_SIZE; ++i){
          //  printf("%d ", sorted_array[i]);
        //}
    }
    delete[] unsorted_array;
    delete[] index;
    delete[] sorted_array;
    delete[] size_list;

    MPI::Finalize();

    
}