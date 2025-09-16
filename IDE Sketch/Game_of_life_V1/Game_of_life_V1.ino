/*
 * =====================================================================
 *                    CONWAY'S GAME OF LIFE
 *                     Arduino Implementation
 * =====================================================================
 * 
 * A complete implementation of John Conway's famous cellular automaton
 * "Game of Life" for Arduino microcontrollers with SSD1306 OLED display.
 * 
 * FEATURES:
 * • Three game modes: Random generation, Custom builder, Preset patterns
 * • 12 speed levels with real-time speed control
 * • 5 famous Conway patterns including Gosper Glider Gun
 * • Interactive pattern editor with blinking cursor
 * • Game statistics tracking (generations, max cells)
 * • Menu system with pattern submenu
 * • Automatic pattern detection and game reset
 * • Battery-optimized resistor ladder button input
 * 
 * HARDWARE REQUIREMENTS:
 * • Arduino-compatible microcontroller (tested on Trinket M0)
 * • SSD1306 128x64 OLED display (I2C)
 * • 5-button resistor ladder on analog pin A3
 * • Optional: Button B on analog pin A4 (menu return)
 * 
 * BUTTON MAPPING (Resistor Ladder on A3):
 * • UP: ~39kΩ to 5V    (Navigation/Cursor up/Speed up)
 * • DOWN: ~33kΩ to 5V  (Navigation/Cursor down/Speed down) 
 * • LEFT: ~20kΩ to 5V  (Cursor left/New random/Back in menu)
 * • RIGHT: ~10kΩ to 5V (Cursor right/New random)
 * • SET: ~5.1kΩ to 5V  (Select/Toggle cell/Start simulation)
 * • B: ~15kΩ to 5V   (Return to main menu from anywhere)
 * 
 * GAME MODES:
 * 1. Random Game: Starts with random cell pattern, auto-resets on death
 * 2. Custom Game: Build your own pattern with cursor, long-press SET to start
 * 3. Preset Patterns: Choose from 5 famous Conway patterns
 * 
 * CONTROLS:
 * • Menu: UP/DOWN navigate, SET selects, LEFT goes back in submenu
 * • Edit: Arrow keys move cursor, SET toggles cell, long-press SET starts
 * • Simulation: UP/DOWN changes speed, LEFT/RIGHT generates new random
 * • Any mode: Button B returns to main menu
 * 
 * DISPLAY:
 * • 64x32 cell grid displayed at 2x scale on 128x64 OLED
 * • Real-time generation counter and cell statistics
 * • Blinking cursor in edit mode
 * • Automatic game over detection and statistics display
 * 
 * ALGORITHM:
 * • Optimized bit-manipulation implementation
 * • Toroidal (wraparound) boundary conditions
 * • Hash-based pattern repetition detection
 * • Efficient neighbor counting using lookup table
 * 
 * AUTHOR: Marcus Dunn
 * VERSION: 1.0
 * DATE: [12/9/2025
 * LICENSE: Open Source - feel free to modify and share
 * 
 * Based on John Conway's "Game of Life" cellular automaton (1970)
 * 
 * Build: """ __DATE__ " " __TIME__ """
 * 
 * =====================================================================
 */

#pragma message (" game_of_life_v1 " 14/9/2025 " ")
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Constants ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BOARD_WIDTH 64
#define BOARD_HEIGHT 32

#define SCALE 2

#define I2C_ADDR 0x3C

// how often to take a hash
#define SHORT_HASH_INTERVAL 6
#define LONG_HASH_INTERVAL 256

// fraction of 32767 to randomly reset if long hash matches
#define LONG_HASH_THRESHOLD 328

#define DEBOUNCE_MS 50 // Debounce value for button presses

// Button input pin using resistor ladder
#define BUTTON_INPUT_PIN A3

// Button value thresholds for resistor ladder
#define BTN_UP_VAL    38
#define BTN_DOWN_VAL  45
#define BTN_LEFT_VAL  73
#define BTN_RIGHT_VAL 139
#define BTN_SET_VAL   250
#define BTN_UP_TOLERANCE 10
#define BTN_DOWN_TOLERANCE 5
#define BTN_RIGHT_TOLERANCE 20
#define BTN_SET_TOLERANCE 20
#define BTN_LEFT_TOLERANCE 10

#define RAND_LONG() ((rand() << 16) ^ rand())

// --- Global Variables and Objects ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint32_t board[BOARD_WIDTH];
uint32_t first_col;
uint32_t this_col;
uint32_t cellCount = 0; // Add cellCount variable declaration
uint32_t last_col;

uint32_t short_check_hash = 0;
uint32_t long_check_hash = 0;
uint32_t generation = 0;
uint32_t start_t = 0;
uint32_t frame_start_t = 0;
uint32_t game_hash = 0;  // Global hash variable for game reset
// Statistics variables
uint32_t max_cellCount = 0;

// Game state variables
bool showingStats = false;
unsigned long statsStartTime = 0;

// Variables for control
unsigned long last_button_press_time = 0;
unsigned long last_button_check_time = 0;
int currentSpeedIndex = 0;
int last_button_value = 0;
bool buttonProcessingInProgress = false;
int cursorX = BOARD_WIDTH / 2;
int cursorY = BOARD_HEIGHT / 2;
bool editMode = true;
bool inMenuMode = true;
int menuSelection = 1; // Default to first option
// Game modes
bool inPatternsSubMenu = false;
int patternSelection = 1; // Default pattern selection
int gameMode = 1; // 1 = Random game, 2 = Build your own
const int frameDelays[] = {500, 400, 300, 250, 200, 150, 100, 75, 50, 40, 30, 25};
const int numSpeeds = sizeof(frameDelays) / sizeof(frameDelays[0]);

