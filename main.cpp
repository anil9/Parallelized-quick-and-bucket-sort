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
    int a = 100;
    int b = 300;
    int num_numbers = 100;
    int * final_bucket = new int[num_numbers];

    int * numbers = new int[num_numbers];
    srand(time(NULL));
    for(int i=0; i<num_numbers; i++){
        numbers[i] = rand() %(b-a+1)+a;
      //  printf("%d ", numbers[i]);
    }

    MPI::Init(argc, argv);
    P = MPI::COMM_WORLD.Get_size();
    p = MPI::COMM_WORLD.Get_rank();

    // Number interval
    //int rest = 0;
    /*if(b%P != 0){
        rest = b%P;
    }*/
    int interval_size = ceil((1.0*b)/P);
    /*if(p == P-1){
        interval_size += rest;
    }*/
    /*int my_a = p*interval_size;
    int my_b = p*interval_size+interval_size-1;

    if(p == P-1){
        my_b = b;
    }else if(p == 0){
        my_a = a;
    }
    */
    // Portion of array to look at. 
    
    int subarray_size = num_numbers/P;
    int start = p*subarray_size;
    int stop;
    if(p==P-1){
        stop = num_numbers;
        //stop = p*subarray_size+subarray_size+rest;
    }else{
        stop = p*subarray_size+subarray_size;
    }

    vector<vector<int> > small_buckets(P,vector<int>());
    vector<int> large_bucket;
    

    for(int i = start; i<stop; ++i){
        int bucket = (numbers[i]-a)/interval_size; 
    
        printf("Number: %d Bucket: %d\n", numbers[i], bucket);
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
            delete[] temp;
        }
    } 
    delete[] requests;
    sort(large_bucket.begin(),large_bucket.end());
    int * size_list = new int[P];
    if(p != 0){
        int send_size = large_bucket.size();
       MPI::Request request = MPI::COMM_WORLD.Isend(&send_size, 1, MPI::INT, 0, 0);
    }else{
       // size_list = new int[P];
        size_list[0] = large_bucket.size();
        for(int i = 1; i<P; i++){
            MPI::COMM_WORLD.Recv(&size_list[i], 1, MPI::INT, i, 0);
        }
             
    }


    int * index = new int[P];
    if(p == 0){
        index[0] = 0;
        for(int i = 1; i<P; ++i){
            
            index[i] = size_list[i-1] + index[i-1];
        }
    }

    MPI::COMM_WORLD.Gatherv(&large_bucket[0], large_bucket.size(), MPI::INT, &final_bucket[0], &size_list[0], &index[0], MPI::INT, 0);

    if(p==0){
        for(int i = 0; i<num_numbers; ++i){
            printf("%d ", final_bucket[i]);
        }
    }

    MPI::COMM_WORLD.Barrier();
    delete[] numbers;
    delete[] index;
    delete[] final_bucket;
    delete[] size_list;

    MPI::Finalize();

    
}