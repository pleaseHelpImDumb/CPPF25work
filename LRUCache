#include <unordered_map>
using namespace std;

class LRUNode{
private:
public:
    int val;
    LRUNode* next;
    LRUNode* prev;
    int key;
    LRUNode(int key, int val){
        this->val = val;
        this->key = key;
    }
};

class LRUCache {
private:
    unordered_map<int, LRUNode*> cache;
    int capacity;
    LRUNode* head;
    LRUNode* tail;

    void addToCache(LRUNode* newNode){
        LRUNode* temp = head->next;
        newNode->next = temp;
        newNode->prev = head;
        
        head->next = newNode;
        temp->prev = newNode;
    }

    void moveToHead(LRUNode* newNode){
        removeNode(newNode);
        addToCache(newNode);
    }

    void removeNode(LRUNode* t){
        t->next->prev = t->prev;
        t->prev->next = t->next;
    }

public:
    LRUCache(int capacity) {
        this->capacity = capacity;
        head = new LRUNode(-1, 0);
        tail = new LRUNode(-1, 0);
        head->next = tail;
        tail->prev = head;
    }
    
    int get(int key) {
        //check for key not found
            //return -1
        // check if key exist
            // move to most recent
            // return value

        // if cache finds a key
        if(cache.find(key) != cache.end()){
            LRUNode* node = cache[key];
            moveToHead(node);
            return node->val;
        }
        return -1;
    }
    
    void put(int key, int value) {
        //check key if found:
            // update value
            // return
        //check for capacity
            //if full, 'update' tail node
            //update head.next node to new node
            //point head => new node
            // return
        // finally, update exisitng value
        
        // if in cache, update
        if(cache.find(key) != cache.end()){
            LRUNode* temp = cache[key];
            temp->val = value;
            moveToHead(temp); //move updated node to front
            return;
        }

        // if cache is full
        if(cache.size() == capacity){
            LRUNode* last = tail->prev;
            cache.erase(last->key);

            //remove last from the list
            removeNode(tail->prev);
        }
        // add node to front
        LRUNode* newNode = new LRUNode(key, value);
        addToCache(newNode);
        cache[key] = newNode;
    }
};

/**
 * Your LRUCache object will be instantiated and called as such:
 * LRUCache* obj = new LRUCache(capacity);
 * int param_1 = obj->get(key);
 * obj->put(key,value);
 */
