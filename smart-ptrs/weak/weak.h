#pragma once

#include "sw_fwd.h"  // Forward declaration

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    template <class Pointer>
    friend class SharedPtr;

    WeakPtr() {
        object_ = nullptr;
        block_weak_ = nullptr;
    }

    WeakPtr(const WeakPtr<T>& other) {
        this->block_weak_ = other.block_weak_;
        this->object_ = other.object_;
        if (block_weak_ != nullptr) {
            ++block_weak_->GetWeakCounter();
        }
    }

    template <class Pointer>
    WeakPtr(const WeakPtr<Pointer>& other) {
        this->block_weak_ = other.block_weak_;
        this->object_ = other.object_;
        if (block_weak_ != nullptr) {
            ++block_weak_->GetWeakCounter();
        }
    }

    WeakPtr(WeakPtr<T>&& other) {
        this->block_weak_ = other.block_weak_;
        this->object_ = other.object_;
        other.block_weak_ = nullptr;
        other.object_ = nullptr;
    }

    template <class Pointer>
    WeakPtr(WeakPtr<Pointer>&& other) {
        this->block_weak_ = other.block_weak_;
        this->object_ = other.object_;
        other.block_weak_ = nullptr;
        other.object_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        this->block_weak_ = other.block_;
        this->object_ = other.pointer_;
        ++block_weak_->GetWeakCounter();
    }

    template <class Pointer>
    WeakPtr(const SharedPtr<Pointer>& other) {
        this->block_weak_ = other.block_;
        this->object_ = other.pointer_;
        ++block_weak_->GetWeakCounter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        Deleter();
        if (other.block_weak_ != nullptr) {
            ++other.block_weak_->GetWeakCounter();
        }
        this->block_weak_ = other.block_weak_;
        this->object_ = other.object_;
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        Deleter();
        this->block_weak_ = other.block_weak_;
        this->object_ = other.object_;
        other.block_weak_ = nullptr;
        other.object_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Deleter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        Deleter();
        block_weak_ = nullptr;
        object_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(this->block_weak_, other.block_weak_);
        std::swap(this->object_, other.object_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (!block_weak_) {
            return 0;
        }
        return block_weak_->GetStrongCounter();
    }

    bool Expired() const {
        if (!UseCount()) {
            return true;
        }
        if (block_weak_->GetStrongCounter() == 0) {
            return true;
        }
        return false;
    }

    SharedPtr<T> Lock() const {
        if (Expired()) {
            return SharedPtr<T>();
        } else {
            return SharedPtr<T>(*this);
        }
    }

private:
    ControlBlock* block_weak_;
    T* object_;
    void Deleter() {
        if (block_weak_) {
            --this->block_weak_->GetWeakCounter();
            if (block_weak_->GetWeakCounter() == 0 && block_weak_->GetStrongCounter() == 0) {
                delete block_weak_;
            } else if (block_weak_->GetWeakCounter() == 0 && block_weak_->GetStrongCounter() > 0) {
                // like a shared_ptr, don't delete while strong = 0;
            } else if (block_weak_->GetWeakCounter() > 0 && block_weak_->GetStrongCounter() == 0) {
                delete object_;
            }
        }
    }
};
