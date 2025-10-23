#include "DynamicArray.hpp"
#include "LinkedList.hpp"

template <typename T> class Container {
public:
    virtual T Get(size_t index) const = 0;
    virtual size_t GetLength() const = 0;
    virtual ~Sequence() {}
protected:
    Container() = default;
    Container(Container<T>* other) = default;
};

template <typename T> class Sequence : public Container<T> {
public:
    virtual T GetFirst() const = 0;
    virtual T GetLast() const = 0;
    virtual T Get(size_t index) const = 0;
    virtual size_t GetLength() const = 0;
    virtual Sequence<T>* GetSubsequence(size_t start, size_t end) const = 0;
    virtual Sequence<T>* Append(T item) = 0;
    virtual Sequence<T>* Prepend(T item) = 0;
    virtual Sequence<T>* InsertAt(T item, size_t index) = 0;
    virtual Sequence<T>* Concat(Sequence<T>* list) const = 0;
    virtual ~Sequence() {}
protected:
    Sequence() = default;
    Sequence(SequenceT>* other) : Container<T>(other) {};
};

template <typename T> class ArraySequence : public Sequence<T> {
private:
    DynamicArray<T> data;

public:
    ArraySequence() : data(0) {}

    ArraySequence(const T* items, size_t count) : data(items, count) {}

    ArraySequence(const LinkedList<T>& list) : data(list.GetLength()) {
        for (size_t i = 0; i < list.GetLength(); i++) {
            data.Set(i, list.Get(i));
        }
    }

    T GetFirst() const override {
        return data.Get(0);
    }

    T GetLast() const override {
        return data.Get(data.GetSize() - 1);
    }

    T Get(size_t index) const override {
        return data.Get(index);
    }

    size_t GetLength() const override {
        return data.GetSize();
    }

    Sequence<T>* GetSubsequence(size_t start, size_t end) const override {
        if (start > end || end >= GetLength()) throw out_of_range("Invalid range");
        T* temp = new T[end - start + 1];
        for (size_t i = 0; i <= end - start; i++)
            temp[i] = data.Get(start + i);
        auto* sub = new ArraySequence<T>(temp, end - start + 1);
        delete[] temp;
        return sub;
    }

    Sequence<T>* Append(T item) override {
        data.Resize(data.GetSize() + 1);
        data.Set(data.GetSize() - 1, item);
        return this;
    }

    Sequence<T>* Prepend(T item) override {
        size_t size = data.GetSize();
        data.Resize(size + 1);
        for (size_t i = size; i > 0; i--)
            data.Set(i, data.Get(i - 1));
        data.Set(0, item);
        return this;
    }

    Sequence<T>* InsertAt(T item, size_t index) override {
        if (index > data.GetSize()) throw out_of_range("Index out of range");
        data.Resize(data.GetSize() + 1);
        for (size_t i = data.GetSize() - 1; i > index; i--)
            data.Set(i, data.Get(i - 1));
        data.Set(index, item);
        return this;
    }

    Sequence<T>* Concat(Sequence<T>* list) const override {
        size_t total = GetLength() + list->GetLength();
        T* combined = new T[total];
        for (size_t i = 0; i < GetLength(); i++)
            combined[i] = Get(i);
        for (size_t i = 0; i < list->GetLength(); i++)
            combined[GetLength() + i] = list->Get(i);
        auto* result = new ArraySequence<T>(combined, total);
        delete[] combined;
        return result;
    }

    Sequence<T>* Add(const Sequence<T>* other) const  {
        if (GetLength() != other->GetLength()) throw invalid_argument("Size mismatch in addition");
        DynamicArray<T> otherData(GetLength());
        for (size_t i = 0; i < GetLength(); i++)
            otherData.Set(i, other->Get(i));
        DynamicArray<T> result = data + otherData;
        return new ArraySequence<T>(result);
    }

    Sequence<T>* MultiplyByScalar(T scalar) const  {
        DynamicArray<T> result = data * scalar;
        return new ArraySequence<T>(result);
    }

    T Dot(const Sequence<T>* other) const  {
        if (GetLength() != other->GetLength()) throw invalid_argument("Size mismatch in dot product");
        DynamicArray<T> otherData(GetLength());
        for (size_t i = 0; i < GetLength(); i++)
            otherData.Set(i, other->Get(i));
        return data.Dot(otherData);
    }

    double Norm() const  {
        return data.Norm();
    }
};

