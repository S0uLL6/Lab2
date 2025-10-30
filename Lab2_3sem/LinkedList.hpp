using namespace std;

template <typename T> struct Node {
    T data;
    Node<T>* next;
    Node(T data, Node<T>* next = nullptr) : data(data), next(next) {}
};

template <typename T> class LinkedList {
private:
    Node<T>* head;
    size_t length;

public:
    LinkedList() : head(nullptr), length(0) {}

    LinkedList(const T* items, size_t count) : head(nullptr), length(0) {
        for (size_t i = 0; i < count; i++)
            Append(items[i]);
    }

    LinkedList(const LinkedList<T>& other) : head(nullptr), length(0) {
        Node<T>* current = other.head;
        while (current) {
            Append(current->data);
            current = current->next;
        }
    }

    ~LinkedList() {
        Node<T>* current = head;
        while (current) {
            Node<T>* next = current->next;
            delete current;
            current = next;
        }
    }

    T GetFirst() const {
        if (!head) throw out_of_range("List is empty");
        return head->data;
    }

    T GetLast() const {
        if (!head) throw out_of_range("List is empty");
        Node<T>* temp = head;
        while (temp->next) temp = temp->next;
        return temp->data;
    }

    T Get(size_t index) const {
        if (index >= length) throw out_of_range("Index out of range");
        Node<T>* temp = head;
        for (size_t i = 0; i < index; i++) temp = temp->next;
        return temp->data;
    }

    LinkedList<T>* GetSubList(size_t start, size_t end) const {
        if (start > end || end >= length) throw out_of_range("Invalid sublist indices");
        LinkedList<T>* sublist = new LinkedList<T>();
        Node<T>* temp = head;
        for (size_t i = 0; i <= end; i++) {
            if (i >= start) sublist->Append(temp->data);
            temp = temp->next;
        }
        return sublist;
    }

    size_t GetLength() const {
        return length;
    }

    void Append(T item) {
        if (!head) {
            head = new Node<T>(item);
        } else {
            Node<T>* temp = head;
            while (temp->next) temp = temp->next;
            temp->next = new Node<T>(item);
        }
        length++;
    }

    void Prepend(T item) {
        head = new Node<T>(item, head);
        length++;
    }

    void InsertAt(T item, size_t index) {
        if (index > length) throw out_of_range("Index out of range");
        if (index == 0) {
            Prepend(item);
            return;
        }
        Node<T>* temp = head;
        for (size_t i = 0; i < index - 1; i++) temp = temp->next;
        temp->next = new Node<T>(item, temp->next);
        length++;
    }

    LinkedList<T>* Concat(LinkedList<T>* list) const {
        LinkedList<T>* result = new LinkedList<T>(*this);
        Node<T>* temp = list->head;
        while (temp) {
            result->Append(temp->data);
            temp = temp->next;
        }
        return result;
    }
};
