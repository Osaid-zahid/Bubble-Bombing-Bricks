/*
 * Q8: Bubble Bombing Bricks (Triple B)
 *
 * A terminal-based brick-breaker game using ANSI escape codes for color
 * and raw terminal input for real-time control.
 *
 * Controls:
 *   A / D   - move cannon left / right
 *   Q       - aim left diagonal
 *   W       - aim straight up (default)
 *   E       - aim right diagonal
 *   SPACE   - fire (costs 1 ammo)
 *   ESC     - quit
 *
 * Brick health: 3 = '#'  2 = '='  1 = '-'  0 = ' ' (destroyed)
 * Ammo starts at 50. Win = clear all bricks. Lose = run out of ammo.
 *
 * Allowed libraries: iostream, unistd.h, termios.h, cstdlib
 */

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <cstdlib>
using namespace std;

// ============================================================
// Terminal setup (from starter code - not to be modified)
// ============================================================

struct termios original_termios;

void restoreTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

void hideCursor() {
    cout << "\033[?25l";
}

void showCursor() {
    cout << "\033[?25h";
}

// Non-blocking single character read (getch imitation)
char getch_linux() {
    static bool first_call = true;
    if (first_call) {
        tcgetattr(STDIN_FILENO, &original_termios);
        atexit(restoreTerminal);
        atexit(showCursor);

        struct termios t = original_termios;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        first_call = false;
    }
    char c = 0;
    read(STDIN_FILENO, &c, 1);
    return c;
}

// ============================================================
// Constants
// ============================================================
const int SCREEN_H = 25;
const int SCREEN_W = 48;
const int CANNON_Y = 23;   // Row where the cannon sits
const int BRICK_ROWS = 2;  // Number of brick rows
const int BRICK_COLS = 9;  // Number of bricks per row

// ============================================================
// ANSI color helper
// Fills anci[] with the ANSI foreground color code for a brick
// based on its row and remaining health.
// ============================================================
void getBrickColor(int y, int x, char anci[], int bricks[BRICK_ROWS][BRICK_COLS][4]) {
    int row   = y - 1;          // brick row (0 or 1)
    int col   = (x - 6) / 4;   // brick column index
    int hp    = bricks[row][col][0];
    bool alt  = (col & 1);      // alternate color bands

    // Color selection: red/yellow row-A, dark-red/gold row-B
    if (alt) {
        if      (hp == 3) { anci[0]='\033';anci[1]='[';anci[2]='9';anci[3]='1';anci[4]='m';anci[5]='\0'; } // bright red
        else if (hp == 2) { anci[0]='\033';anci[1]='[';anci[2]='3';anci[3]='3';anci[4]='m';anci[5]='\0'; } // yellow
        else if (hp == 1) { anci[0]='\033';anci[1]='[';anci[2]='3';anci[3]='2';anci[4]='m';anci[5]='\0'; } // green
        else              { anci[0]='\033';anci[1]='[';anci[2]='3';anci[3]='7';anci[4]='m';anci[5]='\0'; } // white
    } else {
        if      (hp == 3) { anci[0]='\033';anci[1]='[';anci[2]='3';anci[3]='1';anci[4]='m';anci[5]='\0'; } // dark red
        else if (hp == 2) { anci[0]='\033';anci[1]='[';anci[2]='9';anci[3]='3';anci[4]='m';anci[5]='\0'; } // bright yellow
        else if (hp == 1) { anci[0]='\033';anci[1]='[';anci[2]='3';anci[3]='2';anci[4]='m';anci[5]='\0'; } // green
        else              { anci[0]='\033';anci[1]='[';anci[2]='3';anci[3]='7';anci[4]='m';anci[5]='\0'; } // white
    }
}

// ============================================================
// Initialize all bricks to full health (3)
// ============================================================
void initializeBricks(int bricks[BRICK_ROWS][BRICK_COLS][4]) {
    for (int k = 0; k < BRICK_ROWS; k++)
        for (int i = 0; i < BRICK_COLS; i++)
            for (int j = 0; j < 4; j++)
                bricks[k][i][j] = 3;
}

