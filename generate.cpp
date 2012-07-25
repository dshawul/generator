#include "common.h"
#include <cmath>
#include <ctime>
#include <omp.h>

/*
* Store/get mechanisms
*/
static const int VALUE[4] = {
	DRAW, WIN, LOSS, ILLEGAL
};

/*store*/
void SET(MYINT i,int value,UBMP8* ptab) {
	UBMP8* v;
	MYINT q,r;
	int myval;

	q = (i / 4);
	r = (i % 4);
	v = &ptab[q];
	myval = int(*v & ~(3 << (r << 1)));

	switch(value) {
	   case WIN:
		   myval |= (1 << (r << 1));
		   break;
	   case LOSS:
		   myval |= (2 << (r << 1));
		   break;
	   case DRAW:
		   break;
	   case ILLEGAL:
		   myval |= (3 << (r << 1));
		   break;
	   default:
		   break;
	}

	*v = (UBMP8)myval;
}
/*get*/
void GET(MYINT i,int& value, UBMP8* ptab) {
	UBMP8* v;
	MYINT q,r;
	int myval;

	q = (i / 4);
	r = (i % 4);
	v = &ptab[q];
	myval = *v;

	value = VALUE[(myval >> (r << 1)) & 3];
}

/*
* Verify if the value for a positon has become known
* by making all possible moves.
*/
int ENUMERATOR::verify(UBMP8* ptab1,UBMP8* ptab2) {
	int i,j,from,to,move,score;
	MYINT pos_index;

	/*initialize*/
	searcher.pstack->count = 0;
	searcher.gen_noncaps();

	for(i = 0;i < searcher.pstack->count; i++) {
		move = searcher.pstack->move_st[i];

		searcher.do_move(move);
		if(searcher.attacks(searcher.player,
			searcher.plist[COMBINE(searcher.opponent,king)]->sq)
			) {
				searcher.undo_move(move);
				continue;
		}


		from = SQ8864(m_from(move));
		to = SQ8864(m_to(move));

		/*make/unmake*/
		for(j = 0;j < n_piece;j++) {
			if(from == square[j]) {
				square[j] = to;
				break;
			}
		}

		get_index(pos_index);

		square[j] = from;
		/*end*/

		if(searcher.player == white) 
			GET(pos_index,score,ptab1);
		else 
			GET(pos_index,score,ptab2);

		searcher.undo_move(move);

		if(score == ILLEGAL) return score;
		else if(-score != LOSS) return -score;
	}

	return LOSS;
}
/*
* Retrograde analysis of parents
*/

