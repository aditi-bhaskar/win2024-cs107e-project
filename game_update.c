/* game_update.c
* -----------------------------------
* Author: Anjali Sreenivas (anjalisr)
* 
* The game_update.c module consists of all the architecture and functions 
* underlying our game of Tetris, layering on top of the gl.c and fb.c modules.
*/

#include "game_update.h"
#include "malloc.h"
#include "strings.h"
#include "printf.h"
#include "timer.h"
#include "remote.h"
#include "uart.h"
#include "random_bag.h"
#include "passive_buzz_intr.h"
#include "LSD6DS33.h"
#include "console.h"

/* Define the 7 Tetris pieces as piece_t structs, laying out their name, color, and rotational configurations
Rotational configs are stored as hex numbers (bit representations). 
Here's an example of how it works for one 'j' piece configuration: 0x44C0 which is 0100 0100 1100 0000 in binary                                                                           
       8       4      2     1
    +------+------+------+------+
 0  |      |      |      |      |
    |      |   *  |      |      |     
    +------+------+------+------+
 1  |      |      |      |      |
    |      |   *  |      |      |     
    +------+------+------+------+
 2  |      |      |      |      |
    |  *   |   *  |      |      |     
    +------+------+------+------+
 3  |      |      |      |      |
    |      |      |      |      |     
    +------+------+------+------+
This setup supports iterating through the piece squares in a super fast and space-efficient manner! :)
*/
const piece_t i = {'i', 0x1AE6DC, {0x0F00, 0x2222, 0x00F0, 0x4444}};
const piece_t j = {'j', 0x0000E4, {0x44C0, 0x8E00, 0x6440, 0x0E20}};
const piece_t l = {'l', 0xEA9B11, {0x4460, 0x0E80, 0xC440, 0x2E00}};
const piece_t o = {'o', 0xE5E900, {0x6600, 0x6600, 0x6600, 0x6600}};
const piece_t s = {'s', 0x03E800, {0x06C0, 0x8C40, 0x6C00, 0x4620}};
const piece_t t = {'t', 0x9305E2, {0x0E40, 0x4C40, 0x4E00, 0x4640}};
const piece_t z = {'z', 0xE80201, {0x0C60, 0x4C80, 0xC600, 0x2640}};

const piece_t pieces[7] = {i, j, l, o, s, t, z};
piece_t nextFallingPiece;  

static struct {
    int nrows;
    int ncols;
    color_t bg_col;
    void* background_tracker; 
    int gameScore;
    int numLinesCleared; 
    bool gameOver;
} game_config;

const unsigned int SQUARE_DIM = 20;  // game square dimensions in pixels

// Required init 
void game_update_init(int nrows, int ncols) {
    if (game_config.background_tracker != NULL) free(game_config.background_tracker);
    game_config.nrows = nrows;
    game_config.ncols = ncols;
    game_config.bg_col = GL_INDIGO;
    game_config.gameScore = 0;
    game_config.numLinesCleared = 0;
    game_config.gameOver = false;

    int gridSize = game_config.nrows * game_config.ncols;
    game_config.background_tracker = malloc(gridSize * sizeof(color_t));
    memset(game_config.background_tracker, 0, gridSize * sizeof(color_t));

    random_bag_init();
    nextFallingPiece = pieces[random_bag_choose()];
    gl_init(game_config.ncols * SQUARE_DIM, game_config.nrows * SQUARE_DIM, GL_DOUBLEBUFFER);
    gl_clear(game_config.bg_col);
    gl_swap_buffer();
}

// Required init to construct and obtain a new falling piece
// Falling piece type is selected from the elements remaining in random bag
falling_piece_t init_falling_piece(void) {
    draw_background();

    falling_piece_t piece;
    piece.pieceT = nextFallingPiece;
    nextFallingPiece = pieces[random_bag_choose()];
    piece.rotation = 0;

    // Subtract half of each piece's 4x4 grid width from the board's center x-coordinate
    // (Representing each piece config as a hex value / bit sequence denotes the squares filled within a 4 x 4 grid) 
    piece.x = (game_config.ncols / 2) - 2;
    piece.y = 0;
    piece.fallen = false;

    // End game if new piece drawn from random bag is not valid (coordinates out of bounds); otherwise, return chosen piece
    if (!iterateThroughPieceSquares(&piece, checkIfValidMove)) endGame();
    else {
        iterateThroughPieceSquares(&piece, drawFallingSquare);
        gl_swap_buffer();
    }
    return piece;
}

