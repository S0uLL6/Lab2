#include <stdexcept>
#include "Sequence.hpp"
#include "lab2_cpp/Sequence.hpp"

int positive_mod(int a, int b) {
    int r = a % b;
    return (r < 0) ? (r + b) : r;
}

template <typename TKey, typename TValue>
struct Pair {
    TKey key;
    TValue value;
};

template <typename TKey, typename TValue>
class IDictionary {
public:
    virtual ~IDictionary() = default;

    virtual TValue& Get(const TKey& key) = 0;
    virtual bool Contains(const TKey& key) = 0;
    virtual void Add(const TKey& key, const TValue& value) = 0;
    virtual void Set(const TKey& key, const TValue& value) = 0;
    virtual void Delete(const TKey& key) = 0;

    virtual int Size() = 0;
    virtual int Capacity() = 0;
};

template <typename TKey, typename TValue>
class HashMap : public IDictionary<TKey, TValue> {
public:
    typedef Pair<TKey, TValue> KV;

    HashMap(
        int (*hash_func)(const TKey&),
        int initial_cap = 31,
        double grow_factor = 2.0,
        double shrink_factor = 3.0,
        double threshold = 0.7
    ) : _hash(hash_func),
        cap(initial_cap),
        grow_factor(grow_factor),
        shrink_factor(shrink_factor),
        threshold(threshold)
    {
        _buckets = new ArraySequence<ArraySequence<KV*>*>();
        for(int i=0; i < cap; ++i) {
            // TODO error prone
            _buckets->Append((ArraySequence<KV*>*)nullptr);
        }
    }

    ~HashMap() {
        if (_buckets) {
            int n = _buckets->GetLength();
            for(int i = 0; i < n; ++i) {
                ArraySequence<KV*>* bucket = _buckets->Get(static_cast<size_t>(i));
                if (bucket) {
                    delete bucket;
                }
            }
            delete _buckets;
        }
    }

    TValue& Get(const TKey& key) override {
        ArraySequence<KV*>* _bucket = _buckets->Get(bucket_index(key));
        // add throw bucket not found
        int idx = find_in_bucket(_bucket, key);
        // potentially throw if idx < 0
        return _bucket->Get(idx)->value;
    };

    bool Contains(const TKey& key) override {
        ArraySequence<KV*>* _bucket = _buckets->Get(bucket_index(key));
        if (!_bucket) return false;
        return bucket_index(_bucket, key) >= 0;
    }

    void Add(const TKey& key, const TValue& value) override {
        if(Contains(key)) throw std::invalid_argument("duplicate");
        int idx = bucket_index(key);
        ensure_bucket(idx);
        insert_to_bucket(_buckets->Get(idx), key, value);
        ++count;

        if (count >= threshold * cap) {
            // possible implicit cast
            rehash(grow_factor * cap);
        }
    }

    void Set(const TKey& key, const TValue& value) override {
        int bi = bucket_index(key);
        ensure_bucket(bi);
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        int idx = find_in_bucket(bucket, key);
        if(idx >= 0) {
            bucket->Get(static_cast<size_t>(idx))->value = value;
            return;
        }
        insert_to_bucket(bucket, key, value);
        ++count;

        if (count >= threshold * cap) {
            // possible implicit cast
            rehash(grow_factor * cap);
        }
    };

    void Delete(const TKey& key) override {
        int bi = bucket_index(key);
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        // can we delete unexisting element?
        if(!bucket) return;
        int idx = find_in_bucket(bucket, key);
        if(idx >= 0) {
            bucket->Delete(static_cast<size_t>(idx));
            --count;
        }
        if (count <= (1 - threshold) * cap) {
            // possible implicit cast
            rehash(cap / shrink_factor);
        }
    };

    int Size() override {return count;}
    int Capacity() override {return cap;}

private:
    ArraySequence<ArraySequence<KV*>*>* _buckets;
    int (*_hash)(const TKey&);
    int count;
    int cap;
    double grow_factor;
    double shrink_factor;
    double threshold;

    int bucket_index(const TKey& key) {
        int h = _hash(key);
        return positive_mod(h, cap);
    }

    // potentially improve to binary (now linear)
    int find_in_bucket(ArraySequence<KV*>* bucket, const TKey& key) {
        size_t n = bucket->GetLength();
        for(size_t i = 0; i < n; ++i) {
            // assumes operator== on TKey
            if(bucket->Get(i)->key == key) return i;
        }
        return -1;
    }

    void ensure_bucket(int idx) {
        ArraySequence<KV*>* _bucket = _buckets->Get(idx);
        if(!_bucket) {
            // error prone
            _bucket = new ArraySequence<KV*>*();
            _buckets->Delete(idx);
            _buckets->InsertAt(_bucket, idx);
        }
    }

    void insert_to_bucket(ArraySequence<KV*>* _bucket, const TKey& key, const TValue& value) {
        // potentially add smart pointers
        KV* node = new KV{ key, value };
        _bucket->Append(node);
        delete node;
    }

    void rehash(int newCap) {
        if (newCap < 1) newCap = 1;

        ArraySequence<ArraySequence<KV*>*>* old = _buckets;
        int oldCap = cap;

        cap = newCap;
        _buckets = new ArraySequence<ArraySequence<KV*>*>();
        for(int i=0; i < cap; ++i) {
            // TODO error prone
            _buckets->Append((ArraySequence<KV*>*)nullptr);
        }

        for(size_t i = 0; i < oldCap; ++i) {
            ArraySequence<KV>* _bucket = old->Get(i);
            if(!_bucket) continue;

            size_t n = _bucket->GetLength();
            for(size_t j = 0; j < n; ++j) {
                KV* kv = _bucket->Get(j);
                int bi = bucket_index(kv->key);
                ensure_bucket(bi);
                insert_to_bucket(_buckets->Get(bi), kv->key, kv->value);
            }
            delete _bucket;
        }
        delete old;
    }
};

template <typename T>
struct Range {
    T lo;
    T hi;

    bool contains(const T& x) {
        return ((lo <= x) && (x <= hi));
    }

    bool operator==(const Range& other) {
        // assume discrete numbers, intervals are only closed
        return ((lo == lo) && (hi == hi));
    }
};

template<typename T>
int hashRange(const Range<T>& r) {
    long long a = static_cast<long long>(r.lo);
    long long b = static_cast<long long>(r.hi);
    long long x = a * 1315423911LL ^ (b * 2654435761LL);
    if (x < 0) x = -x;
    return (int)(x & 0x7fffffff);
}
