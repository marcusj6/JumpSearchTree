#ifndef BTREE_H
#define BTREE_H

#include <stdlib.h>
#include <stdbool.h>

#define DEFAULT_MAX_CHILDREN 4

/*
* Lightweight B+ tree implementation written in C.
* Insertion is based on the algorithm described in CLRS "Introduction to Algorithms 3rd edition", pg 494-496
* Deletion method is based on "Deletion without Rebalancing in Multiway Search Trees" http://sidsen.azurewebsites.net/papers/b-trees-isaac09.pdf
*/

typedef struct Key{
	int key;
	int id;
} Key;

typedef struct BTreeValue {
	int key;
	int value;
} BTreeValue;

static inline BTreeValue * BTreeValueInit(int key, int value) {
	BTreeValue *v = (BTreeValue *)malloc(sizeof(BTreeValue));
	v->key = key;
	v->value = value;
	return v;
}

static inline void BTreeValueFree(BTreeValue *value) {
	free(value);
}

typedef struct BTreeNode {
	BTreeValue *values;
	struct BTreeNode **children;
	struct BTreeNode *next;
	struct BTreeNode *previous;
	int *keys;
	int num_children;
	int id;
} BTreeNode;

BTreeNode * BTreeNodeInit(bool internal);
BTreeNode * BTreeNodeInitM(bool internal, int max_children);
void BTreeNodeFree(BTreeNode *node);

typedef struct BTree {
	BTreeNode *root;
	BTreeNode *min;
	int max_children;
	int height;
	int number_items;
	int num_leaves;
} BTree;

BTree * BTreeInit();
BTree * BTreeInitM(int max_children);
void BTreeRecursiveFree(BTreeNode *node);
void BTreeFree(BTree *tree);
void BTreeRebuildOnline(BTree **tree);// For rebuilding after insertions or deletions
void BTreeRebuildOffline(BTree **tree, const Key *keys, const int k_num_keys); //Rebuilds assuming that keys is sorted
void BTreeInsert(BTree *tree, const Key *key);
bool BTreeDeleteBalance(BTree **tree, const Key *key);
void BTreeDelete(BTree **tree, const Key *key);
int BTreeFind(BTree *tree, const Key *key);
int BTreeSuccessor(BTree *tree, const Key *key);
int BTreePredecessor(BTree *tree, const Key *key);
int BTreeHeight(BTree *tree);
void BTreePrint(BTree *tree);
double BTreeAverageNodeSize(BTree *tree);

#endif