static uint8_t bit_counts[] = {
    0,  // 0
    1,  // 1
    1,  // 2
    2,  // 3
    1,  // 4
    2,  // 5
    2,  // 6
    3   // 7
};

// --- Function Prototypes ---
void randomizeBoard();
void drawCell(int, int, int);
void collectRandomness(int);
void drawTitle();
void resetGame(uint32_t, uint32_t);
static inline uint32_t rotate_shift_three(uint32_t x, int n);
void handleDirectionalButtons();
void handleSetButton();
void handleButtonB();
void flashLED();
void drawCursor();
void clearBoard();
int readButtonValue();
bool isBtnUp(int val);
bool isBtnDown(int val);
bool isBtnLeft(int val);
bool isBtnRight(int val);
bool isBtnSet(int val);
 void showMenu();
void showGameStatistics();
void startShowingStats();
void loadPresetPattern(int patternId);

// --- Button Functions ---
int readButtonValue() {
    return analogRead(BUTTON_INPUT_PIN);
}

bool isBtnUp(int val) {
    return (val >= 38 && val <= 40);
}

bool isBtnDown(int val) {
    return (val >= 44 && val <= 47);
}

bool isBtnLeft(int val) {
    return (val >= 74 && val <= 75);
}

bool isBtnRight(int val) {
    return (val >= 141 && val <= 143);
}

bool isBtnSet(int val) {
    return (val >= 200 && val <= 300);  // Much wider range
}

  bool isBtnB(int val) {
    return (val >= 1020 && val <= 1023);  // Button B threshold
}

// --- Game Functions ---
void toggleCellAtCursor() {
    uint32_t mask = 1UL << cursorY;
    board[cursorX] ^= mask;
}

void drawCursor() {
    bool cellIsLit = (board[cursorX] & (1UL << cursorY)) != 0;
    bool blinkState = (millis() / 300) % 2;  // Slower blink, every 300ms
    
    if (blinkState) {
// Reversed cursor - empty when no cell, lit when cell exists
        int color = cellIsLit ? WHITE : BLACK;
        display.drawPixel(cursorX * SCALE, cursorY * SCALE, color);
        
        // Add a simple border around cursor position
        if (cursorX > 0) display.drawPixel((cursorX-1) * SCALE, cursorY * SCALE, WHITE);
        if (cursorX < BOARD_WIDTH-1) display.drawPixel((cursorX+1) * SCALE, cursorY * SCALE, WHITE);
        if (cursorY > 0) display.drawPixel(cursorX * SCALE, (cursorY-1) * SCALE, WHITE);
        if (cursorY < BOARD_HEIGHT-1) display.drawPixel(cursorX * SCALE, (cursorY+1) * SCALE, WHITE);
    }
}

void randomizeBoard() {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        uint32_t col;
        col = board[x] = RAND_LONG() & RAND_LONG();
    }
}

void drawCell(int x, int y, int c) {
    display.drawPixel(x * SCALE, y * SCALE, c);
}

void collectRandomness(int t) {
    uint32_t wait_t = millis();
    uint32_t r = 0;
    while (wait_t + t > millis()) {
        r += analogRead(A4);
    }
    randomSeed(r);
}

