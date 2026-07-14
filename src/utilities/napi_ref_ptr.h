#pragma once
#include <memory>
#include <cassert>
#include <atomic>
#include <concepts>
#include <js_native_api.h>
#include <node-addon-api/napi.h>

/**
 * Add this class as a public base class to your Addon class. It allows napi_ref_ptr<T> to automatically manage 
 * itself during environment teardown.
 */
class NapiRefTeardownIndicator {
public:
    NapiRefTeardownIndicator(Napi::Env env):
        teardownFlag(std::make_shared<std::atomic<bool>>(false))
    {
        env.AddCleanupHook([teardownFlag = teardownFlag]() { 
            teardownFlag->store(true, std::memory_order_release);
        });
    }

    std::shared_ptr<std::atomic<bool>> teardownFlag;
};

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

    napi_ref_ptr(const napi_ref_ptr& ptr)
    {
        assignValue(ptr.get());
    }
    
    napi_ref_ptr(napi_ref_ptr&& other) noexcept: 
        value(other.value),
        teardownFlag(std::move(other.teardownFlag))
    {
        other.value = nullptr;
    }

    ~napi_ref_ptr() {
        release();
    }
    
    napi_ref_ptr& operator=(const napi_ref_ptr& other) {
        if (this != &other) {
            assignValue(other.get());
        }
        return *this;
    }
    
    napi_ref_ptr& operator=(napi_ref_ptr&& other) noexcept {
        if (this != &other) {
            release();
            value = other.value;
            teardownFlag = std::move(other.teardownFlag);
            other.value = nullptr;
        }
        return *this;
    }

    napi_ref_ptr& operator=(T* ptr) {
        assignValue(ptr);
        return *this;
    }

    T* get() const {
        if (value && isTearingDown())
            return nullptr;
        return value;
    }

    T* operator->() const {
        assert(value && !isTearingDown());
        return get();
    }

    explicit operator bool() const {
        return get() != nullptr;
    }

    bool operator>(const napi_ref_ptr<T>& other) const {
        return value > other.value; // intentional use of raw value() instead of get() to ensure std::set semantics
    }

    bool operator<(const napi_ref_ptr<T>& other) const {
        return value < other.value; // intentional use of raw value() instead of get() to ensure std::set semantics
    }

    template <std::derived_from<T> U>
    napi_ref_ptr<U> cast() {
        return napi_ref_ptr<U> { static_cast<U*>(get()) };
    }

private:
    T* value = nullptr;

    /**
     * True if the current environment is being torn down (ie cleanup hooks have fired).
     * In this state, v8 is going to be freeing objects with no consideration of references and dependency graph.
     * We need to make sure we do not Unref() an object which was freed (causing use-after-free), and we also 
     * can no longer trust the raw pointer we hold to be valid. 
     * 
     * Since the environment's instance data (ie typically the "Addon" class for a node-api-addon C++ addon) may 
     * *also* have been deleted, we use a std::shared_ptr<std::atomic<bool>> that is allocated by the addon (by way 
     * of deriving from NapiRefTeardownIndicator), which will survive deletion of the Addon object by way of this 
     * object holding a reference to it. 
     * 
     * Thus, when we are destructing, or being dereferenced, but environment cleanup has begun, we can safely avoid 
     * calling Unref(), and we will also return `nullptr` for all dereferences to ensure that we never perform a 
     * use-after-free via a napi_ref_ptr.get() or napi_ref_ptr->... operation, but instead hit a clean null dereference
     * crash.
     */
    std::shared_ptr<std::atomic<bool>> teardownFlag = nullptr;

    [[nodiscard]] bool isTearingDown() const {
        if (teardownFlag)
            return teardownFlag->load(std::memory_order_acquire);
        return false;
    }

    void assignValue(T* ptr) {
        if (value == ptr)
            return;

        release();
        value = ptr;

        if (value) {
            value->Ref();

            auto* indicator = value->Env().template GetInstanceData<NapiRefTeardownIndicator>();
            if (indicator)
                teardownFlag = indicator->teardownFlag;
        }
    }
    
    void release() {
        if (value) {
            if (!isTearingDown() && !value->IsEmpty()) {
                // TODO(liam): This was written this way to avoid crashes when node-addon-api attempted to throw a
                // javascript exception during v8 teardown. Now that we handle teardown using a cleanup hook, there's 
                // really no risk of this, so we could probably move back to value->Unref(), without even requiring a 
                // "catch" here, since catch/ignoring these errors was originally introduced to handle teardown anyway.
                napi_ref ref = *value;
                uint32_t current_count = 0; // unused
                napi_status status = napi_reference_unref(value->Env(), ref, &current_count);
                // we ignore errors on status here intentionally
            }
            value = nullptr;
        }

        teardownFlag = nullptr;
    }
};