#pragma once

#include "compressed_pair.h"
#include "deleters.h"

#include <cstddef>  // std::nullptr_t
#include <algorithm>
#include <iostream>

template <typename Object>
class DefaultDeleter {
public:
    DefaultDeleter() noexcept = default;
    ~DefaultDeleter() = default;

    void operator()(Object* object) noexcept {
        std::cout << "void operator()(Object* object) DefaultDeleter" << object << '\n';
        delete object;
    }

    template <typename ObjectOther>
    DefaultDeleter(DefaultDeleter<ObjectOther>&& deleter) noexcept {
    }
};

template <typename Object>
class DefaultDeleter<Object[]> {
public:
    DefaultDeleter() noexcept = default;

    ~DefaultDeleter() = default;

    void operator()(Object* object) noexcept {
        std::cout << "void operator()(Object* object) DefaultDeleter<Object[]>" << object << '\n';
        delete[] object;
    }

    template <typename ObjectOther>
    DefaultDeleter(DefaultDeleter<ObjectOther>&& deleter) noexcept {
    }
};

// Primary template
template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    template <typename U, typename D>
    friend class UniquePtr;

    explicit UniquePtr(T* ptr = nullptr) noexcept {
        std::cout << "explicit UniquePtr(T* ptr = nullptr) " << this << '\n';
        object_block_.GetFirst() = ptr;
    }

    UniquePtr(T* ptr, Deleter deleter) noexcept {
        std::cout << "UniquePtr(T* ptr, Deleter deleter) noexcept" << this << '\n';
        object_block_.GetFirst() = ptr;
        object_block_.GetSecond() = std::forward<Deleter>(deleter);
    }

    UniquePtr(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept {
        std::cout << "UniquePtr(UniquePtr&& other) noexcept" << this << '\n';
        object_block_.GetFirst() = std::forward<T*>(other.Release());
        object_block_.GetSecond() = std::forward<Deleter>(other.GetDeleter());
    }
    template <class Type, class CustomDeleter>
    UniquePtr(UniquePtr<Type, CustomDeleter>&& other) noexcept {
        object_block_.GetFirst() = std::forward<Type*>(other.Release());
        object_block_.GetSecond() = std::forward<CustomDeleter>(other.GetDeleter());
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(std::nullptr_t) noexcept {
        std::cout << "UniquePtr& operator=(std::nullptr_t)" << '\n';
        T* object_saved = object_block_.GetFirst();
        object_block_.GetFirst() = nullptr;
        if (object_saved) {
            GetDeleter()(object_saved);
        }
        return *this;
    }

    template <class Type, class CustomDeleter>
    UniquePtr& operator=(UniquePtr<Type, CustomDeleter>&& other) noexcept {
        if (this->object_block_.GetFirst() == other.object_block_.GetFirst()) {
            return *this;
        }
        T* object_copy = this->object_block_.GetFirst();
        GetDeleter()(object_copy);
        this->object_block_.GetFirst() = std::forward<Type*>(other.Release());
        this->object_block_.GetSecond() =
            std::forward<CustomDeleter>(other.object_block_.GetSecond());
        return *this;
    }

    UniquePtr& operator=(const UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        std::cout << "~UniquePtr() " << this << '\n';
        if (object_block_.GetFirst() != nullptr) {
            GetDeleter()(object_block_.GetFirst());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* object = object_block_.GetFirst();
        this->object_block_.GetFirst() = nullptr;
        return object;
    }

    void Reset(T* ptr = nullptr) {
        T* object_saved = object_block_.GetFirst();
        object_block_.GetFirst() = ptr;
        if (object_saved != nullptr) {
            GetDeleter()(object_saved);
        }
    };

    void Swap(UniquePtr& other) {
        std::swap(this->object_block_.GetFirst(), other.object_block_.GetFirst());
        std::swap(this->object_block_.GetSecond(), other.object_block_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        if (object_block_.GetFirst()) {
            return object_block_.GetFirst();
        }
        return nullptr;
    }
    Deleter& GetDeleter() {
        return object_block_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return object_block_.GetSecond();
    }
    explicit operator bool() const {
        if (object_block_.GetFirst() != nullptr) {
            return true;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators
    typename std::add_lvalue_reference<T>::type operator*() const {
        return *(object_block_.GetFirst());
    }

    T* operator->() const {
        return object_block_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> object_block_;
};

template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    template <typename U, typename D>
    friend class UniquePtr;
    explicit UniquePtr(T* ptr = nullptr) noexcept {
        std::cout << "explicit UniquePtr(T* ptr = nullptr) " << this << '\n';
        object_block_.GetFirst() = ptr;
    }

    T& operator[](std::size_t index) const {
        return *(object_block_.GetFirst() + index);
    }

    void Reset(std::nullptr_t = nullptr) noexcept {
        T* object_saved = object_block_.GetFirst();
        object_block_.GetFirst() = nullptr;
        if (object_saved != nullptr) {
            GetDeleter()(object_saved);
        }
    }

    template <class S>
    void Reset(S* ptr) noexcept {
        T* object_saved = object_block_.GetFirst();
        object_block_.GetFirst() = ptr;
        if (object_saved != nullptr) {
            GetDeleter()(object_saved);
        }
    };

    Deleter& GetDeleter() {
        return object_block_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return object_block_.GetSecond();
    }

    ~UniquePtr() {
        if (object_block_.GetFirst() != nullptr) {
            GetDeleter()(object_block_.GetFirst());
        }
    }

private:
    CompressedPair<T*, Deleter> object_block_;
};
