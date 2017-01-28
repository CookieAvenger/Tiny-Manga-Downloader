#include "avlTree.h"
#include <stdlib.h>

//NEED TO MAKE REBALANCE METHOD THAT CALLS LEFT-LEFT RIGHT-LEFT ect.

//p search type is predecessor, s search type is successor, and d is dictionary
avlTreeNode *internal_search(avlTree *tree, void *search
        , char searchType) {
    avlTreeNode *toReturn = NULL;
    avlTreeNode *currentNode = tree->root;
    for (EVER) {
        if (currentNode == NULL) {
            return toReturn;
        }
        int result = tree->comparator(currentNode->value, search);
        if (result == 0) {
            toReturn = currentNode;
            return toReturn;
        } else if (result > 0) {
            if (searchType == 's') {
                toReturn = currentNode;
            }
            currentNode = currentNode->leftChild;
        } else {
            if (searchType == 'p') {
                toReturn = currentNode;
            }
            currentNode = currentNode->rightChild;
        }
    }
}

void *successor_search(avlTree *tree, void *search) {
    avlTreeNode *foundNode = internal_search(tree, search, 's');
    if (foundNode == NULL) {
        return NULL;
    }
    return foundNode->value;
}

void *predecessor_search (avlTree *tree, void *search) {
    avlTreeNode *foundNode = internal_search(tree, search, 'p');
    if (foundNode == NULL) {
        return NULL;
    }
    return foundNode->value;
}

void *dictionary_lookup (avlTree *tree, void *search) {
    avlTreeNode *foundNode = internal_search(tree, search, 'd');
    if (foundNode == NULL) {
        return NULL;
    }
    return foundNode->value;
}

//If never used remove
bool is_leaf(avlTreeNode *node) {
    if ((node->leftChild == NULL) && (node->rightChild == NULL)) {
        return true;
    }
    return false;
}

avlTreeNode *create_new_node(void *newValue) {
    avlTreeNode *newNode = (avlTreeNode *) malloc(sizeof(avlTreeNode));
    newNode->value = newValue;
    newNode->leftSubtreeHeight = newNode->rightSubtreeHeight = 0;
    newNode->leftChild = newNode->rightChild = newNode->parent = NULL;
    return newNode;
}

//Updates the height of the currentNode and return the imbalance
int update_height(avlTreeNode *currentNode) {
    int maxHeight = -1;
    if (currentNode->leftChild != NULL) {
        maxHeight = currentNode->leftChild->leftSubtreeHeight;
        if (maxHeight < currentNode->leftChild->rightSubtreeHeight) {
            maxHeight = currentNode->leftChild->rightSubtreeHeight;
        }
    }
    currentNode->leftSubtreeHeight = maxHeight++;
    maxHeight = -1;
    if (currentNode->rightChild != NULL) {
        maxHeight = currentNode->rightChild->leftSubtreeHeight;
        if (maxHeight < currentNode->rightChild->rightSubtreeHeight) {
            maxHeight = currentNode->rightChild->rightSubtreeHeight;
        }
    } 
    currentNode->rightSubtreeHeight = maxHeight++;
    return ((currentNode->leftSubtreeHeight) - (currentNode->leftSubtreeHeight));
}

//If null parent involved root has changed
//'l'rotation type gives left-left, 'r' gives right-right
void single_rotation (avlTree *tree, avlTreeNode *alpha, char rotationType) {
    //alpha = a, beta = b, Bravo = B
    avlTreeNode *beta, *Bravo;
    if (rotationType == 'l') {
        beta = alpha->leftChild;
        Bravo = beta->rightChild;
        alpha->leftChild = Bravo;
        alpha->leftSubtreeHeight = beta->rightSubtreeHeight;
        beta->rightChild = alpha;
        beta->rightSubtreeHeight = alpha->leftSubtreeHeight + 1;
    } else {
        beta = alpha->rightChild;
        Bravo = beta->leftChild;
        alpha->rightChild = Bravo;
        alpha->rightSubtreeHeight = beta->leftSubtreeHeight;
        beta->leftChild = alpha;
        beta->leftSubtreeHeight = alpha->rightSubtreeHeight + 1;
    }
    beta->parent = alpha->parent;
    //or should i tree->root == alpha?
    if (beta->parent == NULL) {
        tree->root = beta;
    } else if (tree->comparator(beta->parent->leftChild->value, alpha->value) == 0) {
        beta->parent->leftChild = beta;
    } else {
        beta->parent->rightChild = beta;
    }
    alpha->parent = beta;
    Bravo->parent = alpha;
}

