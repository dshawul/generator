#include "common.h"

/*external variables*/
const int pawn_dir[2] = {UU,DD};
const int col_tab[15] = {neutral,white,white,white,white,white,white,
black,black,black,black,black,black,neutral};
const int pic_tab[15] = {empty,king,queen,rook,bishop,knight,pawn,
king,queen,rook,bishop,knight,pawn,elephant};

/*
generate non-captures
*/
#define NK_NONCAP(dir) {										\
		to = from + dir;										\
		if(board[to] == empty)									\
			*pmove++ = tmove | (to<<8);							\
};
#define BRQ_NONCAP(dir) {										\
	    to = from + dir;										\
		while(board[to] == empty) {								\
			*pmove++ = tmove | (to<<8);							\
			to += dir;											\
		}														\
};

void SEARCHER::gen_noncaps() {
	int* pmove = &pstack->move_st[pstack->count],*spmove = pmove,tmove;
	int  from,to;
	PLIST current;
	
	if(player == white) {

		/*knight*/
		current = plist[wknight];
		while(current) {
			from = current->sq;
			tmove = from | (wknight<<16);
			NK_NONCAP(RRU);
			NK_NONCAP(LLD);
			NK_NONCAP(RUU);
			NK_NONCAP(LDD);
			NK_NONCAP(LLU);
			NK_NONCAP(RRD);
			NK_NONCAP(RDD);
			NK_NONCAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			from = current->sq;
			tmove = from | (wbishop<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			from = current->sq;
			tmove = from | (wrook<<16);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			from = current->sq;
			tmove = from | (wqueen<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*king*/
		from = plist[wking]->sq;
		tmove = from | (wking<<16);
		NK_NONCAP(RU);
		NK_NONCAP(LD);
		NK_NONCAP(LU);
		NK_NONCAP(RD);
		NK_NONCAP(UU);
		NK_NONCAP(DD);
		NK_NONCAP(RR);
		NK_NONCAP(LL);
		
		/*pawn*/
		current = plist[wpawn];
		while(current) {
			from = current->sq;
			to = from + UU;
			if(board[to] == empty) {
				if(rank(to) != RANK8) {
					*pmove++ = from | (to<<8) | (wpawn<<16);
					
					if(rank(from) == RANK2) {
						to += UU;
						if(board[to] == empty)
							*pmove++ = from | (to<<8) | (wpawn<<16);
					}
				}
			}	
			current = current->next;
		}
	} else {

		/*knight*/
		current = plist[bknight];
		while(current) {
			from = current->sq;
			tmove = from | (bknight<<16);
			NK_NONCAP(RRU);
			NK_NONCAP(LLD);
			NK_NONCAP(RUU);
			NK_NONCAP(LDD);
			NK_NONCAP(LLU);
			NK_NONCAP(RRD);
			NK_NONCAP(RDD);
			NK_NONCAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			from = current->sq;
			tmove = from | (bbishop<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			from = current->sq;
			tmove = from | (brook<<16);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			from = current->sq;
			tmove = from | (bqueen<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		
		/*king*/
		from = plist[bking]->sq;
		tmove = from | (bking<<16);
		NK_NONCAP(RU);
		NK_NONCAP(LD);
		NK_NONCAP(LU);
		NK_NONCAP(RD);
		NK_NONCAP(UU);
		NK_NONCAP(DD);
		NK_NONCAP(RR);
		NK_NONCAP(LL);
		
		/*pawn*/
		current = plist[bpawn];
		while(current) {
			from = current->sq;
			to = from + DD;
			if(board[to] == empty) {
				if(rank(to) != RANK1) {
					*pmove++ = from | (to<<8) | (bpawn<<16);
					
					if(rank(from) == RANK7) {
						to += DD;
						if(board[to] == empty)
							*pmove++ = from | (to<<8) | (bpawn<<16);
					}
				}
			}	
			current = current->next;
		}
	}
	/*count*/
	pstack->count += (pmove - spmove);
}
/*
generate all
*/
#define NK_MOVES(dir) {										            \
		to = from + dir;										        \
		if(board[to] == empty)									        \
			*pmove++ = tmove | (to<<8);							        \
		else if(COLOR(board[to]) == opponent)							\
			*pmove++ = tmove | (to<<8) | (board[to]<<20);				\
};
#define BRQ_MOVES(dir) {												\
	    to = from + dir;												\
		while(board[to] == empty) {										\
			*pmove++ = tmove | (to<<8);									\
			to += dir;													\
		}																\
		if(COLOR(board[to]) == opponent)								\
			*pmove++ = tmove | (to<<8) | (board[to]<<20);				\
};

void SEARCHER::gen_all() {
	int* pmove = &pstack->move_st[pstack->count],*spmove = pmove,tmove;
	int  from,to;
	PLIST current;
	
	if(player == white) {

		/*knight*/
		current = plist[wknight];
		while(current) {
			from = current->sq;
			tmove = from | (wknight<<16);
			NK_MOVES(RRU);
			NK_MOVES(LLD);
			NK_MOVES(RUU);
			NK_MOVES(LDD);
			NK_MOVES(LLU);
			NK_MOVES(RRD);
			NK_MOVES(RDD);
			NK_MOVES(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			from = current->sq;
			tmove = from | (wbishop<<16);
			BRQ_MOVES(RU);
			BRQ_MOVES(LD);
			BRQ_MOVES(LU);
			BRQ_MOVES(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			from = current->sq;
			tmove = from | (wrook<<16);
			BRQ_MOVES(UU);
			BRQ_MOVES(DD);
			BRQ_MOVES(RR);
			BRQ_MOVES(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			from = current->sq;
			tmove = from | (wqueen<<16);
			BRQ_MOVES(RU);
			BRQ_MOVES(LD);
			BRQ_MOVES(LU);
			BRQ_MOVES(RD);
			BRQ_MOVES(UU);
			BRQ_MOVES(DD);
			BRQ_MOVES(RR);
			BRQ_MOVES(LL);
			current = current->next;
		}
		/*king*/
		from = plist[wking]->sq;
		tmove = from | (wking<<16);
		NK_MOVES(RU);
		NK_MOVES(LD);
		NK_MOVES(LU);
		NK_MOVES(RD);
		NK_MOVES(UU);
		NK_MOVES(DD);
		NK_MOVES(RR);
		NK_MOVES(LL);
		
		/*pawn*/
		current = plist[wpawn];
		while(current) {
			from = current->sq;
			//caps
			to = from + RU;
			if(COLOR(board[to]) == black) {
				if(rank(to) == RANK8) {
					tmove = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (wqueen<<24);
					*pmove++ = tmove | (wknight<<24);
					*pmove++ = tmove | (wrook<<24);
					*pmove++ = tmove | (wbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
				}
			}
			to = from + LU;
			if(COLOR(board[to]) == black) {
				if(rank(to) == RANK8) {
					tmove = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (wqueen<<24);
					*pmove++ = tmove | (wknight<<24);
					*pmove++ = tmove | (wrook<<24);
					*pmove++ = tmove | (wbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
				}
			}
			//noncaps
			to = from + UU;
			if(board[to] == empty) {
				if(rank(to) == RANK8) {
					if(board[to] == empty) {
						tmove = from | (to<<8) | (wpawn<<16);
						*pmove++ = tmove | (wqueen<<24);
						*pmove++ = tmove | (wknight<<24);
						*pmove++ = tmove | (wrook<<24);
						*pmove++ = tmove | (wbishop<<24);
					}
				} else {
					*pmove++ = from | (to<<8) | (wpawn<<16);
					if(rank(from) == RANK2) {
						to += UU;
						if(board[to] == empty)
							*pmove++ = from | (to<<8) | (wpawn<<16);
					}
				}
			}	
			current = current->next;
		}
		/*enpassant*/
		if(epsquare) {
			from = epsquare + LD;
			if(board[from] == wpawn)
				*pmove++ = from | (epsquare<<8) | (wpawn<<16) | (bpawn<<20) | EP_FLAG;
			
			from = epsquare + RD;
			if(board[from] == wpawn)
				*pmove++ = from | (epsquare<<8) | (wpawn<<16) | (bpawn<<20) | EP_FLAG;
		}
		/*end*/
	} else {

		/*knight*/
		current = plist[bknight];
		while(current) {
			from = current->sq;
			tmove = from | (bknight<<16);
			NK_MOVES(RRU);
			NK_MOVES(LLD);
			NK_MOVES(RUU);
			NK_MOVES(LDD);
			NK_MOVES(LLU);
			NK_MOVES(RRD);
			NK_MOVES(RDD);
			NK_MOVES(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			from = current->sq;
			tmove = from | (bbishop<<16);
			BRQ_MOVES(RU);
			BRQ_MOVES(LD);
			BRQ_MOVES(LU);
			BRQ_MOVES(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			from = current->sq;
			tmove = from | (brook<<16);
			BRQ_MOVES(UU);
			BRQ_MOVES(DD);
			BRQ_MOVES(RR);
			BRQ_MOVES(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			from = current->sq;
			tmove = from | (bqueen<<16);
			BRQ_MOVES(RU);
			BRQ_MOVES(LD);
			BRQ_MOVES(LU);
			BRQ_MOVES(RD);
			BRQ_MOVES(UU);
			BRQ_MOVES(DD);
			BRQ_MOVES(RR);
			BRQ_MOVES(LL);
			current = current->next;
		}
		
		/*king*/
		from = plist[bking]->sq;
		tmove = from | (bking<<16);
		NK_MOVES(RU);
		NK_MOVES(LD);
		NK_MOVES(LU);
		NK_MOVES(RD);
		NK_MOVES(UU);
		NK_MOVES(DD);
		NK_MOVES(RR);
		NK_MOVES(LL);
		
		/*pawn*/
		current = plist[bpawn];
		while(current) {
			from = current->sq;
			//caps
			to = from + LD;
			if(COLOR(board[to]) == white) {
				if(rank(to) == RANK1) {
					tmove = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (bqueen<<24);
					*pmove++ = tmove | (bknight<<24);
					*pmove++ = tmove | (brook<<24);
					*pmove++ = tmove | (bbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
				}
			}
			to = from + RD;
			if(COLOR(board[to]) == white) {
				if(rank(to) == RANK1) {
					tmove = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (bqueen<<24);
					*pmove++ = tmove | (bknight<<24);
					*pmove++ = tmove | (brook<<24);
					*pmove++ = tmove | (bbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
				}
			}
			//noncaps
			to = from + DD;
			if(board[to] == empty) {
				if(rank(to) == RANK1) {
                    tmove = from | (to<<8) | (bpawn<<16);
					*pmove++ = tmove | (bqueen<<24);
					*pmove++ = tmove | (bknight<<24);
					*pmove++ = tmove | (brook<<24);
					*pmove++ = tmove | (bbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (bpawn<<16);
					if(rank(from) == RANK7) {
						to += DD;
						if(board[to] == empty)
							*pmove++ = from | (to<<8) | (bpawn<<16);
					}
				}
			}	
			current = current->next;
		}
		/*enpassant*/
		if(epsquare) {
			from = epsquare + RU;
			if(board[from] == bpawn)
				*pmove++ = from | (epsquare<<8) | (bpawn<<16) | (wpawn<<20) | EP_FLAG;
			
			from = epsquare + LU;
			if(board[from] == bpawn)
				*pmove++ = from | (epsquare<<8) | (bpawn<<16) | (wpawn<<20) | EP_FLAG;
		}
		/*end*/
	}
	/*count*/
	pstack->count += int(pmove - spmove);
}
/*
generate retro moves
*/
void SEARCHER::gen_retro() {
	int* pmove = &pstack->move_st[pstack->count],*spmove = pmove;
	int  from,to,tmove;
	PLIST current;
	
	if(player == white) {
		/*knight*/
		current = plist[wknight];
		while(current) {
			from = current->sq;
			tmove = from | (wknight<<16);
			NK_NONCAP(RRU);
			NK_NONCAP(LLD);
			NK_NONCAP(RUU);
			NK_NONCAP(LDD);
			NK_NONCAP(LLU);
			NK_NONCAP(RRD);
			NK_NONCAP(RDD);
			NK_NONCAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			from = current->sq;
			tmove = from | (wbishop<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			from = current->sq;
			tmove = from | (wrook<<16);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			from = current->sq;
			tmove = from | (wqueen<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*king*/
		from = plist[wking]->sq;
		tmove = from | (wking<<16);
		NK_NONCAP(RU);
		NK_NONCAP(LD);
		NK_NONCAP(LU);
		NK_NONCAP(RD);
		NK_NONCAP(UU);
		NK_NONCAP(DD);
		NK_NONCAP(RR);
		NK_NONCAP(LL);

		/*pawn*/
		current = plist[wpawn];
		while(current) {
			from = current->sq;
			to = from + DD;
			if(rank(to) != RANK1 && board[to] == empty) {
				*pmove++ = from | (to<<8) | (wpawn<<16);
				
				if(rank(from) == RANK4) {
					to += DD;
					if(board[to] == empty)
						*pmove++ = from | (to<<8) | (wpawn<<16);
				}
			}	
			current = current->next;
		}
	} else {
		/*knight*/
		current = plist[bknight];
		while(current) {
			from = current->sq;
			tmove = from | (bknight<<16);
			NK_NONCAP(RRU);
			NK_NONCAP(LLD);
			NK_NONCAP(RUU);
			NK_NONCAP(LDD);
			NK_NONCAP(LLU);
			NK_NONCAP(RRD);
			NK_NONCAP(RDD);
			NK_NONCAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			from = current->sq;
			tmove = from | (bbishop<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			from = current->sq;
			tmove = from | (brook<<16);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			from = current->sq;
			tmove = from | (bqueen<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		
		/*king*/
		from = plist[bking]->sq;
		tmove = from | (bking<<16);
		NK_NONCAP(RU);
		NK_NONCAP(LD);
		NK_NONCAP(LU);
		NK_NONCAP(RD);
		NK_NONCAP(UU);
		NK_NONCAP(DD);
		NK_NONCAP(RR);
		NK_NONCAP(LL);

		/*pawn*/
		current = plist[bpawn];
		while(current) {
			from = current->sq;
			to = from + UU;
			if(rank(to) != RANK8 && board[to] == empty) {
				*pmove++ = from | (to<<8) | (bpawn<<16);
				
				if(rank(from) == RANK5) {
					to += UU;
					if(board[to] == empty)
						*pmove++ = from | (to<<8) | (bpawn<<16);
				}
			}	
			current = current->next;
		}
	}
	/*count*/
	pstack->count += (pmove - spmove);
}
/*constructor*/
SEARCHER::SEARCHER() : board(&temp_board[48])
{
	int sq;
	for (sq = 0;sq < 128; sq++) {
		list[sq] = new LIST;
	}
	for(sq = 0;sq < 48;sq++)
		temp_board[sq] = elephant;
    for(sq = 176;sq < 224;sq++)
		temp_board[sq] = elephant;
	for(sq = A1;sq < A1 + 128;sq++) {
		if(sq & 0x88)
           board[sq] = elephant;
	}
}

/*init data*/
void SEARCHER::init_data() {
	register int i,sq,pic;

 	ply = 0;
	pstack = stack + 0;

	for(i = wking;i < elephant;i++) {
       plist[i] = 0;
	}

	for(sq = A1;sq <= H8;sq++) {
		if(!(sq & 0x88)) { 
			list[sq]->sq = sq;
			list[sq]->prev = 0;
			list[sq]->next = 0;
			pic = board[sq];
			if(pic != empty) {
				pcAdd(pic,sq);
			}
		}
	}
}

/*set pos*/
void SEARCHER::set_pos(int count,int side,int* piece,int* square) {

    register int i,sq;
	for(sq = A1;sq <= H8;sq++) {
		if(!(sq & 0x88)) {
			board[sq] = empty;
		} else {
			sq += 0x07;
		}
	}

	for(i = 0;i < count;i++) {
		if(piece[i]) 
			board[SQ6488(square[i])] = piece[i];
	}

	player = side;
	opponent = invert(side);
	epsquare = 0;
	init_data();
}
/*attacks*/
static const UBMP8 t_sqatt_pieces[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,
  6,  0,  0,  0,  0,  0,  0, 10,  0,  0, 10,  0,  0,  0,  0,  0,
  6,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0, 10,  0,  0,  0,  0,
  6,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,
  6,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,
  6,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 16,
  6, 16, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 75,
  7, 75, 16,  0,  0,  0,  0,  0,  0,  6,  6,  6,  6,  6,  6,  7,
  0,  7,  6,  6,  6,  6,  6,  6,  0,  0,  0,  0,  0,  0, 16, 43,
  7, 43, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 16,
  6, 16, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,
  6,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,
  6,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,
  6,  0,  0,  0,  0, 10,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,
  6,  0,  0,  0,  0,  0, 10,  0,  0, 10,  0,  0,  0,  0,  0,  0,
  6,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0
};

static const BMP8 t_sqatt_step[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,-17,  0,  0,  0,  0,  0,  0,
-16,  0,  0,  0,  0,  0,  0,-15,  0,  0,-17,  0,  0,  0,  0,  0,
-16,  0,  0,  0,  0,  0,-15,  0,  0,  0,  0,-17,  0,  0,  0,  0,
-16,  0,  0,  0,  0,-15,  0,  0,  0,  0,  0,  0,-17,  0,  0,  0,
-16,  0,  0,  0,-15,  0,  0,  0,  0,  0,  0,  0,  0,-17,  0,  0,
-16,  0,  0,-15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-17,  0,
-16,  0,-15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-17,
-16,-15,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1,
  0,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0, 15,
 16, 17,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,
 16,  0, 17,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0,
 16,  0,  0, 17,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,
 16,  0,  0,  0, 17,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,
 16,  0,  0,  0,  0, 17,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,
 16,  0,  0,  0,  0,  0, 17,  0,  0, 15,  0,  0,  0,  0,  0,  0,
 16,  0,  0,  0,  0,  0,  0, 17,  0,  0,  0,  0,  0,  0,  0,  0
};

const UBMP8* const _sqatt_pieces = t_sqatt_pieces + 0x80;
const BMP8* const _sqatt_step = t_sqatt_step + 0x80;

/*any blocking piece in between?*/
int SEARCHER::blocked(int from, int to) const {
	register int step,sq;
	if(step = sqatt_step(to - from)) {
		sq = from + step;
		while(board[sq] == empty && (sq != to)) sq += step;
		return (sq != to);
	}
	return true;
};

/*is square attacked by color?*/
int SEARCHER::attacks(int col,int sq) const {
    register PLIST current;
	
	if(col == white) {
		/*pawn*/
		if(board[sq + LD] == wpawn) return true;
        if(board[sq + RD] == wpawn) return true;
		/*knight*/
		current = plist[wknight];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & NM)
				return true;
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & BM)
				if(!blocked(current->sq,sq))
					return true;
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & RM)
				if(!blocked(current->sq,sq))
					return true;
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & QM)
				if(!blocked(current->sq,sq))
					return true;
			current = current->next;
		}
		/*king*/
		if(sqatt_pieces(sq - plist[wking]->sq) & KM)
			return true;
	} else if(col == black) {
		/*pawn*/
		if(board[sq + RU] == bpawn) return true;
        if(board[sq + LU] == bpawn) return true;
		/*knight*/
		current = plist[bknight];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & NM)
				return true;
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & BM)
				if(!blocked(current->sq,sq))
					return true;
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & RM)
				if(!blocked(current->sq,sq))
					return true;
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			if(sqatt_pieces(sq - current->sq) & QM)
				if(!blocked(current->sq,sq))
					return true;
			current = current->next;
		}
		/*king*/
		if(sqatt_pieces(sq - plist[bking]->sq) & KM)
			return true;
	}
	return false;
}
/*
print board
*/
static const char rank_name[] = "12345678";
static const char file_name[] = "abcdefgh";

void sq_str(const int& sq,char* s) {
	*s++ = file_name[file(sq)];
	*s++ = rank_name[rank(sq)];
	*s = 0;
}
void print_sq(const int& sq) {
	char f[6];
	sq_str(sq,f);
	printf("%s",f);
}
void print_pc(const int& pc) {
	printf("%c",piece_name[pc]);
}
void print_move(const int& move) {
	char f[6],t[6];
	sq_str(m_from(move),f);
	sq_str(m_to(move),t);
	print("%s %s %c %c %c ep = %d",f,t,piece_name[m_piece(move)],
		piece_name[m_capture(move)],piece_name[m_promote(move)],is_ep(move));
}
void SEARCHER::print_board() {
	int i , j;
	print("\n");
	for(i = 7; i >= 0; i--) {
        print("\t");
		for(j = 0;j < 8; j++) {
			print("%c",piece_name[board[SQ(i,j)]]);
		}
		print("\n");
	}
#ifdef	_DEBUG
	print("play = %d opp = %d ep = %d cas = %d fif = %d\n",player,opponent,epsquare,castle,fifty);
    PLIST current;
	char  str[4];
	for(i = wking;i <= bpawn;i++) {
		current = plist[i];
		print_pc(i);
		print(":");
		while(current) {
			sq_str(current->sq,str);
			print("%5s",str);
			current = current->next;
		}
		print("\n");
	}
#endif
}
