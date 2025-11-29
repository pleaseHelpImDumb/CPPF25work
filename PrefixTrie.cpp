#include <cctype>
#include <string>

class TreeNode {
public:
    TreeNode* children[26];
    bool validWordEnd;

    TreeNode(){
        validWordEnd = false;
        for (int i = 0; i < 26; i++) {
            children[i] = nullptr; 
        }
    };
};

class PrefixTree {

private:
    // for CHAR to INT translation
    int charToInt(char a){
        return tolower(a) - 'a';
    }


public:
    TreeNode* root;
    PrefixTree() {
        root = new TreeNode();
    }
    
    void insert(string word) { // ex: "cat"
        
        TreeNode* temp = root;

        for(int i = 0; i < word.size(); i++){
            char c = word.at(i); // get letter
            int cIndex = charToInt(c); // get alphabet index of letter

            if(temp->children[cIndex] == nullptr){ // if the letter does NOT exist
                temp->children[cIndex] = new TreeNode(); // make a tree node at that index
            }
            //move temp ptr down to new child node
            temp = temp->children[cIndex];
        }

        //mark temp as a valid word end if done with loop! ex: node with T set to true
        temp->validWordEnd = true;
    }
    
    bool search(string word) {
        TreeNode* temp = root;

        for(int i = 0; i < word.size(); i++){
            char c = word.at(i); // get letter
            int cIndex = charToInt(c); // get alphabet index of letter
            
            if(temp->children[cIndex] == nullptr) return false; // if next letter doesn't exist
            temp = temp->children[cIndex]; // or move down to next letter
        }
        if(temp->validWordEnd == true) return true; //only return true if it's a valid word end
        return false;
    }
    
    bool startsWith(string prefix) { // ex: search "ca" with "cat" in tree
        TreeNode* temp = root;

        for(int i = 0; i < prefix.size(); i++){
            char c = prefix.at(i); // get letter
            int cIndex = charToInt(c); // get alphabet index of letter

            if(temp->children[cIndex] == nullptr) return false; // if no letter
            temp = temp->children[cIndex]; //move next
        }
        // if we're here, the for-loop finished without returning false
        return true; // since words can be BOTH prefixes and valid word ends.
    }
};