// Helper function to check if swap attempt is valid; returns true/false
static bool isSwapValid(falling_piece_t piece) {
    falling_piece_t swapPiece;
    swapPiece.pieceT = nextFallingPiece;
    swapPiece.rotation = piece.rotation;
    swapPiece.x = piece.x;
    swapPiece.y = piece.y;
    swapPiece.fallen = false;

    if (iterateThroughPieceSquares(&swapPiece, checkIfValidMove)) return true;
    return false;
}

// Swap function to swap current falling game piece with next queued piece
void swap(falling_piece_t* piece) {
    if (isSwapValid(*piece)) {
        // make swap
        piece_t curr = piece->pieceT;
        piece->pieceT = nextFallingPiece;
        nextFallingPiece = curr;

        draw_background();
        iterateThroughPieceSquares(piece, drawFallingSquare);
        gl_swap_buffer();
    }
}

// Helper function to draw bevel lines within a square given its top left (x, y) cooridinate 
static void drawBevelLines(int x, int y, color_t color) {
    gl_draw_line(x * SQUARE_DIM + 1, y * SQUARE_DIM + 1, x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + 1, color);
    gl_draw_line(x * SQUARE_DIM + 1, y * SQUARE_DIM + 1, x * SQUARE_DIM + 1, y * SQUARE_DIM + SQUARE_DIM - 2, color);
    gl_draw_line(x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + SQUARE_DIM - 2, x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + 1, color);
    gl_draw_line(x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + SQUARE_DIM - 2, x * SQUARE_DIM + 1, y * SQUARE_DIM + SQUARE_DIM - 2, color);
}

// Helper to draw square of FALLEN tetris piece specified by top left coordinate (x, y) into 
// framebuffer (handled by gl / fb modules)
// Function only called after valid move is verified
static void drawFallenSquare(int x, int y, color_t color) {
    gl_draw_rect(x * SQUARE_DIM, y * SQUARE_DIM, SQUARE_DIM, SQUARE_DIM, color);
    drawBevelLines(x, y, GL_INDIGO);
}

// This is the magical function that is frequently called to apply an action (taken in as a functionPtr) to 
// each square in the tetris piece!  The coordinate locations of the squares are accessed using bitshifting!
// If for any square in the tetris piece the action returns false, this function returns false and terminates.
// If the action is successfully applied to all squares in the tetris piece, we return true.
bool iterateThroughPieceSquares(falling_piece_t* piece, functionPtr action) {
    int piece_config = (piece->pieceT).block_rotations[(int) piece->rotation];

    // pieceRow and pieceCol will change from 0 through 3 during loop
    // Indicates where on 4x4 piece grid we are checking
    int pieceRow = 0; int pieceCol = 0;  
    for (int bitSequence = 0x8000; bitSequence > 0; bitSequence = bitSequence >> 1) {
        // if piece has a square in the 4x4 grid spot we're checking, perform action fn on that square
        if (piece_config & bitSequence) {
            if (!action(piece->x + pieceCol, piece->y + pieceRow, piece)) return false;
        }
        pieceCol++;
        // move to next row in grid, reset pieceCol to 0
        if (pieceCol == 4) {
            pieceCol = 0; pieceRow++;
        }
    }
    return true;
}

// Helper function to check if moving a square to location (x, y) is valid
static bool checkIfValidMove(int x, int y, falling_piece_t* piece) {
    // make sure (x, y) is in bounds
    if (x < 0 || y < 0) return false;
    if (x >= game_config.ncols || y >= game_config.nrows) return false;

    // make sure another piece is not there already
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    if ((background[y][x]) != 0) return false;
    return true;
}

// Input (x, y) is the top left coordinate of tetris square being drawn; 
// function checks if square directly below is already filled --> if so, change falling piece state to fallen
bool checkIfFallen(int x, int y, falling_piece_t* piece) {
    if ((y + 1) >= game_config.nrows) {
        piece->fallen = true;
        return true;
    }
    else {
        unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
        if (background[y + 1][x] != 0) {
            piece->fallen = true;
            return true;
        }
        return false;
    }
}

// Helper to draw square of FALLING tetris piece specified by top left coordinate (x, y) into 
// framebuffer (handled by gl / fb modules)
// Returns true always -- function only called after valid move is verified
static bool drawFallingSquare(int x, int y, falling_piece_t* piece) {
    gl_draw_rect(x * SQUARE_DIM, y * SQUARE_DIM, SQUARE_DIM, SQUARE_DIM, piece->pieceT.color);
    
    drawBevelLines(x, y, GL_WHITE);
    checkIfFallen(x, y, piece);
    return true;
}

