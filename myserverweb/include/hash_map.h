/*
MyServer
Copyright � 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

//valueType putList( const list <keyType>&, const list <valueType>&);
//valueType putList( const hashmap <keyType, valueType>&);
//valueType remove(const iterator&);
//valueType removeList(const list <keyType>&);
//valueType removeList(const hashmap <keyType, valueType>&);

#ifndef HASHMAP_H
#define HASHMAP_H

#include <list>
#include <vector>
#include <string>

using namespace std;

//hashmap Hash-Key-Value Struct
template <typename KeyType, typename ValueType>
struct Shkv
{
	unsigned int hash;
	KeyType key;
	ValueType value;
};

//hashmap Hash-Key-Value Struct
template <typename ValueType>
struct Shkv <char*, ValueType>
{
	unsigned int hash;
	string key;
	ValueType value;
};

template <typename KeyType, typename ValueType>
class HashMap;

template <typename KeyType, typename ValueType>
class MyIterator
{
	public:

	template <typename KType, typename VType>
	friend class HashMap;

	MyIterator();

	inline bool operator==(const MyIterator&) const;
	inline bool operator!=(const MyIterator&) const;
	inline ValueType& operator*() const;
	inline MyIterator& operator++();		//prefix
	inline MyIterator operator++(int);	//postfix
	//inline iterator& operator--();	//prefix
	//inline iterator operator--(int);	//postfix

	private:

	vector< list< Shkv<KeyType, ValueType> > > *vectorPtr;
	typename vector< list< Shkv<KeyType, ValueType> > >::iterator vectorIter;
	typename list< Shkv<KeyType, ValueType> >::iterator listIter;
};

template <typename KeyType, typename ValueType>
class HashMap
{
	public:

    typedef MyIterator<KeyType, ValueType> Iterator;

	HashMap();
	HashMap(int);
	HashMap(const float);
	HashMap(int, const float);
	inline bool empty(void) const;
	inline void clear(void);
	inline int size(void) const;
	inline Iterator begin(void);
	inline Iterator back(void);
	inline Iterator end(void);
	bool containsKey(const KeyType&);
	ValueType get(const KeyType&);
	Iterator getI(const KeyType&);
	ValueType put(KeyType&, const ValueType&);
	ValueType remove(const KeyType&);

	private:

	Iterator tempIterator;
	Shkv<KeyType, ValueType> tempShkv;
	vector < list < Shkv<KeyType, ValueType> > > data;
	typename list < Shkv<KeyType, ValueType> >::iterator dataIter, tempdataIter;
	vector <unsigned int> map;
	unsigned int offset, tempHash;
	int capacity, power, mask, load;
	float highLoadFactor, lowLoadFactor;

	void increaseSize(const int);
	void decreaseSize(const int);
	unsigned int superFastHash (const char *, int);
};

template <typename ValueType>
class HashMap <string, ValueType>
{
	public:

	typedef MyIterator<string, ValueType> Iterator;

	HashMap();
	HashMap(int);
	HashMap(const float);
	HashMap(int, const float);
	inline bool empty(void) const;
	inline void clear(void);
	inline int size(void) const;
	inline Iterator begin(void);
	inline Iterator back(void);
	inline Iterator end(void);
	bool containsKey(const string&);
	ValueType get(const string&);
	Iterator getI(const string&);
	ValueType put(string&, const ValueType&);
	ValueType remove(const string&);

	private:

	Iterator tempIterator;
	Shkv<string, ValueType> tempShkv;
	vector < list < Shkv<string, ValueType> > > data;
	typename list < Shkv<string, ValueType> >::iterator dataIter, tempdataIter;
	vector <unsigned int> map;
	unsigned int offset, tempHash;
	int capacity, power, mask, load;
	float highLoadFactor, lowLoadFactor;

	void increaseSize(const int);
	void decreaseSize(const int);
	unsigned int superFastHash (const char *, int);
};

template <typename ValueType>
class HashMap <char*, ValueType>
{
	public:

    typedef MyIterator<char*, ValueType> Iterator;

	HashMap();
	HashMap(int);
	HashMap(const float);
	HashMap(int, const float);
	inline bool empty(void) const;
	inline void clear(void);
	inline int size(void) const;
	inline Iterator begin(void);
	inline Iterator back(void);
	inline Iterator end(void);
	bool containsKey(const char* const);
	ValueType get(const char* const);
	Iterator getI(const char* const);
	ValueType put(char* const, const ValueType&);
	ValueType remove(const char* const);

	private:

	Iterator tempIterator;
	Shkv<char*, ValueType> tempShkv;
	vector < list < Shkv<char*, ValueType> > > data;
	typename list < Shkv<char*, ValueType> >::iterator dataIter, tempdataIter;
	vector <unsigned int> map;
	unsigned int offset, tempHash;
	int capacity, power, mask, load;
	float highLoadFactor, lowLoadFactor;

	void increaseSize(const int);
	void decreaseSize(const int);
	unsigned int superFastHash (const char *, int);
};

template <typename ValueType>
class HashMap <void*, ValueType>
{
	public:

    typedef MyIterator<void*, ValueType> Iterator;

	HashMap();
	HashMap(int);
	HashMap(const float);
	HashMap(int, const float);
	inline bool empty(void) const;
	inline void clear(void);
	inline int size(void) const;
	inline Iterator begin(void);
	inline Iterator back(void);
	inline Iterator end(void);
	bool containsKey(const void* const);
	ValueType get(const void* const);
	Iterator getI(const void* const);
	ValueType put(void* const, const ValueType&);
	ValueType remove(const void* const);

	private:

	Iterator tempIterator;
	Shkv<void*, ValueType> tempShkv;
	vector < list < Shkv<void*, ValueType> > > data;
	typename list < Shkv<void*, ValueType> >::iterator dataIter, tempdataIter;
	vector <unsigned int> map;
	unsigned int offset, tempHash;
	int capacity, power, mask, load;
	float highLoadFactor, lowLoadFactor;

	void increaseSize(const int);
	void decreaseSize(const int);
	unsigned int superFastHash (const char *, int);
};

#ifndef HASHMAP_CPP
#include "../source/hash_map.cpp"
#endif

#endif
