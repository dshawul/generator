#include "common.h"

/*
* Store/get mechanisms
*/
FORCEINLINE void SET(MYINT i,int value,UBMP8* ptab) {
    MYINT q = (i >> 2);
    MYINT r = (i & 3);
    UBMP8* v = &ptab[q];
    int myval = (int)(*v & ~(3 << (r << 1)));
    myval |= ((1 - value) << (r << 1));
    *v = (UBMP8)myval;
}
FORCEINLINE void GET(MYINT i,int& value, UBMP8* ptab) {
    MYINT q = (i >> 2);
    MYINT r = (i & 3);
    value = 1 - ((ptab[q] >> (r << 1)) & 3);
}
#define  SETC(posi,tab)  tab[posi >> 3] |=  (1 << (posi & 7))
#define  CLRC(posi,tab)  tab[posi >> 3] &= ~(1 << (posi & 7))
#define ISSET(posi,tab) (tab[posi >> 3] & (1 << (posi & 7)))

/*
* Verify if the value for a positon has become known
* by making all possible moves.
*/
int ENUMERATOR::verify(UBMP8* ptab1,UBMP8* ptab2,
                       UBMP8* c1,UBMP8* c2) {
    int i,j,from,to,move,score;
    MYINT pos_index;

    /*initialize*/
    searcher.pstack->count = 0;
    searcher.gen_noncaps();

    for(i = 0;i < searcher.pstack->count; i++) {
        move = searcher.pstack->move_st[i];
        searcher.do_move<true>(move);

        /*check if two kings are one distance apart*/
        if(distance(searcher.plist[wking]->sq,searcher.plist[bking]->sq) <= 1) {
            searcher.undo_move<true>(move);
            continue;
        }

        /*squares*/
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

        /*get score of child*/
        if(searcher.player == white)
            GET(pos_index,score,ptab1);
        else
            GET(pos_index,score,ptab2);

        searcher.undo_move<true>(move);

        if(score == ILLEGAL) {
            if(player == black && ISSET(pos_index,c1)) 
                continue;
            if(player == white && ISSET(pos_index,c2)) 
                continue;
            return ILLEGAL;
        } else if(score == DRAW) {
            return ILLEGAL;
        } else if(-score == WIN) {
            return WIN;
        }
    }

    return LOSS;
}

