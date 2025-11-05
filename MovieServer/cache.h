#pragma once
#include <iostream>
#include <unordered_map>
#include <list>
#include <string>
#include <utility>
#include <mutex>

using namespace std;

class Cache {
  private:
    size_t capacity;
    list<string> LRUlist; // stores keys
    unordered_map<string, pair<string, list<string>::iterator>> cacheMap;
    mutable mutex mtx;

  public:
    explicit Cache(size_t cap = 1000);

    bool exists(const string &key);
    string get(const string &key);
    void put(const string &key, const string &value);
    void erase(const string &key);
    void clear();
    size_t size() const;
};