#ifndef AVLTREE
#define AVLTREE
#define EVER ; ;

#include <stdbool.h>

/* 
    Chose to do an avl tree because I was interested, may replace this in the
    future just to try out other dynamic data structres, would like to try out
    skip lists, also thought of a double hashed table on a dynamic array of
    linked lists, and everytime it needs to increase in size we rehash everythin
    giving us O(n) amaortised expected insertion time, if i don't double hash it's
    O(1)
    keep a value that says how much in the hash tree so we know when to expand
*/

struct avltreenode {
    void *value;
    unsigned long leftSubtreeHeight;
    unsigned long rightSubtreeHeight;
    
    struct avltreenode *leftChild;
    struct avltreenode *rightChild;
    struct avltreenode *parent;
};

typedef struct avltreenode avlTreeNode;

typedef struct avltree {
    avlTreeNode *root;
    int (*comparator) (const void *, const void *);
    unsigned long long size;
} avlTree;


//Only ever send malloced values
bool insert_node (avlTree *tree, void *insert); 
//free remove and search after calling - but never insert
bool delete_node (avlTree *tree, void *remove); 
//strictly greater than
void *successor_search (avlTree *tree, void *search);
//less than or equal to
void *predecessor_search (avlTree *tree, void *search);
//equal to
void *dictionary_lookup (avlTree *tree, void *search);
//free sorted keys after wards but not each void * individually
//last key given has to be NULL, thats how we know the end
avlTree *sorted_construction (void **sortedKeys, 
        int (*comparator) (const void *, const void *));
//free tree after callinng this - this gets all values sorted
//last element in array is NULL
//remember to free what is returned and if nesscaary looping through and freeing
//all values
void **get_array (avlTree *tree);
//toFree is weather or not to free the values, note get_array only copies value
//location not actual value, so keep that in mind when using it
void free_tree(avlTree *tree, bool toFree);

#endif
