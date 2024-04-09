# Lockbased Concurrent Hashtable





 We can achieve a good speedup when `NUM_ITEMS` is large. For example, when `NUM_ITEMS=50000000`. Specify `DNUM_THREADS` to different values to see the run time for different threads. Compile the source file with: 

```
g++ -O3 -fopenmp -DNUM_ITEMS=50000000 -DNUM_THREADS=1 -DKEYS=100 -o ./LockFreeHashTable
```

Run the program with:

```
./LockbasedHashTable 30 50
```
