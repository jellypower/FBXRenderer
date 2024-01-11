#pragma once
#include <cassert>

#include "SSEngineDefault/SSNativeKeywords.h"
#include "SSUtilityContainer.h"

namespace SS
{
	template<typename T>
	class PooledList {

	private:
		T* _data;
		uint32 _capacity = 0;
		uint32 _size = 0;

	public:
		PooledList(uint32 capacity);
		~PooledList();

		void PushBack(T&& newData);
		void Resize(uint32 newSize);
		void Clear();
		void IncreaseCapacityAndCopy(uint32 newCapacity);

		FORCEINLINE uint32 GetSize() const { return _size; }
		FORCEINLINE uint32 GetCapacity() const { return _capacity; }

		FORCEINLINE T& operator[](const uint32 idx);

	public:
		class iterator {
			friend class PooledList<T>;
		private:
			uint32 _idx;
			PooledList<T>& _list;

		public:
			iterator& operator++() {
				_idx++;
				return *this;
			}
			iterator& operator--() {
				_idx++;
				return *this;
			}
			bool operator==(const iterator rhs) { return _idx == rhs._idx; }
			bool operator!=(const iterator rhs) { return _idx != rhs._idx; }
			T& operator*() { return _list._data[_idx]; }

		private:
			iterator(uint32 idx, PooledList<T>& list) : _idx(idx), _list(list) { }
		};
		FORCEINLINE iterator begin() { return iterator(0, *this); }
		FORCEINLINE iterator end() { return iterator(_size, *this); }
	};

	template <typename T>
	PooledList<T>::PooledList(uint32 capacity)
	{
		_size = 0;
		if (capacity != 0) {
			_capacity = capacity;
			_data = (T*)DBG_MALLOC(sizeof(T) * _capacity);
		}
		else
		{
			_capacity = 0;
			_data = nullptr;
		}
	}

	template <typename T>
	PooledList<T>::~PooledList()
	{
		for (uint32 i = 0; i < _size; i++)
		{
			_data[i].~T();
		}
		free(_data);
	}

	template <typename T>
	void PooledList<T>::PushBack(T&& newData)
	{
		assert(_size < _capacity);
		new(_data + _size) T(SS::forward<T>(newData));
		_size++;
	}

	template <typename T>
	void PooledList<T>::Resize(uint32 newSize)
	{
		assert(newSize <= _capacity);

		for (uint32 i = _size; i < newSize; i++)
			new(_data + _size) T();

		for (uint32 i = newSize; i < _size; i++)
			_data[i].~T();

		_size = newSize;
	}

	template <typename T>
	void PooledList<T>::Clear()
	{
		for (uint32 i = 0; i < _size; i++) {
			_data[i].~T();
		}
		_size = 0;
	}

	template <typename T>
	void PooledList<T>::IncreaseCapacityAndCopy(uint32 newCapacity)
	{
		T* newData = (T*)malloc(sizeof(T) * newCapacity);
		memcpy(newData, _data, sizeof(T) * _capacity);
		free(_data);
		_data = newData;
		_capacity = newCapacity;
	}

	template <typename T>
	T& PooledList<T>::operator[](const uint32 idx)
	{
		assert(idx < _capacity);
		return _data[idx];
	}
}
