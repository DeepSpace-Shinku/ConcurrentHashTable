// lbht.h

#ifndef LBHT_H
#define LBHT_H

#include "omp.h"

typedef unsigned long long LL;

#define buckets_ct 10000

#define INSERT (0)
#define DELETE (1)
#define CONTAIN (2)

LL *keys;
LL *mthds;
LL *outs;
int keys_ct, threads_ct, keys_rg;

class lbht_node
{
public:
    LL key;
    lbht_node *next;
    lbht_node(LL k);
};

class lbht_list
{
private:
    lbht_node *head;
    omp_lock_t listLock;

public:
    lbht_list();
    ~lbht_list();
    bool Insert(LL key);
    bool Delete(LL key);
    bool Contain(LL key);
};

class lbht
{
private:
    lbht_list *buckets[buckets_ct];
    LL Hash(LL key);

public:
    lbht();
    ~lbht();
    bool Insert(LL key);
    bool Delete(LL key);
    bool Contain(LL key);
};

#endif // LBHT_H
