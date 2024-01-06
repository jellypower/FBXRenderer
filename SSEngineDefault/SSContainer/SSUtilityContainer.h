#pragma once
#pragma warning(disable: 4996)


#include <string.h>
#include "SSEngineDefault/SSNativeKeywords.h"

#include <string>

namespace SS {
	template<typename T1, typename T2>
	class pair
	{
	public:
		T1 first;
		T2 second;
		pair(T1 InFirst, T2 InSecond) : first(InFirst), second(InSecond) { }
	};

	template<uint32 STR_LEN_MAX>
	class FixedStringA {
	private:
		char _dataPool[STR_LEN_MAX];
		uint32 _len;

	public:
		FixedStringA();
		FixedStringA(const char* data);
		FixedStringA(const FixedStringA& rhs);
		
		const char* GetData() const { return _dataPool; }
		FORCEINLINE constexpr uint32 GetCapacity() const { return STR_LEN_MAX; }
		FORCEINLINE const uint32 GetLen() const { return _len; }

		void Assign(const char* data);
		void Assign(const char* data, uint32 len);
		void Clear();
		
	};
#pragma region FixedString implementation
	template<uint32 STR_LEN_MAX>
	FixedStringA<STR_LEN_MAX>::FixedStringA()
	{
		_dataPool[0] = '\0';
		_len = 0;
	}

	template<uint32 STR_LEN_MAX>
	FixedStringA<STR_LEN_MAX>::FixedStringA(const char* data)
	{
		_len = strlen(data);
		strcpy(_dataPool, data);
	}

	template<uint32 STR_LEN_MAX>
	FixedStringA<STR_LEN_MAX>::FixedStringA(const FixedStringA& rhs)
	{
		_len = rhs._len;
		strcpy(_dataPool, rhs._dataPool);
	}

	template<uint32 STR_LEN_MAX>
	void FixedStringA<STR_LEN_MAX>::Assign(const char* data)
	{
		uint32 len = strlen(data);
		assert(len < STR_LEN_MAX);
		_len = len;
		strcpy(_dataPool, data);
	}

	template<uint32 STR_LEN_MAX>
	void FixedStringA<STR_LEN_MAX>::Assign(const char* data, uint32 len)
	{
		assert(len < STR_LEN_MAX);
		_len = len;
		strncpy(_dataPool, data, len);
		_dataPool[len] = '\0';
	}

	template<uint32 STR_LEN_MAX>
	void FixedStringA<STR_LEN_MAX>::Clear()
	{
		_dataPool[0] = '\0';
		_len = 0;
	}
#pragma endregion


};