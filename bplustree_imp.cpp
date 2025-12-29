#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdio>   // For C-style file functions: fopen, fread, fwrite, fseek, ftell, fclose

// Define the order of the B+ tree (maximum number of children per node)
// Order 4 means: max 3 keys per node, max 4 children per node
#define ORDER 5
// Minimum keys required (ceiling of (ORDER-1)/2)
// For ORDER 4: ceiling(3/2) = 2 keys minimum (except root which can have 1)
// NOTE: MIN_KEYS is enforced during DELETION operations (not implemented yet)
#define MIN_KEYS ((ORDER) / 2)  // 4/2 = 2

/**
 * Node structure for B+ tree
 */
struct Node {
    int keys[ORDER - 1];              // Array to store keys (max 3 for order 4)
    int child_offsets[ORDER];         // File offsets pointing to children (max 4 for order 4)
    int next_leaf_offset;             // Points to next leaf node (linked list of leaves)
    int num_keys;                     // Current number of keys stored
    bool is_leaf;                     // True if leaf node, false if internal node
    int self_offset;                  // This node's position in the file
    
    // Constructor: initialize empty node
    Node() : num_keys(0), is_leaf(true), next_leaf_offset(-1), self_offset(-1) {
        std::fill(keys, keys + ORDER - 1, -1);          // Initialize all keys to -1
        std::fill(child_offsets, child_offsets + ORDER, -1);  // -1 means no child
    }
};

class BPlusTree {
private:
    std::string filename;    // Name of the file storing the tree
    int root_offset;         // File position of the root node
    
    /**
     * Write a node to the file using fwrite()
     * @param node - Node to write
     * @return true if successful, false otherwise
     */
    bool writeNode(Node& node) {
        FILE* file = fopen(filename.c_str(), "rb+");  // Open for read+write, binary  c_str ->Need to convert: std::string → const char*
        if (!file) {
            std::cerr << "Error: Cannot open file for writing\n"; 
            return false;
        }
        
        if (node.self_offset == -1) {
            // NEW NODE: Write at end of file
            fseek(file, 0, SEEK_END);        // Move to end
            node.self_offset = ftell(file);   // Get current position
        } else {
            // EXISTING NODE: Overwrite at its position
            fseek(file, node.self_offset, SEEK_SET);  // Move to node's position
        }
        
        // Write entire node structure as binary data
        size_t written = fwrite(&node, sizeof(Node), 1, file);  //fwrite() returns number of items written
        fclose(file);
        
        return (written == 1);  // Return true if 1 item written successfully 
    }
    
    /**
     * Read a node from the file using fread()
     * @param offset - File position to read from
     * @param node - Node to store result
     * @return true if successful, false otherwise
     */
    bool readNode(int offset, Node& node) {
        FILE* file = fopen(filename.c_str(), "rb");  // Open for read, binary
        if (!file) {
            std::cerr << "Error: Cannot open file for reading\n";
            return false;
        }
        
        fseek(file, offset, SEEK_SET);  // Move to offset position
        size_t read = fread(&node, sizeof(Node), 1, file);  // Read node
        fclose(file);
        
        return (read == 1);  // Return true if 1 item read successfully
    }
    
    /**
     * Write the root offset to the beginning of the file
     */
    bool writeRootOffset() {
        FILE* file = fopen(filename.c_str(), "rb+");
        if (!file) return false;
        
        fseek(file, 0, SEEK_SET);  // Go to beginning
        size_t written = fwrite(&root_offset, sizeof(int), 1, file);
        fclose(file);
        
        return (written == 1);
    }
    
    /**
     * Read the root offset from the beginning of the file
     */
    bool readRootOffset(int& offset) {
        FILE* file = fopen(filename.c_str(), "rb");
        if (!file) return false;
        
        fseek(file, 0, SEEK_SET);  // Go to beginning
        size_t read = fread(&offset, sizeof(int), 1, file);
        fclose(file);
        
        return (read == 1);
    }
    
