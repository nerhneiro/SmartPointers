#pragma once

#pragma once

#include <type_traits>
#include <memory>
#include <utility>

template <class T, bool empty = std::is_empty_v<T> && !std::is_final_v<T>>
class FirstElement {
public:
    FirstElement() = default;
    FirstElement(T&& value) : value_(std::move(value)) {
    }
    FirstElement(T& value) : value_(value) {
    }
    const T& GetValue() const {
        return value_;
    }
    T& GetValue() {
        return value_;
    }

private:
    T value_;
};

template <typename T>
class FirstElement<T, true> : public T {
public:
    FirstElement() = default;
    FirstElement(T&& value) : T(std::move(value)) {
    }
    FirstElement(T& value) : T(value) {
    }
    const T& GetValue() const {
        return *this;
    }
    T& GetValue() {
        return *this;
    }
};

template <class T, bool full = std::is_empty_v<T> && !std::is_final_v<T>>
class SecondElement {
public:
    SecondElement() = default;
    SecondElement(T&& value) : value_(std::move(value)) {
    }
    SecondElement(T& value) : value_(value) {
    }
    const T& GetValue() const {
        return value_;
    }
    T& GetValue() {
        return value_;
    }

private:
    T value_;
};

template <typename T>
class SecondElement<T, true> : public T {
public:
    SecondElement() = default;
    SecondElement(T&& value) : T(std::move(value)) {
    }
    SecondElement(T& value) : T(value) {
    }
    const T& GetValue() const {
        return *this;
    }
    T& GetValue() {
        return *this;
    }
};

template <typename F, typename S>
class CompressedPair : public FirstElement<F>, public SecondElement<S> {
public:
    CompressedPair(F&& first, S& second)
        : FirstElement<F>(std::move(first)), SecondElement<S>(second) {
    }

    CompressedPair(F& first, S&& second)
        : FirstElement<F>(first), SecondElement<S>(std::move(second)) {
    }

    CompressedPair(F&& first, S&& second)
        : FirstElement<F>(std::move(first)), SecondElement<S>(std::move(second)) {
    }

    CompressedPair(F& first, S& second)
        : FirstElement<F>(first), SecondElement<S>(std::move(second)) {
    }

    CompressedPair() : FirstElement<F>(), SecondElement<S>() {
    }

    F& GetFirst() {
        return FirstElement<F>::GetValue();
    }

    const F& GetFirst() const {
        return FirstElement<F>::GetValue();
    }

    S& GetSecond() {
        return SecondElement<S>::GetValue();
    }

    const S& GetSecond() const {
        return SecondElement<S>::GetValue();
    };
};