template <typename T> class ListSequence : public Sequence<T> {
private:
    LinkedList<T> list;

public:
    ListSequence() {}

    ListSequence(const T* items, size_t count) : list(items, count) {}

    ListSequence(const LinkedList<T>& linked) : list(linked) {}

    T GetFirst() const override {
        return list.GetFirst();
    }

    T GetLast() const override {
        return list.GetLast();
    }

    T Get(size_t index) const override {
        return list.Get(index);
    }

    size_t GetLength() const override {
        return list.GetLength();
    }

    Sequence<T>* GetSubsequence(size_t start, size_t end) const override {
        return new ListSequence<T>(*list.GetSubList(start, end));
    }

    Sequence<T>* Append(T item) override {
        list.Append(item);
        return this;
    }

    Sequence<T>* Prepend(T item) override {
        list.Prepend(item);
        return this;
    }

    Sequence<T>* InsertAt(T item, size_t index) override {
        list.InsertAt(item, index);
        return this;
    }

    Sequence<T>* Concat(Sequence<T>* other) const override {
        auto* newList = list.Concat(&(dynamic_cast<ListSequence<T>*>(other)->list));
        return new ListSequence<T>(*newList);
    }
};

template <class T> class MutableArraySequence : public ArraySequence<T> {
public:
    using ArraySequence<T>::ArraySequence;
    Sequence<T>* Append(T item) override {
        ArraySequence<T>::Append(item); 
        return this;
    }

    Sequence<T>* Prepend(T item) override {
        ArraySequence<T>::Prepend(item); 
        return this;
    }

    Sequence<T>* InsertAt(T item, int index) override {
        ArraySequence<T>::InsertAT(item, index); 
        return this;
    }
};

template <class T> class ImmutableArraySequence : public ArraySequence<T> {
public:
    using ArraySequence<T>::ArraySequence;
    Sequence<T>* Append(T item) override {
        ArraySequence<T>* clon = new ArraySequence<T>(*this);
        clon->Append(item); 
        return this;
    }

    Sequence<T>* Prepend(T item) override {
        ArraySequence<T>* clon = new ArraySequence<T>(*this);
        clon->Prepend(item); 
        return this;
    }

    Sequence<T>* InsertAt(T item, int index) override {
        ArraySequence<T>* clon = new ArraySequence<T>(*this);
        clon->InsertAt(item, index); 
        return this;
    }
};

template <typename T> class MutableArraySequence : public ArraySequence<T> {
public:

    Sequence<T>* Append(T item) override {
        ArraySequence<T>::Append(item);
        return this;
    }

    Sequence<T>* Prepend(T item) override {
        ArraySequence<T>::Prepend(item);
        return this;
    }

    Sequence<T>* InsertAt(T item, size_t index) override {
        ArraySequence<T>::InsertAt(item, index);
        return this;
    }
};

template <typename T> class ImmutableArraySequence : public ArraySequence<T> {
public:

    Sequence<T>* Append(T item) override {
        ArraySequence<T>* clone = new ArraySequence<T>(*this);
        clone->Append(item);
        return clone;
    }

    Sequence<T>* Prepend(T item) override {
        ArraySequence<T>* clone = new ArraySequence<T>(*this);
        clone->Prepend(item);
        return clone;
    }

    Sequence<T>* InsertAt(T item, size_t index) override {
        ArraySequence<T>* clone = new ArraySequence<T>(*this);
        clone->InsertAt(item, index);
        return clone;
    }
};

template <typename T> class MutableListSequence : public ListSequence<T> {
public:

    Sequence<T>* Append(T item) override {
        ListSequence<T>::Append(item);
        return this;
    }

    Sequence<T>* Prepend(T item) override {
        ListSequence<T>::Prepend(item);
        return this;
    }

    Sequence<T>* InsertAt(T item, size_t index) override {
        ListSequence<T>::InsertAt(item, index);
        return this;
    }
};

template <typename T> class ImmutableListSequence : public ListSequence<T> {
public:

    Sequence<T>* Append(T item) override {
        ListSequence<T>* clone = new ListSequence<T>(*this);
        clone->Append(item);
        return clone;
    }

    Sequence<T>* Prepend(T item) override {
        ListSequence<T>* clone = new ListSequence<T>(*this);
        clone->Prepend(item);
        return clone;
    }

    Sequence<T>* InsertAt(T item, size_t index) override {
        ListSequence<T>* clone = new ListSequence<T>(*this);
        clone->InsertAt(item, index);
        return clone;
    }
};