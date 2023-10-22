#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <type_traits>

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr;

class ControlBlock {
public:
    virtual ~ControlBlock() = default;

    virtual int& GetStrongCounter() {
        return strong_counter_;
    }
    virtual int& GetWeakCounter() {
        return weak_counter_;
    }

protected:
    int strong_counter_ = 0;
    int weak_counter_ = 0;
};

template <typename T>
class ControlBlockForExistedObject : public ControlBlock {
public:
    ControlBlockForExistedObject() {
        strong_counter_ = 1;
    }

    ControlBlockForExistedObject(T* ptr) {
        object_ = ptr;
        strong_counter_ = 1;
    }

    int& GetStrongCounter() override {
        return strong_counter_;
    }
    int& GetWeakCounter() override {
        return weak_counter_;
    }
    T* GetObject() {
        return object_;
    }
    ~ControlBlockForExistedObject() override {
        delete object_;
    }

private:
    T* object_;
};

template <typename T>
class ControlBlockForNewObject : public ControlBlock {
public:
    ControlBlockForNewObject() {
        strong_counter_ = 1;
        weak_counter_ = 0;
    };
    template <typename... Args>
    ControlBlockForNewObject(Args&&... args) {
        strong_counter_ = 1;
        ::new (&memory_block_) T(std::forward<Args>(args)...);
    }
    T* GetObject() {
        return reinterpret_cast<T*>(&memory_block_);
    }
    int& GetStrongCounter() override {
        return strong_counter_;
    }
    int& GetWeakCounter() override {
        return weak_counter_;
    }
    ~ControlBlockForNewObject() {
        if (GetObject() != nullptr) {
            GetObject()->~T();
        }
    }

private:
    //    void* memory_piece_ = ::operator new(sizeof(T));
    std::aligned_storage_t<sizeof(T), alignof(T)> memory_block_;
};

template <typename T>
class SharedPtr {
public:
    template <typename Pointer>
    friend class SharedPtr;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
        block_ = nullptr;
        pointer_ = nullptr;
    }

    SharedPtr(std::nullptr_t) {
        block_ = nullptr;
        pointer_ = nullptr;
    }

    explicit SharedPtr(T* ptr) {
        pointer_ = ptr;
        block_ = new ControlBlockForExistedObject<T>(ptr);
    }
    template <class Pointer>
    explicit SharedPtr(Pointer* ptr) {
        pointer_ = ptr;
        block_ = new ControlBlockForExistedObject<Pointer>(ptr);
    }

    SharedPtr(ControlBlockForNewObject<T>* block) {
        block_ = block;
        pointer_ = block->GetObject();
    }

    template <class Pointer>
    SharedPtr(ControlBlockForNewObject<Pointer>* block) {
        block_ = block;
        pointer_ = block->GetObject();
        ++block_->GetStrongCounter();
    }

    SharedPtr(const SharedPtr& other) {
        pointer_ = other.pointer_;
        block_ = other.block_;
        if (other.block_ != nullptr) {
            ++block_->GetStrongCounter();
        }
    }

    template <class Pointer>
    SharedPtr(const SharedPtr<Pointer>& other) {
        pointer_ = other.pointer_;
        block_ = other.block_;
        if (other.block_ != nullptr) {
            ++block_->GetStrongCounter();
        }
    }

    SharedPtr(SharedPtr&& other) {
        pointer_ = other.pointer_;
        block_ = std::move(other.block_);
        other.block_ = nullptr;
        other.pointer_ = nullptr;
    }

    template <class Pointer>
    SharedPtr(SharedPtr<Pointer>&& other) {
        pointer_ = other.pointer_;
        block_ = std::move(other.block_);
        other.block_ = nullptr;
        other.pointer_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Pointer>
    SharedPtr(const SharedPtr<Pointer>& other, T* ptr) {
        this->pointer_ = ptr;
        if (block_) {
            --this->block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0) {
                delete block_;
            }
        }
        this->block_ = other.block_;
        ++block_->GetStrongCounter();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0) {
                delete block_;
            }
        }
        block_ = other.block_;
        pointer_ = other.pointer_;
        if (block_ != nullptr) {
            ++block_->GetStrongCounter();
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0) {
                delete block_;
            }
        }
        block_ = std::move(other.block_);
        pointer_ = other.pointer_;
        other.block_ = nullptr;
        other.pointer_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_) {
            if (block_->GetStrongCounter() == 1) {
                --block_->GetStrongCounter();
                delete block_;
            } else {
                --block_->GetStrongCounter();
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            --this->block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0) {
                delete block_;
            }
        }
        pointer_ = nullptr;
        block_ = nullptr;
    }
    void Reset(T* ptr) {
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0) {
                delete block_;
            }
        }
        this->pointer_ = ptr;
        block_ = new ControlBlockForExistedObject(ptr);
    }
    template <class Pointer>
    void Reset(Pointer* ptr) {
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0) {
                delete block_;
            }
        }
        this->pointer_ = ptr;
        block_ = new ControlBlockForExistedObject(ptr);
    }
    void Swap(SharedPtr& other) {
        std::swap(this->pointer_, other.pointer_);
        std::swap(this->block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pointer_;
    }
    T& operator*() const {
        return *pointer_;
    }
    T* operator->() const {
        return pointer_;
    }
    size_t UseCount() const {
        if (!block_) {
            return 0;
        }
        return block_->GetStrongCounter();
    }
    explicit operator bool() const {
        if (block_) {
            return true;
        }
        return false;
    }

private:
    T* pointer_;
    ControlBlock* block_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    return SharedPtr<T>(new ControlBlockForNewObject<T>(std::forward<Args>(args)...));
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
