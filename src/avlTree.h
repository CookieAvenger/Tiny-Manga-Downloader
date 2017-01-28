#ifndef AVLTREE
#define AVLTREE
#define EVER ; ;

#include <stdbool.h>

struct avltreenode {
    void *value;
    int leftSubtreeHeight;
    int rightSubtreeHeight;
    
    struct avltreenode *leftChild;
    struct avltreenode *rightChild;
    struct avltreenode *parent;
};

typedef struct avltreenode avlTreeNode;

typedef struct avltree {
    avlTreeNode *root;
    int (*comparator) (const void *, const void *);
} avlTree;


//Only ever send malloced values
bool insert_node(avlTree *tree, void *insert); 
//free remove and search after calling - but never insert
bool delete_node(avlTree *tree, void *remove); 
void *successor_search(avlTree *tree, void *search);
void *predecessor_search(avlTree *tree, void *search);
void *dictionary_lookup(avlTree *tree, void *search);
//do a sanity check when constructing and ensure it actually is sorted
//just comparator every side ny side term then fast construct O(n) time
//if not just insert each term one by one - O(nlogn) same time as merge sorting
//and fast constructing, so insert instead
//free sorted keys after wards but not each void * individually
avlTree *fast_construction(void **sortedKeys, 
        int (*comparator) (const void *, const void *));
//free tree after callinng this - this gets all values sorted
void **get_array(avlTree *tree);

#endif
