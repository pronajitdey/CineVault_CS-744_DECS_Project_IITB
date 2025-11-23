#include "cache.h"

// Constructor
Cache::Cache(size_t cap) : capacity(cap) {}

// Check whether key present in cache or not
bool Cache::exists(const string &key) {
  lock_guard<mutex> lock(mtx);
  return cacheMap.find(key) != cacheMap.end();
}

// Get data from cache
string Cache::get(const string &key) {
  lock_guard<mutex> lock(mtx);
  auto it = cacheMap.find(key);
  if (it == cacheMap.end()) {
    cout << "Cache miss for: " << key << endl;
    return "";
  }

  // it is iterator to cacheMap, it->second.second is iterator to LRUlist
  string value = it->second.first;

  LRUlist.erase(it->second.second);
  LRUlist.push_front(key);
  it->second.second = LRUlist.begin();
  cout << "Cache hit for: " << key << endl;
  return value;
}

// Put data into cache
void Cache::put(const string &key, const string &value) {
  lock_guard<mutex> lock(mtx);

  auto it = cacheMap.find(key);
  if (it != cacheMap.end()) {
    // Key already exists in cache
    it->second.first = value;
    LRUlist.erase(it->second.second);
    LRUlist.push_front(key);
    it->second.second = LRUlist.begin();
    return;
  }

  if (cacheMap.size() >= capacity) {
    // Cache full, evict
    string leastUsed = LRUlist.back();
    LRUlist.pop_back();
    cacheMap.erase(leastUsed);
    cout << "Key: " << leastUsed << " evicted from cache" << endl;
  }

  // Insert new key
  LRUlist.push_front(key);
  cacheMap[key] = make_pair(value, LRUlist.begin());
  cout << "Put into cache: " << key << endl;
}

// Remove key from cache
void Cache::erase(const string &key) {
  lock_guard<mutex> lock(mtx);
  auto it = cacheMap.find(key);
  if (it == cacheMap.end()) return;
  LRUlist.erase(it->second.second);
  cacheMap.erase(it);
  cout << "Removed from cache: " << key << endl;
}

// Clear cache
void Cache::clear() {
  lock_guard<mutex> lock(mtx);
  cacheMap.clear();
  LRUlist.clear();
  cout << "Cache cleared" << endl;
}

// Get number of items stored in cache
size_t Cache::size() const {
  lock_guard<mutex> lock(mtx);
  return cacheMap.size();
}