// Embeds square (of tetris piece) into background tracker
// Returns true always -- function only called after valid move is verified
bool update_background(int x, int y, falling_piece_t* piece) {
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    background[y][x] = piece->pieceT.color;
    return true;
}

// Variant of iterateThroughPieceSquares; here, if the action returns true on any piece square, this function stops
// and returns true. 
// Used in game loop client (located in testing.c) to check if a piece has fallen and support the "tuck" feature  
bool iterateVariant(falling_piece_t* piece, functionPtr action) {
    int piece_config = (piece->pieceT).block_rotations[(int) piece->rotation];

    // pieceRow and pieceCol will change from 0 through 3 during loop
    // Indicates where on 4x4 piece grid we are checking
    int pieceRow = 0; int pieceCol = 0;  
    for (int bitSequence = 0x8000; bitSequence > 0; bitSequence = bitSequence >> 1) {
        // if piece has a square in the 4x4 grid spot we're checking, perform action fn on that square
        if (piece_config & bitSequence) {
            if (action(piece->x + pieceCol, piece->y + pieceRow, piece)) return true;
        }
        pieceCol++;
        // move to next row in grid, reset pieceCol to 0
        if (pieceCol == 4) {
            pieceCol = 0; pieceRow++;
        }
    }
    return false;
}

// Clears and redraws screen according to what's stored in the background tracker 
// Called as prologue to every move/rotate function
static void draw_background(void) {
    gl_clear(game_config.bg_col);
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    for (int y = 0; y < game_config.nrows; y++) {
        for (int x = 0; x < game_config.ncols; x++) {
            // if colored square in background (from fallen piece), draw
            if (background[y][x] != 0) {
                // gl_draw_rect(x * SQUARE_DIM, y * SQUARE_DIM, SQUARE_DIM, SQUARE_DIM, background[y][x]);
                drawFallenSquare(x, y, background[y][x]);
            }
        }
    }
    // Draw in top right corner the color of next piece to fall
    gl_draw_rect((game_config.ncols - 1) * SQUARE_DIM, 0, SQUARE_DIM, SQUARE_DIM, nextFallingPiece.color);

    // Draw score (top left of screen)
    char buf[20];
    int bufsize = sizeof(buf);
    memset(buf, '\0', bufsize);
    snprintf(buf, bufsize, "SCORE %d", game_config.gameScore);
    gl_draw_string(0, 0, buf, GL_WHITE);
}

// Helper function to clear a single row, specified by the row number (y coordinate)
static void clearRow(int row) {
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    for (int col = 0; col < game_config.ncols; col++) {
        background[row][col] = 0;
    }
    draw_background();
    gl_swap_buffer();
    timer_delay_ms(500);

    for (int destRow = row; destRow > 0; destRow--) {
        for (int col = 0; col < game_config.ncols; col++) {
            background[destRow][col] = background[destRow - 1][col];
        }
    }
    // reset 1st row of background 
    memset(background, 0, game_config.ncols * sizeof(color_t));
    draw_background();
    gl_swap_buffer();
}

// Function to clear rows and update game score accordingly
void clearRows(void) {
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    int rowsFilled = 0;
    for (int row = 0; row < game_config.nrows; row++) {
        bool rowFilled = true;
        for (int col = 0; col < game_config.ncols; col++) {
            // if we find an empty square, the row is not filled
            if (background[row][col] == 0) {
                rowFilled = false;
                break;
            }
        }
        if (rowFilled) {
            clearRow(row); 
            remote_vibrate(2); // remote_vibrate(rowsFilled + 1);
            buzzer_intr_set_tempo(buzzer_intr_get_tempo() + 2) ;
            game_config.numLinesCleared++ ;
            rowsFilled++;
        }
    }
    if (rowsFilled == 1) game_config.gameScore += 40;
    else if (rowsFilled == 2) game_config.gameScore += 100;
    else if (rowsFilled == 3) game_config.gameScore += 300;
    else if (rowsFilled == 4) game_config.gameScore += 1200;
}

// Helper to draw falling tetris piece
static void drawPiece(falling_piece_t* piece) {
    draw_background();
    iterateThroughPieceSquares(piece, drawFallingSquare);
    gl_swap_buffer();
}

// These next functions are move and rotate functions which do nothing for an invalid move 
void move_down(falling_piece_t* piece) {
    piece->y += 1;
    if (!iterateThroughPieceSquares(piece, checkIfValidMove)) {
        piece->y -= 1;
        return;
    };
    drawPiece(piece);
}

