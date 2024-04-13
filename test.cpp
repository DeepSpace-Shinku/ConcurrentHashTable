#include "lbht.h"
#include <iostream>

int main() {
    lbht_list list;  // Create an instance of lbht_list

    LL testKey = 42; // Define a test key

    // Test inserting the key the first time
    bool firstInsertResult = list.Insert(testKey);
    if (firstInsertResult) {
        std::cout << "First insert successful (expected)." << std::endl;
    } else {
        std::cout << "First insert failed (unexpected)." << std::endl;
    }

    // Test inserting the same key a second time
    bool secondInsertResult = list.Insert(testKey);
    if (!secondInsertResult) {
        std::cout << "Second insert returned false as expected (key exists)." << std::endl;
    } else {
        std::cout << "Second insert successful (unexpected, error in Insert function)." << std::endl;
    }

    return 0;
}