    /**
     * Split a full child node giving parent with free space 
     */
    void splitChild(Node& parent, int idx) {
        Node full_child;
        readNode(parent.child_offsets[idx], full_child);
        
        Node new_node;
        new_node.is_leaf = full_child.is_leaf;
        int mid = ORDER / 2;
        
        if (full_child.is_leaf) {
            // LEAF NODE SPLIT
            new_node.num_keys = ORDER - 1 - mid;
            for (int i = 0; i < new_node.num_keys; i++) {
                new_node.keys[i] = full_child.keys[mid + i];
            }
            full_child.num_keys = mid;
            
            new_node.next_leaf_offset = full_child.next_leaf_offset;
            writeNode(new_node);
            full_child.next_leaf_offset = new_node.self_offset;
            
            for (int i = parent.num_keys; i > idx; i--) {
                parent.keys[i] = parent.keys[i - 1];
                parent.child_offsets[i + 1] = parent.child_offsets[i];
            }
            parent.keys[idx] = new_node.keys[0];
            parent.child_offsets[idx + 1] = new_node.self_offset;
            parent.num_keys++;
        } else {
            // INTERNAL NODE SPLIT
            new_node.num_keys = (ORDER - 1) - mid - 1;
            for (int i = 0; i < new_node.num_keys; i++) {
                new_node.keys[i] = full_child.keys[mid + 1 + i];
            }
            for (int i = 0; i <= new_node.num_keys; i++) {
                new_node.child_offsets[i] = full_child.child_offsets[mid + 1 + i];
            }
            full_child.num_keys = mid;
            writeNode(new_node);
            
            for (int i = parent.num_keys; i > idx; i--) {
                parent.keys[i] = parent.keys[i - 1];
                parent.child_offsets[i + 1] = parent.child_offsets[i];
            }
            parent.keys[idx] = full_child.keys[mid];
            parent.child_offsets[idx + 1] = new_node.self_offset;
            parent.num_keys++;
        }
        
        writeNode(full_child);
        writeNode(parent);
    }
    
    /**
     * Insert key into a node that is NOT full
     */
    void insertNonFull(Node& node, int key) {
        int i = node.num_keys - 1;
        
        if (node.is_leaf) {
            while (i >= 0 && key < node.keys[i]) {
                node.keys[i + 1] = node.keys[i];
                i--;
            }
            node.keys[i + 1] = key;
            node.num_keys++;
            writeNode(node);
        } else {
            while (i >= 0 && key < node.keys[i]) {
                i--;
            }
            i++;
            
            Node child;
            readNode(node.child_offsets[i], child);
            
            if (child.num_keys == ORDER - 1) {
                splitChild(node, i);
                if (key > node.keys[i]) {
                    i++;
                }
                readNode(node.child_offsets[i], child);
            }
            insertNonFull(child, key);
        }
    }
    
    /**
     * Print node and its descendants
     */
    void printNodeToStream(int offset, int level, std::ostream& out) {
        if (offset == -1) return;
        
        Node node;
        if (!readNode(offset, node)) return;
        
        for (int i = 0; i < level; i++) {
            out << "  ";
        }
        
        out << "Level " << level << ": [";
        for (int i = 0; i < node.num_keys; i++) {
            out << node.keys[i];
            if (i < node.num_keys - 1) out << ", ";
        }
        out << "]";
        if (node.is_leaf) out << " (leaf)";
        out << "\n";
        
        if (!node.is_leaf) {
            for (int i = 0; i <= node.num_keys; i++) {
                if (node.child_offsets[i] != -1) {
                    printNodeToStream(node.child_offsets[i], level + 1, out);
                }
            }
        }
    }
    
public:
    /**
     * Constructor: Initialize tree from file or create new one
     */
    BPlusTree(const std::string& fname) : filename(fname), root_offset(-1) {
        FILE* file = fopen(filename.c_str(), "rb");
        
        if (file) {
            // File exists: load root offset
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fclose(file);
            
            if (file_size > 0) {
                readRootOffset(root_offset);
                return;
            }
        }
        
        // File doesn't exist or is empty: create new tree
        file = fopen(filename.c_str(), "wb");
        if (!file) {
            std::cerr << "Error: Cannot create file\n";
            return;
        }
        
        Node root;
        root.is_leaf = true;
        fseek(file, 0, SEEK_END);
        root.self_offset = ftell(file);
        fwrite(&root, sizeof(Node), 1, file);
        
        root_offset = root.self_offset;
        fseek(file, 0, SEEK_SET);
        fwrite(&root_offset, sizeof(int), 1, file);
        
        fclose(file);
    }
    
