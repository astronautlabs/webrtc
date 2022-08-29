#pragma once
#include <node-addon-api/napi.h>

template<class T>
class napi_ref_ptr {
public:
	napi_ref_ptr() {
		value = nullptr;
		ref = Napi::Reference<Napi::Value>();
	}

	napi_ref_ptr(napi_ref_ptr&) = delete;
	napi_ref_ptr(T* ptr) {
		assignValue(ptr);
	}

	~napi_ref_ptr() {
		assignValue(nullptr);
	}

	T *get() {
		return value;
	}

	napi_ref_ptr operator=(T* ptr) {
		assignValue(ptr);
		return napi_ref_ptr(ptr);
	}

	T *operator->() {
		return value;
	}

	operator T*() {
		return value;
	}

private:
	T* value = nullptr;
	Napi::Reference<Napi::Value> ref = Napi::Reference<Napi::Value>();

	void assignValue(T* ptr) {
		if (value == ptr)
			return;

		if (value)
			ref.Unref();
		
		value = ptr;

		if (ptr)
			ref = Napi::Reference<Napi::Value>::New(ptr->Value(), 1);
		else
			ref = Napi::Reference<Napi::Value>();
	}
};