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
	Object() {}

	template<class T>
	Object(T value) : impl_(new ObjectImpl<typename TypeWrapper<T>::TYPE>(value)) {}

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

private:
	struct AbstractObjectImpl
	{
		virtual ~AbstractObjectImpl() {}
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
