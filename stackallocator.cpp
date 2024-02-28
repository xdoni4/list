#include <iostream>



template<size_t N>
struct StackStorage {
    char buf[N];
    size_t b = 0; 
    
    StackStorage() {}
    StackStorage(const StackStorage &oth) = delete;
    StackStorage& operator=(const StackStorage &oth) = delete;
};

template<typename T, size_t N>
class StackAllocator {
public:
    StackStorage<N> *p;

public:
    using value_type = T;
    
    StackAllocator() = default;
    StackAllocator(StackStorage<N> &st) {
        p = &st;
    }

    template <typename U>
    StackAllocator(const StackAllocator<U, N> &oth) {
        p = oth.p;
    }
    StackAllocator& operator=(const StackAllocator<T, N> &oth) {
        p = oth.p;
        return *this;
    }
    ~StackAllocator() = default;
    
    template <typename U>
    struct rebind {
        typedef StackAllocator<U, N> other;
    };

    T* allocate(size_t n) {
        int offset = reinterpret_cast<uintptr_t>(p->buf+p->b) % sizeof(T); 
        p->b += n*sizeof(T) + (offset ? sizeof(T)-offset : 0);
        return reinterpret_cast<T*>(p->buf+(p->b-n*sizeof(T)));
    }

    void deallocate(T*, size_t) {return;}
    
    template <typename... Args>
    void construct(T* ptr, const Args&... args) {
        new(ptr) T(args...);
    }

    void destroy(T* ptr) {
        ptr->~T();
    } 
};


template<typename T, typename Allocator=std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode *next;
        BaseNode *prev;
        BaseNode(): next(this), prev(this) {}
        BaseNode(BaseNode *next, BaseNode *prev): next(next), prev(prev) {}
    };

    struct Node : public BaseNode {
        T key;

        Node(): BaseNode() {}
        Node(T val): BaseNode(), key(val) {}
        Node(BaseNode *next, BaseNode *prev): BaseNode(next, prev), key(T()) {}
        Node(BaseNode *next, BaseNode *prev, const T &val): BaseNode(next, prev), key(val) {}
    };

    BaseNode *fakeNode;
    BaseNode *head;
    size_t sz;

