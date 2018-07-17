#include "bptree.h"

#include <stdlib.h>
#include <math.h>

#define TREE_HEIGHT_THRESHOLD(n, b) (int)(log(n/b)/log(ceil(b/2)))+4
#define BTREE_MAX_SPACE_CONSUMPTION(n) 0
#define BTREE_SPACE_THRESHOLD(n) 0

static void BTreeSplitChild(BTree *tree, BTreeNode *parent, int child_index);
static void BTreeInsertRecursive(BTree *tree, BTreeNode *current, const Key *key);
static void BTreeDeleteRecursion(BTree *tree, BTreeNode *current, const Key *key);
static void BTreePrintRecursive(BTree *tree, BTreeNode *current);
static double BTreeAverageNodeSizeRecursive(BTreeNode *current, double *total_nodes);

BTreeNode * BTreeNodeInit(bool internal) {
	BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
	node->keys = (int *)calloc(DEFAULT_MAX_CHILDREN - 1, sizeof(int));
	node->num_children = 0;
	if (!internal) {
		node->values = (BTreeValue *)calloc(DEFAULT_MAX_CHILDREN, sizeof(BTreeValue));
		node->children = NULL;
	}
	else {
		node->children = (BTreeNode **)calloc(DEFAULT_MAX_CHILDREN, sizeof(BTreeNode *));
		node->values = NULL;
	}
	node->next = NULL;
	node->previous = NULL;
	node->id = rand();
	return node;
}

BTreeNode * BTreeNodeInitM(bool internal, int max_children) {
	BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
	node->keys = (int *)calloc(max_children - 1, sizeof(int));
	node->num_children = 0;
	if (!internal) {
		node->values = (BTreeValue *)calloc(max_children, sizeof(BTreeValue));
		node->children = NULL;
	}
	else {
		node->children = (BTreeNode **)calloc(max_children, sizeof(BTreeNode *));
		node->values = NULL;
	}
	node->next = NULL;
	node->previous = NULL;
	node->id = rand();
	return node;
}

void BTreeNodeFree(BTreeNode *node) {
	if (node == NULL) {
		return;
	}
	else if (node->children != NULL) {//Not leaf, has children
		free(node->children);
	}
	else {//Leaf, has values
		free(node->values);
	}
	free(node->keys);
	free(node);
}


BTree * BTreeInit() {
	BTree *tree = (BTree*)malloc(sizeof(BTree));
	tree->root = NULL;
	tree->min = NULL;
	tree->max_children = DEFAULT_MAX_CHILDREN;//Defaults to 2-3-4 tree
	tree->height = -1;
	tree->number_items = 0;
	tree->num_leaves = 0;
	return tree;
}

BTree * BTreeInitM(int max_children) {
	BTree *tree = (BTree*)malloc(sizeof(BTree));
	tree->root = NULL;
	tree->min = NULL;
	tree->max_children = max_children;
	tree->height = -1;
	tree->number_items = 0;
	tree->num_leaves = 0;
	return tree;
}

void BTreeRecursiveFree(BTreeNode *node) {
	if (node == NULL) {
		return;
	}
	else if (node->children == NULL) { // Leaf
		BTreeNodeFree(node);
	}
	else {
		int i;
		for (i = 0; i < node->num_children; ++i) {
			BTreeRecursiveFree(node->children[i]);
		}
		BTreeNodeFree(node);
	}
}

void BTreeFree(BTree *tree) {
	if (tree != NULL) {
		BTreeRecursiveFree(tree->root);
		free(tree);
	}
}

