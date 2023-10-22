#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        ++count_;
        return count_;
    }
    size_t DecRef() {
        --count_;
        return count_;
    }
    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (RefCount() == 1) {
            Deleter::Destroy(static_cast<Derived*>(this));
        } else {
            counter_.DecRef();
        }
    }

    RefCounted& operator=(const RefCounted& other) {
        return *this;
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() {
        object_ = nullptr;
    }
    IntrusivePtr(std::nullptr_t) {
        object_ = nullptr;
    }
    IntrusivePtr(T* ptr) {
        object_ = ptr;
        if (object_) {
            object_->IncRef();
        }
    }
    template <class P>
    IntrusivePtr(P* ptr) {
        object_ = ptr;
        if (object_) {
            object_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) {
        //            Deleter();
        object_ = other.object_;
        if (object_) {
            object_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) {
        object_ = other.object_;
        other.object_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) {
        object_ = other.object_;
        if (object_) {
            object_->IncRef();
        }
    }
    IntrusivePtr(IntrusivePtr&& other) {
        object_ = other.object_;
        //            other.object_->DecRef();
        other.object_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (object_ == other.object_ || this == &other) {
            return *this;
        }
        if (object_) {
            Deleter();
        }
        object_ = other.object_;
        if (object_) {
            object_->IncRef();
        }
        return *this;
    }

    template <class P>
    IntrusivePtr& operator=(const IntrusivePtr<P>& other) {
        if (object_ == other.object_) {
            return *this;
        }
        if (object_) {
            Deleter();
        }
        object_ = other.object_;
        if (object_) {
            object_->IncRef();
        }
        return *this;
    }

    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (object_ == other.object_ || this == &other) {
            return *this;
        }
        if (object_) {
            Deleter();
        }
        object_ = other.object_;
        other.object_ = nullptr;
        return *this;
    }

    template <class P>
    IntrusivePtr& operator=(IntrusivePtr<P>&& other) {
        if (object_ == other.object_) {
            return *this;
        }
        if (object_) {
            Deleter();
        }
        object_ = other.object_;
        other.object_ = nullptr;
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        Deleter();
    }

    // Modifiers
    void Reset() {
        if (object_) {
            Deleter();
        }
        this->object_ = nullptr;
    }
    void Reset(T* ptr) {
        if (object_) {
            Deleter();
        }
        this->object_ = ptr;
        if (object_) {
            object_->IncRef();
        }
    }
    void Swap(IntrusivePtr& other) {
        std::swap(object_, other.object_);
    }

    // Observers
    T* Get() const {
        return object_;
    }
    T& operator*() const {
        return *object_;
    }
    T* operator->() const {
        return object_;
    }
    size_t UseCount() const {
        if (!object_) {
            return 0;
        }
        return object_->RefCount();
    }
    explicit operator bool() const {
        if (object_) {
            return true;
        }
        return false;
    }

private:
    T* object_;
    void Deleter() {
        if (object_) {
            object_->DecRef();
            //            if (object_->RefCount() == 0) {
            //                delete object_;
            //            }
        }
    }
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    return IntrusivePtr(new T(std::forward<Args>(args)...));
}
