// lbht.cpp

#include "lbht.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "omp.h"
#include "sys/time.h"
#include <iostream>

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

int main() {
    lbht_list list;  // Create an instance of lbht_list


    LL duplicateKey = 1; 
    #pragma omp parallel for
    for (int i = 0; i < 5; i++) {
        if (!list.Insert(duplicateKey)) {
            std::cout << "Thread " << omp_get_thread_num() << ": Duplicate insert of key " << duplicateKey << " detected and prevented." << std::endl;
        } else if (i == 0) { // Assume first thread successfully inserts it
            std::cout << "Thread " << omp_get_thread_num() << ": First insert of duplicate key " << duplicateKey << " successful." << std::endl;
        }
    }

    return 0;
}
