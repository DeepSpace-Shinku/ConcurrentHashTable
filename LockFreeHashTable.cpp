#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "omp.h"
#include "assert.h"
#include "sys/time.h"
#include <iostream>


typedef unsigned long long LL; // Use 64-bit unsigned long long for 64-bit system

// Number of hash table buckets
#define NUM_BUCKETS 10000

// Supported operations
#define ADD (0)
#define DELETE (1)
#define SEARCH (2)

LL items[NUM_ITEMS];    // Array of keys associated with operations
LL op[NUM_ITEMS];       // Array of operations
LL result[NUM_ITEMS];   // Array of outcomes

class __attribute__((aligned (16))) Node; // The generic node class

class AtomicReference {
public:
  LL reference;

  // Create a next field from a reference and mark bit
  AtomicReference(Node* ref, bool mark) {
    reference = (LL)(ref) | mark;
  }

  AtomicReference() {
    reference = 0;
  }

  bool CompareAndSet(Node* expectedRef, Node* newRef, bool oldMark, bool newMark);
  Node* Get(bool* marked);
  void Set(Node* newRef, bool newMark);
  Node* GetReference();
};

class LockFreeList;

// Definition of generic node class
class __attribute__((aligned (16))) Node {
public:
  LL key;
  AtomicReference next;

  Node(LL k) {
    key = k;
  }
};

// CompareAndSet wrapper
bool AtomicReference::CompareAndSet(Node* expectedRef, Node* newRef, bool oldMark, bool newMark) {
  LL oldVal = (LL)expectedRef | oldMark;
  LL newVal = (LL)newRef | newMark;
  LL oldValOut;
  bool result;

  asm(
    "lock cmpxchgq %4, %1 \n setzb %0"
    :"=qm"(result), "+m"(reference), "=a"(oldValOut)
    :"a"(oldVal), "r"(newVal)
    :
  );

  return result;
}

Node* AtomicReference::Get(bool* marked) {
  *marked = reference % 2;
  return (Node*)((reference >> 1) << 1);
}

void AtomicReference::Set(Node* newRef, bool newMark) {
  reference = (LL)newRef | newMark;
}

Node* AtomicReference::GetReference() {
  return (Node*)((reference >> 1) << 1);
}

class Window
{
  public:
    Node* pred;         // Predecessor of node holding the key being searched
    Node* curr;         // The node holding the key being searched (if present)

    Window(Node* myPred, Node* myCurr)
    {
      pred=myPred;
      curr=myCurr;
    }
};

Window 
Find(Node* head, LL key)
{
  Node* pred;
  Node* curr;
  Node* succ;
  bool marked[]={false};
  bool snip;

  retry: 
  while(true) {
     pred=head;
     curr=pred->next.GetReference();
     while(true) {
        succ=curr->next.Get(marked);
        while(marked[0]) {
           snip=pred->next.CompareAndSet(curr, succ, false, false);
           if(!snip) goto retry;
       curr=succ;
       succ=curr->next.Get(marked);
    }
    if (curr->key >= key) {
           Window* w=new Window(pred, curr);
       return *w;
        }
        pred=curr;
        curr=succ;
     }
  }
}

class LockFreeList
{
  public:
    Node* head;     // Head sentinel
    Node* tail;     // Tail sentinel

    bool Add(Node*);
    bool Add(LL, Node*);
    bool Search(LL);
    bool Delete(LL);

    LockFreeList()
    {
      head=new Node(0);

      tail=new Node((LL)0xffffffffffffffff);

      head->next.Set(tail, false);
      tail->next.Set(NULL, false);
    }

    LockFreeList(LL key)
    {
      head=new Node(key);
      tail=NULL;
      head->next.Set(NULL, false);
    }
};

bool
LockFreeList::Add(Node* pointer)
{
  LL key=pointer->key;
  while (true) {
     Window w=Find(head, key);
     Node* pred=w.pred;
     Node* curr = w.curr;
     if (curr->key==key) return false;
     else{
        pointer->next.Set(curr, false);
        if (pred->next.CompareAndSet(curr, pointer, false, false))
       return true;
     }
  }
}

bool
LockFreeList::Add(LL key, Node *n)
{
  while (true) {
      Window w=Find(head, key);
      Node* pred=w.pred;
      Node* curr = w.curr;
      if (curr->key==key) return false;
      else{
         Node* pointer=new Node(key);
         pointer->next.Set(curr, false);
         if (pred->next.CompareAndSet(curr, pointer, false, false))
        return true;
      }
   }
}

bool LockFreeList::Search(LL key) {
    bool marked = false;
    Node* curr = head->next.GetReference();
    while (curr != tail && curr->key < key) {
        curr = curr->next.GetReference();
    }
    if (curr == tail || curr->key != key) {
        return false; // Key not found or reached the end of the list.
    }
    // Check if the found node is marked.
    curr->next.Get(&marked);
    return !marked; // Return true if the node is not marked, false otherwise.
}
   
bool
LockFreeList::Delete(LL key)
{
  bool snip;
  while (true) {
     Window w=Find(head, key);
     Node* curr=w.curr;
     Node* pred=w.pred;
     if (curr->key!=key) {
        return false;
     }
     else{
        Node* succ = curr->next.GetReference();
        snip=curr->next.CompareAndSet(succ, succ, false, true);
    if (!snip) continue;
    pred->next.CompareAndSet(curr, succ, false, false);
    return true;
     }
  }
}