// ============================================================
// Write brick characters into the screen buffer based on health
// ============================================================
void renderBricks(int bricks[BRICK_ROWS][BRICK_COLS][4], char screen[SCREEN_H][SCREEN_W + 1]) {
    for (int k = 0; k < BRICK_ROWS; k++) {
        for (int i = 0; i < BRICK_COLS; i++) {
            for (int j = 0; j < 4; j++) {
                char ch;
                if      (bricks[k][i][j] == 3) ch = '#';
                else if (bricks[k][i][j] == 2) ch = '=';
                else if (bricks[k][i][j] == 1) ch = '-';
                else                            ch = ' ';
                screen[k + 1][i * 4 + 6 + j] = ch;
            }
        }
    }
}

// ============================================================
// Handle ball hitting a brick: reduce all 4 sub-cells by 1,
// award 10 points, and bounce the ball upward.
// ============================================================
void checkBrickHit(int bricks[BRICK_ROWS][BRICK_COLS][4],
                   int ballX, int ballY,
                   char screen[SCREEN_H][SCREEN_W + 1],
                   bool &movingUp, int &score) {
    int row = ballY - 1;
    int col = (ballX - 6) / 4;
    if (bricks[row][col][0] > 0) {
        for (int i = 0; i < 4; i++)
            bricks[row][col][i]--;
        score   += 10;
        movingUp = false; // bounce: reverse vertical direction
    }
}

// ============================================================
// Return true if any brick still has health > 0
// ============================================================
bool bricksRemaining(int bricks[BRICK_ROWS][BRICK_COLS][4]) {
    for (int k = 0; k < BRICK_ROWS; k++)
        for (int i = 0; i < BRICK_COLS; i++)
            if (bricks[k][i][0] > 0) return true;
    return false;
}

// ============================================================
// Render the screen buffer to the terminal using ANSI cursor
// movement to avoid flicker.
// ============================================================
void renderScreen(char screen[SCREEN_H][SCREEN_W + 1],
                  int bricks[BRICK_ROWS][BRICK_COLS][4]) {
    char anci[10];
    cout << "\033[H"; // move cursor to top-left
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            // Color brick cells
            if (y > 0 && y < 3 && x >= 6 && x <= SCREEN_W - 7)
                getBrickColor(y, x, anci, bricks);
            else {
                // Reset to default
                anci[0]='\033'; anci[1]='['; anci[2]='0'; anci[3]='m'; anci[4]='\0';
            }
            cout << anci << screen[y][x];
        }
        if (y < SCREEN_H - 1) cout << '\n';
    }
    // Reset color at the end
    cout << "\033[0m";
}

