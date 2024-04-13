# Lockbased Concurrent Hashtable





 We can achieve a good speedup when `NUM_ITEMS` is large. For example, when `NUM_ITEMS=50000000`. Specify `DNUM_THREADS` to different values to see the run time for different threads. Compile the source file with: 

```
g++ -O3 -fopenmp -o LockbasedHashTable LockbasedHashTable.cpp
```

Run the program with:

```
./LockbasedHashTable 20000000 16 100 30 50
```
Where command line arguments are: `NUM_ITEMS`, `NUM_THREADS`, `KEYS`, `adds`, `deletes`. 
