// start.cpp
#include "Sequence.hpp"

#include <iostream>
#include <new>
#include <stdexcept>

template <typename T>
inline T MaxT(const T& a, const T& b) { return (a < b ? b : a); }

template <typename T>
inline T MinT(const T& a, const T& b) { return (b < a ? b : a); }

static inline int positive_mod(int x, int m) {
    int r = x % m;
    return (r < 0) ? (r + m) : r;
}

template <typename TKey, typename TValue>
struct KVPair {
    TKey   key;
    TValue value;
};

// ------------------------------ IDictionary ----------------------------------

template <typename TKey, typename TValue>
class IDictionary {
public:
    virtual ~IDictionary() {}

    virtual TValue& Get(const TKey& key) = 0;              // throw, если нет
    virtual bool    ContainsKey(const TKey& key) = 0;
    virtual void    Add(const TKey& key, const TValue& v) = 0;   // throw, если есть
    virtual void    Set(const TKey& key, const TValue& v) = 0;   // upsert
    virtual void    Remove(const TKey& key) = 0;           // throw, если нет

    virtual int     GetCount() const = 0;
    virtual int     GetCapacity() const = 0;
};

// ------------------------------ HashMap (закрытого типа, коллизии — последовательностями) ----

template <typename TKey, typename TValue>
class HashMap : public IDictionary<TKey, TValue> {
public:
    typedef KVPair<TKey, TValue> KV;

    // hashFn: пользовательская хеш-функция (обязательна)
    // initial_capacity: число бакетов (>=1)
    // p >= q > 1: параметры shrink/grow
    HashMap(int (*hashFn)(const TKey&),
            int initial_capacity = 25,
            double p = 4.0,
            double q = 2.0)
    : _hash(hashFn), _count(0), _capacity(initial_capacity),
    _p(p), _q(q)
    {
        if (!_hash)                 throw std::invalid_argument("HashMap: hashFn is null");
        if (_capacity < 1)          _capacity = 1;
        if (!(_p >= _q && _q > 1))  throw std::invalid_argument("HashMap: require p >= q > 1");

        _buckets = new ArraySequence< ArraySequence<KV*>* >();
        // Растянем массив бакетов до _capacity, заполнив nullptr
        _buckets->SetAt(0, (ArraySequence<KV*>*)nullptr);
        // fill the rest
        for (int i = 1; i < _capacity; ++i) {
            _buckets->Append((ArraySequence<KV*>*)nullptr);
        }
    }

    ~HashMap() {
        // Удаляем содержимое бакетов и сами бакеты
        if (_buckets) {
            int n = _buckets->GetLength();
            for (int i = 0; i < n; ++i) {
                ArraySequence<KV*>* bucket = _buckets->Get(i);
                // new code
                if (!bucket) continue;
                const int m = bucket->GetLength();
                for (int j = 0; j < m; ++j) {
                    KV* kv = bucket->Get(j);
                    if (kv) delete kv;
                }
                delete bucket;
            }
            delete _buckets;
        }
    }

    // IDictionary
    TValue& Get(const TKey& key) override {
        int bi = bucket_index(key);
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        if (!bucket) throw std::out_of_range("Get: key not found (empty bucket)");

        int idx = index_in_bucket(bucket, key);
        if (idx < 0) throw std::out_of_range("Get: key not found");
        return bucket->Get(idx)->value;
    }

    bool ContainsKey(const TKey& key) override {
        int bi = bucket_index(key);
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        if (!bucket) return false;
        return index_in_bucket(bucket, key) >= 0;
    }

    void Add(const TKey& key, const TValue& v) override {
        if (ContainsKey(key)) throw std::invalid_argument("Add: duplicate key");

        ensure_bucket(bucket_index(key));
        insert_to_bucket(_buckets->Get(bucket_index(key)), key, v);
        ++_count;

        if (_count == _capacity) {
            rehash(static_cast<int>(_capacity * _q)); // grow ×q
        }
    }

    void Set(const TKey& key, const TValue& v) override {
        int bi = bucket_index(key);
        ensure_bucket(bi);
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        int idx = index_in_bucket(bucket, key);
        if (idx >= 0) {
            bucket->Get(idx)->value = v;
            return;
        }
        // не было — вставляем
        insert_to_bucket(bucket, key, v);
        ++_count;

        if (_count == _capacity) {
            rehash(static_cast<int>(_capacity * _q)); // grow ×q
        }
    }

    void Remove(const TKey& key) override {
        int bi = bucket_index(key);
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        if (!bucket) throw std::out_of_range("Remove: key not found");

        int m = bucket->GetLength();
        for (int i = 0; i < m; ++i) {
            if (equal_keys(bucket->Get(i)->key, key)) {
                // new code
                KV* victim = bucket->Get(i);
                delete victim;
                bucket->SetAt(i, nullptr);
                bucket->Delete(i);
                --_count;

                // shrink при n ≤ c/p
                if (_count <= static_cast<int>(_capacity / _p) && _capacity > 1) {
                    int newCap = static_cast<int>(_capacity / _q);
                    if (newCap < 1) newCap = 1;
                    rehash(newCap);
                }
                return;
            }
        }
        throw std::out_of_range("Remove: key not found");
    }

