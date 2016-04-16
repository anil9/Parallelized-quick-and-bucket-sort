merge:
	mpic++ merge_sort.cpp -o merge.out -lm
	mpirun -np 4 ./merge.out 1000000

bucket: 
	mpic++ bucket_sort.cpp -o bucket.out -lm
	mpirun -np 4 ./bucket.out 1000000