#pragma once
#ifdef _WINDOWS

typedef long long int			int64;
typedef int						int32;
typedef short					int16;
typedef char					int8;

typedef unsigned long long		uint64;
typedef unsigned int			uint32;
typedef unsigned short			uint16;
typedef unsigned char			uint8;

constexpr uint32 SS_UINT32_MAX = 0xffffffff;

#define FORCEINLINE __forceinline
#define PATH_LEN_MAX 260

#endif


#define SS_DECLARE_AS_SINGLETON(CLASS_NAME)				\
private:												\
	static CLASS_NAME* g_instance;						\
public:													\
	static FORCEINLINE CLASS_NAME* Get() {				\
		assert(g_instance != nullptr);					\
		return g_instance;								\
	}													\
	static FORCEINLINE void Release() {					\
		delete g_instance;								\
		g_instance = nullptr;							\
	}													\
private:												