    int GetCount() const override    { return _count; }
    int GetCapacity() const override { return _capacity; }

private:
    ArraySequence< ArraySequence<KV*>* >* _buckets = nullptr;
    int (*_hash)(const TKey&);
    int _count;
    int _capacity;
    double _p;
    double _q;

    // Хеш в индекс бакета
    int bucket_index(const TKey& key) const {
        int h = _hash(key);
        return positive_mod(h, _capacity);
    }

    static bool equal_keys(const TKey& a, const TKey& b) {
        // Требуется оператор== у ключа
        return (a == b);
    }

    // Линейный поиск в бакете
    static int index_in_bucket(ArraySequence<KV*>* bucket, const TKey& key) {
        int n = bucket->GetLength();
        for (int i = 0; i < n; ++i) {
            // guard agains nulls
            KV* kv = bucket->Get(i);
            if (kv && equal_keys(kv->key, key)) return i;
        }
        return -1;
    }

    void ensure_bucket(int bi) {
        ArraySequence<KV*>* bucket = _buckets->Get(bi);
        if (!bucket) {
            bucket = new ArraySequence<KV*>();
            // new code
            bucket->SetAt(0, (KV*)nullptr);
            _buckets->SetAt(static_cast<size_t>(bi), bucket);
        }
    }

    static void insert_to_bucket(ArraySequence<KV*>* bucket,
                                 const TKey& key,
                                 const TValue& v)
    {
        // Вставка в конец
        KV* node = new KV{ key, v };
        if (bucket->GetLength() >= 1 && bucket->Get(0) == nullptr) {
            bucket->SetAt(0, node);
        } else {
            bucket->Append(node);
        }
        // Так как ArraySequence<KV> хранит элементы по значению,
        // удобнее Append именно значением. Но по твоим сигнатурам
        // используется Append(T*). Тогда перегоняем через копию:
        // DO NOT delete node here — bucket owns it now.  <-- FIX
        // delete node;
    }

    void rehash(int newCapacity) {
        if (newCapacity < 1) newCapacity = 1;

        // Сохраняем старые бакеты
        ArraySequence< ArraySequence<KV*>* >* old = _buckets;
        int oldCap = _capacity;

        // Создаем новые
        _capacity = newCapacity;
        _buckets = new ArraySequence< ArraySequence<KV*>* >();
        _buckets->SetAt(0, (ArraySequence<KV*>*)nullptr);
        // fill the rest
        for (int i = 1; i < _capacity; ++i) {
            _buckets->Append((ArraySequence<KV*>*)nullptr);
        }

        // Пересыпаем
        for (int i = 0; i < oldCap; ++i) {
            ArraySequence<KV*>* bucket = old->Get(i);
            if (!bucket) continue;

            int m = bucket->GetLength();
            for (int j = 0; j < m; ++j) {
                // move
                KV* kv = bucket->Get(j);
                const int bi = bucket_index(kv->key);
                ensure_bucket(bi);
                _buckets->Get(bi)->Append(kv);
                bucket->SetAt(j, nullptr);
            }
            delete bucket;
        }
        delete old;

        // _count не меняется
    }
};

// ------------------------------ Диапазон (бин) -------------------------------

template <typename T>
struct Range {
    T lo; // inclusive
    T hi; // exclusive
    bool contains(const T& x) const {
        // [lo, hi)
        return !(x < lo) && (x < hi);
    }
    bool operator==(const Range& o) const {
        return !(lo < o.lo) && !(o.lo < lo) && !(hi < o.hi) && !(o.hi < hi);
    }
};

// Хеш для Range<T> (упрощённый; без STL)
template <typename T>
int HashRange(const Range<T>& r) {
    // Требуется, чтобы у T были преобразования к целому (или перегрузка для double/int).
    long long a = (long long)r.lo;
    long long b = (long long)r.hi;
    long long x = a * 1315423911LL ^ (b * 2654435761LL);
    if (x < 0) x = -x;
    return (int)(x & 0x7fffffff);
}

// Построение по Sequence<T> и равномерным бинам в [minVal, maxVal) по проектору:
//   Key Projector(const T&)
// Возвращает IDictionary<Range<Key>, int>*.

template <typename T, typename Key>
struct HistogramParams {
    Key minVal;
    Key maxVal;
    int binCount;
    Key (*Projector)(const T&); // указатель на функцию-проектор
};

