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
    
    int * unsorted_array = new int[ARRAY_SIZE];

    // data generation
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

    int sub_array_size = ARRAY_SIZE/P;
    
    vector<int> sub_array;
    
    int start_index = p*sub_array_size;
    int stop_index;
    if(p==P-1){
        stop_index = ARRAY_SIZE;
        //stop = p*subarray_size+subarray_size+rest;
    }else{
        stop_index = start_index+sub_array_size;
    }
    //printf("start: %d, stop: %d", start_index, stop_index);
    for(int i = start_index; i < stop_index; ++i){
        sub_array.push_back(unsorted_array[i]);
    }


    sort(sub_array.begin(), sub_array.end());
    

    int step = 1;
    while(step < P){

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

            int parent = p-step;    // maybe check for negative parent
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
        /*for(int i = 0; i<sub_array.size(); ++i){
            printf("%d\n", sub_array[i]);
        }
        */
        printf("That took %f seconds\n",end_time-start_time);
        
        
    }
    delete[] unsorted_array;
    MPI::Finalize();


}

void merge (vector<int>& sub_array, int* received_array, int received_size){
    
    int c1, c2; 
    c1=c2=0;
    vector<int> resulting_array;

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