class LockFreeHashTable
{
  private:

    LL MakeSentinelKey(LL x)
    {

       return (LL)(0x8000000000000000|x);

    }

    LL Hash(LL key)
    {
      return key%NUM_BUCKETS;
    }
    
  public:
    
    LockFreeList* buckets[NUM_BUCKETS];

    bool Add(LL, Node*);
    bool Delete(LL);
    bool Search(LL);

    LockFreeHashTable()
    {
      buckets[0]=new LockFreeList();
      int i;
      for(i=1;i<NUM_BUCKETS;i++){
        buckets[i]=new LockFreeList(MakeSentinelKey(i));
        buckets[0]->Add(buckets[i]->head);
      }
    }

} h;

bool
LockFreeHashTable::Add(LL k, Node *n)
{
  LL key=k;
  LL b=Hash(key);
  assert(b<NUM_BUCKETS);
  return buckets[b]->Add(key, n);
}

bool LockFreeHashTable::Delete(LL k)
{
  LL key=k;
  LL b=Hash(key);
  assert(b<NUM_BUCKETS);
  return buckets[b]->Delete(key);
}

bool LockFreeHashTable::Search(LL k)
{
  LL key=k;
  LL b=Hash(key);
  assert(b<NUM_BUCKETS);
  return buckets[b]->Search(key);
}

void Thread (int tid)
{  
  int i;
#pragma omp for
  for (i=0;i<NUM_ITEMS;i++) {
     unsigned int item = items[i];
     switch(op[i]){
       case ADD:
         result[i]=10+h.Add(item, NULL);
         break;
       case DELETE:
         result[i]=20+h.Delete(item);
         break;
       case SEARCH:
         result[i]=30+h.Search(item);
         break;
     }
  }
}

int main(int argc, char** argv) {

  if (argc != 4) {
     printf("Usage: %s <percent add ops> <percent delete ops> <num threads>\n", argv[0]);
     printf("Example: %s 30 50 4\n", argv[0]);
     exit(1);
  }

  int adds = atoi(argv[1]);
  int deletes = atoi(argv[2]);
  int num_threads = atoi(argv[3]); // Number of threads from command line

  if (adds + deletes > 100) {
     printf("Sum of add and delete percentages exceeds 100.\nAborting...\n");
     exit(1);
  }

  omp_set_num_threads(num_threads); // Set the number of threads
  srand(0);
  int i;
  for(i=0;i<NUM_ITEMS;i++){
    items[i]=10+rand()%KEYS;
  }

  for(i=0;i<(NUM_ITEMS*adds)/100;i++){
    op[i]=ADD;
  }
  for(;i<(NUM_ITEMS*(adds+deletes))/100;i++){
    op[i]=DELETE;
  }
  for(;i<NUM_ITEMS;i++){
    op[i]=SEARCH;
  }
  
  struct timeval tv0,tv1;
  struct timezone tz0,tz1;

  gettimeofday(&tv0,&tz0);
  #pragma omp parallel
  {
    // printf("Thread %d of %d\n", omp_get_thread_num(), omp_get_num_threads());
    Thread(omp_get_thread_num());

  }
  gettimeofday(&tv1,&tz1);

  printf("%lf\n",((float)((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec)))/1000.0);
  
//   printf("Operation results:\n");
// for(int i = 0; i < NUM_ITEMS; i++) {
//     printf("result[%d] = %lld\n", i, result[i]);
// }
  
    // srand(time(NULL));
    // LockFreeHashTable h;

    // const int numKeys = 100;
    // LL keys[numKeys];
    // Node* nodes[numKeys];

    // // Generate and add 10 keys
    // for (int i = 0; i < numKeys; ++i) {
    //     keys[i] = rand(); // Generate a random key
    //     nodes[i] = new Node(keys[i]); // Create a node for each key
    //     bool addResult = h.Add(keys[i], nodes[i]);
    //     std::cout << "Adding key " << keys[i] << ": " << (addResult ? "Success" : "Failure") << std::endl;
    // }

    // // Search for the 10 keys
    // for (int i = 0; i < numKeys; ++i) {
    //     bool searchResult = h.Search(keys[i]);
    //     std::cout << "Searching for key " << keys[i] << ": " << (searchResult ? "Found" : "Not Found") << std::endl;
    // }

    // // Delete the 10 keys
    // for (int i = 0; i < numKeys; ++i) {
    //     bool deleteResult = h.Delete(keys[i]);
    //     std::cout << "Deleting key " << keys[i] << ": " << (deleteResult ? "Success" : "Failure") << std::endl;
    // }

    // // Search for the 10 keys again
    // for (int i = 0; i < numKeys; ++i) {
    //     bool searchResult = h.Search(keys[i]);
    //     std::cout << "Searching for key " << keys[i] << " after deletion: " << (searchResult ? "Found" : "Not Found") << std::endl;
    // }

    // // Cleanup dynamically allocated nodes
    // for (int i = 0; i < numKeys; ++i) {
    //     delete nodes[i];
    // }  
  
  
  return 0;
}
