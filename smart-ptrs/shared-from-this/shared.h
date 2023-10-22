#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <type_traits>

// https://en.cppreference.com/w/cpp/memory/shared_ptr
class EnableSharedFromThisBase {};

template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    template <typename Pointer>
    friend class SharedPtr;

    SharedPtr<T> SharedFromThis() {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            return SharedPtr<T>(weak_ptr_);
        }
    }
    SharedPtr<const T> SharedFromThis() const {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            return SharedPtr<T>(weak_ptr_);
        }
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_ptr_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_ptr_;
    }

protected:
    WeakPtr<T> weak_ptr_;
};

template <typename T>
class SharedPtr {
public:
    template <typename Pointer>
    friend class SharedPtr;

    template <typename Pointer>
    friend class WeakPtr;

    template <class P>
    friend class EnableSharedFromThis;
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
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitializeWeak(ptr);
        }
    }

    template <typename Y>
    void InitializeWeak(EnableSharedFromThis<Y>* enable_shated_object) {
        enable_shated_object->weak_ptr_ = *this;
    }

    template <class Pointer>
    explicit SharedPtr(Pointer* ptr) {
        pointer_ = ptr;
        block_ = new ControlBlockForExistedObject<Pointer>(ptr);
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitializeWeak(ptr);
        }
    }

    SharedPtr(ControlBlockForNewObject<T>* block) {
        block_ = block;
        pointer_ = block->GetObject();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitializeWeak(pointer_);
        }
    }

    template <class Pointer>
    SharedPtr(ControlBlockForNewObject<Pointer>* block) {
        block_ = block;
        pointer_ = block->GetObject();
        ++block_->GetStrongCounter();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitializeWeak(pointer_);
        }
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
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
            }
        }
        this->block_ = other.block_;
        ++block_->GetStrongCounter();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        this->block_ = other.block_weak_;
        this->pointer_ = other.object_;
        ++this->block_->GetStrongCounter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
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
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
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
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
            }
        }
        pointer_ = nullptr;
        block_ = nullptr;
    }
    void Reset(T* ptr) {
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
            }
        }
        this->pointer_ = ptr;
        block_ = new ControlBlockForExistedObject(ptr);
    }
    template <class Pointer>
    void Reset(Pointer* ptr) {
        if (block_) {
            --block_->GetStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            } else if (block_->GetStrongCounter() == 0) {
                block_->DeleteObject();
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