void BTreeRebuildOnline(BTree **tree) { 
	//Initialize empty tree while anticipating insert
	BTree *new_tree = BTreeInitM((*tree)->max_children);
	new_tree->min = new_tree->root = BTreeNodeInitM(false, new_tree->max_children);
	new_tree->height = 0;
	new_tree->num_leaves = 1;

	int *right_spine = (int *)calloc((*tree)->height + 2, sizeof(int));

	BTreeNode *current = (*tree)->min;
	BTreeNode *iter = NULL;
	while (current != NULL) {
		int i, j;
		for (i = 0; i < current->num_children; ++i) {
			if (new_tree->root->num_children == new_tree->max_children) { // Root needs to be split
				BTreeNode *new_root = BTreeNodeInitM(true, new_tree->max_children);
				++new_tree->height;
				new_root->num_children = 1;
				new_root->children[0] = new_tree->root;
				new_tree->root = new_root;
				BTreeSplitChild(new_tree, new_root, 0);
				right_spine[new_tree->height] = 1;
				right_spine[new_tree->height - 1] -= (new_tree->max_children + 1) / 2;
			}
			iter = new_tree->root;
			for (j = new_tree->height; j > 0; j--) { // Split right spine if needs to be split (excluding root)
				if (iter->children[right_spine[j]]->num_children == new_tree->max_children) { // Need to split
					BTreeSplitChild(new_tree, iter, right_spine[j]);
					++right_spine[j];
					right_spine[j - 1] -= (new_tree->max_children + 1) / 2;
				}
				iter = iter->children[right_spine[j]];
			}
			if (iter->children != NULL) {
				iter = iter->children[right_spine[j]];
			}
			if (iter->num_children != 0) {
				iter->keys[iter->num_children - 1] = iter->values[iter->num_children - 1].key;
			}
			iter->values[iter->num_children++] = current->values[i];
			++right_spine[j];
			++new_tree->number_items;
		}
		current = current->next;
	}
	free(right_spine);
	BTreeFree(*tree); // Free memory for old tree
	*tree = new_tree;
}// For rebuilding after insertions or deletions

void BTreeRebuildOffline(BTree **tree, const Key *keys, const int k_num_keys) {
	BTree *new_tree = BTreeInitM((*tree)->max_children);
	new_tree->min = new_tree->root = BTreeNodeInitM(false, new_tree->max_children);
	new_tree->height = 0;
	new_tree->num_leaves = 1;

	int *right_spine = (int *)calloc((int)(log((k_num_keys+1)/2)/log((*tree)->max_children/2))+1, sizeof(int));

	BTreeNode *iter = NULL;
	int i, j;
	for (i = 0; i < k_num_keys; ++i) {
		if (new_tree->root->num_children == new_tree->max_children) { // Root needs to be split
			BTreeNode *new_root = BTreeNodeInitM(true, new_tree->max_children);
			++new_tree->height;
			new_root->num_children = 1;
			new_root->children[0] = new_tree->root;
			new_tree->root = new_root;
			BTreeSplitChild(new_tree, new_root, 0);
			right_spine[new_tree->height] = 1;
			right_spine[new_tree->height - 1] -= (new_tree->max_children + 1) / 2;
		}
		iter = new_tree->root;
		for (j = new_tree->height; j > 0; j--) { // Split right spine if needs to be split (excluding root)
			if (iter->children[right_spine[j]]->num_children == new_tree->max_children) { // Need to split
				BTreeSplitChild(new_tree, iter, right_spine[j]);
				++right_spine[j];
				right_spine[j - 1] -= (new_tree->max_children + 1) / 2;
			}
			iter = iter->children[right_spine[j]];
		}
		if (iter->children != NULL) {
			iter = iter->children[right_spine[j]];
		}
		if (iter->num_children != 0) {
			iter->keys[iter->num_children - 1] = iter->values[iter->num_children - 1].key;
		}
		iter->values[iter->num_children].key = keys[i].key;
		iter->values[iter->num_children].value = keys[i].id;
		++iter->num_children;
		++right_spine[j];
		++new_tree->number_items;
	}
	free(right_spine);
	BTreeFree(*tree); // Free memory for old tree
	*tree = new_tree;
}// For rebuilding before any insertion or deletions (identical to online, just uses key list instead of node list)

