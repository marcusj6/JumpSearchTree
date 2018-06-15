#pragma once
#include "bptree.h"
#include "../Simulation.h"

#include <string>

/*
 * JumpTree is a modification of a B- tree 
 * (see "Deletion without Rebalancing in Multiway Search Trees" by Siddhartha Sen and Robert E. Tarjan)
 * that allows dynamic use of the Jump Search technique 
 * (see "Jump searching: a fast sequential search technique" by Ben Shneiderman).
 * Insert, delete, and search are all O(kn^(1/k)) amortized time complexity.
 */

void JumpTreeInsert(BTree **tree, const Key *key, int k);
void JumpTreeDelete(BTree **tree, const Key *key, int k);
void JumpTreeRebuildOffline(BTree **tree, const Key *keys, const int k_num_keys, int k);

class JumpTree : public MultidimensionalKeyDictionary {
public:
	JumpTree(int height = 2, int max_children = 4) : k(height), b(max_children){
		if (max_children < 4)
			b = 4;
		if (height < 0)
			k = 0;
	}
	~JumpTree() { if (tree != NULL) { BTreeFree(tree); } }

	virtual void ConstructDictionary(const std::vector<Key>& keys) {
		if (tree != NULL) {
			BTreeFree(tree);
		}
		tree = BTreeInitM(b);
		std::vector<Key> m_keys = keys;
		sort(m_keys.begin(), m_keys.end(), [](Key k1, Key k2) { return k1.key < k2.key; });
		JumpTreeRebuildOffline(&tree, &m_keys[0], m_keys.size(), k);//No reason to perform a ton of insertions
	}

	virtual inline int Search(const Key& q) { return BTreeFind(tree, &q); }
	virtual std::string GetName() const { return "JumpTree "+std::to_string(k); }
	virtual int TreeHeight() const { return BTreeHeight(tree); }
	virtual void InsertKey(const Key& key) { JumpTreeInsert(&tree, &key, k); }
	virtual void DeleteKey(const Key& key) { JumpTreeDelete(&tree, &key, k); }
	virtual void Print() const {
		BTreePrint(tree);
	}

private:
	BTree *tree = NULL;
	int k;
	int b;
};