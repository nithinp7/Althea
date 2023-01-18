#pragma once

#if defined(ALTHEA_SHARED)
#ifdef ALTHEABUILDING
#define ALTHEA_API __declspec(dllexport)
#else
#define ALTHEA_API __declspec(dllimport)
#endif
#else
#define ALTHEA_API
#endif