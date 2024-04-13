#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "omp.h"
#include "assert.h"
#include "sys/time.h"

#if __WORDSIZE == 64
typedef unsigned long long LL;
#else
typedef unsigned int LL;
#endif

// Number of hash table buckets
#define NUM_BUCKETS 10000

// Supported operations
#define ADD (0)
#define DELETE (1)
#define SEARCH (2)

LL* items;     
LL* op;        
LL* result;   
int NUM_ITEMS, NUM_THREADS, KEYS;

class Node
{
public:
    LL key;
    Node* next;

    Node(LL k) : key(k), next(NULL) {}
};

class LockBasedList
{
private:
    Node* head;
    omp_lock_t listLock;

public:
    LockBasedList()
    {
        head = new Node(0); // Initialize head with dummy value
        omp_init_lock(&listLock); // Initialize the lock
    }

    ~LockBasedList()
    {
        omp_destroy_lock(&listLock); // Destroy the lock
        // Free list nodes
        Node* current = head;
        while(current != NULL) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }

    bool Add(LL key) {
      omp_set_lock(&listLock); // Acquire lock

      Node* pred = head;
      Node* curr = head->next;

      // Traverse the list to find the insert location or existing key
      while (curr != nullptr && curr->key < key) {
        pred = curr;
        curr = curr->next;
      }

      if (curr != nullptr && curr->key == key) {
        omp_unset_lock(&listLock); // Key found, release lock and return false
        return false;
      } else {
        Node* newNode = new Node(key);
        newNode->next = curr;
        pred->next = newNode;
        omp_unset_lock(&listLock); // Release lock
        return true;
      }
    }

    bool Delete(LL key)
    {
        omp_set_lock(&listLock); // Lock the list
        Node *prev = head;
        Node *curr = head->next;
        while(curr != NULL) {
            if(curr->key == key) {
                prev->next = curr->next;
                delete curr;
                omp_unset_lock(&listLock); // Unlock the list
                return true; // Key found and deleted
            }
            prev = curr;
            curr = curr->next;
        }
        omp_unset_lock(&listLock); // Unlock the list
        return false; // Key not found
    }

    bool Search(LL key)
    {
        omp_set_lock(&listLock); // Lock the list
        Node* curr = head->next;
        while(curr != NULL) {
            if(curr->key == key) {
                omp_unset_lock(&listLock); // Unlock the list
                return true; // Key found
            }
            curr = curr->next;
        }
        omp_unset_lock(&listLock); // Unlock the list
        return false; // Key not found
    }
};

class LockBasedHashTable
{
private:
    LockBasedList* buckets[NUM_BUCKETS];

    LL Hash(LL key)
    {
        return key % NUM_BUCKETS;
    }

public:
    LockBasedHashTable()
    {
        for(int i = 0; i < NUM_BUCKETS; ++i)
            buckets[i] = new LockBasedList();
    }

    ~LockBasedHashTable()
    {
        for(int i = 0; i < NUM_BUCKETS; ++i)
            delete buckets[i];
    }

    bool Add(LL key)
    {
        LL index = Hash(key);
        return buckets[index]->Add(key);
    }

    bool Delete(LL key)
    {
        LL index = Hash(key);
        return buckets[index]->Delete(key);
    }

    bool Search(LL key)
    {
        LL index = Hash(key);
        return buckets[index]->Search(key);
    }
};

#include <omp.h> // Already included for lock management, also used for parallelism

// Assume NUM_THREADS is defined somewhere
// omp.h is included for OpenMP support

int main(int argc, char** argv) {
    if (argc != 6) {
        printf("Usage: %s <NUM_ITEMS> <NUM_THREADS> <KEYS> <percent add ops> <percent delete ops>\n", argv[0]);
        exit(1);
    }

    // Parse the command-line arguments for the parameters
    NUM_ITEMS = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);
    KEYS = atoi(argv[3]);
    int adds = atoi(argv[4]);
    int deletes = atoi(argv[5]);

    // Allocate memory for the arrays based on NUM_ITEMS
    items = new LL[NUM_ITEMS];
    op = new LL[NUM_ITEMS];
    result = new LL[NUM_ITEMS];

    if (adds + deletes > 100) {
        printf("Sum of add and delete percentages exceeds 100.\nAborting...\n");
        exit(1);
    }

    srand(0);
    // Initialize operation arrays similar to the original code

    for (int i = 0; i < NUM_ITEMS; i++) {
        items[i] = 10 + rand() % KEYS; // KEYS is the range of integer keys
    }

    int totalAdds = (NUM_ITEMS * adds) / 100;
    int totalDeletes = (NUM_ITEMS * deletes) / 100;

    for (int i = 0; i < totalAdds; i++) {
        op[i] = ADD;
    }
    for (int i = totalAdds; i < totalAdds + totalDeletes; i++) {
        op[i] = DELETE;
    }
    for (int i = totalAdds + totalDeletes; i < NUM_ITEMS; i++) {
        op[i] = SEARCH;
    }

    LockBasedHashTable h; // Use LockBasedHashTable instead of LockFreeHashTable

    struct timeval tv0, tv1;
    struct timezone tz0, tz1;

    gettimeofday(&tv0, &tz0);

    // Parallel section
    // We will use OpenMP to parallelize this loop
#pragma omp parallel num_threads(NUM_THREADS)
    {
        int tid = omp_get_thread_num(); // Get the thread ID in the current context
        #pragma omp for
        for (int i = tid; i < NUM_ITEMS; i += NUM_THREADS) {
            // Perform operations based on the op array
            switch(op[i]) {
                case ADD:
                    result[i] = 10 + h.Add(items[i]);
                    break;
                case DELETE:
                    result[i] = 20 + h.Delete(items[i]);
                    break;
                case SEARCH:
                    result[i] = 30 + h.Search(items[i]);
                    break;
            }
        }
    }

    gettimeofday(&tv1, &tz1);

    // Calculate and print elapsed time in ms
    printf("%lf\n", ((double)((tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec))) / 1000.0);

    delete[] items;
    delete[] op;
    delete[] result;
    return 0;
}
