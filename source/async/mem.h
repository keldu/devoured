#pragma once

#include <memory>

namespace dvr {
	template<typename T>
	class Own {
	private:
		std::unique_ptr<T> ptr;
	public:
		Own(std::unique_ptr<T>&& p);
		T& operator*();

		Own<T> clone();
	};

	/*
	 * build helper for Own
	 */
	template<typename T, class... Args>
	Own<T> makeOwn(Args&&... args);
	
	template<typename T>
	class Our {
	private:
		std::shared_ptr<T> ptr;
	public:
		Our(std::shared_ptr<T>&& p);

		T& operator*();

		size_t ownerAmount() const;

		Our<T> share();
	};
	
	/*
	 *
	 */
	template<typename T, class... Args>
	Our<T> makeOur(Args&&... args);

	/*
	 * Socialize this capitalistic wretched system of ownership and let's desconstruct the foundation
	 * of our nation. Jk
	 * This is for the async structure of Conveyors, since they can be forked and this may require the sharing of
	 * objects falling into that fork. On a join it may be required or recommendable to "privatize()" the Our class
	 */
	template<typename T>
	Our<T> socialize(Own<T>&& ptr);

	/*
	 * Privatize will only work if the amount of owning parties is at most 1
	 */
	template<typename T>
	Own<T> privatize(Our<T>&& ptr);

	/*
	 * Inline
	 */
	template<typename T>
	Own<T>::Own(std::unique_ptr<T>&& p):
		ptr{std::move(p)}
	{}

	template<typename T>
	T& Own<T>::operator*(){
		return *ptr;
	}

	template<typename T>
	Own<T> Own<T>::clone(){
		return Own<T>{ptr?std::make_unique<T>(*ptr):nullptr};
	}

	template<typename T, class... Args>
	Own<T> makeOwn(Args&&... args){
		return Own<T>{std::make_unique<T>(args...)};
	}

	template<typename T>
	Our<T>::Our(std::shared_ptr<T>&& p):
		ptr{std::move(p)}
	{}

	template<typename T>
	T& Our<T>::operator*(){
		return *ptr;
	}
	
	template<typename T>
	size_t Our<T>::ownerAmount()const{
		return ptr.use_count();
	}

	template<typename T>
	Our<T> Our<T>::share(){
		return ptr;
	}

	template<typename T, class... Args>
	Our<T> makeOur(Args&&... args){
		return Our<T>{std::make_shared<T>(args...)};
	}
}
