#pragma once
#include "../Simulation.h"

#include <stdlib.h>
#include <stdbool.h>

#define DEFAULT_MAX_CHILDREN 4

/*
* Lightweight B+ tree implementation written in C.
* Insertion is based on the algorithm described in CLRS "Introduction to Algorithms 3rd edition", pg 494-496
* Deletion is based on Jan Jannink's algorithm in "Implementing deletion in B+-trees" http://delivery.acm.org/10.1145/210000/202666/P033.pdf?ip=35.9.34.73&id=202666&acc=ACTIVE%20SERVICE&key=B5D9E165A72B697C%2E47E2B4B107F7155E%2E4D4702B0C3E38B35%2E4D4702B0C3E38B35&__acm__=1528486052_408cfa66992c424cde970b9ea05495ab
* and the paper "Optimizing Jan Jannink's Implementation of B+-tree deletion" https://dl.acm.org/citation.cfm?id=211999
* Second deletion method is based on "Deletion without Rebalancing in Multiway Search Trees" http://sidsen.azurewebsites.net/papers/b-trees-isaac09.pdf
*/

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
void BTreeRebuildOffline(BTree **tree, const Key *keys, const int k_num_keys);
void BTreeInsert(BTree *tree, const Key *key);
void BTreeDeleteBalance(BTree **tree, const Key *key);
void BTreeDelete(BTree **tree, const Key *key);
int BTreeFind(BTree *tree, const Key *key);
int BTreeHeight(BTree *tree);
void BTreePrint(BTree *tree);


class BPlusTree : public MultidimensionalKeyDictionary{
public:
	BPlusTree(int b = 4) : b(b){}
	~BPlusTree() { if (tree != NULL) { BTreeFree(tree); } }

	virtual void ConstructDictionary(const std::vector<Key>& keys) {
		if (tree != NULL) {
			BTreeFree(tree);
		}
		tree = BTreeInitM(b);
		std::vector<Key> m_keys = keys;
		m_keys = keys; // MUST BE SORTED
		sort(m_keys.begin(), m_keys.end(), [](Key k1, Key k2) { return k1.key < k2.key; });
		BTreeRebuildOffline(&tree, &m_keys[0], m_keys.size());//No reason to perform a ton of insertions
	}

	virtual inline int Search(const Key& q) { return BTreeFind(tree, &q); }
	virtual std::string GetName() const { return "B+-Tree"; }
	virtual int TreeHeight() const { return BTreeHeight(tree); }
	virtual void InsertKey(const Key& key) {
		BTreeInsert(tree, &key);
	}
	virtual void DeleteKey(const Key& key) { BTreeDeleteBalance(&tree, &key); }
	virtual void Print() const {
		BTreePrint(tree);
	}

private:
	BTree *tree = NULL;
	int b;
};