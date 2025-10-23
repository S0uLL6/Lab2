#include <cmath>
#include <stdexcept>
using namespace std;
template <typename T> class DynamicArray {
private:
    T* data;
    size_t size;

public:
    DynamicArray(size_t size) : size(size) {
        if (size == 0) throw invalid_argument("Size cannot be zero");
        try {
            data = new T[size];
        } catch (const bad_alloc& e) {
            throw runtime_error("Memory allocation failed in constructor");
        }
    }

    DynamicArray(const T* items, size_t count) : size(count) {
        try {
            data = new T[count];
            for (size_t i = 0; i < count; i++)
                data[i] = items[i];
        } catch (const bad_alloc& e) {
            throw runtime_error("Memory allocation failed in initializer");
        }
    }

    DynamicArray(const DynamicArray& other) : size(other.size) {
        try {
            data = new T[size];
            for (size_t i = 0; i < size; i++)
                data[i] = other.data[i];
        } catch (const bad_alloc& e) {
            throw runtime_error("Memory allocation failed in copy constructor");
        }
    }

    ~DynamicArray() {
        delete[] data;
    }

    T Get(size_t index) const {
        if (index >= size) throw out_of_range("Index out of range");
        return data[index];
    }

    void Set(size_t index, T value) {
        if (index >= size) throw out_of_range("Index out of range");
        data[index] = value;
    }

    size_t GetSize() const {
        return size;
    }

    void Resize(size_t newSize) {
        if (newSize == 0)
            throw invalid_argument("Resize size cannot be zero");
        try {
            T* newData = new T[newSize];
            for (size_t i = 0; i < (newSize < size ? newSize : size); i++)
                newData[i] = data[i];
            delete[] data;
            data = newData;
            size = newSize;
        } catch (const bad_alloc& e) {
            throw runtime_error("Memory allocation failed in Resize");
        }
    }

    DynamicArray operator+(const DynamicArray& other) const {
        if (size != other.size) throw invalid_argument("Size mismatch in addition");
        DynamicArray result(size);
        for (size_t i = 0; i < size; i++)
            result.data[i] = data[i] + other.data[i];
        return result;
    }

    DynamicArray operator*(T scalar) const {
        DynamicArray result(size);
        for (size_t i = 0; i < size; i++)
            result.data[i] = data[i] * scalar;
        return result;
    }

    T Dot(const DynamicArray& other) const {
        if (size != other.size) throw invalid_argument("Size mismatch in dot product");
        T result = T();
        for (size_t i = 0; i < size; i++)
            result += data[i] * other.data[i];
        return result;
    }

    double Norm() const {
        double sum = 0;
        for (size_t i = 0; i < size; i++)
            sum += static_cast<double>(data[i]) * data[i];
        return std::sqrt(sum);
    }
};