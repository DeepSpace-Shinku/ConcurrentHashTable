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

LL items[NUM_ITEMS];     // Array of keys associated with operations
LL op[NUM_ITEMS];        // Array of operations
LL result[NUM_ITEMS];    // Array of outcomes

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

    bool Add(LL key)
    {
        omp_set_lock(&listLock); // Lock the list
        Node* newNode = new Node(key);
        // Always add at the beginning for simplicity (not sorted)
        newNode->next = head->next;
        head->next = newNode;
        omp_unset_lock(&listLock); // Unlock the list
        return true; // Simplification: always returns true
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
    if (argc != 3) {
        printf("Need two arguments: percent add ops and percent delete ops (e.g., 30 50 for 30%% add and 50%% delete).\nAborting...\n");
        exit(1);
    }

    int adds = atoi(argv[1]);
    int deletes = atoi(argv[2]);

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

    return 0;
}

