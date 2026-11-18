#pragma once
#define AnnoPluginAbi __cdecl
#ifdef  ANNO5EXTENDER_EXPORTS
#define AnnoDll __declspec(dllexport)
#else 
#define AnnoDll __declspec(dllimport)
#endif //  ANNO5EXTENDER_EXPORTS
