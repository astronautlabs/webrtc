#pragma once
#include <concepts>
#include <js_native_api.h>
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
        // We have to be careful here, because during teardown our object may have been destroyed while there were 
        // still outstanding napi_ref_ptrs. If we use the C++ Unref() function, it will try to throw an error which 
        // is not valid during destructors nor during V8 teardown. 
        // Previously we used an empty try/catch here but that won't help if any V8 allocations need to happen to 
        // construct the error being thrown. Much better to avoid the exception entirely.

        if (value) {
            if (!value->IsEmpty()) {
                napi_ref ref = *value;
                uint32_t current_count = 0; // unused
                napi_status status = napi_reference_unref(value->Env(), ref, &current_count);
                // we ignore errors on status here intentionally
            }
            value = nullptr;
        }
    }
};