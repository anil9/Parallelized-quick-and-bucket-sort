#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>
#include <vector>

using namespace std;

int main(int argc, char **argv)
{
    unsigned int P;
    unsigned int p;
    int a = 0;
    int b = 20;
    int num_numbers = 20;

    int * numbers = new int[num_numbers];
    for(int i=0; i<num_numbers; i++){
        numbers[i] = rand() % b + a;
        printf("%d ", numbers[i]);
    }

    MPI::Init(argc, argv);
    P = MPI::COMM_WORLD.Get_size();
    p = MPI::COMM_WORLD.Get_rank();

    // Number interval
    int interval_size = b/P;
    int my_a = p*interval_size;
    int my_b = p*interval_size+interval_size-1;

    if(p == P-1){
        my_b = b;
    }else if(p == 0){
        my_a = a;
    }

    // Portion of array to look at. 
    int subarray_size = num_numbers/P;
    int start = p*subarray_size;
    int stop = p*subarray_size+subarray_size;

    vector<vector<int> > small_buckets(P,vector<int>());
    vector<int> large_bucket;

    for(int i = start; i<stop; ++i){
        int bucket = numbers[i]/interval_size;
        //printf("Number: %d Bucket: %d\n", numbers[i], bucket);
        if(bucket != p){
            small_buckets[bucket].push_back(numbers[i]);
        }else{
            large_bucket.push_back(numbers[i]);
        }
    }

    MPI::Request * requests = new MPI::Request[P];
    for(int i = 0; i < P; ++i){
        if(i != p){
          int send_size = small_buckets[i].size();
          MPI::COMM_WORLD.Isend(&send_size, 1, MPI::INT, i, 0);  
          requests[i] = MPI::COMM_WORLD.Isend(&small_buckets[i][0], small_buckets[i].size(), MPI::INT, i, 1);
        }
    } 

    for(int i = 0; i < P; ++i){
       if(i!= p){
            int size; 
            MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, i, 0);
            int * temp = new int[size];
            MPI::COMM_WORLD.Recv(&temp[0], size, MPI::INT, i, 1);
            for(int j = 0; j<size; ++j){
                large_bucket.push_back(temp[j]);
            }
        }
    } 
    
    

    MPI::COMM_WORLD.Barrier();
    delete[] numbers;
    
    MPI::Finalize();
}