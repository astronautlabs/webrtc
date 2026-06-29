#pragma once
#include <concepts>
#include <node-addon-api/napi.h>

template <class T>
class napi_ref_ptr {
public:
    napi_ref_ptr() :
        napi_ref_ptr(nullptr)
    {
    }

    napi_ref_ptr(T* ptr) {
        assignValue(ptr);
    }

    napi_ref_ptr(const napi_ref_ptr& ptr): 
        napi_ref_ptr(ptr.get()) 
    {
    }
    
    napi_ref_ptr(napi_ref_ptr&& other) noexcept: 
        value(other.value) 
    {
        other.value = nullptr;
    }

    ~napi_ref_ptr() {
        release();
    }
    
    napi_ref_ptr& operator=(const napi_ref_ptr& other) {
        if (this != &other) {
            assignValue(other.value);
        }
        return *this;
    }
    
    napi_ref_ptr& operator=(napi_ref_ptr&& other) noexcept {
        if (this != &other) {
            release();
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }

    napi_ref_ptr& operator=(T* ptr) {
        assignValue(ptr);
        return *this;
    }

    T* get() const {
        return value;
    }

    T* operator->() const {
        return value;
    }

    explicit operator bool() const {
        return !!value;
    }

    bool operator>(const napi_ref_ptr<T>& other) const {
        return get() > other.get();
    }

    bool operator<(const napi_ref_ptr<T>& other) const {
        return get() < other.get();
    }

    template <std::derived_from<T> U>
    napi_ref_ptr<U> cast() {
        return napi_ref_ptr<U> { static_cast<U*>(get()) };
    }

private:
    T* value = nullptr;
    
    void assignValue(T* ptr) {
        if (value == ptr)
            return;

        release();
        value = ptr;

        if (value)
            value->Ref();
    }
    
    void release() {
        if (value) {
            try {
                value->Unref();
            } catch (const Napi::Error& e) {
                // If we are inside a Finalize() callback or Node is shutting down,
                // the N-API reference is already dead. Calling Unref() throws.
                // We must catch and swallow it to prevent std::terminate.
            }
            value = nullptr;
        }
    }
};