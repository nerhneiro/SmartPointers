#pragma once

#include <exception>

class BadWeakPtr : public std::exception {};

class ControlBlock {
public:
    virtual ~ControlBlock() = default;

    virtual int& GetStrongCounter() {
        return strong_counter_;
    }
    virtual int& GetWeakCounter() {
        return weak_counter_;
    }
    virtual void DeleteObject() {
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
    void DeleteObject() override {
        delete object_;
    }
    ~ControlBlockForExistedObject() override {
        //        delete object_;
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
        ::new (&memory_block_) T();
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
    void DeleteObject() override {
        if (GetObject() != nullptr) {
            GetObject()->~T();
        }
    }
    ~ControlBlockForNewObject() {
        //        if (GetObject() != nullptr) {
        //            GetObject()->~T();
        //        }
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> memory_block_;
};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;