//If null parent involved root has changed
//'l'rotation type gives left-right, 'r' gives right-left
void double_rotation (avlTree *tree, avlTreeNode *alpha, char rotationType) {
    //alpha = a, beta = b, coulomb = c, Bravo = B, Charlie = C
    avlTreeNode *beta, *coulomb, *Bravo, *Charlie;
    if (rotationType == 'l') {
        beta = alpha->leftChild;
        coulomb = beta->rightChild;
        Bravo = coulomb->leftChild;
        Charlie = coulomb->rightChild;
        beta->rightChild = Bravo;
        beta->rightSubtreeHeight = coulomb->leftSubtreeHeight;
        alpha->leftChild = Charlie;
        alpha->leftSubtreeHeight = coulomb->rightSubtreeHeight;
        coulomb->rightChild = alpha;
        coulomb->rightSubtreeHeight = alpha->rightSubtreeHeight + 1;
        coulomb->leftChild = beta;
        coulomb->leftSubtreeHeight = beta->leftSubtreeHeight + 1;
    } else {
        beta = alpha->rightChild;
        coulomb = beta->leftChild;
        Bravo = coulomb->rightChild;
        Charlie = coulomb->leftChild;
        beta->leftChild = Bravo;
        beta->leftSubtreeHeight = coulomb->rightSubtreeHeight;
        alpha->rightChild = Charlie;
        alpha->rightSubtreeHeight = coulomb->leftSubtreeHeight;
        coulomb->leftChild = alpha;
        coulomb->leftSubtreeHeight = alpha->leftSubtreeHeight + 1;
        coulomb->rightChild = beta;
        coulomb->rightSubtreeHeight = beta->rightSubtreeHeight + 1;
    }
    coulomb->parent = alpha->parent;
    if (coulomb->parent == NULL) {
        tree->root = coulomb;
    } else if (tree->comparator(coulomb->parent->leftChild->value, alpha->value) == 0) {
        coulomb->parent->leftChild = coulomb;
    } else {
        coulomb->parent->rightChild = coulomb;
    }
    alpha->parent = beta->parent = coulomb;
    Bravo->parent = beta;
    Charlie->parent = alpha;
}

//Rebalance tree and update values of parents
void rebalance_tree(avlTree *tree, avlTreeNode *currentNode) {
    int imbalance;
    while (currentNode != NULL) {
        imbalance = update_height(currentNode);
        if (imbalance > 1) {
            imbalance = ((currentNode->leftChild->leftSubtreeHeight) -
                    (currentNode->leftChild->rightSubtreeHeight));
            if (imbalance < 0) {
                //LEFT RIGHT
                double_rotation(tree, currentNode, 'l');
            } else  {
                //LEFT LEFT
                single_rotation(tree, currentNode, 'l');
            }
        } else if (imbalance < - 1) {
            imbalance = ((currentNode->rightChild->rightSubtreeHeight) -
                (currentNode->rightChild->leftSubtreeHeight));
            if (imbalance < 0) {
                //RIGHT LEFT
                double_rotation(tree, currentNode, 'r');
            } else {
                //RIGHT RIGHT
                single_rotation(tree, currentNode, 'r');
            }

        }
        currentNode = currentNode->parent;
    }
}

bool insert_node(avlTree *tree, void *insert) {
    avlTreeNode *newNode = create_new_node(insert);
    if (tree->root == NULL) {
        tree->root = newNode;
        return true;
    }
    avlTreeNode *currentNode = tree->root;
    for (EVER) {
        //greater means currentNode is bigger, negative means insert is bigger
        int result = tree->comparator(currentNode->value, insert);
        if (result == 0) {
            free(newNode);
            return false;
        } else if (result > 0) {
            if (currentNode->leftChild != NULL) {
                currentNode = currentNode->leftChild;
            } else {
                newNode->parent = currentNode;
                currentNode->leftChild = newNode;
                rebalance_tree(tree, currentNode);
                return true;
            }
        } else {
            if (currentNode->rightChild != NULL) {
                currentNode = currentNode->rightChild;
            } else {
                newNode->parent = currentNode;
                currentNode->rightChild = newNode;
                rebalance_tree(tree, currentNode);
                return true;
            }
        }
    }
}

//Remember to free remove if required after calling this
bool delete_node(avlTree *tree, void *remove) {
    avlTreeNode *toRemove = internal_search(tree, remove, 'd');
    if (toRemove == NULL) {
        return false;
    }
    bool valueFree = true;
    if (toRemove->rightChild != NULL) {
        avlTreeNode *successor = internal_search(tree, remove, 's');
        free(toRemove->value);
        toRemove->value = successor->value;
        valueFree = false;
        toRemove = successor;
        if (toRemove->rightChild != NULL) {
            toRemove->value = toRemove->rightChild->value;
            toRemove = toRemove->rightChild;
        }
    } else if (toRemove->leftChild != NULL) {
        //left child has to be a leaf or the tree would be imbalanced
        free(toRemove->value);
        toRemove->value = toRemove->leftChild->value;
        valueFree = false;
        toRemove = toRemove->leftChild;

    }
    //ToRemove now has to be a leaf
    if(tree->comparator (toRemove->parent->leftChild->value, 
            toRemove->value) == 0) {
        toRemove->parent->leftChild = NULL;
        if (valueFree) {
            free(toRemove->value);
        }
    } else {
        toRemove->parent->rightChild = NULL;
        if (valueFree) {
            free(toRemove->value);
        }
    }
    rebalance_tree(tree, toRemove->parent);
    free(toRemove);
    return true;
}
