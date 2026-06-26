#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/*
 * Z80 C23 Compiler Test: Binary Search Tree (BST)
 *
 * This single data structure (BST Node) comprehensively tests:
 * - pointers: single *, double **, struct member pointers, pointer returns
 * - memory management: malloc/free with allocation tracking
 * - recursion: insert, search, traversals, delete, free_tree, count_nodes
 * - struct access: classic linked tree node using -> and direct assignment
 */

struct Node {
    int data;
    struct Node* left;
    struct Node* right;
};

static int alloc_count = 0;
static int free_count = 0;

struct Node* create_node(int data) {
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    alloc_count++;
    return node;
}

void insert(struct Node** root, int data) {
    if (*root == NULL) {
        *root = create_node(data);
        return;
    }
    if (data < (*root)->data) {
        insert(&((*root)->left), data);
    } else if (data > (*root)->data) {
        insert(&((*root)->right), data);
    }
}

struct Node* search(struct Node* root, int key) {
    if (root == NULL || root->data == key) {
        return root;
    }
    if (key < root->data) {
        return search(root->left, key);
    }
    return search(root->right, key);
}

struct Node* min_value_node(struct Node* node) {
    struct Node* current = node;
    while (current != NULL && current->left != NULL) {
        current = current->left;
    }
    return current;
}

struct Node* delete_node(struct Node* root, int key) {
    if (root == NULL) {
        return root;
    }
    if (key < root->data) {
        root->left = delete_node(root->left, key);
    } else if (key > root->data) {
        root->right = delete_node(root->right, key);
    } else {
        if (root->left == NULL) {
            struct Node* temp = root->right;
            free(root);
            free_count++;
            return temp;
        }
        if (root->right == NULL) {
            struct Node* temp = root->left;
            free(root);
            free_count++;
            return temp;
        }

        struct Node* temp = min_value_node(root->right);
        root->data = temp->data;
        root->right = delete_node(root->right, temp->data);
    }
    return root;
}

void inorder(struct Node* root) {
    if (root == NULL) {
        return;
    }
    inorder(root->left);
    printf("%d ", root->data);
    inorder(root->right);
}

void preorder(struct Node* root) {
    if (root == NULL) {
        return;
    }
    printf("%d ", root->data);
    preorder(root->left);
    preorder(root->right);
}

void postorder(struct Node* root) {
    if (root == NULL) {
        return;
    }
    postorder(root->left);
    postorder(root->right);
    printf("%d ", root->data);
}

void free_tree(struct Node* root) {
    if (root == NULL) {
        return;
    }
    free_tree(root->left);
    free_tree(root->right);
    free(root);
    free_count++;
}

int count_nodes(struct Node* root) {
    if (root == NULL) {
        return 0;
    }
    return 1 + count_nodes(root->left) + count_nodes(root->right);
}

int main(void) {
    struct Node* root = NULL;
    int values[] = {50, 30, 70, 20, 40, 60, 80, 10, 25, 35, 45};
    size_t n = sizeof(values) / sizeof(values[0]);
    int initial_count;
    int after_delete_count;
    int search_key;
    int delete_key;
    struct Node* found;
    struct Node* manual1;
    struct Node* manual2;
    size_t i;

    printf("=== Z80 C23 BST Memory & Pointer Test ===\n");
    printf("Inserting values: ");
    for (i = 0; i < n; i++) {
        printf("%d ", values[i]);
        insert(&root, values[i]);
    }
    printf("\n");

    initial_count = count_nodes(root);
    printf("Node count after inserts: %d (expected 11)\n", initial_count);

    printf("Inorder traversal (sorted): ");
    inorder(root);
    printf("\n");

    printf("Preorder traversal: ");
    preorder(root);
    printf("\n");

    printf("Postorder traversal: ");
    postorder(root);
    printf("\n");

    search_key = 40;
    found = search(root, search_key);
    if (found != NULL && found->data == search_key) {
        printf("Search for %d: FOUND (pointer test passed)\n", search_key);
    } else {
        printf("Search for %d: NOT FOUND (ERROR)\n", search_key);
    }

    delete_key = 30;
    printf("Deleting node with key %d...\n", delete_key);
    root = delete_node(root, delete_key);
    after_delete_count = count_nodes(root);
    printf("Node count after delete: %d (expected 10)\n", after_delete_count);

    printf("Inorder after delete %d: ", delete_key);
    inorder(root);
    printf("\n");

    printf("Freeing remaining tree (recursive dealloc)...\n");
    free_tree(root);
    root = NULL;

    printf("Additional manual pointer/allocation test...\n");
    manual1 = create_node(100);
    manual2 = create_node(200);
    manual1->left = manual2;
    manual1->right = NULL;
    printf("Manual nodes: root=%d, left child=%d (pointer link test)\n",
           manual1->data, manual1->left->data);

    free_tree(manual1);

    printf("\n=== RESULTS ===\n");
    printf("Total allocations: %d\n", alloc_count);
    printf("Total deallocations (frees): %d\n", free_count);

    if (alloc_count == free_count && initial_count == 11 && after_delete_count == 10) {
        printf("MEMORY MANAGEMENT TEST: PASSED (balanced alloc/free)\n");
        printf("POINTERS & TRAVERSALS: PASSED\n");
        printf("DELETE & RESTRUCTURE: PASSED\n");
        printf("TEST COMPLETED SUCCESSFULLY.\n");
    } else {
        printf("MEMORY MANAGEMENT TEST: FAILED (leak or count mismatch)\n");
    }

    return 0;
}