/*
* Retrograde analysis of parents
*/
void ENUMERATOR::get_retro_score(UBMP8* ptab1,UBMP8* ptab2,
                                 UBMP8* c1,UBMP8* c2,
                                 int score,bool compact_count) {
    int i,j,move,from,to,tscore;
    bool tried;
    MYINT pos_index;

    searcher.pstack->count = 0;
    searcher.gen_retro();
    score = -score;

    for(i = 0;i < searcher.pstack->count; i++) {
        move = searcher.pstack->move_st[i];
        searcher.do_move<true>(move);

        /*check legality of move*/
        if(searcher.attacks(searcher.opponent,
            searcher.plist[COMBINE(searcher.player,king)]->sq)
            ) {
                searcher.undo_move<true>(move);
                continue;
        }
        /*squares*/
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
#ifdef PARALLEL
            /*find bank and lock it*/
            int locki = int(pos_index / bank_size);
            l_lock(locks[locki]);
#endif
            /*set paren't value*/
            if(!compact_count) {
                if(score == WIN) {
                    /*mark as sure win*/
                    if(player == white)
                        SET(pos_index,score,ptab1);
                    else
                        SET(pos_index,score,ptab2);
                } else if (score == LOSS)  {
                    /*mark as potential loss*/
                    if(player == white) {
                        if(c1[pos_index] == 0 || c1[pos_index] == 0xff) {
                        } else {
                            c1[pos_index]--;
                        }
                    } else {
                        if(c2[pos_index] == 0 || c2[pos_index] == 0xff) {
                        } else {
                            c2[pos_index]--;
                        }
                    }
                }
            } else {
                if(score == WIN) {
                    /*mark as sure win*/
                    if(player == white)
                        SET(pos_index,score,ptab1);
                    else
                        SET(pos_index,score,ptab2);
                } else if (score == LOSS)  {
                    /*mark as potential loss*/
                    if(player == white) 
                        GET(pos_index,tscore,ptab1);
                    else 
                        GET(pos_index,tscore,ptab2);
                    if(tscore == ILLEGAL) {
                        if(player == white) {
                            SET(pos_index,DRAW,ptab1);
                            SETC(pos_index,c1);
                        } else {
                            SET(pos_index,DRAW,ptab2);
                            SETC(pos_index,c2);
                        }
                    }
                }
            }
#ifdef PARALLEL
            /*unlock bank*/
            l_unlock(locks[locki]);
            /*end*/
#endif
        }
        
        square[j] = from;

        /*special position*/
        int wk = searcher.plist[wking]->sq;
        int bk = searcher.plist[bking]->sq;
        if(!n_pawn
            && ( 
            (rank(wk) == file(wk) && abs(sqatt_step(wk - bk)) == RU)
            ||
            (rank(wk) + file(wk) == 7 && abs(sqatt_step(wk - bk)) == LU)
            )) {
                if(!tried) {
                    tried = true;
                    goto BACK;
                }
        }

        /*undo*/
        searcher.undo_move<true>(move);
    }
}
/*
* Forward analysis of children nodes
*/
int ENUMERATOR::get_init_score(UBMP8& counter,int w_checks,int b_checks,bool compact_count) {
    int i,move,
        score,best_score,legal_moves;

    /*initialize*/
    best_score = ILLEGAL;
    legal_moves = 0;
    searcher.pstack->count = 0;
    searcher.gen_all();
    
    for(i = 0;i < searcher.pstack->count; i++) {
        move = searcher.pstack->move_st[i];

        if(is_cap_prom(move)) {
            searcher.do_move<false>(move);

            /*check legality of move*/
            if(searcher.attacks(searcher.player,
                searcher.plist[COMBINE(searcher.opponent,king)]->sq)
                ) {
                    searcher.undo_move<false>(move);
                    continue;
            }

            legal_moves++;


            if(searcher.probe_bitbases(&score)) {
                if(score > 0) {
                    score = WIN; 
                } else if(score < 0) {
                    score = LOSS;
                } else {
                    score = DRAW;
                }
            }

            searcher.undo_move<false>(move);

            score = -score;
            if(score == WIN) {
                counter = 0;
                return WIN;
            }
            if(score > best_score)
                best_score = score;
        } else {
            searcher.do_move<true>(move);
            /*check legality of move*/
            if(searcher.attacks(searcher.player,
                searcher.plist[COMBINE(searcher.opponent,king)]->sq)
                ) {
                    searcher.undo_move<true>(move);
                    continue;
            }

            legal_moves++;

            int wk = searcher.plist[wking]->sq;
            int bk = searcher.plist[bking]->sq;
            if(!n_pawn
                && ( 
                (rank(wk) == file(wk) && abs(sqatt_step(wk - bk)) == RU)
                ||
                (rank(wk) + file(wk) == 7 && abs(sqatt_step(wk - bk)) == LU)
                ))   
                counter += 2;
            else 
                counter += 1;

            searcher.undo_move<true>(move);
            continue;
        }
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
* Initial pass
*/
void ENUMERATOR::initial_pass(
                              UBMP8* ptab1,UBMP8* ptab2,
                              UBMP8* c1,UBMP8* c2,
                              const MYINT& s,const MYINT& e,
                              bool compact_count) {
    int j,w_checks,b_checks,first,score;
    UBMP8 temp;

    first = true;
    clock_t start,end;
    {
        start = clock();

        MYINT icount = 0;
        for(MYINT i = s;i < e;i++) {
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
            
            while((i < e) && !get_pos(i)) {
                SET(i,ILLEGAL,ptab1);
                SET(i,ILLEGAL,ptab2);
                if(compact_count) {
                    SETC(i,c1);
                    SETC(i,c2);
                }
                i++;
            }
            if(i >= e) break;
            icount++;

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
                if(compact_count)
                    SETC(i,c1);
            } else {
                if(!compact_count) {
                    score = get_init_score(c1[i],w_checks,b_checks,compact_count);
                    if(score == ILLEGAL) score = LOSS;
                    SET(i,score,ptab1);
                } else {
                    temp = 0;
                    score = get_init_score(temp,w_checks,b_checks,compact_count);
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
                if(compact_count)
                    SETC(i,c2);
            } else {
                if(!compact_count) {
                    score = get_init_score(c2[i],w_checks,b_checks,compact_count);
                    if(score == ILLEGAL) score = LOSS;
                    SET(i,score,ptab2);
                } else {
                    temp = 0;
                    score = get_init_score(temp,w_checks,b_checks,compact_count);
                    if(score == LOSS && temp) score = ILLEGAL;
                    SET(i,score,ptab2);
                }
            }
        }

        end = clock();
        print("\r<>iteration %3d [%.2f sec] " FMT64 "\t\t\n",
            0,float(end - start) / CLOCKS_PER_SEC,icount);
    }
}
/*
 * Backward pass
 */
void ENUMERATOR::backward_pass(
                               UBMP8* ptab1,UBMP8* ptab2,
                               UBMP8* c1,UBMP8* c2,
                               const MYINT& s,const MYINT& e,
                               bool compact_count
                               ) {
    MYINT i,icount;
    clock_t start,end;
    int j,v1,v2,r1,r2,first;
    int iteration = 0;
    do
    {
        /*wait for all threads*/
        l_barrier();

        /*set count to 0*/
        more_to_do = 0;
        icount = 0;
        first = true;
        start = clock();
        int pend = (compact_count ? 3 : 2);

        for(int pass = 0;pass < pend;pass++) {
            for(i = s;i < e;i++) {
                /*one of the scores should be known*/
                GET(i,v1,ptab1);
                GET(i,v2,ptab2);
                if(!compact_count) {
                    if(pass == 0) {
                        r1 = (v1 == LOSS && c1[i] == 0);
                        r2 = (v2 == LOSS && c2[i] == 0);
                    } else {
                        r1 = (v1 == WIN && c1[i] != 0xff);
                        r2 = (v2 == WIN && c2[i] != 0xff);
                    }
                } else {
                    if(pass == 0) {
                        r1 = (v1 == LOSS && !ISSET(i,c1));
                        r2 = (v2 == LOSS && !ISSET(i,c2));
                    } else if(pass == 1) {
                        r1 = (v1 == WIN && !ISSET(i,c1));
                        r2 = (v2 == WIN && !ISSET(i,c2));
                    } else {
                        r1 = (v1 == DRAW && ISSET(i,c1));
                        r2 = (v2 == DRAW && ISSET(i,c2));
                    }
                }

                if(!(r1 || r2))
                    continue;
                icount++;

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
                    if(pass == 2) {
                        player = white;
                        searcher.player = white;
                        searcher.opponent = black;
                        int tscore = verify(ptab1,ptab2,c1,c2);
                        if(tscore != ILLEGAL)
                            SET(i,tscore,ptab1);
                        else
                            SET(i,ILLEGAL,ptab1);
                        CLRC(i,c1);
                    } else {
                        player = black;
                        searcher.player = black;
                        searcher.opponent = white;
                        get_retro_score(ptab1,ptab2,c1,c2,v1,compact_count);
                        if(compact_count) 
                            SETC(i,c1);
                        else 
                            c1[i] = 0xff;
                    }
                    more_to_do++;
                }

                /*black*/
                if(r2) {
                    if(pass == 2) {
                        player = black;
                        searcher.player = black;
                        searcher.opponent = white;
                        int tscore = verify(ptab1,ptab2,c1,c2);
                        if(tscore != ILLEGAL)
                            SET(i,tscore,ptab2);
                        else
                            SET(i,ILLEGAL,ptab2);
                        CLRC(i,c2);
                    } else {
                        player = white;
                        searcher.player = white;
                        searcher.opponent = black;
                        get_retro_score(ptab1,ptab2,c1,c2,v2,compact_count);
                        if(compact_count) 
                            SETC(i,c2);
                        else 
                            c2[i] = 0xff;
                    }
                    more_to_do++;
                }
            }

            /*wait for all threads*/
            l_barrier();
        }
        iteration++;
        end = clock();
        print("\r<-iteration %3d [%.2f sec] " FMT64 "\t\t\n",
            iteration,float(end - start) / CLOCKS_PER_SEC,icount);
    } while (more_to_do);
}
/*
 * Generate from existing
 */
void ENUMERATOR::regenerate(
    UBMP8* ptab1,UBMP8* ptab2,
    const MYINT& s,const MYINT& e
    ) {
    MYINT i;
    int j;
    bool first;
    first = true;
    for(i = s;i < e;i++) {
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

        while((i < e) && !get_pos(i)) {
            if(i >= 4) {
                int v;
                if(i >= 4) GET(i - 4,v,ptab1);
                else v = DRAW;
                SET(i,v,ptab1);

                if(i >= 4) GET(i - 4,v,ptab2);
                else v = DRAW;
                SET(i,v,ptab2);
            }
            i++;
        }
        if(i >= e) break;

        
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
        int w_checks = searcher.attacks(white,searcher.plist[bking]->sq);
        int b_checks = searcher.attacks(black,searcher.plist[wking]->sq);
        if(w_checks) {
            int v;
            if(i >= 4) GET(i - 4,v,ptab1);
            else v = DRAW;
            SET(i,v,ptab1);
        }
        if(b_checks) {
            int v;
            if(i >= 4) GET(i - 4,v,ptab2);
            else v = DRAW;
            SET(i,v,ptab2);
        }

        /*white*
        player = white;
        searcher.player = white;
        searcher.opponent = black;
        int score[2];
        if(searcher.probe_bitbases(score,true)) {
            if(score[0] > 0) {
                SET(i,WIN,ptab1);
            } else if(score[0] < 0) {
                SET(i,LOSS,ptab1);
            } else {
                SET(i,DRAW,ptab1);
            }

            if(score[1] > 0) {
                SET(i,WIN,ptab2);
            } else if(score[1] < 0) {
                SET(i,LOSS,ptab2);
            } else {
                SET(i,DRAW,ptab2);
            }
        }
        */
    }
}
/*
* Generate
*/
unsigned int ENUMERATOR::cumm_comp_time;
unsigned int ENUMERATOR::cumm_gen_time;
unsigned int ENUMERATOR::more_to_do;
unsigned int ENUMERATOR::bank_size;
LOCK ENUMERATOR::locks[MAX_BANKS];
static size_t total_size  = 0;
static size_t oneside_size = 0;
static int mem_usage = -1;
static int npawn_slices = -1;
static int action = 0;
static int open_after_gen = 1;

void generate_slice(ENUMERATOR* penum,FILE* fw,FILE *fb) {
    register MYINT i;
    UBMP8 *ptab1=0,*ptab2=0,*c1=0,*c2=0;
    MYINT sz1,sz2,esize = penum->size;
    int n_slices,v;

    /*pawn-sliced size*/
    n_slices = 1;
    if(penum->n_pawn) {
        if(penum->n_piece > 5) n_slices = 4;
        if(npawn_slices != -1) n_slices = npawn_slices;
    }
    esize /= n_slices;
    
    /*calculate sizes*/
    bool compact_count = (penum->n_piece > 5);
    if(mem_usage == 0) compact_count = true;
    else if(mem_usage == 1) compact_count = false;
    if(compact_count) 
        sz1 = (esize / 8);
    else 
        sz1 = (esize);
    sz2 = (esize / 4);

    /*Reserve memory*/
    print("Allocating %d MB\n",
        int(2 * (sz1/1024 + sz2/1024)) / (1024));

    if(action == 0) {
        aligned_reserve<UBMP8>(c1,sz1);
        aligned_reserve<UBMP8>(c2,sz1);
        if(!c1 || !c2) {
            print("Memory allocation failed\n");
            exit(0);
        }
    }
    if(action == 0 || action == 3) {
        aligned_reserve<UBMP8>(ptab1,sz2);
        aligned_reserve<UBMP8>(ptab2,sz2);
        if(!ptab1 || !ptab2) {
            print("Memory allocation failed\n");
            exit(0);
        }
    }

    /*generate slice*/
    penum->slice_size = esize;
    ENUMERATOR::bank_size = int(esize / MAX_BANKS + 1);
    for(int s = 0;s < n_slices;s++)
    {
        penum->slice_i = s;
        print("Slice %d/%d of size " FMT64 "\n",s+1,n_slices,esize);

        /*read slice if requested*/
        if(action == 3) {
            penum->player = white;
            penum->read_slice(ptab1,sz2,n_slices);
            penum->player = black;
            penum->read_slice(ptab2,sz2,n_slices);
        }

        /*Initialize*/
        if(action == 0) {
            memset(c1,0,sz1);
            memset(c2,0,sz1);
            for(i = 0;i < esize;i++) {
                if(compact_count) {
                    SET(i,ILLEGAL,ptab1);
                    SET(i,ILLEGAL,ptab2);
                } else {
                    SET(i,LOSS,ptab1);
                    SET(i,LOSS,ptab2);
                }
            }
        }
        /*parallel scans*/
#ifdef PARALLEL
        #pragma omp parallel
#endif
        {
#ifdef PARALLEL
            int threadId = omp_get_thread_num();
            int nThreads = omp_get_num_threads();
#else
            int threadId = 0;
            int nThreads = 1;
#endif
            MYINT block = esize / nThreads;
            MYINT start = threadId * block;
            MYINT end = start + block;
            ENUMERATOR e;
            e.copy(*penum);

            print("Thread %d/%d: start " FMT64 " end " FMT64 "\n",
                threadId + 1,nThreads,start,end);

            if(action == 3) {
                e.regenerate(ptab1,ptab2,start,end);
            } else {
                e.initial_pass(ptab1,ptab2,c1,c2,start,end,compact_count);
                e.backward_pass(ptab1,ptab2,c1,c2,start,end,compact_count);
            }
        }

        if(action == 0) {
            UBMP32 counts[2][4];
            for(i = 0;i < 4;i++) {
                counts[0][i] = counts[1][i] = 0;
            }
            /*count*/
            bool first = true;
            for(i = 0;i < esize;i++) {
                GET(i,v,ptab1);
                if((compact_count && v == ILLEGAL && !ISSET(i,c1)) ||
                    (!compact_count && v == LOSS && c1[i] > 0 && c1[i] != 0xff))
                    SET(i,DRAW,ptab1);

                GET(i,v,ptab2);
                if((compact_count && v == ILLEGAL && !ISSET(i,c2)) ||
                    (!compact_count && v == LOSS && c2[i] > 0 && c2[i] != 0xff))
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
                    if(i >= 4) GET(i - 4,v,ptab2);
                    else v = DRAW;
                    SET(i,v,ptab2);
                }
            }

            /*print stat*/
            print("       Illegal            Loss            Draw             Win");
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
        }

        /*save to file*/
        fwrite(ptab1,1,sz2,fw);
        fwrite(ptab2,1,sz2,fb);
    }
    
    /*delete*/
    aligned_free(c1);
    aligned_free(c2);
    aligned_free(ptab1);
    aligned_free(ptab2);
    /*end*/
}