// ============================================================
// Main game loop
// ============================================================
void runGame(char screen[SCREEN_H][SCREEN_W + 1],
             int &score, int &ammo, bool &win) {
    int cannonX = SCREEN_W / 2;
    int orient  = 1;          // 0=left, 1=straight, 2=right
    int ballX   = cannonX;
    int ballY   = SCREEN_H - 2;
    int hDir    = 0;          // horizontal: -1=right, +1=left, 0=straight
    bool movingUp  = true;
    bool ballActive = false;
    bool firstFrame = true;
    bool running    = true;

    int bricks[BRICK_ROWS][BRICK_COLS][4];
    initializeBricks(bricks);

    while (running) {
        // --- Input ---
        char c = getch_linux();
        if      (c == 'a' || c == 'A') { if (cannonX - 2 > 0)           cannonX--; }
        else if (c == 'd' || c == 'D') { if (cannonX + 2 < SCREEN_W - 1) cannonX++; }
        else if (c == 'q') orient = 0;
        else if (c == 'w') orient = 1;
        else if (c == 'e') orient = 2;
        else if (c == ' ' && !ballActive && ammo > 0) {
            ballX   = cannonX;
            ballY   = CANNON_Y + 1;
            movingUp   = true;
            firstFrame = true;
            ballActive = true;
            hDir = (orient == 1) ? 0 : (orient == 2) ? -1 : 1;
            ammo--;
        }
        else if (c == '\033') running = false;

        // --- Clear screen buffer ---
        for (int y = 0; y < SCREEN_H; y++) {
            for (int x = 0; x < SCREEN_W; x++) screen[y][x] = ' ';
            screen[y][SCREEN_W] = '\0';
        }

        // --- Draw border ---
        for (int y = 0; y < SCREEN_H; y++) {
            if (y == 0 || y == SCREEN_H - 1) {
                for (int x = 0; x < SCREEN_W; x++) screen[y][x] = '#';
            } else {
                screen[y][0]          = '#';
                screen[y][SCREEN_W-1] = '#';
            }
        }

        // --- Ball physics ---
        if (ballY == 1) movingUp = false; // top wall bounce

        // Ball reaches the bottom: deactivate
        if (ballActive && ballY == SCREEN_H - 2) {
            if (firstFrame) firstFrame = false;
            else ballActive = false;
        }

        // Side wall bounces
        if (ballX == 1)            hDir = -1;
        else if (ballX == SCREEN_W-2) hDir =  1;

        // Corner bumper posts
        if (ballY == SCREEN_H - 9) {
            if (ballX == 11           && hDir ==  1) hDir = -1;
            if (ballX == SCREEN_W-12  && hDir == -1) hDir =  1;
        }

        // Move ball
        if (ballActive) {
            if (movingUp)  ballY--;
            else           ballY++;
            if (hDir ==  1) ballX--;
            else if (hDir == -1) ballX++;
        }

        // --- Draw bumper posts ---
        screen[SCREEN_H - 9][11]          = '|';
        screen[SCREEN_H - 9][SCREEN_W-12] = '|';

        // --- Draw cannon ---
        if      (orient == 1) { screen[CANNON_Y][cannonX-1]='|'; screen[CANNON_Y][cannonX]=' '; screen[CANNON_Y][cannonX+1]='|'; }
        else if (orient == 0) { screen[CANNON_Y][cannonX-1]='\\';screen[CANNON_Y][cannonX]=' '; screen[CANNON_Y][cannonX+1]='\\'; }
        else if (orient == 2) { screen[CANNON_Y][cannonX-1]='/'; screen[CANNON_Y][cannonX]=' '; screen[CANNON_Y][cannonX+1]='/'; }

        // --- HUD: score and ammo ---
        // Score label
        screen[SCREEN_H-1][5]='S'; screen[SCREEN_H-1][6]='c'; screen[SCREEN_H-1][7]='o';
        screen[SCREEN_H-1][8]='r'; screen[SCREEN_H-1][9]='e'; screen[SCREEN_H-1][10]=':';
        screen[SCREEN_H-1][11] = score / 100       + '0';
        screen[SCREEN_H-1][12] = (score / 10) % 10 + '0';
        screen[SCREEN_H-1][13] = score % 10        + '0';
        // Ammo label
        screen[SCREEN_H-1][25]='A'; screen[SCREEN_H-1][26]='m'; screen[SCREEN_H-1][27]='m';
        screen[SCREEN_H-1][28]='o'; screen[SCREEN_H-1][29]=':';
        screen[SCREEN_H-1][30] = ammo / 100       + '0';
        screen[SCREEN_H-1][31] = (ammo / 10) % 10 + '0';
        screen[SCREEN_H-1][32] = ammo % 10        + '0';

        // --- Check brick collision (ball in brick zone) ---
        if (ballActive && ballY > 0 && ballY < 3 && ballX >= 6 && ballX <= SCREEN_W - 7)
            checkBrickHit(bricks, ballX, ballY, screen, movingUp, score);

        // --- Draw bricks and ball ---
        renderBricks(bricks, screen);
        if (ballActive) screen[ballY][ballX] = 'O';

        // --- Render to terminal ---
        renderScreen(screen, bricks);

        usleep(16000); // ~60 FPS

        // --- Win / Lose conditions ---
        bool bricksLeft = bricksRemaining(bricks);
        if (!bricksLeft) { running = false; win = true;  }
        if (bricksLeft && ammo == 0) { running = false; win = false; }
    }
}

int main() {
    char screen[SCREEN_H][SCREEN_W + 1];
    char anci[10];
    int  score = 0, ammo = 50;
    bool running, win = false;

    system("clear");
    hideCursor();

    runGame(screen, score, ammo, win);

    system("clear");

    // --- End screen ---
    for (int i = 0; i < 10; i++) cout << '\n';
    if (win) {
        cout << "                      You win!\n";
        cout << "                      Final Score: " << score << '\n';
    } else {
        cout << "                      You lost.\n";
        cout << "                      Final Score: " << score << '\n';
    }
    for (int i = 0; i < 10; i++) cout << '\n';

    return 0;
}