void loadPresetPattern(int patternId) {
    clearBoard();
    int centerX = BOARD_WIDTH / 2;
    int centerY = BOARD_HEIGHT / 2;
    
    switch (patternId) {
            case 1: // Coe Ship
    // Row 1: 4 empty, 6 cells
    board[centerX+4] |= (1UL << (centerY-4));
    board[centerX+5] |= (1UL << (centerY-4));
    board[centerX+6] |= (1UL << (centerY-4));
    board[centerX+7] |= (1UL << (centerY-4));
    board[centerX+8] |= (1UL << (centerY-4));
    board[centerX+9] |= (1UL << (centerY-4));
    
    // Row 2: 2 empty, 2 cells, 5 empty, 1 cell
    board[centerX+2] |= (1UL << (centerY-3));
    board[centerX+3] |= (1UL << (centerY-3));
    board[centerX+9] |= (1UL << (centerY-3));
    
    // Row 3: 2 cells, 1 empty, 1 cell, 5 empty, 1 cell
    board[centerX] |= (1UL << (centerY-2));
    board[centerX+1] |= (1UL << (centerY-2));
    board[centerX+3] |= (1UL << (centerY-2));
    board[centerX+9] |= (1UL << (centerY-2));
    
    // Row 4: 4 empty, 1 cell, 3 empty, 1 cell, 1 empty
    board[centerX+4] |= (1UL << (centerY-1));
    board[centerX+8] |= (1UL << (centerY-1));
    
    // Row 5: 6 empty, 1 cell, 3 empty
    board[centerX+6] |= (1UL << centerY);
    
    // Row 6: 6 empty, 2 cells, 2 empty
    board[centerX+6] |= (1UL << (centerY+1));
    board[centerX+7] |= (1UL << (centerY+1));
    
    // Row 7: 5 empty, 4 cells, 1 empty
    board[centerX+5] |= (1UL << (centerY+2));
    board[centerX+6] |= (1UL << (centerY+2));
    board[centerX+7] |= (1UL << (centerY+2));
    board[centerX+8] |= (1UL << (centerY+2));
    
    // Row 8: 5 empty, 2 cells, 1 empty, 2 cells
    board[centerX+5] |= (1UL << (centerY+3));
    board[centerX+6] |= (1UL << (centerY+3));
    board[centerX+8] |= (1UL << (centerY+3));
    board[centerX+9] |= (1UL << (centerY+3));
    
    // Row 9: 7 empty, 2 cells, 1 empty
    board[centerX+7] |= (1UL << (centerY+4));
    board[centerX+8] |= (1UL << (centerY+4));
    break;
     case 2: // Gosper Glider Gun
    // Left square
    board[centerX-18] |= (1UL << (centerY));
    board[centerX-18] |= (1UL << (centerY+1));
    board[centerX-17] |= (1UL << (centerY));
    board[centerX-17] |= (1UL << (centerY+1));
    
    // Left pattern
    board[centerX-8] |= (1UL << (centerY));
    board[centerX-8] |= (1UL << (centerY-1));
    board[centerX-8] |= (1UL << (centerY+1));
    board[centerX-7] |= (1UL << (centerY-2));
    board[centerX-7] |= (1UL << (centerY+2));
    board[centerX-6] |= (1UL << (centerY-3));
    board[centerX-6] |= (1UL << (centerY+3));
    board[centerX-5] |= (1UL << (centerY-3));
    board[centerX-5] |= (1UL << (centerY+3));
    board[centerX-4] |= (1UL << (centerY));
    board[centerX-3] |= (1UL << (centerY-2));
    board[centerX-3] |= (1UL << (centerY+2));
    board[centerX-2] |= (1UL << (centerY-1));
    board[centerX-2] |= (1UL << (centerY));
    board[centerX-2] |= (1UL << (centerY+1));
    board[centerX-1] |= (1UL << (centerY));
    
    // Right pattern (fully corrected)
    board[centerX+2] |= (1UL << (centerY-1));
    board[centerX+2] |= (1UL << (centerY-2));
    board[centerX+2] |= (1UL << (centerY-3));
    board[centerX+3] |= (1UL << (centerY-1));
    board[centerX+3] |= (1UL << (centerY-2));
    board[centerX+3] |= (1UL << (centerY-3));
    board[centerX+4] |= (1UL << (centerY));
    board[centerX+4] |= (1UL << (centerY-4));
    board[centerX+6] |= (1UL << (centerY-5));
    board[centerX+6] |= (1UL << (centerY-4));
    board[centerX+6] |= (1UL << (centerY));
    board[centerX+6] |= (1UL << (centerY+1));
    
    // Right square
    board[centerX+16] |= (1UL << (centerY-2));
    board[centerX+16] |= (1UL << (centerY-3));
    board[centerX+17] |= (1UL << (centerY-2));
    board[centerX+17] |= (1UL << (centerY-3));
    break;
    
   case 3: // 4-8-12 Diamond
    // Row 1: 4 empty - 4 cells - 4 empty
    board[centerX-2] |= (1UL << (centerY-4));
    board[centerX-1] |= (1UL << (centerY-4));
    board[centerX] |= (1UL << (centerY-4));
    board[centerX+1] |= (1UL << (centerY-4));
    
    // Row 2: empty
    
    // Row 3: 2 empty - 8 cells - 2 empty
    board[centerX-4] |= (1UL << (centerY-2));
    board[centerX-3] |= (1UL << (centerY-2));
    board[centerX-2] |= (1UL << (centerY-2));
    board[centerX-1] |= (1UL << (centerY-2));
    board[centerX] |= (1UL << (centerY-2));
    board[centerX+1] |= (1UL << (centerY-2));
    board[centerX+2] |= (1UL << (centerY-2));
    board[centerX+3] |= (1UL << (centerY-2));
    
    // Row 4: empty
    
    // Row 5: 12 cells
    board[centerX-6] |= (1UL << centerY);
    board[centerX-5] |= (1UL << centerY);
    board[centerX-4] |= (1UL << centerY);
    board[centerX-3] |= (1UL << centerY);
    board[centerX-2] |= (1UL << centerY);
    board[centerX-1] |= (1UL << centerY);
    board[centerX] |= (1UL << centerY);
    board[centerX+1] |= (1UL << centerY);
    board[centerX+2] |= (1UL << centerY);
    board[centerX+3] |= (1UL << centerY);
    board[centerX+4] |= (1UL << centerY);
    board[centerX+5] |= (1UL << centerY);
    
    // Row 6: empty
    
    // Row 7: 2 empty - 8 cells - 2 empty
    board[centerX-4] |= (1UL << (centerY+2));
    board[centerX-3] |= (1UL << (centerY+2));
    board[centerX-2] |= (1UL << (centerY+2));
    board[centerX-1] |= (1UL << (centerY+2));
    board[centerX] |= (1UL << (centerY+2));
    board[centerX+1] |= (1UL << (centerY+2));
    board[centerX+2] |= (1UL << (centerY+2));
    board[centerX+3] |= (1UL << (centerY+2));
    
    // Row 8: empty
    
    // Row 9: 4 empty - 4 cells - 4 empty
    board[centerX-2] |= (1UL << (centerY+4));
    board[centerX-1] |= (1UL << (centerY+4));
    board[centerX] |= (1UL << (centerY+4));
    board[centerX+1] |= (1UL << (centerY+4));
    break;
    
     case 4: // Achim's p144 oscillator
// Row 1 (y-9): 2 cells, 24 empty, 2 cells
board[centerX-14] |= (1UL << (centerY-9));
board[centerX-13] |= (1UL << (centerY-9));
board[centerX+12] |= (1UL << (centerY-9));
board[centerX+13] |= (1UL << (centerY-9));

// Row 2 (y-8): 2 cells, 24 empty, 2 cells
board[centerX-14] |= (1UL << (centerY-8));
board[centerX-13] |= (1UL << (centerY-8));
board[centerX+12] |= (1UL << (centerY-8));
board[centerX+13] |= (1UL << (centerY-8));

// Row 3 (y-7): 18 empty, 2 cells, 8 empty
board[centerX+4] |= (1UL << (centerY-7));
board[centerX+5] |= (1UL << (centerY-7));

// Row 4 (y-6): 17 empty, 1 cell, 2 empty, 1 cell, 7 empty
board[centerX+3] |= (1UL << (centerY-6));
board[centerX+6] |= (1UL << (centerY-6));

// Row 5 (y-5): 18 empty, 2 cells, 8 empty
board[centerX+4] |= (1UL << (centerY-5));
board[centerX+5] |= (1UL << (centerY-5));

// Row 6 (y-4): 14 empty, 1 cell, 13 empty
board[centerX] |= (1UL << (centerY-4));

// Row 7 (y-3): 13 empty, 1 cell, 1 empty, 1 cell, 12 empty
board[centerX-1] |= (1UL << (centerY-3));
board[centerX+1] |= (1UL << (centerY-3));

// Row 8 (y-2): 12 empty, 1 cell, 3 empty, 1 cell, 11 empty
board[centerX-2] |= (1UL << (centerY-2));
board[centerX+2] |= (1UL << (centerY-2));

// Row 9 (y-1): 12 empty, 1 cell, 2 empty, 1 cell, 12 empty
board[centerX-2] |= (1UL << (centerY-1));
board[centerX+1] |= (1UL << (centerY-1));

// Row 10 (y): Empty - 28 empty

// Row 11 (y+1): 12 empty, 1 cell, 2 empty, 1 cell, 12 empty
board[centerX-2] |= (1UL << (centerY+1));
board[centerX+1] |= (1UL << (centerY+1));

// Row 12 (y+2): 11 empty, 1 cell, 3 empty, 1 cell, 12 empty
board[centerX-3] |= (1UL << (centerY+2));
board[centerX+1] |= (1UL << (centerY+2));

// Row 13 (y+3): 12 empty, 1 cell, 1 empty, 1 cell, 13 empty
board[centerX-2] |= (1UL << (centerY+3));
board[centerX] |= (1UL << (centerY+3));

// Row 14 (y+4): 13 empty, 1 cell, 14 empty
board[centerX-1] |= (1UL << (centerY+4));

// Row 15 (y+5): 8 empty, 2 cells, 18 empty
board[centerX-6] |= (1UL << (centerY+5));
board[centerX-5] |= (1UL << (centerY+5));

// Row 16 (y+6): 7 empty, 1 cell, 2 empty, 1 cell, 17 empty
board[centerX-7] |= (1UL << (centerY+6));
board[centerX-4] |= (1UL << (centerY+6));

// Row 17 (y+7): 8 empty, 2 cells, 18 empty
board[centerX-6] |= (1UL << (centerY+7));
board[centerX-5] |= (1UL << (centerY+7));

// Row 18 (y+8): 2 cells, 24 empty, 2 cells
board[centerX-14] |= (1UL << (centerY+8));
board[centerX-13] |= (1UL << (centerY+8));
board[centerX+12] |= (1UL << (centerY+8));
board[centerX+13] |= (1UL << (centerY+8));

// Row 19 (y+9): 2 cells, 24 empty, 2 cells
board[centerX-14] |= (1UL << (centerY+9));
board[centerX-13] |= (1UL << (centerY+9));
board[centerX+12] |= (1UL << (centerY+9));
board[centerX+13] |= (1UL << (centerY+9));
break;

   case 5: // 56P6H1V0 - Corrected pattern
 // Row 1: 5 empty, 3 cells, 10 empty, 3 cells, 5 empty
    board[centerX-8] |= (1UL << (centerY-6));
    board[centerX-7] |= (1UL << (centerY-6));
    board[centerX-6] |= (1UL << (centerY-6));
    board[centerX+5] |= (1UL << (centerY-6));
    board[centerX+6] |= (1UL << (centerY-6));
    board[centerX+7] |= (1UL << (centerY-6));
    
    // Row 2: 3 cells, 1 empty, 1 cell, 7 empty, 2 cells, 7 empty, 1 cell, 1 empty, 3 cells
    board[centerX-13] |= (1UL << (centerY-5));
    board[centerX-12] |= (1UL << (centerY-5));
    board[centerX-11] |= (1UL << (centerY-5));
    board[centerX-9] |= (1UL << (centerY-5));
    board[centerX-1] |= (1UL << (centerY-5));
    board[centerX] |= (1UL << (centerY-5));
    board[centerX+8] |= (1UL << (centerY-5));
    board[centerX+10] |= (1UL << (centerY-5));
    board[centerX+11] |= (1UL << (centerY-5));
    board[centerX+12] |= (1UL << (centerY-5));
    
    // Row 3: 4 empty, 1 cell, 3 empty, 1 cell, 2 empty, 1 cell, 2 empty, 1 cell, 2 empty, 1 cell, 3 empty, 1 cell, 4 empty
    board[centerX-9] |= (1UL << (centerY-4));
    board[centerX-5] |= (1UL << (centerY-4));
    board[centerX-2] |= (1UL << (centerY-4));
    board[centerX+1] |= (1UL << (centerY-4));
    board[centerX+4] |= (1UL << (centerY-4));
    board[centerX+8] |= (1UL << (centerY-4));
    
    // Row 4: 4 empty, 1 cell, 5 empty, 1 cell, 4 empty, 1 cell, 5 empty, 1 cell, 4 empty
    board[centerX-9] |= (1UL << (centerY-3));
    board[centerX-3] |= (1UL << (centerY-3));
    board[centerX+2] |= (1UL << (centerY-3));
    board[centerX+8] |= (1UL << (centerY-3));
    
    // Row 5: 10 empty, 2 cells, 2 empty, 2 cells, 10 empty
    board[centerX-3] |= (1UL << (centerY-2));
    board[centerX-2] |= (1UL << (centerY-2));
    board[centerX+1] |= (1UL << (centerY-2));
    board[centerX+2] |= (1UL << (centerY-2));
    
    // Row 6: 11 empty, 1 cell, 2 empty, 1 cell, 3 empty, 1 cell, 7 empty
    board[centerX-6] |= (1UL << (centerY-1));
    board[centerX-2] |= (1UL << (centerY-1));
    board[centerX+1] |= (1UL << (centerY-1));
    board[centerX+5] |= (1UL << (centerY-1));
    
    // Row 7: 7 empty, 1 cell, 1 empty, 1 cell, 6 empty, 1 cell, 1 empty, 1 cell, 7 empty
    board[centerX-6] |= (1UL << centerY);
    board[centerX-4] |= (1UL << centerY);
    board[centerX+3] |= (1UL << centerY);
    board[centerX+5] |= (1UL << centerY);
    
    // Row 8: 8 empty, 10 cells, 8 empty
    board[centerX-5] |= (1UL << (centerY+1));
    board[centerX-4] |= (1UL << (centerY+1));
    board[centerX-3] |= (1UL << (centerY+1));
    board[centerX-2] |= (1UL << (centerY+1));
    board[centerX-1] |= (1UL << (centerY+1));
    board[centerX+0] |= (1UL << (centerY+1));
    board[centerX+1] |= (1UL << (centerY+1));
    board[centerX+2] |= (1UL << (centerY+1));
    board[centerX+3] |= (1UL << (centerY+1));
    board[centerX+4] |= (1UL << (centerY+1));
    
    // Row 9: 10 empty, 1 cell, 4 empty, 1 cell, 10 empty
    board[centerX-3] |= (1UL << (centerY+2));
    board[centerX+2] |= (1UL << (centerY+2));
    
    // Row 10: 8 empty, 1 cell, 8 empty, 1 cell, 8 empty
    board[centerX-5] |= (1UL << (centerY+3));
    board[centerX+4] |= (1UL << (centerY+3));
    
    // Row 11: 7 empty, 1 cell, 10 empty, 1 cell, 7 empty
    board[centerX-6] |= (1UL << (centerY+4));
    board[centerX+5] |= (1UL << (centerY+4));
    
    // Row 12: 8 empty, 1 cell, 8 empty, 1 cell, 8 empty
    board[centerX-5] |= (1UL << (centerY+5));
    board[centerX+4] |= (1UL << (centerY+5));
    break;
    }
  
    cellCount = 0;
    max_cellCount = 0;
    generation = 0;
}
void clearBoard() {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board[x] = 0;
    }
    cellCount = 0;
    max_cellCount = 0;
    generation = 0;
    display.clearDisplay();
    display.display();
}

