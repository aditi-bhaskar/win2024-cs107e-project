#include "gl.h"
#include <stdbool.h>

// Define constant tetris pieces and specify their 4 rotations via a 4x4 grid
// ADD IN DIAGRAM TO SHOW HOW TO COME UP WITH THIS ***
typedef struct _piece {
    char name;
    color_t color;
    int block_rotations[4];
} piece_t;

extern const piece_t i;
extern const piece_t j;
extern const piece_t l;
extern const piece_t o;
extern const piece_t s;
extern const piece_t t;
extern const piece_t z;

extern const piece_t pieces[7];

typedef struct {
    piece_t pieceT;
    char rotation;  // direction of rotation (0 through 3)
    char x;
    char y;
    bool fallen;
} falling_piece_t;

falling_piece_t init_falling_piece(void);

// Required init 
void game_update_init(int nrows, int ncols);

typedef bool (*functionPtr)(int x, int y, color_t color); 

bool iterateThroughPieceSquares(falling_piece_t piece, functionPtr action);

bool checkIfValidMove(int x, int y, color_t color);

bool drawSquare(int x, int y, color_t color);

bool update_background(int x, int y, color_t color);

void wipe_screen(void);

bool move_down(falling_piece_t* piece);

void move_left(falling_piece_t* piece);

void move_right(falling_piece_t* piece);

void rotate(falling_piece_t* piece);