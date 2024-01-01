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

#define SS_CLASS_ERR_LOG(STR, ...) printf_s("Error(%s:%s, Line: %d): "##STR "\n", __CLASS__, __func__, __LINE__, __VA_ARGS__); __debugbreak();
#define WSS_CLASS_ERR_LOG(STR, ...) printf_s("Error(%s:%s, Line: %d): "##STR "\n", __CLASS__, __func__, __LINE__, __VA_ARGS__); __debugbreak();
#define SS_CLASS_WARNING_LOG(STR, ...) printf_s("Warning(%s:%s, Line: %d): "##STR "\n", __CLASS__, __func__, __LINE__, __VA_ARGS__)

#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#define DBG_NEW new (_NORMAL_BLOCK , __FILE__ , __LINE__)
#define DBG_MALLOC(s) _malloc_dbg(s,_NORMAL_BLOCK,__FILE__,__LINE__)
#include <stdlib.h>



// ======================================================================================
#else
#define SS_LOG(...) 
#define WSS_LOG(...)
#define __CLASS__ 
#define SS_CLASS_ERR_LOG(...)
#define SS_CLASS_WARNING_LOG(...)

#define WSS_CLASS_ERR_LOG(...)

#define DBG_NEW new
#define DBG_MALLOC(s) malloc(s)

#endif