void flashLED() {
    digitalWrite(LED_BUILTIN, HIGH);
    // Remove the delay(5) - it was blocking execution
    digitalWrite(LED_BUILTIN, LOW);
}

void drawTitle() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.println("GAME OF");
    display.setCursor(20, 30);
    display.println("LIFE");
    display.setTextSize(1);
    display.setCursor(20, 50);
    display.println("Press SET to start");
    display.display();
}

void showMenu() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 5);
    display.setTextSize(1);
    display.println("GAME OF LIFE");
    display.setTextSize(2);
    
    // Menu options
    display.setTextSize(1);
    
if (!inPatternsSubMenu) {
        // Main menu
        if (menuSelection == 1) {
            display.fillRect(0, 35, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 37);
        display.println("1: Random Game");

        if (menuSelection == 2) {
            display.fillRect(0, 45, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 47);
        display.println("2: Custom Game");

        if (menuSelection == 3) {
            display.fillRect(0, 55, 128, 10, WHITE);;
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 57);
        display.println("3: Preset Patterns");
    } else {
        // Show pattern submenu
        if (patternSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Coe Ship");
        
        if (patternSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Gosper Glider Gun");
        
        if (patternSelection == 3) {
            display.fillRect(0, 35, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 37);
        display.println("3: 4-8-12 Diamond");
        
        if (patternSelection == 4) {
            display.fillRect(0, 45, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 47);
        display.println("4: Achim's p144");
  
        if (patternSelection == 5) {
            display.fillRect(0, 55, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 57);
        display.println("5: 56P6H1V0");
     }
    
    display.display();
}

// Replace your current handleButtons() function with these two functions:

void handleDirectionalButtons() {
    unsigned long now = millis();
    static unsigned long lastDirectionalPress = 0;
    
    // Single timing check - remove conflicting delays
    if (now - last_button_check_time < 100) return;
    
    last_button_check_time = now;
    int btnValue = readButtonValue();
   
    // Much shorter debounce for edit mode cursor movement
    int debounceTime = (editMode && gameMode == 2) ? 120 : 200;
    if (now - lastDirectionalPress < debounceTime) return;
    
    if (inMenuMode) {
        // Reset the button processing flag if it was previously set
        if (buttonProcessingInProgress) {
            if (!isBtnSet(btnValue)) {
                buttonProcessingInProgress = false;
            }
            return;
        }
        
        if (isBtnUp(btnValue)) {
            if (!inPatternsSubMenu) {
                menuSelection = (menuSelection > 1) ? menuSelection - 1 : 3;
            } else {
                patternSelection = (patternSelection > 1) ? patternSelection - 1 : 5;
            }   
            lastDirectionalPress = now;
            showMenu();
        }
        else if (isBtnDown(btnValue)) {
            if (!inPatternsSubMenu) {
                menuSelection = (menuSelection < 3) ? menuSelection + 1 : 1;
            } else {
                patternSelection = (patternSelection < 5) ? patternSelection + 1 : 1;
            }
            lastDirectionalPress = now;
            showMenu();
        }
        else if (isBtnLeft(btnValue) && inPatternsSubMenu) {
            inPatternsSubMenu = false;
            lastDirectionalPress = now;
            showMenu();
        }
        return;
    }
    
    if (editMode) {
        if (isBtnUp(btnValue)) {
         if (now - lastDirectionalPress < 120) return;  // Extra safety check 
            cursorY--;
            if (cursorY < 0) cursorY = BOARD_HEIGHT - 1;
            lastDirectionalPress = now;
            flashLED();
        }
        else if (isBtnDown(btnValue)) {
          if (now - lastDirectionalPress < 120) return;
            cursorY++;
            if (cursorY >= BOARD_HEIGHT) cursorY = 0;
            lastDirectionalPress = now;
            flashLED();
        }
        else if (isBtnLeft(btnValue)) {
          if (now - lastDirectionalPress < 120) return;
            cursorX--;
            if (cursorX < 0) cursorX = BOARD_WIDTH - 1;
            lastDirectionalPress = now;
            flashLED();
        }
        else if (isBtnRight(btnValue)) {
          if (now - lastDirectionalPress < 120) return;
            cursorX++;
            if (cursorX >= BOARD_WIDTH) cursorX = 0;
            lastDirectionalPress = now;
            flashLED();
        }
        return;
    } 
    else {
        // Simulation mode speed controls
        if (isBtnUp(btnValue)) {
            if (currentSpeedIndex < numSpeeds - 1) {
                currentSpeedIndex++;
                frame_start_t = now;
            }
            lastDirectionalPress = now;
        } 
        else if (isBtnDown(btnValue)) {
            if (currentSpeedIndex > 0) {
                currentSpeedIndex--;
                frame_start_t = now;
                flashLED();
            }
            lastDirectionalPress = now;
        } 
        else if (isBtnLeft(btnValue)) {
            randomizeBoard();
            currentSpeedIndex = 0;
            generation = 0;
            start_t = micros();
            frame_start_t = millis();
            lastDirectionalPress = now;
        }
        else if (isBtnRight(btnValue)) {
            randomizeBoard();
            currentSpeedIndex = 0;
            generation = 0;
            start_t = micros();
            frame_start_t = millis();
            lastDirectionalPress = now;
        }
    }
}

void handleSetButton() {
    static unsigned long lastSetPress = 0;
    static bool setWasPressed = false;
    static unsigned long setPressStartTime = 0;
    static bool justEnteredSubmenu = false;
    static unsigned long submenuEntryTime = 0;
    static bool longPressTriggered = false;
    
    unsigned long now = millis();
    int btnValue = readButtonValue();
    bool setPressed = isBtnSet(btnValue);
    
    // Reduce debounce time to prevent conflicts
    if (now - lastSetPress < 150) return;
    
    // Detect button press (rising edge)
    if (setPressed && !setWasPressed) {
        setWasPressed = true;
        setPressStartTime = now;
        lastSetPress = now;
        longPressTriggered = false;
        
        if (inMenuMode) {
            if (!inPatternsSubMenu) {
                switch (menuSelection) {
                    case 1: // Random Game
                        gameMode = 1;
                        inMenuMode = false;
                        editMode = false;
                        randomizeBoard();
                        generation = 0;
                        start_t = micros();
                        frame_start_t = millis();
                        currentSpeedIndex = 0;
                        break;
                        
                    case 2: // Custom Game
                        gameMode = 2;
                        inMenuMode = false;
                        editMode = true;
                        clearBoard();
                        cursorX = BOARD_WIDTH / 2;
                        cursorY = BOARD_HEIGHT / 2;
                        break;
                        
                    case 3: // Preset Patterns
                        inPatternsSubMenu = true;
                        patternSelection = 1;
                        justEnteredSubmenu = true;
                        submenuEntryTime = now;
                        showMenu();
                        break;
                }
            } else if (!justEnteredSubmenu || (now - submenuEntryTime > 400)) {
                // Clear the flag to prevent getting stuck
                justEnteredSubmenu = false;
               // Single press in submenu - load pattern and start immediately
                gameMode = 3;
                loadPresetPattern(patternSelection);
                inMenuMode = false;
                inPatternsSubMenu = false;
                editMode = false;
                generation = 0;
                start_t = micros();
                frame_start_t = millis();
                currentSpeedIndex = 0; 
            }
            flashLED();
        }
    }
    // Check for long press while button is held down
    else if (setPressed && setWasPressed && !longPressTriggered) {
        unsigned long pressDuration = now - setPressStartTime;
        
        // Reduce long press time and add safety check
        if (editMode && gameMode == 2 && pressDuration >= 800 && pressDuration < 5000) {
            // Long press detected - start simulation immediately
            editMode = false;
            start_t = micros();
            frame_start_t = millis();
            currentSpeedIndex = 0;
            longPressTriggered = true;
            flashLED();
            flashLED(); // Double flash to confirm
        }
        else if (!editMode && gameMode == 3 && pressDuration >= 800 && pressDuration < 5000) {
            generation = 0;
            start_t = micros();
            frame_start_t = millis();
            currentSpeedIndex = 0;
            longPressTriggered = true;
            flashLED();
        }
    }
    else if (!setPressed && setWasPressed) {
        // Button released
        setWasPressed = false;
        
       // Always clear this flag on button release
        if (justEnteredSubmenu) {
            justEnteredSubmenu = false;
        }
        
        unsigned long pressDuration = now - setPressStartTime;
        
        // Only handle short press for cell toggling if long press wasn't triggered
        if (editMode && gameMode == 2 && pressDuration < 800 && !longPressTriggered) {
            toggleCellAtCursor();
            flashLED();
        }
        
        // Reset the flag after handling button release
        longPressTriggered = false;
    }
}

void handleButtonB() {
    static unsigned long lastButtonBPress = 0;
    static bool buttonBWasPressed = false;
    
    unsigned long now = millis();
    int btnBValue = analogRead(A4);  // Read Button B on A4
    bool buttonBPressed = isBtnB(btnBValue);
    
    // Debounce timing
    if (now - lastButtonBPress < 200) return;  // 200ms debounce for Button B
    
    if (buttonBPressed && !buttonBWasPressed) {
        // Button B just pressed - return to main menu from anywhere
        buttonBWasPressed = true;
        lastButtonBPress = now;
        
        // Reset all states and return to main menu
        inMenuMode = true;
        inPatternsSubMenu = false;
        editMode = false;
        menuSelection = 1;
        buttonProcessingInProgress = false;
        showingStats = false;
        
        showMenu();
        flashLED(); // Visual feedback
    }
    else if (!buttonBPressed && buttonBWasPressed) {
        // Button B released
        buttonBWasPressed = false;
    }
}

void resetGame(uint32_t hash, uint32_t gen_time) {
    // Pattern is repeating, reset game
    
    // Only show age screen in game mode 1 (random game)
    if (gameMode == 1) {
        // Display the game age screen
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(4, 14);
        display.setTextColor(WHITE, BLACK);
        display.println("GAME OVER");
        
        // Display generation count
        display.setCursor(4, 32);
        display.setTextSize(1);
        display.print("Generations: ");
        display.println(generation);
        
        // Display max cell count
        display.setCursor(4, 44);
        display.print("Max cells: ");
        display.println(max_cellCount);
        
        display.display();

        delay(3000);
    }

     display.clearDisplay();
     display.display();
    randomizeBoard();
  cursorX = BOARD_WIDTH / 2;  // Center cursor
    currentSpeedIndex = 0;  // Reset speed
    cursorY = BOARD_HEIGHT / 2;
  
  // Return to menu instead of auto-randomizing
  inMenuMode = true;
  menuSelection = 1;
  showMenu();
  max_cellCount = 0;  // Reset max cell count for next game
}
static inline uint32_t rotate_shift_three(uint32_t x, int n) {
    const unsigned int mask = (CHAR_BIT * sizeof(x) - 1);
    if (n == 0) {
        n = 31;
    } else {
        n -= 1;
    }
    n &= mask;
    return ((x >> n) | ((x << ((-n) & mask)))) & 0x7;
}

    void loop() {
     
    // Check for button presses
    handleDirectionalButtons();
    handleSetButton();
    handleButtonB();

    // Skip game logic if in menu mode
    if (inMenuMode) {
        return;
    }

    if (!editMode) {
        // Game of Life simulation
        unsigned long current_time = millis();
        game_hash = 5381;  // Store in global variable

        // Handle statistics display state
        if (showingStats) {
            // Stay in statistics display for 4 seconds
            if (current_time - statsStartTime >= 4000) {
                showingStats = false;
                display.clearDisplay();
                display.display();
                
                // Return to menu instead of randomizing
                inMenuMode = true; 
                menuSelection = 1;
                showMenu();
                display.display();
                generation = 0;
                start_t = micros();
            }
            return; // Skip simulation while showing stats
        }
        else if (current_time - frame_start_t >= frameDelays[currentSpeedIndex]) {
            frame_start_t = current_time;

          generation++; // Count generations only when simulation actually advances
            display.clearDisplay();

            cellCount = 0; // Reset cell count before each generation
            
            for (int x = 0; x < BOARD_WIDTH; x++) {
                uint32_t left;
                if (x == 0) {
                    left = board[BOARD_WIDTH - 1];
                } else {
                    left = board[x - 1];
                }
                uint32_t center = board[x];
                uint32_t right;
                if (x == BOARD_WIDTH - 1) {
                    right = board[0];
                } else {
                    right = board[x + 1];
                }
                this_col = 0;
                uint32_t hash_acc = 0;
                
                for (int y = 0; y < BOARD_HEIGHT; y++) {
                    // Calculate next generation and hash
                    hash_acc <<= 1;
                    uint32_t shift_center = rotate_shift_three(center, y);
                    int count = bit_counts[rotate_shift_three(left, y)] +
                                bit_counts[shift_center] +
                                bit_counts[rotate_shift_three(right, y)];
                    bool alive = shift_center & 0x2;
                    if ((count == 3) || (count == 4 && alive)) {
                        this_col |= 1 << y;
                        hash_acc |= 1; cellCount++;  // Count live cells
                    }
                    if ((y & 0x7) == 0x7) {
                       game_hash = ((game_hash << 5) + game_hash) ^ hash_acc;
                        hash_acc = 0; 
                    }
                }
                
                if (x > 1) {
                    board[x - 1] = last_col;
                    for (int y = 0; y < BOARD_HEIGHT; y++) {
                        if (last_col & 1) {
                            drawCell(x - 1, y, 1);
                        }
                        last_col >>= 1;
                    } 

                    // Update max cell count if current count is higher
                    if (cellCount > max_cellCount) {
                        max_cellCount = cellCount;
                    }
                } else if (x == 0) {
                    first_col = this_col;
                }
                last_col = this_col;
            }
            
            board[0] = first_col;
            board[BOARD_WIDTH - 1] = last_col;
            for (int y = 0; y < BOARD_HEIGHT; y++) {
                if (first_col & 1) {
                    drawCell(0, y, 1);
                }
                first_col >>= 1;
                
                if (last_col & 1) {
                    drawCell(BOARD_WIDTH - 1, y, 1);
                }
                last_col >>= 1;
            }
            // Check for end conditions
          // Check for stilllife (no changes this generation)
            static uint32_t last_game_hash = 0;
            if (last_game_hash == game_hash && generation > 10 && (gameMode == 1 || gameMode == 2)) {
              last_game_hash = game_hash;  // Update for next comparison
                startShowingStats();
                return;
            }
          last_game_hash = game_hash;  // Always update the hash
            if (cellCount < 5 && generation > 20 && (gameMode == 1 || gameMode == 2)) {
                // Game is dead (too few cells)
                
                // Trigger statistics display
                if (!showingStats) {
                    startShowingStats();
                }
                return; // Skip further processing after triggering stats
            }
            
            // Allow repeating pattern detection for both game modes
            static uint32_t prev_hash = 0;
            if ((short_check_hash == game_hash || prev_hash == game_hash) && generation > 20 && (gameMode == 1 || gameMode == 2)) {
                // Pattern repeating

                // Show statistics only once
                startShowingStats();
                return; // Skip further processing
            }  
      prev_hash = game_hash;
        }
    
        // Update statistics tracking
        if (generation % SHORT_HASH_INTERVAL == 0) {
            short_check_hash = game_hash;
        }
        if (generation % LONG_HASH_INTERVAL == 0) {
            long_check_hash = game_hash;
        }

      display.display();
      
    }

      else if (editMode && gameMode == 2) {  // Only show cursor for custom games
        // In edit mode, just draw the current state and cursor
        display.clearDisplay();
        for (int x = 0; x < BOARD_WIDTH; x++) {
            uint32_t col = board[x];
            for (int y = 0; y < BOARD_HEIGHT; y++) {
                if (col & (1UL << y)) {
                    drawCell(x, y, WHITE);
                }
            }
        }
        
        drawCursor();
        display.display();
    }
      else if (!editMode && gameMode == 3) {
        // Pattern loaded but not running - show static pattern without cursor
        display.clearDisplay();
        for (int x = 0; x < BOARD_WIDTH; x++) {
            uint32_t col = board[x];
            for (int y = 0; y < BOARD_HEIGHT; y++) {
                if (col & (1UL << y)) {
                    drawCell(x, y, WHITE);
                }
             }
        }
        display.display();
    }
}        

// Add this new function to display game statistics
void showGameStatistics() {
   
    display.setTextSize(2);
    display.setCursor(4, 0);
    display.setTextColor(WHITE);
    
    // Show appropriate title based on game mode
    if (gameMode == 1) {
        display.println("GAME OVER");
    } else {
        display.println("GAME OVER");  // Same title for both modes
    }
    
    display.setTextSize(1);
    display.setCursor(4, 25);
    display.print("Generations: ");
    display.println(generation);
    
    display.setCursor(4, 35);
    display.print("Max cells: ");
    display.println(max_cellCount);
    display.display();
}

// New function to start showing statistics
void startShowingStats() {
    showingStats = true;
    statsStartTime = millis();

  // We'll return to the menu after showing statistics
  
    display.clearDisplay();
    showGameStatistics();
}
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // Stabilize analog readings
    for (int i = 0; i < 20; i++) {
        analogRead(BUTTON_INPUT_PIN);
        delay(5);
    }

     // Initialize display
     if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR)) {
         for (;;);
     }
 
  // Initialize display
      if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR)) {
          for (;;);
      }
     
     // Set maximum brightness
     display.ssd1306_command(SSD1306_SETCONTRAST);
     display.ssd1306_command(255);  // Maximum brightness
    
    // Show title screen
    display.clearDisplay();

  // Background cells animation
    for (int i = 0; i < 40; i++) {
      // Add random cells to background throughout the intro
      int x = random(BOARD_WIDTH);
      int y = random(BOARD_HEIGHT);
      drawCell(x, y, WHITE);
      
      // Show "Game" for 0.5 seconds
      if (i == 0) {
        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(25, 24);
        display.println("GAME");
        display.display();
        delay(500);
      }
      
      // Show "of" for 0.5 seconds
      else if (i == 10) {
        display.clearDisplay();
        // Redraw random cells that we've drawn so far
       for (int x = 0; x < BOARD_WIDTH; x++) {
          for (int y = 0; y < BOARD_HEIGHT; y++) {
            if (board[x] & (1UL << y)) {
              drawCell(x, y, WHITE);
            }
          }
        }
         display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(50, 24);
        display.println("OF");
        display.display();
        delay(500);
      }
      
      // Show "Life" for 0.5 seconds
      else if (i == 20) {
        display.clearDisplay();
        // Redraw random cells
        for (int x = 0; x < BOARD_WIDTH; x++) {
          for (int y = 0; y < BOARD_HEIGHT; y++) {
            if (board[x] & (1UL << y)) {
              drawCell(x, y, WHITE);
            }
          }
        }
        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(32, 24);
        display.println("LIFE");
        display.display();
        delay(500);
      }
      display.display();
      delay(50);
     }
     
    
    // After animation, show menu
    showMenu();
    delay(50);

    // No DotStar initialization needed anymore

    // Seed the random generator
    collectRandomness(100);
    
    // Game will be initialized when user selects a menu option
}