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
    list<string> LRUlist;
    unordered_map<string, pair<string, list<string>::iterator>> cacheMap;
    mutex mtx;

  public:
    explicit Cache(size_t cap = 1000);

    bool read(const string &key, string &value);
    void insert(const string &key, const string &value);
    void remove(const string &key);
    void clear();
};