static void BTreeSplitChild(BTree *tree, BTreeNode *parent, int child_index) {
	BTreeNode *split = parent->children[child_index];
	bool is_internal = split->values == NULL; // New node should be leaf if old node was leaf, internal if internal
	BTreeNode *new_node = BTreeNodeInitM(is_internal, tree->max_children);
	new_node->num_children = (tree->max_children / 2);
	int i;
	for (i = 0; i < (tree->max_children / 2) - 1; ++i) { // Split keys evenly
		new_node->keys[i] = split->keys[(tree->max_children + 1) / 2 + i];
	}
	if (is_internal) { // Internal node
		for (i = 0; i < tree->max_children / 2; ++i) {
			new_node->children[i] = split->children[((tree->max_children + 1) / 2) + i];
		}
	}
	else { // Leaf
		for (i = 0; i < tree->max_children / 2; ++i) {
			new_node->values[i] = split->values[((tree->max_children + 1)/ 2) + i];
		}
	}
	split->num_children = (tree->max_children + 1) / 2; // If max_children odd, split receives extra child
	for (i = parent->num_children - 1; i >= child_index + 1; --i) { // Update parent's children
		parent->children[i + 1] = parent->children[i];
	}
	parent->children[i + 1] = new_node;
	if (!is_internal) { // Update LinkedList if node is leaf
		new_node->next = parent->children[i]->next;
		if (new_node->next != NULL) {
			new_node->next->previous = new_node;
		}
		parent->children[i]->next = new_node;
		new_node->previous = parent->children[i];
		++tree->num_leaves;
	}
	for (i = parent->num_children - 2; i >= child_index; --i) { // Update parent's keys
		parent->keys[i + 1] = parent->keys[i];
	}
	++i;
	parent->keys[i] = split->keys[(tree->max_children + 1) / 2 - 1]; // Child no longer needs this key, belongs to parent
	++parent->num_children;
}
//PRE CONDITIONS: current is nonfull
static void BTreeInsertRecursive(BTree *tree, BTreeNode *current, const Key *key) {
	int i = current->num_children - 2; // i = num_keys - 1
	if (current->values != NULL) { // current is leaf
		if (key->key > current->values[i + 1].key) { // key should be last child
			current->values[i + 2].key = key->key;
			current->values[i + 2].value = key->id;
			current->keys[i + 1] = current->values[i + 1].key;
			++current->num_children;
			++tree->number_items;
		}
		else if (key->key == current->values[i + 1].key) { // Key replaces last child
			current->values[i + 2].key = key->key;
			current->values[i + 2].value = key->id;
		}
		else { // Key should not be last child
			while (i >= 0 && key->key <= current->keys[i]) { i--; }
			++i;
			if (current->values[i].key == key->key) { // Already exists in tree, replace
				current->values[i].key = key->key;
				current->values[i].value = key->id;
			}
			else { // Key does not exist, shift all elements
				int j = current->num_children - 1;
				current->values[current->num_children] = current->values[current->num_children - 1]; // Shift last child
				while (j > i) {
					current->values[j] = current->values[j - 1];
					current->keys[j] = current->keys[j - 1];
					--j;
				}
				current->values[i].key = key->key;
				current->values[i].value = key->id;
				current->keys[i] = key->key;
				++current->num_children;
				++tree->number_items;
			}
			/*current->values[i + 2] = current->values[i + 1];
			while (i >= 0 && key->key < current->keys[i]) {
				current->keys[i + 1] = current->keys[i];
				current->values[i + 1] = current->values[i];
				--i;
			}
			current->values[i + 1] = key;
			current->keys[i + 1] = key->key;*/
			//current->num_children++;
		}
	}
	else { // Internal node
		while (i >= 0 && key->key <= current->keys[i]) { --i; } // Find appropriate node for insertion
		++i;
		if (current->children[i]->num_children == tree->max_children) { // Child needs to be split
			BTreeSplitChild(tree, current, i);
			if (key->key > current->keys[i]) // If key belongs in new child, increment i
				++i;
		}
		BTreeInsertRecursive(tree, current->children[i], key);
	}
}

void BTreeInsert(BTree *tree, const Key *key) {
	if (tree->root == NULL) { // Empty tree
		tree->root = BTreeNodeInitM(false, tree->max_children);
		++tree->height;
		tree->root->keys[0] = key->key;
		tree->root->num_children = 1; 
		tree->root->values[0].key = key->key;
		tree->root->values[0].value = key->id;
		tree->min = tree->root;// Maintain linked list between leaf nodes (min only changes with deletion)
		++tree->num_leaves;
		++tree->number_items;
	}
	else if (tree->root->num_children == 1) { // Root must be leaf still
		// Need to perform one more insert before 
		// num_keys = num_children - 1 and can use recursive call
		if (key->key < tree->root->keys[0]) { // Belongs before other item (MAY CAUSE ISSUE WITH DELETION ALLOWED)
			tree->root->keys[0] = key->key;
			tree->root->values[1] = tree->root->values[0];
			tree->root->values[0].key = key->key;
			tree->root->values[0].value = key->id;
			++tree->root->num_children;
			++tree->number_items;
		}
		else if (key->key > tree->root->keys[0]){ // Belongs after
			tree->root->values[1].key = key->key;
			tree->root->values[1].value = key->id;
			++tree->root->num_children;
			++tree->number_items;
		}
		else { // Replaces
			tree->root->values[0].key = key->key;
			tree->root->values[0].value = key->id;
		}
	}
	else { // B-tree is valid, perform normal insert
		if (tree->root->num_children == tree->max_children) { // Root needs to be split
			BTreeNode *new_root = BTreeNodeInitM(true, tree->max_children);
			++tree->height;
			new_root->num_children = 1;
			new_root->children[0] = tree->root;
			tree->root = new_root;
			BTreeSplitChild(tree, new_root, 0);
		}
		BTreeInsertRecursive(tree, tree->root, key);
	}
	//printf("\nInserted key %d:%d\n", key->key, key->id);
	//printf("Number items: %d\n", tree->number_items);
}

