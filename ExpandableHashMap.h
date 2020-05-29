#ifndef P4A_EXPANDABLEHASHMAP_H
#define P4A_EXPANDABLEHASHMAP_H

#include <vector>
#include <list>
#include <utility>
#include <string>
#include <algorithm>

template<typename KeyType, typename ValueType>
class ExpandableHashMap {
 public:
  ExpandableHashMap(double maximumLoadFactor = 0.5);
  ~ExpandableHashMap();
  void reset();
  int size() const;
  void associate(const KeyType &key, const ValueType &value);

  // for a map that can't be modified, return a pointer to const ValueType
  const ValueType *find(const KeyType &key) const;

  // for a modifiable map, return a pointer to modifiable ValueType
  ValueType *find(const KeyType &key) {
	return const_cast<ValueType *>(const_cast<const ExpandableHashMap *>(this)->find(key));
  }

  // C++11 syntax for preventing copying and assignment
  ExpandableHashMap(const ExpandableHashMap &) = delete;
  ExpandableHashMap &operator=(const ExpandableHashMap &) = delete;

 private:
  struct Item {
	KeyType key;
	ValueType value;
  };

  double max_load_factor;
  int num_buckets = 8;
  int num_items = 0;
  std::list<Item *> *hash_table;
};

template<typename KeyType, typename ValueType>
ExpandableHashMap<KeyType, ValueType>::ExpandableHashMap(double maximumLoadFactor) {
  hash_table = new std::list<Item *>[num_buckets];
  max_load_factor = maximumLoadFactor <= 0.0 ? 0.5 : maximumLoadFactor;
}

template<typename KeyType, typename ValueType>
ExpandableHashMap<KeyType, ValueType>::~ExpandableHashMap() {
  for (int i = 0; i < num_buckets; i++) {
	for (auto j = hash_table[i].begin(); j != hash_table[i].end(); j++) {
	  delete *j; //Deallocate memory for individual items
	}
  }
  delete[] hash_table; //Just a regular dynamic array so we can delete all at once
}

template<typename KeyType, typename ValueType>
void ExpandableHashMap<KeyType, ValueType>::reset() {
  for (int i = 0; i < num_buckets; i++) {
	for (auto j = hash_table[i].begin(); j != hash_table[i].end(); j++) {
	  delete *j; //Deallocate memory for individual items
	}
  }
  delete[] hash_table;
  hash_table = new std::list<Item *>[num_buckets];
  num_buckets = 8;
  num_items = 0;
}

template<typename KeyType, typename ValueType>
int ExpandableHashMap<KeyType, ValueType>::size() const {
  return num_items;
}

template<typename KeyType, typename ValueType>
void ExpandableHashMap<KeyType, ValueType>::associate(const KeyType &key, const ValueType &value) {
  ValueType *val = find(key);

  if (val != nullptr) {
	*val = value; //If the key is in our map, just modify the value and return
	return;
  }

  unsigned int hasher(const KeyType &k); // prototype

  ++num_items;
  if (((double) num_items / (double) num_buckets) > max_load_factor) { //If adding the new item makes us go over our max_load_factor
	int new_size = num_buckets * 2;
	auto *temp_hash_table = new std::list<Item *>[new_size]; //Create new hash table
	for (int i = 0; i < num_buckets; ++i) {
	  std::list p = hash_table[i];
	  for (auto it = p.begin(); it != p.end(); it++) {
		unsigned int h = hasher((*it)->key) % new_size; //Re-hash key
		temp_hash_table[h].push_back(*it); //Add to new hash table in (probably) a different bucket
	  }
	}
	delete[] hash_table; //Delete lists in old hash table (but don't deallocate memory for individual items bc those are still being used)
	hash_table = temp_hash_table;
	num_buckets = new_size;
  }

  unsigned int h = hasher(key) % num_buckets; //Hash key to figure out which bucket we should put the data in
  Item *i = new Item{key, value};
  hash_table[h].push_back(i);
}

template<typename KeyType, typename ValueType>
const ValueType *ExpandableHashMap<KeyType, ValueType>::find(const KeyType &key) const {
  unsigned int hasher(const KeyType &k); // prototype
  unsigned int h = hasher(key) % num_buckets;

  std::list p = hash_table[h];
  for (auto it = p.begin(); it != p.end(); it++) { //Search through the correct bucket
	if ((*it)->key == key) { //If the key matches
	  return &((*it)->value);
	}
  }

  return nullptr; //We couldn't find the key in the appropriate bucket so it's not in our map.
}

#endif //P4A_EXPANDABLEHASHMAP_H
