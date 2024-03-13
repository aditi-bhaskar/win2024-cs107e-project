#include "game_update.h"
#include "malloc.h"
#include "strings.h"
#include "printf.h"
#include "timer.h"
#include "remote.h"
#include "uart.h"

const piece_t i = {'i', 0x1AE6DC, {0x0F00, 0x2222, 0x00F0, 0x4444}};
const piece_t j = {'j', 0x0000E4, {0x44C0, 0x8E00, 0x6440, 0x0E20}};
const piece_t l = {'l', 0xEA9B11, {0x4460, 0x0E80, 0xC440, 0x2E00}};
const piece_t o = {'o', 0xE5E900, {0x6600, 0x6600, 0x6600, 0x6600}};
const piece_t s = {'s', 0x03E800, {0x06C0, 0x8C40, 0x6C00, 0x4620}};
const piece_t t = {'t', 0x9305E2, {0x0E40, 0x4C40, 0x4E00, 0x4640}};
const piece_t z = {'z', 0xE80201, {0x0C60, 0x4C80, 0xC600, 0x2640}};

const piece_t pieces[7] = {i, j, l, o, s, t, z};

static struct {
    int nrows;
    int ncols;
    color_t bg_col;
    void* background_tracker; 
    int gameScore;
    int numLinesCleared; 
} game_config;

const unsigned int SQUARE_DIM = 20; // game pixel dimensions

// Required init 
void game_update_init(int nrows, int ncols) {
    if (game_config.background_tracker != NULL) free(game_config.background_tracker);
    game_config.nrows = nrows;
    game_config.ncols = ncols;
    game_config.bg_col = GL_INDIGO;
    game_config.gameScore = 0;
    game_config.numLinesCleared = 0;

    int gridSize = game_config.nrows * game_config.ncols;
    game_config.background_tracker = malloc(gridSize * sizeof(color_t));
    memset(game_config.background_tracker, 0, gridSize * sizeof(color_t));

    gl_init(game_config.ncols * SQUARE_DIM, game_config.nrows * SQUARE_DIM, GL_DOUBLEBUFFER);
    gl_clear(game_config.bg_col);
    gl_swap_buffer();
}

falling_piece_t init_falling_piece(void) {
    wipe_screen();

    falling_piece_t piece;
    piece.pieceT = pieces[timer_get_ticks() % 7]; 
    piece.rotation = 0;
    // subtract half of 4x4 grid width from board center x coordinate since bits sequence denotes 4x4 grid 
    piece.x = (game_config.ncols / 2) - 2;
    piece.y = 0;
    piece.fallen = false;

    if (!iterateThroughPieceSquares(&piece, checkIfValidMove)) endGame();
    else {
        iterateThroughPieceSquares(&piece, drawSquare);
        gl_swap_buffer();
    }
    return piece;
}

void pause(const char *message) {
    if (message) printf("\n%s\n", message);
    printf("[PAUSED] type any key in minicom/terminal to continue: ");
    int ch = uart_getchar();
    uart_putchar(ch);
    uart_putchar('\n');
}

void endGame(void) {
    wipe_screen();
    char buf[20];
    int bufsize = sizeof(buf);
    snprintf(buf, bufsize, " GAME OVER ");
    gl_draw_string(SQUARE_DIM, game_config.ncols / 2 * SQUARE_DIM, buf, GL_WHITE);
    gl_swap_buffer();
    pause("END GAME");
}
//////////////// STATICS 
// typedef bool (*functionPtr)(int x, int y, color_t color); 

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

bool checkIfValidMove(int x, int y, falling_piece_t* piece) {
    // make sure (x, y) is in bounds
    if (x < 0 || y < 0) return false;
    if (x >= game_config.ncols || y >= game_config.nrows) return false;

    // make sure another piece is not there already
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    if ((background[y][x]) != 0) return false;
    return true;
}

// Draws square (of tetris piece) specified by top left coordinate (x, y) into 
// framebuffer (handled by gl / fb modules)
// Returns true always -- function only called after valid move is verified
bool drawSquare(int x, int y, falling_piece_t* piece) {
    gl_draw_rect(x * SQUARE_DIM, y * SQUARE_DIM, SQUARE_DIM, SQUARE_DIM, piece->pieceT.color);
    
    //bevel
    // todo update the color; todo update the bevels to have the diag lines
    gl_draw_line(x * SQUARE_DIM + 1, y *SQUARE_DIM + 1, x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + 1, GL_WHITE) ;
    gl_draw_line(x * SQUARE_DIM + 1, y * SQUARE_DIM + 1, x * SQUARE_DIM + 1, y * SQUARE_DIM + SQUARE_DIM - 2, GL_WHITE) ;
    gl_draw_line(x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + SQUARE_DIM - 2, x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + 1, GL_WHITE) ;
    gl_draw_line(x * SQUARE_DIM + SQUARE_DIM - 2, y * SQUARE_DIM + SQUARE_DIM - 2, x * SQUARE_DIM + 1, y * SQUARE_DIM + SQUARE_DIM - 2, GL_WHITE) ;

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

// Input (x, y) top left coordinate of tetris square being drawn; 
// check if square directly is already filled --> if so, change piece state to fallen
void checkIfFallen(int x, int y, falling_piece_t* piece) {
    if ((y + 1) >= game_config.nrows) piece->fallen = true;
    else {
        unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
        if (background[y + 1][x] != 0) piece->fallen = true;
    }
}

// Called as prologue to every move/rotate function
// Sets up screen according to background tracker 
void wipe_screen(void) {
    gl_clear(game_config.bg_col);
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    for (int y = 0; y < game_config.nrows; y++) {
        for (int x = 0; x < game_config.ncols; x++) {
            // if colored square in background (from fallen piece), draw
            if (background[y][x] != 0) {
                gl_draw_rect(x * SQUARE_DIM, y * SQUARE_DIM, SQUARE_DIM, SQUARE_DIM, background[y][x]);
            }
        }
    }


    char buf[20];
    int bufsize = sizeof(buf);
    memset(buf, '\0', bufsize);
    snprintf(buf, bufsize, "SCORE %d", game_config.gameScore);
    gl_draw_string(0, 0, buf, GL_WHITE);
}

void clearRow(int row) {
    unsigned int (*background)[game_config.ncols] = game_config.background_tracker;
    for (int col = 0; col < game_config.ncols; col++) {
        background[row][col] = 0;
    }
    wipe_screen();
    gl_swap_buffer();
    timer_delay_ms(500);

    for (int destRow = row; destRow > 0; destRow--) {
        for (int col = 0; col < game_config.ncols; col++) {
            background[destRow][col] = background[destRow - 1][col];
        }
    }
    // reset 1st row of background 
    memset(background, 0, game_config.ncols * sizeof(color_t));
    wipe_screen();
    gl_swap_buffer();
}

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
            remote_vibrate(2);
            game_config.numLinesCleared++ ;
            rowsFilled++;
        }
    }
    if (rowsFilled == 1) game_config.gameScore += 40;
    else if (rowsFilled == 2) game_config.gameScore += 100;
    else if (rowsFilled == 3) game_config.gameScore += 300;
    else if (rowsFilled == 4) game_config.gameScore += 1200;
}

void drawPiece(falling_piece_t* piece) {
    wipe_screen();
    iterateThroughPieceSquares(piece, drawSquare);
    gl_swap_buffer();
    if (piece->fallen) {
        iterateThroughPieceSquares(piece, update_background);
        clearRows();
    }
}
///////

// Move and rotate functions do nothing for invalid move 
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

int get_rows_cleared(void) {
    return game_config.numLinesCleared ;
}
