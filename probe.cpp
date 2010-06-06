#include "common.h"

enum egbb_colors {
	_WHITE,_BLACK
};
enum egbb_occupancy {
	_EMPTY,_WKING,_WQUEEN,_WROOK,_WBISHOP,_WKNIGHT,_WPAWN,
    _BKING,_BQUEEN,_BROOK,_BBISHOP,_BKNIGHT,_BPAWN
};
enum egbb_load_types {
	LOAD_NONE,LOAD_4MEN,SMART_LOAD,LOAD_5MEN
};

#define _NOTFOUND 99999
#define MAX_PIECES 9

typedef int (CDECL *PPROBE_EGBB) (int player, int* piece,int* square);
typedef void (CDECL *PLOAD_EGBB) (char* path,int cache_size,int load_options);
static PPROBE_EGBB probe_egbb;
POPEN_EGBB open_egbb;

int SEARCHER::egbb_is_loaded;
int SEARCHER::egbb_load_type = LOAD_4MEN;

/*
Load the dll and get the address of the load and probe functions.
*/

#ifdef _MSC_VER
#	ifdef ARC_64BIT
#		define EGBB_NAME "egbbdll64.dll"
#	else
#		define EGBB_NAME "egbbdll.dll"
#	endif
#else
#	ifdef ARC_64BIT
#		define EGBB_NAME "egbbso64.so"
#	else
#		define EGBB_NAME "egbbso.so"
#	endif
#endif

#ifndef _MSC_VER
#    define HMODULE void*
#    define LoadLibrary(x) dlopen(x,RTLD_LAZY)
#    define GetProcAddress dlsym
#endif

int LoadEgbbLibrary(char* main_path,int egbb_cache_size) {
	HMODULE hmod;
	PLOAD_EGBB load_egbb;
	char path[256];
	char terminator;
	size_t plen = strlen(main_path);
	strcpy(path,main_path);
	if (plen) {
		terminator = main_path[strlen(main_path)-1];
		if (terminator != '/' && terminator != '\\') {
			if (strchr(path, '\\') != NULL)
				strcat(path, "\\");
			else
				strcat(path, "/");
		}
	}
	strcat(path,EGBB_NAME);
	if((hmod = LoadLibrary(path)) != 0) {
		load_egbb = (PLOAD_EGBB) GetProcAddress(hmod,"load_egbb_xmen");
     	probe_egbb = (PPROBE_EGBB) GetProcAddress(hmod,"probe_egbb_xmen");
		open_egbb = (POPEN_EGBB)  GetProcAddress(hmod,"open_egbb");
        load_egbb(main_path,egbb_cache_size,SEARCHER::egbb_load_type);
		return true;
	} else {
		printf("EgbbProbe not Loaded!\n");
		return false;
	}
}
/*
Probe:
Change interanal scorpio board representaion to [A1 = 0 ... H8 = 63]
board representation and then probe bitbase.
*/

int SEARCHER::probe_bitbases(int& score) {
	
	register PLIST current;
	int piece[MAX_PIECES],square[MAX_PIECES],count = 0;

#define ADD_PIECE(list,type) {					\
	   current = list;							\
	   while(current) {							\
	      piece[count] = type;					\
		  square[count] = SQ8864(current->sq);	\
		  current = current->next;				\
		  count++;								\
	   }										\
	};
	ADD_PIECE(plist[wking],_WKING);
	ADD_PIECE(plist[bking],_BKING);
	ADD_PIECE(plist[wpawn],_WPAWN);
	ADD_PIECE(plist[bpawn],_BPAWN);
	ADD_PIECE(plist[wqueen],_WQUEEN);
	ADD_PIECE(plist[bqueen],_BQUEEN);
	ADD_PIECE(plist[wrook],_WROOK);
	ADD_PIECE(plist[wbishop],_WBISHOP);
	ADD_PIECE(plist[brook],_BROOK);
	ADD_PIECE(plist[bbishop],_BBISHOP);
	ADD_PIECE(plist[wknight],_WKNIGHT);
	ADD_PIECE(plist[bknight],_BKNIGHT);
	piece[count] = _EMPTY;
	square[count] = -1;

	score = probe_egbb(player,piece,square);
	
    return (score != _NOTFOUND);
}