void ENUMERATOR::get_retro_score(UBMP8* ptab1,UBMP8* ptab2,
								 UBMP8* c1,UBMP8* c2,
								 int score,bool is_6man) {
	int i,j,wk,bk,move,from,to,count,tscore;
	bool tried,special;
	MYINT pos_index;

	searcher.pstack->count = 0;
	searcher.gen_retro();
	score = -score;

	count = 0;
	for(i = 0;i < searcher.pstack->count; i++) {
		move = searcher.pstack->move_st[i];

		searcher.do_move(move);
		if(searcher.attacks(searcher.opponent,
			searcher.plist[COMBINE(searcher.player,king)]->sq)) {
			searcher.undo_move(move);
			continue;
		}

		tried = false;
		from = SQ8864(m_from(move));
		to = SQ8864(m_to(move));

BACK:
		/*move*/
		for(j = 0;j < n_piece;j++) {
			if(from == square[j]) {
				square[j] = to;
				break;
			}
		}

		if(get_index(pos_index,tried)) {
			/*update score*/
			if(!is_6man) {
				if(player == white) {
					if(c1[pos_index] == 0 || c1[pos_index] == 0xff) {
					} else {
						c1[pos_index]--;
						GET(pos_index,tscore,ptab1);
						if(tscore < score) 
							SET(pos_index,score,ptab1);
					}
				} else {
					if(c2[pos_index] == 0 || c2[pos_index] == 0xff) {
					} else {
						c2[pos_index]--;
						GET(pos_index,tscore,ptab2);
						if(tscore < score) 
							SET(pos_index,score,ptab2);
					}
				}
			} else {
				if(score == WIN) {
					if(player == white)
						SET(pos_index,score,ptab1);
					else
						SET(pos_index,score,ptab2);
				} else  {
					if(player == white) 
						GET(pos_index,tscore,ptab1);
					else 
						GET(pos_index,tscore,ptab2);

					if(tscore == ILLEGAL) {
						player = invert(player);
						searcher.player = invert(searcher.player);
						searcher.opponent = invert(searcher.opponent);

						tscore = verify(ptab1,ptab2);

						player = invert(player);
						searcher.player = invert(searcher.player);
						searcher.opponent = invert(searcher.opponent);

						if(tscore != ILLEGAL && tscore != DRAW) {
							if(player == white)
								SET(pos_index,tscore,ptab1);
							else
								SET(pos_index,tscore,ptab2);
						}
					}
				}
			}
		}

		/*unmove*/
		square[j] = from;


		/*special position*/
		special = false;
		wk = searcher.plist[wking]->sq;
		bk = searcher.plist[bking]->sq;
		if(!n_pawn
			&& ( (rank(wk) == file(wk) && abs(sqatt[wk - bk].step) == RU)
			||
			(rank(wk) + file(wk) == 7 && abs(sqatt[wk - bk].step) == LU)
			)
			)   
			special = true;

		/*redo for special postion*/
		if(!n_pawn && !tried && special) {
			tried = true;
			goto BACK;
		}

		searcher.undo_move(move);
	}
}
/*
* Forward analysis of children nodes
*/
int ENUMERATOR::get_init_score(UBMP8& counter,int w_checks,int b_checks,bool is_6man) {
	int i,move,wk,bk,
		score,best_score,legal_moves;

	/*initialize*/
	best_score = ILLEGAL;
	legal_moves = 0;
	searcher.pstack->count = 0;
	searcher.gen_all();

	for(i = 0;i < searcher.pstack->count; i++) {
		move = searcher.pstack->move_st[i];

		searcher.do_move(move);
		if(searcher.attacks(searcher.player,searcher.plist[COMBINE(searcher.opponent,king)]->sq)) {
			searcher.undo_move(move);
			continue;
		}

		legal_moves++;

		if(is_cap_prom(move)) {
			if(searcher.probe_bitbases(score)) {
				if(score > 0) {
					score = WIN; 
				} else if(score < 0) {
					score = LOSS;
				} else {
					score = DRAW;
				}
			}
		} else {
			wk = searcher.plist[wking]->sq;
			bk = searcher.plist[bking]->sq;
			if(!n_pawn
				&& ( (rank(wk) == file(wk) && abs(sqatt[wk - bk].step) == RU)
				||
				(rank(wk) + file(wk) == 7 && abs(sqatt[wk - bk].step) == LU)
				)
				)   
				counter += 2;
			else 
				counter += 1;

			searcher.undo_move(move);
			continue;
		}

		score = -score;
		if(score == WIN) {
			counter = 0;
			searcher.undo_move(move);
			best_score = WIN;
			break;
		}
		if(score > best_score) { 
			best_score = score;
		}

		searcher.undo_move(move);
	}

	if(legal_moves == 0) {
		if(searcher.player == white) {
			if(b_checks) {
				best_score = LOSS;
			} else {
				best_score = DRAW;
			}
		} else {
			if(w_checks) {
				best_score = LOSS;
			} else {
				best_score = DRAW;
			} 
		}
	}

	return best_score;
}
/*
* Illegal positon filtering
*/
bool ENUMERATOR::is_illegal(MYINT i,int side,bool& first) {
	int j;
	/*set up position*/
	if(!first) {
		for(j = 0;j < n_piece;j++) {
			searcher.pcRemove(piece[j],SQ6488(square[j]));
			searcher.board[SQ6488(square[j])] = empty;
		}
	}
	if(!get_pos(i))
		return true;

	if(first) {
		searcher.set_pos(n_piece,player,piece,square);
		first = false;
	} else {
		for(j = 0;j < n_piece;j++) {	
			searcher.pcAdd(piece[j],SQ6488(square[j]));
			searcher.board[SQ6488(square[j])] = piece[j];
		}
	}
	/*test*/
	if(side == white) {
		if(searcher.attacks(white,searcher.plist[bking]->sq)) return true;
	} else {
		if(searcher.attacks(black,searcher.plist[wking]->sq)) return true;
	}
	return false;
}
/*
* Initial pass
*/
void ENUMERATOR::initial_pass(
							  UBMP8* ptab1,UBMP8* ptab2,
							  UBMP8* c1,UBMP8* c2,
							  const MYINT& s,const MYINT& e,
							  bool is_6man) {
	int j,w_checks,b_checks,first,score;
	UBMP8 temp;

	first = true;
	clock_t start,end;
	{
		start = clock();

		for(MYINT i = s;i < e;i++) {

			/*verbose*/
			if(is_6man) {
				if((i % (size / 100)) == 0) printf(".");
			} else {
				if((i % (size / 10)) == 0) printf(".");
			}

			/*
			set up position
			*/
			if(first) {
			} else {
				for(j = 0;j < n_piece;j++) {
					searcher.pcRemove(piece[j],SQ6488(square[j]));
					searcher.board[SQ6488(square[j])] = empty;
				}
			}
			
			while(!get_pos(i)) {
				SET(i,ILLEGAL,ptab1);
				SET(i,ILLEGAL,ptab2);
				i++;
			}

			if(first) {
				searcher.set_pos(n_piece,player,piece,square);
				first = false;
			} else {
				for(j = 0;j < n_piece;j++) {	
					searcher.pcAdd(piece[j],SQ6488(square[j]));
					searcher.board[SQ6488(square[j])] = piece[j];
				}
			}

			/*checks*/
			w_checks = searcher.attacks(white,searcher.plist[bking]->sq);
			b_checks = searcher.attacks(black,searcher.plist[wking]->sq);

			/*white*/
			searcher.player = white;
			searcher.opponent = black;
			player = white;

			if(w_checks) {
				SET(i,ILLEGAL,ptab1);
			} else {
				if(!is_6man) {
					score = get_init_score(c1[i],w_checks,b_checks,is_6man);
					if(score == ILLEGAL) score = LOSS;
					SET(i,score,ptab1);
				} else {
					temp = 0;
					score = get_init_score(temp,w_checks,b_checks,is_6man);
					if(score == LOSS && temp) score = ILLEGAL;
					SET(i,score,ptab1);
				}
			}

			/*black*/
			searcher.player = black;
			searcher.opponent = white;
			player = black;

			if(b_checks) {
				SET(i,ILLEGAL,ptab2);
			} else {
				if(!is_6man) {
					score = get_init_score(c2[i],w_checks,b_checks,is_6man);
					if(score == ILLEGAL) score = LOSS;
					SET(i,score,ptab2);
				} else {
					temp = 0;
					score = get_init_score(temp,w_checks,b_checks,is_6man);
					if(score == LOSS && temp) score = ILLEGAL;
					SET(i,score,ptab2);
				}
			}
		}

		end = clock();
		print("\r<>iteration %3d [%.2f sec]\t\t\n",0,float(end - start) / CLOCKS_PER_SEC);
	}
}
/*
 * Backward pass
 */