void generate(ENUMERATOR* penum) {
    /*open files*/
    FILE* fw,*fb;
    char name[256];
    size_t len,sz1,sz2;
    bool exists = true;

    len = strlen(penum->name);

    penum->name[len - 1] = 'w';
    strcpy(name,penum->name);
    strcat(name,".cmp");
    fw = fopen(name,"rb");
    if(fw && (action != 3)) {
        fseek(fw, 0, SEEK_END);
        sz1 = ftell(fw);
        print("Skipped %s: file size %.2f KB\n",
            penum->name,sz1 / (1024.0));
        fclose(fw);
    } else {
        if(action == 0 || action == 3) {
            if(!(fw = fopen(penum->name,"rb")))
                fw = fopen(penum->name,"wb");
            else {
                fclose(fw);
                fw = 0;
            }
        }
        exists = false;
    }

    penum->name[len - 1] = 'b';
    strcpy(name,penum->name);
    strcat(name,".cmp");
    fb = fopen(name,"rb");
    if(fb && (action != 3)) {
        fseek(fb, 0, SEEK_END);
        sz2 = ftell(fb);
        print("Skipped %s: file size %.2f KB\n",
            penum->name,sz2 / (1024.0));
        fclose(fb);
    } else {
        if(action == 0 || action == 3) {
            if(!(fb = fopen(penum->name,"rb")))
                fb = fopen(penum->name,"wb");
            else {
                fclose(fb);
                fb = 0;
            }
        }
        exists = false;
    }

    if(exists) {
        total_size += sz1 + sz2;
        if(sz1 < sz2) oneside_size += sz1;
        else oneside_size += sz2;

        print("Total %.2f MB, Oneside %.2f MB\n",
            total_size / (1024.0 * 1024.0), 
            oneside_size / (1024.0 * 1024.0));

        if(action == 2) {
            penum->name[len - 1] = ((sz1 < sz2) ? 'w' : 'b');
            strcpy(name,penum->name);
            strcat(name,".cmp");

            char command[256];
#ifdef _MSC_VER
            sprintf(command,"copy %s %s%s",
                name,SEARCHER::egbb_path,name);
#else
            sprintf(command,"cp -p %s %s%s",
                name,SEARCHER::egbb_path,name);
#endif
            print(command);
            print("\n");
            system(command);
        }
        return;
    }

    if(action != 0 && action != 3)
        return;
    
    if(!fw || !fb) {
        print("Skipped %s : Uncompressed file exists.\n",penum->name);
        return;
    }

    /*generate*/
    penum->print_header();
    clock_t start,end;
    start = clock();
    if(action == 3) {
        penum->player = white;
        penum->decompress();
        penum->player = black;
        penum->decompress();
    }
    generate_slice(penum,fw,fb);
    end = clock();
    ENUMERATOR::cumm_gen_time += (end - start);
    print("Generated in %.2f sec : Cummulative %.2f sec\n",
        float(end - start) / CLOCKS_PER_SEC,
        float(ENUMERATOR::cumm_gen_time) / CLOCKS_PER_SEC
        );

    /*close files*/
    fclose(fw);
    fclose(fb);

    /*compression*/
    start = clock();
    penum->player = white;
    penum->compress();
    penum->player = black;
    penum->compress();
    end = clock();
    ENUMERATOR::cumm_comp_time += (end - start);
    print("Compressed in %.2f sec : Cummulative %.2f sec\n",
        float(end - start) / CLOCKS_PER_SEC,
        float(ENUMERATOR::cumm_comp_time) / CLOCKS_PER_SEC
        );

    /*open egbb*/
    if(action == 0 && open_after_gen) {
        penum->piece[penum->n_piece] = 0;
        open_egbb(penum->piece);
    }
    /*end*/
}
/*
Compressor
*/
void ENUMERATOR::compress() {
    char command[256];
    char out_name[128];
    char temp[256];
    name[strlen(name) - 1] = (player == white) ? 'w' : 'b';
    size_t len = strlen(name);
    strcpy(out_name,name);
    strcat(out_name,".cmp");
    strcpy(command,SEARCHER::egbb_path);

    if(action == 3) {
        sprintf(temp,"compress -c %s -o %s%s",
            name,SEARCHER::egbb_path,out_name);
    } else {
        sprintf(temp,"compress -c %s -o %s",
            name,out_name);
    }
    strcat(command,temp);

    if(system(command))
        print("Compressor not found: %s\n",command);
    else
        remove(name);
}
void ENUMERATOR::decompress() {
    char command[256];
    char out_name[128];
    char temp[256];
    name[strlen(name) - 1] = (player == white) ? 'w' : 'b';
    size_t len = strlen(name);
    strcpy(out_name,name);
    strcat(out_name,".cmp");
    strcpy(command,SEARCHER::egbb_path);

    sprintf(temp,"compress -d %s -o %s%s",
        out_name,SEARCHER::egbb_path,name);
    strcat(command,temp);

    if(system(command))
        print("Compressor not found: %s\n",command);
}
void ENUMERATOR::read_slice(UBMP8* ptab, const MYINT& sz,int n_slices) {
    char temp[256];
    name[strlen(name) - 1] = (player == white) ? 'w' : 'b';
    strcpy(temp,SEARCHER::egbb_path);
    strcat(temp,name);
    FILE* fw = fopen(temp,"rb");
    long int offset = long(slice_i * sz);
    fseek(fw, offset, SEEK_SET);
    fread(ptab,1,sz,fw);
    fclose(fw);
    if(slice_i == n_slices - 1)
        remove(temp);
}
/*
* print to logfile / stdout
*/
void print(const char* format,...) {
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    fflush(stdout);
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
    print("\nSize " FMT64 "\n",size);
}
/*
Main
*/
void print_help() {
    printf("Usage: generate [options] -i <input> -o <output>\n\t"
        "-n -- number of pieces including pawns\n\t"
        "-p -- number of pawns\n\t"
        "-i -- index of a single bitbase to generate with in the selected set\n\t"
        "-h -- show this help message\n\t"
        "-t -- number of threads to use\n\t"
        "-pslice -- number of pawn slices 1,2,4\n\t"
        "-mem -- memory usage 0-low 1-high\n\t"
        "-ecache -- egbb cache size in MB (default 32)\n\t"
        "-epath -- egbb path (default egbb/)\n\t"
        "-action -- Actions to do:\n\t"
         "\t\t0-generate\n\t"
         "\t\t1-show sizes\n\t"
         "\t\t2-move smaller file\n\t"
         "\t\t3-generate from existing egbbs\n\t"
         "-open -- open egbb after generation\n"
        );
}
int SEARCHER::egbb_cache_size;
char SEARCHER::egbb_path[128];

