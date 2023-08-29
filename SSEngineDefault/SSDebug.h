#pragma once
#ifdef _DEBUG

#ifdef _WINDOWS
#include<assert.h>
#include<intrin.h>
#include<dxgidebug.h>

#endif


#include<stdio.h>
#include<typeinfo>
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console") 

#define SS_LOG(...) printf_s(__VA_ARGS__) 
#define WSS_LOG(...) wprintf(__VA_ARGS__)

#define __CLASS__ (typeid(*this).name())

#define SS_CLASS_ERR_LOG(STR, ...) printf_s("Error(%s:%s, Line: %d): "##STR, __CLASS__, __func__, __LINE__, __VA_ARGS__)



// ======================================================================================
#else
#define SS_LOG(n) 
#define WSS_LOG(n)



#endif