static void BTreeDeleteRecursion(BTree *tree, BTreeNode *current, const Key *key) {
	int i;
	for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Locate key
	if (current->values != NULL) { //Leaf
		if (current->values[i].key == key->key) { // Found the key
			for (++i; i < current->num_children - 1; ++i) {
				current->keys[i - 1] = current->keys[i];
				current->values[i - 1] = current->values[i];
			}
			current->values[i - 1] = current->values[i]; // One more value than key
			current->num_children--;
			tree->number_items--;
			if (current == tree->root && current->num_children == 0) { //Tree empty
				BTreeNodeFree(current);
				tree->root = NULL;
				tree->min = NULL;
				tree->height--;
				tree->num_leaves--;
			}
		}
	}
	else { //External Node
		BTreeDeleteRecursion(tree, current->children[i], key);
		if (current->children[i]->num_children == 0) { // Need to delete child
			if (current->children[i]->values != NULL) { // Deleting leaf, update linked list
				if (current->children[i]->previous == NULL) {
					tree->min = current->children[i]->next;
				}
				else {
					current->children[i]->previous->next = current->children[i]->next;
				}
				if(current->children[i]->next != NULL)
					current->children[i]->next->previous = current->children[i]->previous;
				tree->num_leaves--;
			}
			BTreeNodeFree(current->children[i]);
			for (++i; i < current->num_children - 1; ++i) {
				current->keys[i - 1] = current->keys[i];
				current->children[i - 1] = current->children[i];
			}
			current->children[i - 1] = current->children[i]; // One more value than key
			current->num_children--;
		}
		if (current == tree->root && current->num_children == 1) { // Need to delete root
			tree->root = current->children[0];
			BTreeNodeFree(current);
			tree->height--;
		}
	}
}

bool BTreeDeleteBalance(BTree **tree, const Key *key) {
	//Will use no rebalance with periodic rebuild (Tarjan's method)
	if ((*tree)->root == NULL || (*tree)->root->num_children == 0) // Nothing to delete
		return false;
	BTreeDeleteRecursion((*tree), (*tree)->root, key);
	if ((*tree)->height > TREE_HEIGHT_THRESHOLD((*tree)->number_items, (*tree)->max_children)) {
		//Tree height too great, rebuild
		BTreeRebuildOnline(tree);
		return true;
	}
	//if (BTREE_MAX_SPACE_CONSUMPTION((*tree)->num_leaves) > BTREE_SPACE_THRESHOLD((*tree)->number_items)) {
	//	//Tree space consumption too high, rebuild
	//	BTreeRebuildOnline(tree);
	//}
	return false;
	//printf("\nDeleted key %d:%d\n", key->key, key->id);
	//printf("Number items: %d\n", (*tree)->number_items);
}

void BTreeDelete(BTree **tree, const Key *key) {
	if ((*tree)->root == NULL || (*tree)->root->num_children == 0) // Nothing to delete
		return;
	BTreeDeleteRecursion((*tree), (*tree)->root, key);
}

static void BTreePrintRecursive(BTree *tree, BTreeNode *current) {
	/*
	 * Prints the B+ Tree Nodes preorder in the following format:
	 * 
	 * --------------------------
	 * ID: id
	 * Number of children: num_children
	 * Keys: key1:id1, key2:id2, ... keyn:idn,
	 * {Children, Values}: child1, child2, ... childn,
	 * Is leaf? {YES, NO}
	 * Next: next->id
	 * Previous: previous->id
	 * --------------------------
	 *
	 * Prints "Empty" if current is NULL
	 */
	printf("\n--------------------------\n");

	if (current == NULL) {
		printf("Empty\n");
		printf("--------------------------\n");
		return;
	}

	printf("ID: %d\nNumber of children: %d\nKeys: ", current->id, current->num_children);

	int i;

	for (i = 0; i < current->num_children - 1; ++i) {
		printf("%d, ", current->keys[i]);
	}

	if (current->values != NULL) { // Leaf
		printf("\nValues: ");
		for (i = 0; i < current->num_children; ++i) {
			printf("%d:%d, ", current->values[i].key, current->values[i].value);
		}
		printf("\nIs leaf? YES\nNext: %d\nPrevious: %d\n", current->next == NULL ? 0 : current->next->id, current->previous == NULL ? 0 : current->previous->id);
		printf("--------------------------\n");
	}
	else { // Internal node, need to continue recursion
		printf("\nChildren: ");
		for (i = 0; i < current->num_children; ++i) {
			printf("%d, ", current->children[i]->id);
		}
		printf("\nIs leaf? NO\nNext: %d\nPrevious: %d\n", 0, 0);
		printf("--------------------------\n");
		for (i = 0; i < current->num_children; ++i){
			BTreePrintRecursive(tree, current->children[i]);
		}
	}

}

