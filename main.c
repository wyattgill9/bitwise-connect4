#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned long long ull;

#define BOARD_HEIGHT 6
#define BOARD_WIDTH 7
#define BOARD_SIZE 42
#define FULL_BOARD 0xFFFFFFFFFFF // 42 bits set (6x7)
#define CONNECT_N 4

#define SET1(bitboard, pos) ((bitboard) |= (1ULL << (pos)))
#define SET0(bitboard, pos) ((bitboard) &= ~(1ULL << (pos)))
#define QUERY(bitboard, pos) (((bitboard) >> (pos)) & 1ULL)

typedef enum {
    NOTHING = -1,
    RED = 0,
    YELLOW = 1
} Side;

typedef struct {
    Side turn;
    ull occupied; 
    ull color;    
} Board;

ull *win_patterns = NULL;
int win_patterns_count = 0;
ull column_masks[BOARD_WIDTH];

void init_patterns(void);
void new_board(Board *board);
bool add_piece(Board *board, int col);
Side check_winner(const Board *board);
bool is_full(const Board *board);
void print_board(const Board *board, bool print_turn);
void cleanup(void);

void init_patterns(void) {
    if (win_patterns != NULL) return;
    
    win_patterns = malloc(0);
    win_patterns_count = 0;
    
    for (int row = 0; row < BOARD_HEIGHT; row++) {
        for (int col = 0; col <= BOARD_WIDTH - CONNECT_N; col++) {
            ull pattern = 0;
            for (int i = 0; i < CONNECT_N; i++) {
                int pos = row * BOARD_WIDTH + col + i;
                pattern |= 1ULL << pos;
            }
            win_patterns = realloc(win_patterns, sizeof(ull) * (win_patterns_count + 1));
            win_patterns[win_patterns_count++] = pattern;
        }
    }
    
    for (int col = 0; col < BOARD_WIDTH; col++) {
        for (int row = 0; row <= BOARD_HEIGHT - CONNECT_N; row++) {
            ull pattern = 0;
            for (int i = 0; i < CONNECT_N; i++) {
                int pos = (row + i) * BOARD_WIDTH + col;
                pattern |= 1ULL << pos;
            }
            win_patterns = realloc(win_patterns, sizeof(ull) * (win_patterns_count + 1));
            win_patterns[win_patterns_count++] = pattern;
        }
    }
    
    for (int row = 0; row <= BOARD_HEIGHT - CONNECT_N; row++) {
        for (int col = 0; col <= BOARD_WIDTH - CONNECT_N; col++) {
            ull pattern = 0;
            for (int i = 0; i < CONNECT_N; i++) {
                int pos = (row + i) * BOARD_WIDTH + (col + i);
                pattern |= 1ULL << pos;
            }
            win_patterns = realloc(win_patterns, sizeof(ull) * (win_patterns_count + 1));
            win_patterns[win_patterns_count++] = pattern;
        }
    }
    
    for (int row = 0; row <= BOARD_HEIGHT - CONNECT_N; row++) {
        for (int col = CONNECT_N - 1; col < BOARD_WIDTH; col++) {
            ull pattern = 0;
            for (int i = 0; i < CONNECT_N; i++) {
                int pos = (row + i) * BOARD_WIDTH + (col - i);
                pattern |= 1ULL << pos;
            }
            win_patterns = realloc(win_patterns, sizeof(ull) * (win_patterns_count + 1));
            win_patterns[win_patterns_count++] = pattern;
        }
    }
    
    for (int col = 0; col < BOARD_WIDTH; col++) {
        ull mask = 0;
        for (int row = 0; row < BOARD_HEIGHT; row++) {
            mask |= 1ULL << (row * BOARD_WIDTH + col);
        }
        column_masks[col] = mask;
    }
}

void new_board(Board *board) {
    board->color = 0;
    board->occupied = 0;
    board->turn = RED;
}

bool add_piece(Board *board, int col) {
    if (col < 0 || col >= BOARD_WIDTH) return false;
    
    ull column_bits = column_masks[col] & ~board->occupied;
    if (!column_bits) return false;
    
    ull position = column_bits & (~column_bits + 1); 
    
    board->occupied |= position;
    
    if (board->turn == RED) {
    } else {
        board->color |= position;
    }
    
    board->turn = (board->turn == RED) ? YELLOW : RED;
    return true;
}

Side check_winner(const Board *board) {
    ull red_pieces = board->occupied & ~board->color; 
    ull yellow_pieces = board->occupied & board->color;
    
    for (int i = 0; i < win_patterns_count; i++) {
        ull pattern = win_patterns[i];
        if ((red_pieces & pattern) == pattern) return RED;
        if ((yellow_pieces & pattern) == pattern) return YELLOW;
    }
    
    return NOTHING;
}

bool is_full(const Board *board) {
    return board->occupied == FULL_BOARD;
}

void print_board(const Board *board, bool print_turn) {
    printf("  1   2   3   4   5   6   7\n");
    printf("╭───┬───┬───┬───┬───┬───┬───╮\n");
    
    for (int row = 0; row < BOARD_HEIGHT; row++) {
        for (int col = 0; col < BOARD_WIDTH; col++) {
            int pos = (BOARD_HEIGHT - 1 - row) * BOARD_WIDTH + col;
            printf("│");
            
            if (!QUERY(board->occupied, pos)) {
                printf("   ");
            } else if (!QUERY(board->color, pos)) {
                printf("\033[30;91m O \033[0m");
            } else {
                printf("\033[30;93m O \033[0m");
            }
        }
        
        printf("│");
        if (row < BOARD_HEIGHT - 1) {
            printf("\n├───┼───┼───┼───┼───┼───┼───┤\n");
        } else {
            printf("\n╰───┴───┴───┴───┴───┴───┴───╯\n");
        }
    }
    
    if (print_turn) {
        printf("Turn: %s\n", board->turn == RED ? "red" : "yellow");
    }
}

void cleanup(void) {
    free(win_patterns);
    win_patterns = NULL;
    win_patterns_count = 0;
}

void clear_screen(void) {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

int main(void) {
    init_patterns();
    
    Board board;
    new_board(&board);
    
    Side winner = NOTHING;
    int col;
    
    while (1) {
        clear_screen();
        
        winner = check_winner(&board);
        if (winner != NOTHING || is_full(&board)) {
            break;
        }
        
        print_board(&board, true);
        
        int valid_input = 0;
        while (!valid_input) {
            printf("Move (1-7): ");
            if (scanf("%d", &col) != 1) {
                while (getchar() != '\n');
                printf("\033[F\033[K"); 
                continue;
            }
            
            col--;
            
            if (col >= 0 && col < BOARD_WIDTH && add_piece(&board, col)) {
                valid_input = 1;
            } else {
                printf("\033[F\033[K"); 
            }
        }
    }
    
    clear_screen();
    print_board(&board, false);
    
    if (winner != NOTHING) {
        printf("%s wins!\n", winner == RED ? "Red" : "Yellow");
    } else {
        printf("Tie\n");
    }
    
    cleanup();
    return 0;
}