void ENUMERATOR::backward_pass(
							   UBMP8* ptab1,UBMP8* ptab2,
							   UBMP8* c1,UBMP8* c2,
							   const MYINT& s,const MYINT& e,
							   bool is_6man
							   ) {
	MYINT i;
	clock_t start,end;
	int j,v1,v2,r1,r2,first,pass;
	int more_to_do = 0;
	int prev_more_to_do;
	int iteration = 0;
	do
	{
		prev_more_to_do = more_to_do;
		more_to_do = 0;
		first = true;
		start = clock();

		for(pass = 0;pass < 1;pass++) {
			for(i = s;i < e;i++) {
				
				/*verbose*/
				if(is_6man) {
					if((i % (size / 50)) == 0) printf(".");
				} else {
					if((i % (size / 10)) == 0) printf(".");
				}
				
				/*one of the scores should be known*/
				GET(i,v1,ptab1);
				GET(i,v2,ptab2);
				if(!is_6man) {
					r1 = ((v1 == WIN && c1[i] != 0xff) || (v1 == LOSS && c1[i] == 0));
					r2 = ((v2 == WIN && c2[i] != 0xff) || (v2 == LOSS && c2[i] == 0));
				} else {
					if(!pass) {
						if(iteration > 1) {
							r1 = (v1 == WIN && !(c1[i >> 3] & (1 << (i & 7))));
							r2 = (v2 == WIN && !(c2[i >> 3] & (1 << (i & 7))));
						} else {
							r1 = (v1 == WIN);
							r2 = (v2 == WIN);
						}
					} else {
						if(iteration > 1) {
							r1 = (v1 == LOSS && !(c1[i >> 3] & (1 << (i & 7))));
							r2 = (v2 == LOSS && !(c2[i >> 3] & (1 << (i & 7))));
						} else {
							r1 = (v1 == LOSS);
							r2 = (v2 == LOSS);
						}
					}
				}
	
				if(!(r1 || r2))
					continue;

				/*
				set up position
				*/
				if(first) {
				} else {
					for(j = 0;j < n_piece;j++) {
						searcher.pcRemove(piece[j],SQ6488(square[j]));
						searcher.board[SQ6488(square[j])] = empty;
					}
				}

				get_pos(i);
				
				if(first) {
					searcher.set_pos(n_piece,player,piece,square);
					first = false;
				} else {
					for(j = 0;j < n_piece;j++) {	
						searcher.pcAdd(piece[j],SQ6488(square[j]));
						searcher.board[SQ6488(square[j])] = piece[j];
					}
				}

				/*white*/
				if(r1) {
					player = black;
					searcher.player = black;
					searcher.opponent = white;
					
					get_retro_score(ptab1,ptab2,c1,c2,v1,is_6man);
					if(is_6man) 
						c1[i >> 3] |= (1 << (i & 7));
					else 
						c1[i] = 0xff;
					more_to_do++;
				}
				
				/*black*/
				if(r2) {
					player = white;
					searcher.player = white;
					searcher.opponent = black;		
					
					get_retro_score(ptab1,ptab2,c1,c2,v2,is_6man);
					if(is_6man) 
						c2[i >> 3] |= (1 << (i & 7));
					else 
						c2[i] = 0xff;
					more_to_do++;
				}
				
				/*white to move*/
				player = white;
				searcher.player = white;
				searcher.opponent = black;
				/*end*/
			}
			if(!is_6man) break;
		}

		iteration++;
		end = clock();
		print("\r<-iteration %3d [%.2f sec]\t\t\n",
			iteration,float(end - start) / CLOCKS_PER_SEC);
		if(prev_more_to_do == more_to_do) 
			break;
	} while (more_to_do);
}
/*
* Generate
*/
void generate_slice(ENUMERATOR* penum,FILE* fw,FILE *fb) {
	register MYINT i;
	UBMP8* ptab1,*ptab2,*c1,*c2;
	MYINT sz1,sz2,esize = penum->size,n_slices;
	int v;

	/*generate*/
	UBMP32 counts[2][4];
	for(i = 0;i < 4;i++) {
		counts[0][i] = counts[1][i] = 0;
	}

	/*pawn-sliced size*/
	n_slices = 1;
	if(penum->n_pawn)
		n_slices = 4;
	esize /= n_slices;

	/*calculate sizes*/
	bool is_6man = (penum->n_piece > 5);
	if(is_6man) 
		sz1 = (esize / 8);
	else 
		sz1 = (esize);
	sz2 = (esize / 4);

	/*Reserve memory*/
	print("Allocating %d MB\n",
		(2 * (sz1/1024 + sz2/1024)) / (1024));

	c1 = (UBMP8*)malloc((size_t)sz1);
	c2 = (UBMP8*)malloc((size_t)sz1);
	ptab1 = (UBMP8*)malloc((size_t)sz2);
	ptab2 = (UBMP8*)malloc((size_t)sz2);

	if(!c1 || !c2 || !ptab1 || !ptab2) {
		print("Memory allocation failed\n");
		exit(0);
	} else {
		print("Memory reserved\n");
	}

	/*generate slice*/
	penum->slice_size = esize;
	for(int s = 0;s < n_slices;s++)
	{
		penum->slice_i = s;
		print("Slice %d of size "FMT64"\n",s,esize);

		/*Initialize*/
		for(i = 0;i < sz1;i++) {
			c1[i] = 0;
			c2[i] = 0;
		}
		for(i = 0;i < esize;i++) {
			if(is_6man) {
				SET(i,ILLEGAL,ptab1);
				SET(i,ILLEGAL,ptab2);
			} else {
				SET(i,LOSS,ptab1);
				SET(i,LOSS,ptab2);
			}
		}

		/*parallel initial scan*/
		#pragma omp parallel
		{
			int threadId = omp_get_thread_num();
			int nThreads = omp_get_num_threads();
			MYINT block = esize / nThreads;
			MYINT start = threadId * block;
			MYINT end = start + block;
			ENUMERATOR e;
			e.copy(*penum);

			print("Thread %d/%d: start "FMT64" end "FMT64"\n",
				threadId + 1,nThreads,start,end);

			#pragma omp barrier
			e.initial_pass(ptab1,ptab2,c1,c2,start,end,is_6man);
		}

		/*passes*/
		penum->backward_pass(ptab1,ptab2,c1,c2,0,esize,is_6man);

		bool first = true;
		for(i = 0;i < esize;i++) {

			GET(i,v,ptab1);
			if((is_6man && v == ILLEGAL && !penum->is_illegal(i,white,first)) ||
				(!is_6man && v == LOSS && c1[i] > 0 && c1[i] != 0xff))
				SET(i,DRAW,ptab1);

			GET(i,v,ptab2);
			if((is_6man && v == ILLEGAL && !penum->is_illegal(i,black,first)) ||
				(!is_6man && v == LOSS && c2[i] > 0 && c2[i] != 0xff))
				SET(i,DRAW,ptab2);

			GET(i,v,ptab1);
			counts[0][v + 2]++;
			if(v == ILLEGAL) {
				if(i >= 4) GET(i - 4,v,ptab1);
				else v = DRAW;
				SET(i,v,ptab1);
			}

			GET(i,v,ptab2); 
			counts[1][v + 2]++;
			if(v == ILLEGAL) {
				if(i >= 4) GET(i - 4,v,ptab1);
				else v = DRAW;
				SET(i,v,ptab2);
			}
		}

		/*print stat*/
		print("	      Illegal	         Loss	         Draw	          Win");
		print("\n");
		print("white");
		int j;
		for(j = 0;j < 4;j++)
			print("%16d",counts[0][j]);
		print("\n");
		print("black");
		for(j = 0;j < 4;j++)
			print("%16d",counts[1][j]);
		print("\n");

		/*save to file*/
		for(i = 0;i < sz2;i++)
			fprintf(fw,"%c",ptab1[i]);
		for(i = 0;i < sz2;i++)
			fprintf(fb,"%c",ptab2[i]);
	}

	/*delete*/
	free(c1);
	free(c2);
	free(ptab1);
	free(ptab2);	
	/*end*/
}
void generate(ENUMERATOR* penum) {
	/*header*/
	penum->print_header();

	/*open files*/
	FILE* fw,*fb;
	char name[256];
	int len;
	len = strlen(penum->name);

	penum->name[len - 1] = 'w';
	strcpy(name,penum->name);
	strcat(name,".cmp");
	fw = fopen(name,"rb");
	if(fw) {
		print("Skipped %s\n",penum->name);
		fclose(fw);
		return;
	}
	fw = fopen(penum->name,"wb");

	penum->name[len - 1] = 'b';
	strcpy(name,penum->name);
	strcat(name,".cmp");
	fb = fopen(name,"rb");
	if(fb) {
		print("Skipped %s\n",penum->name);
		fclose(fb);
		return;
	}
	fb = fopen(penum->name,"wb");

	/*generate*/
	clock_t start,end;
	start = clock();
	generate_slice(penum,fw,fb);
	end = clock();
	print("Generated in %.2f sec\n",float(end - start) / CLOCKS_PER_SEC);

	/*close files*/
	fclose(fw);
	fclose(fb);

	/*compression*/
	start = clock();
	penum->player = white;
	penum->name[strlen(penum->name) - 1] = 'w';
	penum->compress();
	penum->player = black;
	penum->name[strlen(penum->name) - 1] = 'b';
	penum->compress();
	end = clock();
	print("Compressed in %.2f sec\n",float(end - start) / CLOCKS_PER_SEC);

	/*open egbb*/
	penum->piece[penum->n_piece] = 0;
	open_egbb(penum->piece);
}
/*
Compressor
*/
void ENUMERATOR::compress() {

	char command[256];
	char out_name[15];
	int len = strlen(name);

	strcpy(out_name,name);
	strcat(out_name,".cmp");

#ifdef _MSC_VER
	strcpy(command,"");
	sprintf(command,"egbb\\compress -c -i %s -o %s",name,out_name);
	system(command);
#else
	strcpy(command,"");
	sprintf(command,"./egbb/compress -c -i %s -o %s",name,out_name);
	system(command);
#endif
	remove(name);
}
/*
* print to logfile / stdout
*/
static FILE* log_file = 0;