public:
    Allocator allocator;
    using AllocRebind =  typename Allocator::template rebind<Node>::other;
    using AllocTraits = std::allocator_traits<AllocRebind>;
    using AllocRebindBase =  typename Allocator::template rebind<BaseNode>::other;
    using AllocTraitsBase = std::allocator_traits<AllocRebindBase>;
    
    int szof() {
        return sizeof(Node);
    }

    List() {
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode);
        head = fakeNode;
        sz = 0;
    }

    List(int count) {
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode); 
        head = fakeNode;
        Node *tmp;
        try {
            for (int i = 0; i < count; i++) {
                tmp = AllocTraits::allocate(alloc, 1);
                AllocTraits::construct(alloc, tmp, fakeNode, head);
                head->next = tmp;
                head = tmp;
            }
            fakeNode->prev = head;
            sz = count;
        } catch(...) {
            AllocTraits::deallocate(alloc, tmp, 1);
        }
    }

    List(int count, const T &val) {
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode); 
        head = fakeNode;
        for (int i = 0; i < count; i++) {
            Node *tmp = AllocTraits::allocate(alloc, 1);
            AllocTraits::construct(alloc, tmp, fakeNode, head, val);
            head->next = tmp;
            head = tmp;
        }
        fakeNode->prev = head;
        sz = count;
    }

    List(Allocator a) {
        allocator = a;
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode);
        head = fakeNode;
        sz = 0;
    }

    List(int count, Allocator a) {
        allocator = a;
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode); 
        head = fakeNode;
        for (int i = 0; i < count; i++) {
            Node *tmp = AllocTraits::allocate(alloc, 1);
            AllocTraits::construct(alloc, tmp, fakeNode, head);
            head->next = tmp;
            head = tmp;
        }
        fakeNode->prev = head;
        sz = count;
    }

    List(int count, const T &val, Allocator a) {
        allocator = a;
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode); 
        head = fakeNode;
        for (int i = 0; i < count; i++) {
            Node *tmp = AllocTraits::allocate(alloc, 1);
            AllocTraits::construct(alloc, tmp, fakeNode, head, val);
            head->next = tmp;
            head = tmp;
        }
        fakeNode->prev = head;
        sz = count;
    }

    List(const List &other) {
        allocator = std::allocator_traits<Allocator>::select_on_container_copy_construction(other.allocator);
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode);        
        head = fakeNode;
        Node *tmp;
        try {
            for (BaseNode* p = other.fakeNode->next; p != other.fakeNode; p = p->next) {
                tmp = AllocTraits::allocate(alloc, 1);
                AllocTraits::construct(alloc, tmp, fakeNode, head, reinterpret_cast<Node*>(p)->key);
                head->next = tmp;
                head = tmp;
            }
            fakeNode->prev = head;
            sz = other.sz;
        } catch(...) {
            AllocTraits::deallocate(alloc, tmp, 1);
        }
    }

    List& operator=(const List &other) {
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        BaseNode *cur = fakeNode->next;
        AllocTraitsBase::destroy(allocBase, fakeNode);
        AllocTraitsBase::deallocate(allocBase, fakeNode, 1);
        for (size_t i = sz; i >= 1; i--) {
            BaseNode *tmp = cur->next;
            AllocTraits::destroy(alloc, reinterpret_cast<Node*>(cur));
            AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(cur), 1);
            cur = tmp;
        }
        if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) allocator = other.allocator;

        alloc = AllocRebind(allocator);
        allocBase = AllocRebindBase(allocator);
        fakeNode = AllocTraitsBase::allocate(allocBase, 1);
        AllocTraitsBase::construct(allocBase, fakeNode);        
        head = fakeNode;
        Node *tmp;
        try {
            for (BaseNode* p = other.fakeNode->next; p != other.fakeNode; p = p->next) {
                tmp = AllocTraits::allocate(alloc, 1);
                AllocTraits::construct(alloc, tmp, fakeNode, head, reinterpret_cast<Node*>(p)->key);
                head->next = tmp;
                head = tmp;
            }
            fakeNode->prev = head; 
            sz = other.sz; 
        } catch(...) {
            AllocTraits::deallocate(alloc, tmp, 1);
        }
        return *this;
    }

    ~List() {
        AllocRebind alloc(allocator);
        AllocRebindBase allocBase(allocator);
        for (; head != fakeNode; ) {
            BaseNode *tmp = head->prev;
            AllocTraits::destroy(alloc, reinterpret_cast<Node*>(head));
            AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(head), 1);
            head = tmp;
        }
        AllocTraitsBase::destroy(allocBase, fakeNode);
        AllocTraitsBase::deallocate(allocBase, fakeNode, 1);
    }

    Allocator get_allocator() const {
        return allocator; 
    }

    size_t size() const {
        return sz;
    }

    void push_back(const T &val) {
        AllocRebind alloc(allocator);
        Node *tmp = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, tmp, fakeNode, head, val);
        head->next = tmp; 
        head = tmp;
        fakeNode->prev = head;
        sz++;
    }

    void push_front(const T& val) {
        bool f = (head == fakeNode ? 1 : 0);
        AllocRebind alloc(allocator);
        Node *tmp = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, tmp, fakeNode->next, fakeNode, val);
        tmp->next->prev = tmp;
        fakeNode->next = tmp;
        if (f) head = tmp;
        sz++;
    }

    void pop_back() {
        AllocRebind alloc(allocator);
        BaseNode *tmp = head->prev;
        AllocTraits::destroy(alloc, reinterpret_cast<Node*>(head));
        AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(head), 1);
        head = tmp;
        head->next = fakeNode;
        fakeNode->prev = head;
        sz--;
    }

    void pop_front() {
        bool f = (head->prev == fakeNode ? 1 : 0);
        AllocRebind alloc(allocator);
        BaseNode *tmp = fakeNode->next->next;
        AllocTraits::destroy(alloc, reinterpret_cast<Node*>(fakeNode->next));
        AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(fakeNode->next), 1);
        fakeNode->next = tmp;
        fakeNode->next->prev = fakeNode;
        if (f) head = fakeNode;
        sz--;
    }

    template <typename PtrType, typename RefType>
    struct List_iterator {
        using Self = List_iterator; 
        using value_type = T;
        using pointer = PtrType;
        using reference = RefType;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using iterator = List_iterator<T*, T&>;
        using const_iterator = List_iterator<const T*, const T&>;

        BaseNode *ptr;
        
        List_iterator(BaseNode *p): ptr(p) {}
        List_iterator(const iterator& it): ptr(it.ptr) {}

        reference operator*() const {
            return reinterpret_cast<Node*>(ptr)->key;
        }

        pointer operator->() const {
            return &reinterpret_cast<Node*>(ptr)->key;
        }

        Self& operator++() {
            ptr = ptr->next;
            return *this;
        }

        Self& operator--() {
            ptr = ptr->prev;
            return *this;
        }

        Self operator++(int) {
            Self tmp = *this;
            ptr = ptr->next;
            return tmp;
        }

        Self operator--(int) {
            Self tmp = *this;
            ptr = ptr->prev;
            return tmp;
        }

        bool operator==(const Self& other) {
            return ptr == other.ptr;
        }

        bool operator!=(const Self& other) {
            return !(*this == other);
        }
    };

    using iterator = List_iterator<T*, T&>;
    using const_iterator = List_iterator<const T*, const T&>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return iterator(fakeNode->next);
    }

    iterator end() {
        return iterator(fakeNode);
    }

    const_iterator begin() const {
        return const_iterator(fakeNode->next);
    }

    const_iterator end() const {
        return const_iterator(fakeNode);
    }

    const_iterator cbegin() const {
        return const_iterator(fakeNode->next);
    }

    const_iterator cend() const {
        return const_iterator(fakeNode);
    }

    reverse_iterator rbegin() {
        return std::reverse_iterator(this->end());
    }

    reverse_iterator rend() {
        return std::reverse_iterator(this->begin());
    }

    const_reverse_iterator rbegin() const {
        return std::reverse_iterator(this->end());
    }

    const_reverse_iterator rend() const {
        return std::reverse_iterator(this->begin());
    }

    const_reverse_iterator crbegin() const {
        return std::reverse_iterator(this->cend());
    }

    const_reverse_iterator crend() const {
        return std::reverse_iterator(this->cbegin());
    }

    void insert(iterator it, const T &val) {
        bool f = (it == this->end() ? 1 : 0);
        AllocRebind alloc(allocator);
        BaseNode *tmp = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, reinterpret_cast<Node*>(tmp), it.ptr, it.ptr->prev, val);
        it.ptr->prev->next = tmp;
        it.ptr->prev = tmp;
        if (f) head = tmp;
        sz++;
    }

    void insert(const_iterator it, const T &val) {
        bool f = (it == this->cend() ? 1 : 0);
        AllocRebind alloc(allocator);
        BaseNode *tmp = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, reinterpret_cast<Node*>(tmp), it.ptr, it.ptr->prev, val);
        it.ptr->prev->next = tmp;
        it.ptr->prev = tmp;
        if (f) head = tmp;
        sz++;
    }

    void erase(iterator it) {
        bool f = (it.ptr == (this->end().ptr->prev) ? 1 : 0);
        AllocRebind alloc(allocator);
        BaseNode *prev = it.ptr->prev, *next = it.ptr->next;
        AllocTraits::destroy(alloc, reinterpret_cast<Node*>(it.ptr)); 
        AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(it.ptr), 1);
        prev->next = next;
        next->prev = prev;
        if (f) head = prev;
        sz--;
    }

    void erase(const_iterator it) {
        bool f = (it.ptr == (this->cend().ptr->prev) ? 1 : 0);
        AllocRebind alloc(allocator);
        BaseNode *prev = it.ptr->prev, *next = it.ptr->next;
        AllocTraits::destroy(alloc, reinterpret_cast<Node*>(it.ptr)); 
        AllocTraits::deallocate(alloc, reinterpret_cast<Node*>(it.ptr), 1);
        prev->next = next;
        next->prev = prev;
        if (f) head = prev;
        sz--;
    }
};
