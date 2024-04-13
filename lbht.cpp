// lbht.cpp

#include "lbht.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "omp.h"
#include "sys/time.h"

// Constructor
lbht_node::lbht_node(LL k) : key(k), next(NULL) {}

// lbht_list constructor
lbht_list::lbht_list()
{
    head = new lbht_node(0);  // Initialize head with dummy value
    omp_init_lock(&listLock); // Initialize the lock
}

// lbht_list destructor
lbht_list::~lbht_list()
{
    omp_destroy_lock(&listLock); // Destroy the lock
    // Free list nodes
    lbht_node *current = head;
    while (current != NULL)
    {
        lbht_node *next = current->next;
        delete current;
        current = next;
    }
}

// Insert method for lbht_list
bool lbht_list::Insert(LL key) {
    omp_set_lock(&listLock); // Acquire lock

    lbht_node* pred = head;
    lbht_node* curr = head->next;

    // Traverse the list to find the insert location or existing key
    while (curr != nullptr && curr->key < key) {
        pred = curr;
        curr = curr->next;
    }

    if (curr != nullptr && curr->key == key) {
        omp_unset_lock(&listLock); // Key found, release lock and return false
        return false;
    } else {
        // Key not found, insert new node
        lbht_node* newNode = new lbht_node(key);
        newNode->next = curr;
        pred->next = newNode;
        omp_unset_lock(&listLock); // Release lock
        return true;
    }
}

// Delete method for lbht_list
bool lbht_list::Delete(LL key)
{
    omp_set_lock(&listLock); // Lock the list
    lbht_node *prev = head;
    lbht_node *curr = head->next;
    while (curr != NULL)
    {
        if (curr->key == key)
        {
            prev->next = curr->next;
            delete curr;
            omp_unset_lock(&listLock); // Unlock the list
            return true;               // Key found and deleted
        }
        prev = curr;
        curr = curr->next;
    }
    omp_unset_lock(&listLock); // Unlock the list
    return false;              // Key not found
}

// Contain method for lbht_list
bool lbht_list::Contain(LL key)
{
    omp_set_lock(&listLock); // Lock the list
    lbht_node *curr = head->next;
    while (curr != NULL)
    {
        if (curr->key == key)
        {
            omp_unset_lock(&listLock); // Unlock the list
            return true;               // Key found
        }
        curr = curr->next;
    }
    omp_unset_lock(&listLock); // Unlock the list
    return false;              // Key not found
}

// lbht constructor
lbht::lbht()
{
    for (int i = 0; i < buckets_ct; ++i)
    {
        buckets[i] = new lbht_list();
    }
}

// lbht destructor
lbht::~lbht()
{
    for (int i = 0; i < buckets_ct; ++i)
    {
        delete buckets[i];
    }
}

// Hash method for lbht
LL lbht::Hash(LL key)
{
    return key % buckets_ct;
}

// Insert method for lbht
bool lbht::Insert(LL key)
{
    LL index = Hash(key);
    return buckets[index]->Insert(key);
}

// Delete method for lbht
bool lbht::Delete(LL key)
{
    LL index = Hash(key);
    return buckets[index]->Delete(key);
}

// Contain method for lbht
bool lbht::Contain(LL key)
{
    LL index = Hash(key);
    return buckets[index]->Contain(key);
}

// Main function
int main(int argc, char **argv)
{
    if (argc != 6)
    {
        printf("Usage: %s <Number of keys> <Number of threads> <Range of keys> <Insert methods percentage> <Delete methods percentage>\n", argv[0]);
        exit(1);
    }

    keys_ct = atoi(argv[1]);
    threads_ct = atoi(argv[2]);
    keys_rg = atoi(argv[3]);
    int inserts = atoi(argv[4]);
    int deletes = atoi(argv[5]);

    keys = new LL[keys_ct];
    mthds = new LL[keys_ct];
    outs = new LL[keys_ct];

    if (inserts + deletes > 100)
    {
        printf("Sum of Insert and Delete percentages is greater than 100%.\nexiting...\n");
        exit(1);
    }

    srand(0);

    for (int i = 0; i < keys_ct; i++)
    {
        keys[i] = 10 + rand() % keys_rg; // keys_rg is the range of integer keys
    }

    int totalIns = (keys_ct * inserts) / 100;
    int totalDeletes = (keys_ct * deletes) / 100;

    for (int i = 0; i < totalIns; i++)
    {
        mthds[i] = INSERT;
    }
    for (int i = totalIns; i < totalIns + totalDeletes; i++)
    {
        mthds[i] = DELETE;
    }
    for (int i = totalIns + totalDeletes; i < keys_ct; i++)
    {
        mthds[i] = CONTAIN;
    }

    lbht h;

    struct timeval val_s, val_e;

    gettimeofday(&val_s, NULL);

    // Parallel section
    // We will use OpenMP to parallelize this loop
#pragma omp parallel num_threads(threads_ct)
    {
        int tid = omp_get_thread_num(); // Get the thread ID in the current context
#pragma omp for
        for (int i = tid; i < keys_ct; i += threads_ct)
        {
            // Perform operations based on the mthds array
            switch (mthds[i])
            {
            case INSERT:
                outs[i] = 200 + h.Insert(keys[i]);
                break;
            case DELETE:
                outs[i] = 400 + h.Delete(keys[i]);
                break;
            case CONTAIN:
                outs[i] = 600 + h.Contain(keys[i]);
                break;
            }
        }
    }

    gettimeofday(&val_e, NULL);

    // Calculate elapsed time in ms
    printf("%lf\n", ((double)((val_e.tv_sec - val_s.tv_sec) * 1000000 + (val_e.tv_usec - val_s.tv_usec))) / 1000.0);

    // printf("Operation results:\n");
    // for(int i = 0; i < keys_ct; i++) {
    //     printf("result[%d] = %lld\n", i, outs[i]);
    // }

    delete[] keys;
    delete[] mthds;
    delete[] outs;

    // srand(time(NULL));
    // lbht h2;

    // const int numKeys = 20;
    // LL keys[numKeys];
    // lbht_node* nodes[numKeys];

    // // Generate and add 10 keys
    // for (int i = 0; i < numKeys; ++i) {
    //     keys[i] = rand(); // Generate a random key
    //     // nodes[i] = new lbht_node(keys[i]); // Create a node for each key
    //     bool addResult = h2.Insert(keys[i]);
    //     std::cout << "Adding key " << keys[i] << ": " << (addResult ? "Success" : "Failure") << std::endl;
    // }

    // // Contain for the 10 keys
    // for (int i = 0; i < numKeys; ++i) {
    //     bool searchResult = h2.Contain(keys[i]);
    //     std::cout << "Searching for key " << keys[i] << ": " << (searchResult ? "Found" : "Not Found") << std::endl;
    // }

    // // Delete the 10 keys
    // for (int i = 0; i < numKeys; ++i) {
    //     bool deleteResult = h2.Delete(keys[i]);
    //     std::cout << "Deleting key " << keys[i] << ": " << (deleteResult ? "Success" : "Failure") << std::endl;
    // }

    // // Contain for the 10 keys again
    // for (int i = 0; i < numKeys; ++i) {
    //     bool searchResult = h2.Contain(keys[i]);
    //     std::cout << "Searching for key " << keys[i] << " after deletion: " << (searchResult ? "Found" : "Not Found") << std::endl;
    // }

    return 0;
}
