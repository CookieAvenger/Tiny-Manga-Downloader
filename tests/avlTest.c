#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avlTree.h"

char *readLine() {
    char *readLine = malloc(sizeof(char) * 1024);
    gets(readLine);
    return readLine;
}

int main() {
/*
    avlTree *toTest = malloc(sizeof(avlTree));
    toTest->comparator = strcmp; 
    insert_node(toTest, readLine());
    insert_node(toTest, readLine());
    insert_node(toTest, readLine());
    insert_node(toTest, readLine());
    delete_node(toTest, readLine());
    delete_node(toTest, readLine());
    delete_node(toTest, readLine());
*/
    char **testing = malloc(sizeof(char *) * 9);
    for (int i = 0; i < 8; i++) {
        testing[i] = readLine();
    }
    testing[8] = NULL;
    
    avlTree *toTest = sorted_construction(testing, strcmp);
    char *test = dictionary_lookup(toTest, "abcd");
    printf("%s\n", test);
    char **toCheck = get_array(toTest);
    int i = 0;                                       
    char *toPrint;                                   
    while (toPrint = toCheck[i++], toPrint != NULL) {
        printf("%s\n", (char *) toPrint);            
    }                                                
}
