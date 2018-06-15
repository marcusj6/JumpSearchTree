#include "JumpTree.h"

#include <math.h>

#define JT_INSERTION_THRESHOLD(b, k) (int)(2*pow(floor(b/2), k)) 
#define JT_DELETION_THRESHOLD(b, k) (int)(2*pow(floor((b-4)/2), k))

void JumpTreeInsert(BTree **tree, const Key *key, int k) {
	if ((*tree)->number_items + 1 >= JT_INSERTION_THRESHOLD((*tree)->max_children, k)) {
		//printf("Rebuilding online\n");
		(*tree)->max_children += 2;//New tree needs to have more children to keep height less than k
		BTreeRebuildOnline(tree);
	}
	BTreeInsert(*tree, key);
}

void JumpTreeDelete(BTree **tree, const Key *key, int k) {
	if ((*tree)->max_children > 4 && (*tree)->number_items - 1 <= JT_DELETION_THRESHOLD((*tree)->max_children, k)) {
		//printf("Rebuilding online\n");
		(*tree)->max_children -= 2; // Rebuild only if b > 4 and n below threshold
		BTreeRebuildOnline(tree);
	}
	BTreeDelete(tree, key);
}

void JumpTreeRebuildOffline(BTree **tree, const Key *keys, const int k_num_keys, int k) {
	(*tree)->max_children = 2 * (pow(k_num_keys / 2, 1 / (double)(k)) + 2); // Ensure tree will not exceed height k on rebuild
	BTreeRebuildOffline(tree, keys, k_num_keys);
}