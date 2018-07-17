#ifndef JUMP_TREE_H
#define JUMP_TREE_H

#include "bptree.h"

/*
 * JumpTree is a modification of a B- tree 
 * (see "Deletion without Rebalancing in Multiway Search Trees" by Siddhartha Sen and Robert E. Tarjan)
 * that allows dynamic use of the Jump Search technique 
 * (see "Jump searching: a fast sequential search technique" by Ben Shneiderman).
 * Insert, delete, and search are all O(kn^(1/k)) amortized time complexity.
 */
 
typedef struct JumpTree{
	struct BTree *internal_tree;
	int k;
} JumpTree;

static inline JumpTree * JumpTreeInit(){
	JumpTree *tree =  (JumpTree *)malloc(sizeof(JumpTree));
	tree->internal_tree = BTreeInit();
	tree->k = 5;
	return tree;
}

JumpTree * JumpTreeInitK(int k){
	JumpTree *tree =  (JumpTree *)malloc(sizeof(JumpTree));
	tree->internal_tree = BTreeInit();
	tree->k = k;
	return tree;
}

static inline void JumpTreeFree(JumpTree *tree){
	BTreeFree(tree->internal_tree);
	free(tree);
}

static inline int JumpTreeFind(JumpTree *tree, const Key *key){ return BTreeFind(tree->internal_tree, key); }
static inline int JumpTreeSuccessor(JumpTree *tree, const Key *key){ return BTreeSuccessor(tree->internal_tree, key); }
static inline int JumpTreePredecessor(JumpTree *tree, const Key *key){ return BTreePredecessor(tree->internal_tree, key); }
static inline int JumpTreeHeight(JumpTree *tree){ return BTreeHeight(tree->internal_tree); }
static inline void JumpTreePrint(JumpTree *tree){ BTreePrint(tree->internal_tree); }
static inline double JumpTreeAverageNodeSize(JumpTree *tree){ return BTreeAverageNodeSize(tree->internal_tree);}

bool JumpTreeInsert(JumpTree *tree, const Key *key);
bool JumpTreeDelete(JumpTree *tree, const Key *key);
void JumpTreeRebuildOffline(JumpTree *tree, const Key *keys, const int k_num_keys); //Assumes keys are already sorted

#endif
