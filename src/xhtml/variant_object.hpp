// Adapted from http://stackoverflow.com/questions/5319216/implementing-a-variant-class
// Used under license http://creativecommons.org/licenses/by-sa/3.0/
// Attributed to: http://stackoverflow.com/users/75889/stackedcrooked

#pragma once

#include <memory>

template <typename T>
struct TypeWrapper
{
	typedef T TYPE;
	typedef const T CONSTTYPE;
	typedef T& REFTYPE;
	typedef const T& CONSTREFTYPE;
};

template <typename T>
struct TypeWrapper<const T>
{
	typedef T TYPE;
	typedef const T CONSTTYPE;
	typedef T& REFTYPE;
	typedef const T& CONSTREFTYPE;
};

template <typename T>
struct TypeWrapper<const T&>
{
	typedef T TYPE;
	typedef const T CONSTTYPE;
	typedef T& REFTYPE;
	typedef const T& CONSTREFTYPE;
};

template <typename T>
struct TypeWrapper<T&>
{
	typedef T TYPE;
	typedef const T CONSTTYPE;
	typedef T& REFTYPE;
	typedef const T& CONSTREFTYPE;
};

class Object
{
public:
	Object() : inherit_(false) {}

	template<class T>
	explicit Object(T value) : inherit_(false), impl_(new ObjectImpl<typename TypeWrapper<T>::TYPE>(value)) {}

	template<>
	explicit Object(bool in) : inherit_(in) {}

	template<class T> typename TypeWrapper<T>::REFTYPE getValue() 
	{ 
		return dynamic_cast<ObjectImpl<typename TypeWrapper<T>::TYPE>&>(*impl_.get()).value_;
	}

	template<class T>
	typename TypeWrapper<T>::CONSTREFTYPE getValue() const
	{
		return dynamic_cast<ObjectImpl<typename TypeWrapper<T>::TYPE>&>(*impl_.get()).value_;
	}

	template<class T>
	void setValue(typename TypeWrapper<T>::CONSTREFTYPE value)
	{
		impl_.reset(new ObjectImpl<typename TypeWrapper<T>::TYPE>(value));
	}

	void setImportant(bool importance=true) {
		impl_->important_ = importance;
	}

	void setInherit(bool inherit=true) {
		inherit_ = inherit;
	}

	bool isImportant() const { return impl_ != nullptr ? impl_->important_ : false; }
	bool empty() const { return impl_ == nullptr; }
	bool shouldInherit() const { return inherit_; }
private:
	bool inherit_;
	struct AbstractObjectImpl
	{
		AbstractObjectImpl() : important_(false) {}
		virtual ~AbstractObjectImpl() {}
		bool important_;
	};

	template<class T>
	struct ObjectImpl : public AbstractObjectImpl
	{
		ObjectImpl(T value) : value_(value) { }
		~ObjectImpl() {}
		T value_;
	};

	std::shared_ptr<AbstractObjectImpl> impl_;
};