void print(const char* format,...) {

	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	fflush(stdout);

	if(log_file) {
		va_start(ap, format);
		vfprintf(log_file, format, ap);
		va_end(ap);
		fflush(log_file);
	}
}

/*
* print header
*/
void ENUMERATOR::print_header() {
	register int i;
	print(name);
	print("\n");
	for(i = 0; i < n_piece;i++) {
		print(FMT64" ",index[i]);
	}
	print("\n");
	for(i = 0; i < n_piece;i++) {
		print(FMT64" ",divisor[i]);
	}
	print("\nSize "FMT64"\n",size);
}
/*
Main
*/
void print_help() {
	printf("Usage: generate [options] -i <input> -o <output>\n\t"
		"-n -- number of pieces including pawns\n\t"
		"-p -- number of pawns\n\t"
		"-i -- index of a single bitbase to generate with in the selected set\n\t"
		"-h -- show this help message\n"
		);
}
int main(int argc,char* argv[]) {
	SEARCHER searcher;
	ENUMERATOR enumerator,*penum = &enumerator; 
	int piece[MAX_PIECES];

	int egbb_cache_size = (32 * 1024 * 1024);
#ifdef _MSC_VER
	SEARCHER::egbb_is_loaded = LoadEgbbLibrary("egbb/",egbb_cache_size);
#else
	SEARCHER::egbb_is_loaded = LoadEgbbLibrary("./egbb/",egbb_cache_size);
#endif

	log_file = fopen("log.txt" ,"w");
	init_sqatt();
	init_indices();

	int npieces = 0,npawns = 0,bindex = -1,nenums = 0;
	for(int i = 0; i < argc; i++) {
		if(!strcmp(argv[i],"-n")) {
			npieces = atoi(argv[i+1]);
		} else if(!strcmp(argv[i],"-p")) {
			npawns = atoi(argv[i+1]);
		} else if(!strcmp(argv[i],"-i")) {
			bindex = atoi(argv[i+1]);
		} else if(!strcmp(argv[i],"-h")) {
			print_help();
			return 0;
		}
	}

	/*ADD files*/
#define ADD() {										\
	penum->add(white,piece);						\
	if(npieces != 0 && penum->n_piece > npieces) {	\
		penum->clear();								\
		continue;									\
	}												\
	if(penum->n_piece == npieces) {					\
		if(penum->n_pawn != npawns) {				\
			penum->clear();							\
			continue;								\
		}											\
		nenums++;									\
		if(bindex > 0 && bindex + 1 != nenums) {	\
			penum->clear();							\
			continue;								\
		}											\
	}												\
	penum->sort(1);									\
	penum->init();									\
	generate(penum);								\
	penum->clear();									\
};

	/*pieces*/
	piece[0] = wking;
	piece[1] = bking;
	int& piece1 = piece[2];
	int& piece2 = piece[3];
	int& piece3 = piece[4];
	int& piece4 = piece[5];

	/*3 pieces*/

	piece[3] = 0;

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		ADD();
	}

	/*4 pieces*/

	piece[4] = 0;

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = COMBINE(black,PIECE(piece1)); piece2 <= bpawn; piece2++) {
			ADD();
		}
	}

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			ADD();
		}
	}

	/*5 pieces*/

	piece[5] = 0;

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = bqueen; piece3 <= bpawn; piece3++) {
				ADD();
			}
		}
	}
	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = piece2; piece3 <= wpawn; piece3++) {
				ADD();
			}
		}
	}

	/*6 pieces*/

	piece[6] = 0;

	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = bqueen; piece3 <= bpawn; piece3++) {
				for(piece4 = piece3; piece4 <= bpawn; piece4++) {
					ADD();
				}
			}
		}
	}
	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = piece2; piece3 <= wpawn; piece3++) {
				for(piece4 = bqueen; piece4 <= bpawn; piece4++) {
					ADD();
				}
			}
		}
	}
	for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
		for(piece2 = piece1; piece2 <= wpawn; piece2++) {
			for(piece3 = piece2; piece3 <= wpawn; piece3++) {
				for(piece4 = piece3; piece4 <= wpawn; piece4++) {
					ADD();
				}
			}
		}
	}

#undef ADD
	/*close log file*/
	fclose(log_file);

	return 0;
}

