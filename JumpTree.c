#include "JumpTree.h"

#include <math.h>

#define JT_INSERTION_THRESHOLD(b, k) (int)(2*pow(floor(b/2), k)) 
#define JT_DELETION_THRESHOLD(b, k) (int)(2*pow(floor((b-4)/2), k))

bool JumpTreeInsert(JumpTree *tree, const Key *key) {
	bool rebuilt = false;
	if (tree->internal_tree->number_items + 1 >= JT_INSERTION_THRESHOLD(tree->internal_tree->max_children, tree->k)) {
		//printf("Rebuilding online\n");
		tree->internal_tree->max_children += 2;//New tree needs to have more children to keep height less than k
		BTreeRebuildOnline(&(tree->internal_tree));
		rebuilt = true;
	}
	BTreeInsert(tree->internal_tree, key);
	return rebuilt;
}

bool JumpTreeDelete(JumpTree *tree, const Key *key) {
	bool rebuilt = false;
	if (tree->internal_tree->max_children > 4 && tree->internal_tree->number_items - 1 <= JT_DELETION_THRESHOLD(tree->internal_tree->max_children, tree->k)) {
		//printf("Rebuilding online\n");
		tree->internal_tree->max_children -= 2; // Rebuild only if b > 4 and n below threshold
		BTreeRebuildOnline(&(tree->internal_tree));
		rebuilt =  true;
	}
	BTreeDelete(&(tree->internal_tree), key);
	return rebuilt;
}

void JumpTreeRebuildOffline(JumpTree *tree, const Key *keys, const int k_num_keys) {
	tree->internal_tree->max_children = 2 * ((int)pow(k_num_keys / 2, 1 / (double)(tree->k)) + 2); // Ensure tree will not exceed height k on rebuild

	BTreeRebuildOffline(&(tree->internal_tree), keys, k_num_keys);
}