void move_left(falling_piece_t* piece) {
    piece->x -= 1;
    if (!iterateThroughPieceSquares(piece, checkIfValidMove)) {
        piece->x += 1;
        return;
    };
    drawPiece(piece);
}

void move_right(falling_piece_t* piece) {
    piece->x += 1;
    if (!iterateThroughPieceSquares(piece, checkIfValidMove)) {
        piece->x -= 1;
        return;
    };
    drawPiece(piece);
}

void rotate(falling_piece_t* piece) {
    char origRotation = piece->rotation;
    piece->rotation = (origRotation + 1) % 4;
    if (!iterateThroughPieceSquares(piece, checkIfValidMove)) {
        piece->rotation = origRotation;
        return;
    };
    drawPiece(piece);
}

// Getters 
int game_update_get_rows_cleared(void) {
    return game_config.numLinesCleared;
}

int game_update_get_score(void) {
    return game_config.gameScore;
}

bool game_update_is_game_over(void) {
    return game_config.gameOver;
}

// Draw game start screen
void startGame(void) {
    gl_clear(game_config.bg_col);

    // Draw text
    gl_draw_string(2 * SQUARE_DIM, 2 * SQUARE_DIM, "TILTRIS!", 0xCB4899);
    gl_draw_string(SQUARE_DIM / 5, 5 * SQUARE_DIM, "Button: On/Off", 0xf9d740);
    gl_draw_string(6 * SQUARE_DIM, 6 * SQUARE_DIM, "Music", 0xf9d740);
    gl_draw_string(SQUARE_DIM / 2, 8 * SQUARE_DIM, "Tilt to Play!", 0x219756);

    // DRAW 107 MANGO
    // Draw 1 (as i piece)
    drawFallenSquare(0, 15, i.color);
    drawFallenSquare(0, 16, i.color);
    drawFallenSquare(0, 17, i.color);
    drawFallenSquare(0, 18, i.color);

    // Draw 0 as (2 l pieces)
    drawFallenSquare(1, 16, l.color);
    drawFallenSquare(2, 16, l.color);
    drawFallenSquare(3, 16, l.color);
    drawFallenSquare(1, 17, l.color);
    drawFallenSquare(3, 17, l.color);
    drawFallenSquare(1, 18, l.color);
    drawFallenSquare(2, 18, l.color);
    drawFallenSquare(3, 18, l.color);

    // Draw 7 (as j and t pieces)
    drawFallenSquare(3, 14, j.color);
    drawFallenSquare(4, 14, j.color);
    drawFallenSquare(5, 14, j.color);
    drawFallenSquare(5, 15, j.color);
    drawFallenSquare(5, 16, t.color);  
    drawFallenSquare(5, 17, t.color);
    drawFallenSquare(5, 18, t.color);  
    drawFallenSquare(4, 17, t.color);  

    // Draw mango (as o and z pieces)
    drawFallenSquare(8, 17, o.color);
    drawFallenSquare(9, 17, o.color);  
    drawFallenSquare(8, 18, o.color);  
    drawFallenSquare(9, 18, o.color);  
    drawFallenSquare(7, 15, s.color);   
    drawFallenSquare(8, 15, s.color); 
    drawFallenSquare(8, 16, s.color); 
    drawFallenSquare(9, 16, s.color); 

    gl_swap_buffer();

    // Wait for downward tilt of remote
    timer_delay(2) ;
    int pitch = 0 ; int roll = 0 ;
    remote_get_x_y_status(&pitch, &roll) ;
    while (pitch != X_FAST) { 
        remote_get_x_y_status(&pitch, &roll) ; 
        if (remote_is_button_press()) {
            buzzer_intr_is_playing() ? buzzer_intr_pause() : buzzer_intr_play() ;
        }
    }

}

// End game screen
void endGame(void) {
    draw_background();
    char buf[20];
    int bufsize = sizeof(buf);
    snprintf(buf, bufsize, " GAME OVER ");
    gl_draw_string(SQUARE_DIM, game_config.ncols / 2 * SQUARE_DIM, buf, GL_WHITE);
    gl_swap_buffer();
    game_config.gameOver = true;
}

// uart-driven pause function - helpful for testing purposes
void pause(const char *message) {
    if (message) printf("\n%s\n", message);
    printf("[PAUSED] type any key in minicom/terminal to continue: ");
    int ch = uart_getchar();
    uart_putchar(ch);
    uart_putchar('\n');
}