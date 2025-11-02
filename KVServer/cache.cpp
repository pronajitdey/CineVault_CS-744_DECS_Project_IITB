#include "cache.h"

// constructor
Cache::Cache(size_t cap) : capacity(cap) {}

// Read key from cache
bool Cache::read(const string &key, string &value) {
  lock_guard<mutex> lock(mtx);
  auto it = cacheMap.find(key);
  if (it == cacheMap.end()) return false; // cache miss

  // bring the key to the front of LRUlist(most recent)
  LRUlist.erase(it->second.second);
  LRUlist.push_front(key);
  it->second.second = LRUlist.begin();
  value = it->second.first;
  return true;  // cache hit
}

// Insert key into cache
void Cache::insert(const string &key, const string &value) {
  lock_guard<mutex> lock(mtx);

  auto it = cacheMap.find(key);
  // Key already exists and is in cache
  if (it != cacheMap.end()) {
    it->second.first = value;
    LRUlist.erase(it->second.second);
    LRUlist.push_front(key);
    it->second.second = LRUlist.begin();
    cout << "Key: " << key << " updated in cache" << endl;
    return;
  }

  // If cache is full, evict LRU element (last of list)
  if (cacheMap.size() > capacity) {
    string leastUsed = LRUlist.back();
    LRUlist.pop_back();
    cacheMap.erase(leastUsed);
    cout << "Key: " << leastUsed << " evicted from cache" << endl;
  }

  // Insert new key
  LRUlist.push_front(key);
  cacheMap[key] = make_pair(value, LRUlist.begin());
}

// Remove key from cache on deletion
void Cache::remove(const string &key) {
  lock_guard<mutex> lock(mtx);
  auto it = cacheMap.find(key);
  if (it != cacheMap.end()) {
    LRUlist.erase(it->second.second);
    cacheMap.erase(it);
  }
}

// Clear cache
void Cache::clear() {
  lock_guard<mutex> lock(mtx);
  cout << cacheMap.size() << endl;
  cacheMap.clear();
  LRUlist.clear();
  cout << cacheMap.size() << endl;
}