// Создание равномерных бинов
template <typename Key>
ArraySequence< Range<Key> >* MakeUniformBins(Key minVal, Key maxVal, int binCount) {
    if (binCount <= 0) throw std::invalid_argument("binCount must be > 0");
    if (!(minVal <= maxVal)) throw std::invalid_argument("minVal must be <= maxVal");

    ArraySequence< Range<Key> >* bins = new ArraySequence< Range<Key> >();
    // Ширина (для целых типов делим поровну, последний бин — до maxVal)
    Key width = (Key)((maxVal - minVal) / (Key)binCount);
    if (width <= (Key)0) width = (Key)1;

    Key cur = minVal;
    // first bin at slot 0
    {
        Key next = (binCount == 1) ? maxVal : (Key)(cur + width);
        Range<Key> bin{ cur, next };
        bins->SetAt(0, bin);                // use existing slot 0
        cur = next;
    }

    // remaining bins
    for (int i = 1; i < binCount; ++i) {
        Key next = (i == binCount - 1) ? maxVal : (Key)(cur + width);
        Range<Key> bin{ cur, next };
        bins->Append(bin);
        cur = next;
    }
    return bins;
}

template <typename T, typename Key>
IDictionary< Range<Key>, int >*
BuildHistogram(ArraySequence<T>* seq, const HistogramParams<T,Key>& par) {
    if (!seq) throw std::invalid_argument("BuildHistogram: seq is null");
    if (!par.Projector) throw std::invalid_argument("BuildHistogram: projector is null");
    if (par.binCount <= 0) throw std::invalid_argument("BuildHistogram: binCount <= 0");

    // Бины
    ArraySequence< Range<Key> >* bins = MakeUniformBins<Key>(par.minVal, par.maxVal, par.binCount);

    // Словарь: ключ — Range<Key>, значение — int
    HashMap< Range<Key>, int >* dict =
    new HashMap< Range<Key>, int >(&HashRange<Key>, MaxT(25, par.binCount*2), 4.0, 2.0);

    // Инициализируем нулями для детерминированного вывода
    int B = bins->GetLength();
    for (int i = 0; i < B; ++i) {
        Range<Key> bin = bins->Get(i);
        dict->Set(bin, 0);
    }

    // Проход по данным
    int n = seq->GetLength();
    for (int i = 0; i < n; ++i) {
        T item = seq->Get(i);
        Key value = par.Projector(item);

        // Линейный поиск бина
        for (int j = 0; j < B; ++j) {
            Range<Key> bin = bins->Get(j);
            if (bin.contains(value)) {
                int cur = dict->Get(bin);
                dict->Set(bin, cur + 1);
                break;
            }
        }
    }

    delete bins;
    return dict;
}

// ------------------------------ Пример -----------------
struct Person { int age; };
// could write lambda to extract age from Person inside BuildHistogram
// and pass lambda to the function (add argument to BuildHistogram)
static int ProjectAge(const Person& p) { return p.age; }

int main() {
    try {
        // 1) Build sample data
        auto* people = new ArraySequence<Person>();
        for (int a = 0; a < 100; ++a) {
            people->Append(Person{a});
        }

        // 2) Histogram params
        HistogramParams<Person,int> hp;
        hp.minVal = 0; hp.maxVal = 100; hp.binCount = 10;
        hp.Projector = &ProjectAge;

        // 3) Build histogram
        IDictionary< Range<int>, int >* H = BuildHistogram<Person,int>(people, hp);

        // 4) Recreate the same bins for printing
        ArraySequence< Range<int> >* bins = MakeUniformBins<int>(hp.minVal, hp.maxVal, hp.binCount);

        // 5) Read counts & find maximum (for bar scaling)
        int total = 0;
        int maxCount = 0;
        for (int i = 0; i < (int)bins->GetLength(); ++i) {
            Range<int> bin = bins->Get(i);
            int c = 0;
            if (H->ContainsKey(bin)) c = H->Get(bin);
            total += c;
            if (c > maxCount) maxCount = c;
        }

        // 6) Print table header
        std::cout << "Histogram of ages (" << hp.minVal << "–" << hp.maxVal
        << "), " << hp.binCount << " bins\n";
        std::cout << "Total items: " << total << "\n\n";
        std::cout << "Bin range         Count   Bar\n";
        std::cout << "----------------  ------  -----------------------------------------------\n";

        // 7) Print each bin with an ASCII bar (scaled to 50 chars)
        const int BAR_WIDTH = 50;
        for (int i = 0; i < static_cast<int>(bins->GetLength()); ++i) {
            Range<int> bin = bins->Get(i);
            int c = 0;
            if (H->ContainsKey(bin)) c = H->Get(bin);

            int barLen = (maxCount > 0) ? (c * BAR_WIDTH) / maxCount : 0;
            std::cout.width(5);
            std::cout << bin.lo << "–";
            std::cout.width(5);
            std::cout << bin.hi << "   ";

            std::cout.width(6);
            std::cout << c << "   ";

            for (int k = 0; k < barLen; ++k) std::cout << '#';
            std::cout << "\n";
        }

        // 8) Clean up
        delete bins;
        delete H;
        delete people;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