    /**
     * Insert a key into the B+ tree
     */
    void insert(int key) {
        Node root;
        if (!readNode(root_offset, root)) {
            std::cerr << "Error reading root\n";
            return;
        }
        
        if (root.num_keys == ORDER - 1) {
            Node new_root;
            new_root.is_leaf = false;
            new_root.child_offsets[0] = root_offset;
            writeNode(new_root);
            root_offset = new_root.self_offset;
            
            splitChild(new_root, 0);
            insertNonFull(new_root, key);
            writeRootOffset();
        } else {
            insertNonFull(root, key);
        }
    }
    
    /**
     * Print the entire tree structure
     */
    void printTree() {
        printNodeToStream(root_offset, 0, std::cout);
    }
    
    /**
     * Search for a key in the tree
     */
    bool search(int key) {
        return searchNode(root_offset, key);
    }
    
    /**
     * Recursive search helper
     */
    bool searchNode(int offset, int key) {
        if (offset == -1) return false;
        
        Node node;
        if (!readNode(offset, node)) return false;
        
        int i = 0;
        while (i < node.num_keys && key > node.keys[i]) i++;
        
        if (i < node.num_keys && key == node.keys[i]) {
            return true;
        }
        
        if (node.is_leaf) return false;
        
        return searchNode(node.child_offsets[i], key);
    }
};

void displayMenu() {
    std::cout << "\n=================================\n";
    std::cout << "    B+ Tree (C-Style Files)\n";
    std::cout << "=================================\n";
    std::cout << "1. Insert a number\n";
    std::cout << "2. Search for a number\n";
    std::cout << "3. Display tree structure\n";
    std::cout << "4. Clear tree (delete file)\n";
    std::cout << "5. Exit\n";
    std::cout << "=================================\n";
    std::cout << "Enter your choice: ";
}

int main() {
    std::string filename = "bplustree.dat";
    BPlusTree tree(filename);
    
    std::cout << "B+ Tree Application (C-Style File Functions)\n";
    std::cout << "Using: fopen, fread, fwrite, fseek, ftell, fclose\n";
    std::cout << "Data file: " << filename << "\n";
    std::cout << "Order: " << ORDER << " (max " << (ORDER-1) << " keys per node)\n";
    
    int choice;
    while (true) {
        displayMenu();
        std::cin >> choice;
        
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "\nInvalid input! Please enter a number.\n";
            continue;
        }
        
        switch (choice) {
            case 1: {
                int num;
                std::cout << "\nEnter number to insert: ";
                std::cin >> num;
                
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Invalid number!\n";
                } else {
                    tree.insert(num);
                    std::cout << "Successfully inserted " << num << "\n";
                    std::cout << "\nCurrent tree structure:\n";
                    std::cout << "----------------------------\n";
                    tree.printTree();
                    std::cout << "----------------------------\n";
                }
                break;
            }
            case 2: {
                int num;
                std::cout << "\nEnter number to search: ";
                std::cin >> num;
                
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Invalid number!\n";
                } else {
                    bool found = tree.search(num);
                    if (found) {
                        std::cout << "Number " << num << " FOUND in the tree!\n";
                    } else {
                        std::cout << "Number " << num << " NOT FOUND in the tree.\n";
                    }
                }
                break;
            }
            case 3:
                std::cout << "\nCurrent B+ Tree Structure:\n";
                std::cout << "----------------------------\n";
                tree.printTree();
                std::cout << "----------------------------\n";
                break;
            case 4:
                std::cout << "\nAre you sure you want to delete the tree? (y/n): ";
                char confirm;
                std::cin >> confirm;
                if (confirm == 'y' || confirm == 'Y') {
                    if (remove(filename.c_str()) == 0) {
                        std::cout << "Tree cleared successfully!\n";
                        tree = BPlusTree(filename);
                        std::cout << "New empty tree created.\n";
                    } else {
                        std::cout << "Error clearing tree.\n";
                    }
                }
                break;
            case 5:
                std::cout << "\nExiting program. Tree saved to " << filename << "\n";
                return 0;
            default:
                std::cout << "\nInvalid choice! Please enter 1-5.\n";
        }
    }
    
    return 0;
}