int main(int argc,char* argv[]) {
    SEARCHER searcher;
    ENUMERATOR enumerator,*penum = &enumerator; 
    int piece[MAX_PIECES];
    static const int piece_v[15] = {
        0,0,975,500,326,325,100,0,975,500,326,325,100,0
    };
    
    /*options*/
    int npieces = 0,npawns = -1,bindex = -1,nenums = 0,nThreads = 1;
    SEARCHER::egbb_cache_size = 32;
#ifdef _MSC_VER
    strcpy(SEARCHER::egbb_path,"egbb\\");
#else
    strcpy(SEARCHER::egbb_path,"egbb/");
#endif

    for(int i = 0; i < argc; i++) {
        if(!strcmp(argv[i],"-n")) {
            npieces = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-p")) {
            npawns = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-i")) {
            bindex = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-t")) {
            nThreads = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-mem")) {
            mem_usage = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-pslice")) {
            npawn_slices = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-ecache")) {
            SEARCHER::egbb_cache_size = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-epath")) {
            strcpy(SEARCHER::egbb_path,argv[++i]);
        } else if(!strcmp(argv[i],"-action")) {
            action = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-open")) {
            open_after_gen = atoi(argv[++i]);
        } else if(!strcmp(argv[i],"-h")) {
            print_help();
            return 0;
        }
    }
    SEARCHER::egbb_cache_size *= (1024 * 1024);

    /*load egbbs*/
    if(action == 0 || action == 3) {
        if(action == 3) 
            SEARCHER::egbb_load_type = 0;
        if(action == 0)
            SEARCHER::egbb_is_loaded = LoadEgbbLibrary(
            SEARCHER::egbb_path,SEARCHER::egbb_cache_size);
        init_indices();
    }

    /*reset time*/
    ENUMERATOR::cumm_gen_time = 0;
    ENUMERATOR::cumm_comp_time = 0;
    for(int i = 0;i < MAX_BANKS;i++)
        l_create(ENUMERATOR::locks[i]);
#ifdef PARALLEL
    omp_set_num_threads(nThreads);
#endif
    /*ADD files*/
#define ADD() {                                     \
    penum->add(white,piece);                        \
    if(npieces != 0 && penum->n_piece != npieces) { \
        penum->clear();                             \
        continue;                                   \
    }                                               \
    if(npawns != -1 && penum->n_pawn != npawns) {   \
        penum->clear();                             \
        continue;                                   \
    }                                               \
    nenums++;                                       \
    if(bindex != -1 && nenums - 1 != bindex) {      \
        penum->clear();                             \
        continue;                                   \
    }                                               \
    penum->sort(1);                                 \
    penum->init();                                  \
    generate(penum);                                \
    penum->clear();                                 \
};

    /*pieces*/
    piece[0] = wking;
    piece[1] = bking;
    int& piece1 = piece[2];
    int& piece2 = piece[3];
    int& piece3 = piece[4];
    int& piece4 = piece[5];
    int& piece5 = piece[6];

    /*3 pieces*/
    piece[3] = 0;

    for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
        ADD();
    }

    /*4 pieces*/
    piece[4] = 0;

    for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
        for(piece2 = bqueen; piece2 <= bpawn; piece2++) {
            if(piece_v[piece1] < piece_v[piece2])
                continue;
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
                    if(piece_v[piece1] + piece_v[piece2] < 
                        piece_v[piece3] + piece_v[piece4])
                        continue;
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

    /*7 pieces*/
    piece[7] = 0;

    for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
        for(piece2 = piece1; piece2 <= wpawn; piece2++) {
            for(piece3 = piece2; piece3 <= wpawn; piece3++) {
                for(piece4 = bqueen; piece4 <= bpawn; piece4++) {
                    for(piece5 = piece4; piece5 <= bpawn; piece5++) {
                        ADD();
                    }
                }
            }
        }
    }
    for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
        for(piece2 = piece1; piece2 <= wpawn; piece2++) {
            for(piece3 = piece2; piece3 <= wpawn; piece3++) {
                for(piece4 = piece3; piece4 <= wpawn; piece4++) {
                    for(piece5 = bqueen; piece5 <= bpawn; piece5++) {
                        ADD();
                    }
                }
            }
        }
    }
    for(piece1 = wqueen; piece1 <= wpawn; piece1++) {
        for(piece2 = piece1; piece2 <= wpawn; piece2++) {
            for(piece3 = piece2; piece3 <= wpawn; piece3++) {
                for(piece4 = piece3; piece4 <= wpawn; piece4++) {
                    for(piece5 = piece4; piece5 <= wpawn; piece5++) {
                        ADD();
                    }
                }
            }
        }
    }
    /*end*/
#undef ADD

    return 0;
}