void BTreePrint(BTree *tree) { 
	/*
	 * Prints the B+ tree with the following format:
	 *
	 * ==========================
	 * Height: height
	 * Max Children: max_children
	 * Min: min->id
	 * Number of items: number_items
	 * Number of leaves: num_leaves
	 * Nodes in preorder, see BTreePrintRecursive
	 * ==========================
	 *
	 */
	printf("==========================\n");
	printf("Height: %d\nMax Children: %d\nMin: %d\nNumber of items: %d\nNumber of leaves: %d\n", tree->height, tree->max_children, tree->min == NULL ? 0 : tree->min->id, tree->number_items, tree->num_leaves);
	BTreePrintRecursive(tree, tree->root);
	printf("==========================\n");
}

/*static int BTreeFindRecursive(BTreeNode *current, const Key *key) {
	if (current == NULL) {
		return -1;
	}
	int i;
	for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {}
	if (current->values != NULL) { // Leaf node
		if (current->values[i].key != key->key) {
			return -1;
		}
		else { // Found the right node
			return current->values[i].value;
		}
	}
	// Check appropriate child for key
	return BTreeFindRecursive(current->children[i], key);
}*/

int BTreeFind(BTree *tree, const Key *key) {
	if (tree == NULL || tree->root == NULL) {
		return -1;
	}
	BTreeNode *current = tree->root;
	int i;
	while (current->values == NULL) { // Until we reach a leaf
		for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Find appropriate child
		current = current->children[i];
	}
	for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Find appropriate value
	if (current->values[i].key != key->key) {
		return -1;
	}
	else { // Found the right node
		return current->values[i].value;
	}
}

int BTreeHeight(BTree *tree) {
	if (tree == NULL) {
		return -1;
	}
	return tree->height;
}


static double BTreeAverageNodeSizeRecursive(BTreeNode *current, double *total_nodes) {
	(*total_nodes)++;
	if (current->values != NULL) { // Leaf
		return (double)current->num_children;
	}
	else {
		double total_size = 0.0;
		int i;
		for (i = 0; i < current->num_children; ++i) {
			total_size += BTreeAverageNodeSizeRecursive(current->children[i], total_nodes);
		}
		return total_size + (double)current->num_children;
	}
}

double BTreeAverageNodeSize(BTree *tree) {
	if (tree == NULL || tree->root == NULL)
		return 0;
	double total_nodes = 0;
	double total_size = BTreeAverageNodeSizeRecursive(tree->root, &total_nodes);
	return total_size / total_nodes;
}


int BTreeSuccessor(BTree *tree, const Key *key) {
	if (tree == NULL || tree->root == NULL) {
		return -1;
	}
	BTreeNode *current = tree->root;
	int i;
	while (current->values == NULL) { // Until we reach a leaf
		for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Find appropriate child
		current = current->children[i];
	}
	for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Find appropriate value
	if (current->values[i].key != key->key) {
		return -1; // Node doesn't exist, no successor
	}
	else { // Found the right node
		if (i == current->num_children - 1) { // i is rightmost child, check next node
			if (current->next != NULL) {
				return current->next->values[0].value;
			}
			else {
				return -1; // No successor
			}
		}
		else {
			return current->values[i].value;
		}
	}
}

int BTreePredecessor(BTree *tree, const Key *key) {
	if (tree == NULL || tree->root == NULL) {
		return -1;
	}
	BTreeNode *current = tree->root;
	int i;
	while (current->values == NULL) { // Until we reach a leaf
		for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Find appropriate child
		current = current->children[i];
	}
	for (i = 0; i < current->num_children - 1 && key->key > current->keys[i]; ++i) {} // Find appropriate value
	if (current->values[i].key != key->key) {
		return -1; // Node doesn't exist, no predecessor
	}
	else { // Found the right node
		if (i == 0) { // i is leftmost child, check previous node
			if (current->previous != NULL) {
				current = current->previous;
				return current->values[current->num_children - 1].value;
			}
			else {
				return -1; // No predecessor
			}
		}
		else {
			return current->values[i].value;
		}
	}
}
