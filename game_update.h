#ifndef _GAME_UPDATE_H
#define _GAME_UPDATE_H

#include <stdbool.h>
#include "gl.h"

typedef struct {
    char name;
    color_t color;
    int block_rotations[4];
} piece_t;

extern const piece_t i, j, l, o, s, t, z;
extern const piece_t pieces[7];
extern piece_t nextFallingPiece;

typedef struct {
    piece_t pieceT;
    char rotation;  // direction of rotation (0 through 3)
    int x;
    int y;
    bool fallen;    // true/false to specify whether piece has fallen in its place
} falling_piece_t;

falling_piece_t init_falling_piece(void);

void game_update_init(int nrows, int ncols);

typedef bool (*functionPtr)(int x, int y, falling_piece_t* piece); 

bool iterateThroughPieceSquares(falling_piece_t* piece, functionPtr action);

static bool checkIfValidMove(int x, int y, falling_piece_t* piece);

static bool drawFallingSquare(int x, int y, falling_piece_t* piece);

static void drawFallenSquare(int x, int y, color_t color);

static void drawBevelLines(int x, int y, color_t color);

bool update_background(int x, int y, falling_piece_t* piece);

static void draw_background(void);

void swap(falling_piece_t* piece);

void move_down(falling_piece_t* piece);

void move_left(falling_piece_t* piece);

void move_right(falling_piece_t* piece);

void rotate(falling_piece_t* piece);

bool checkIfFallen(int x, int y, falling_piece_t* piece);

static void drawPiece(falling_piece_t* piece);

void endGame(void);

void startGame(void);

void pause(const char *message);

void clearRows(void);

int game_update_get_rows_cleared(void) ;

int game_update_get_score(void) ;

bool game_update_is_game_over(void) ;

bool iterateVariant(falling_piece_t* piece, functionPtr action);

#endif