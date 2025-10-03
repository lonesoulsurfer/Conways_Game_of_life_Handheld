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
 * • Five game modes: Random generation, Custom builder, Preset patterns, Symmetric patterns, Alt Games
 * • 12 speed levels with real-time speed control
 * • 5 famous Conway patterns + Symmetric pattern generator
 * • 4 Alternative cellular automata: Brian's Brain, Day & Night, Seeds, Langton's Ant
 * • Interactive pattern editor with blinking cursor
 * • Game statistics tracking (generations, max cells)
 * • Comprehensive menu system with multiple submenus
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
 * 1. Preset Patterns: Choose from 5 famous Conway patterns
 * 2. Random Game: Starts with random cell pattern, auto-resets on death
 * 3. Symmetric Game: Generate symmetric patterns (Small/Medium/Large)
 * 4. Custom Game: Build your own pattern with cursor, long-press SET to start
 * 5. Alt Games: Alternative cellular automata collection
 *    • Brian's Brain: 3-state automaton with birth/dying/dead phases
 *    • Day & Night: Inverse Conway rules creating dense stable patterns
 *    • Seeds: Birth-only automaton creating explosive wave patterns
 *    • Langton's Ant: Single ant creating emergent highway patterns
 * 
 * CONTROLS:
 * • Menu: UP/DOWN navigate, SET selects, LEFT goes back in submenu
 * • Edit: Arrow keys move cursor, SET toggles cell, long-press SET starts
 * • Simulation: UP/DOWN changes speed, LEFT/RIGHT generates new random
 * • Any mode: Button B returns to main menu
 * • SYMMETRIC PATTERNS: 3 symmetry types (Vertical, Horizontal, 4-way Rotational)
 * 
 * DISPLAY:
 * • 64x32 cell grid displayed at 2x scale on 128x64 OLED
 * • Real-time generation counter and cell statistics
 * • Blinking cursor in edit mode
 * • Automatic game over detection and statistics display
 * • Game-specific visual indicators (ant position, dying cells, etc.)
 * 
 * ALGORITHM:
 * • Optimized bit-manipulation implementation
 * • Toroidal (wraparound) boundary conditions
 * • Hash-based pattern repetition detection
 * • Efficient neighbor counting using lookup table
 * • Game-specific rule engines for each cellular automaton
 * 
 * AUTHOR: Marcus Dunn
 * VERSION: 2.0
 * DATE: 25/09/2025
 * LICENSE: Open Source - feel free to modify and share
 * 
 * Based on John Conway's "Game of Life" cellular automaton (1970)
 * Additional automata: Brian's Brain (1984), Day & Night, Seeds, Langton's Ant (1986)
 * 
 * =====================================================================
 */

#pragma message (" game_of_life_V2 " 25/9/2025 " ")
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
int currentSymmetryType = 0;   // Track current symmetry type (1-4)
int currentSymmetrySize = 0;   // Track current size (1=Small, 2=Medium, 3=Large)
// Game modes
bool inPatternsSubMenu = false;
int patternSelection = 1; // Default pattern selection
int gameMode = 1; // 1 = Random game, 2 = Build your own
const int frameDelays[] = {500, 400, 300, 250, 200, 150, 100, 75, 50, 40, 30, 25};
const int numSpeeds = sizeof(frameDelays) / sizeof(frameDelays[0]);
int currentBriansBrainSize = 1; // Store the size selection for pattern resets
// Symmetric submenu variables
bool inSymmetricSubMenu = false;
int symmetricSizeSelection = 1; // 1=Small, 2=Medium, 3=Large
bool inAltGamesSubMenu = false;
int altGameSelection = 1; // Default alt game selection
// Seeds submenu variables
bool inSeedsSubMenu = false;
int seedsGameSelection = 1; // 1=Random, 2=Custom
bool buttonJustReleased = false;
unsigned long buttonReleaseTime = 0;
bool seedsWasCustom = false;  // Track if Seeds was started in custom mode
// Brian's Brain game state
bool briansBrainMode = false;
uint32_t briansBrain_dying[BOARD_WIDTH];  // Cells that are dying (will become dead next generation)
bool inBriansBrainSubMenu = false;  // ADD THIS LINE
int briansBrainSizeSelection = 1;   // ADD THIS LINE (1=Small, 2=Medium, 3=Large)

// Day & Night game state
bool dayNightMode = false;

// Langton's Ant variables
bool langtonAntMode = false;

// Seeds variables
bool seedsMode = false;
int antX, antY;           // Ant position
int antDirection;         // 0=North, 1=East, 2=South, 3=West
unsigned long lastAntMove = 0;
const unsigned long ANT_MOVE_DELAY = 100; // ms between ant moves

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
void generateSymmetricPattern(int symmetryType);
void generateVerticalSymmetric();
void generateHorizontalSymmetric();
void generateRotationalSymmetric();
void generateDiagonalSymmetric();
void generateSmallSymmetricPattern(int symmetryType);
void generateMediumSymmetricPattern(int symmetryType);
void updateDayNight();
void drawDayNight();
void initializeDayNight();
void drawLangtonsAnt();
void initializeLangtonsAnt();
void updateSeeds();
void drawSeeds();
void initializeSeeds();
void updateLangtonsAnt();
  
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
        // Check if we're in Brian's Brain custom mode
        if (gameMode == 8 && briansBrainMode && editMode) {
            // Brian's Brain cursor - show at the same offset position as the actual cells
            int color = cellIsLit ? BLACK : WHITE;  // Invert colors for visibility
            display.drawPixel(cursorX * SCALE + 1, cursorY * SCALE + 1, color);
            
            // Add border around cursor position (at normal scale boundaries)
            if (cursorX > 0) display.drawPixel((cursorX-1) * SCALE, cursorY * SCALE + 1, WHITE);
            if (cursorX < BOARD_WIDTH-1) display.drawPixel((cursorX+1) * SCALE, cursorY * SCALE + 1, WHITE);
            if (cursorY > 0) display.drawPixel(cursorX * SCALE + 1, (cursorY-1) * SCALE, WHITE);
            if (cursorY < BOARD_HEIGHT-1) display.drawPixel(cursorX * SCALE + 1, (cursorY+1) * SCALE, WHITE);
        } else {
            // Regular cursor for other games
            int color = cellIsLit ? WHITE : BLACK;
            display.drawPixel(cursorX * SCALE, cursorY * SCALE, color);
            
            // Add a simple border around cursor position
            if (cursorX > 0) display.drawPixel((cursorX-1) * SCALE, cursorY * SCALE, WHITE);
            if (cursorX < BOARD_WIDTH-1) display.drawPixel((cursorX+1) * SCALE, cursorY * SCALE, WHITE);
            if (cursorY > 0) display.drawPixel(cursorX * SCALE, (cursorY-1) * SCALE, WHITE);
            if (cursorY < BOARD_HEIGHT-1) display.drawPixel(cursorX * SCALE, (cursorY+1) * SCALE, WHITE);
        }
    }
}

void randomizeBoard() {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        uint32_t col;
        col = board[x] = RAND_LONG() & RAND_LONG() & RAND_LONG();
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

void generateSymmetricPattern(int symmetryType) {
    clearBoard();
    
    switch(symmetryType) {
        case 1: // Vertical symmetry (mirror left-right)
            generateVerticalSymmetric();
            break;
        case 2: // Horizontal symmetry (mirror top-bottom)  
            generateHorizontalSymmetric();
            break;
        case 3: // 4-way rotational symmetry
            generateRotationalSymmetric();
            break;
    }
}

void generateVerticalSymmetric() {
    int halfWidth = BOARD_WIDTH / 2;
    for (int x = 0; x < halfWidth; x++) {
        uint32_t col = RAND_LONG() & RAND_LONG() & 0x0F0F0F0F; // Sparse pattern
        board[x] = col;
        board[BOARD_WIDTH - 1 - x] = col; // Mirror to right side
    }
}

void generateHorizontalSymmetric() {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        uint32_t col = 0;
        int halfHeight = BOARD_HEIGHT / 2;
        
        for (int y = 0; y < halfHeight; y++) {
            if (rand() % 5 == 0) { // 20% density
                col |= (1UL << y);
                col |= (1UL << (BOARD_HEIGHT - 1 - y)); // Mirror vertically
            }
        }
        board[x] = col;
    }
}

void generateRotationalSymmetric() {
    int centerX = BOARD_WIDTH / 2;
    int centerY = BOARD_HEIGHT / 2;
    
    // Generate in one quadrant, mirror to all 4
    for (int x = 0; x < centerX; x++) {
        for (int y = 0; y < centerY; y++) {
            if (rand() % 6 == 0) { // Sparse 16% density
                // Set all 4 rotational positions
                board[x] |= (1UL << y);
                board[BOARD_WIDTH - 1 - x] |= (1UL << (BOARD_HEIGHT - 1 - y));
                board[BOARD_WIDTH - 1 - x] |= (1UL << y);
                board[x] |= (1UL << (BOARD_HEIGHT - 1 - y));
            }
        }
    }
}

void generateSmallSymmetricPattern(int symmetryType) {
    clearBoard();
    
    switch(symmetryType) {
        case 1: // Vertical symmetry - Small (guaranteed 20-30 cells)
            {
                int halfWidth = BOARD_WIDTH / 2;
                int startX = halfWidth - 8;  // Tighter range for density
                int endX = halfWidth - 1;
                for (int x = startX; x < endX; x++) {
                    uint32_t col = 0;
                    for (int y = 12; y < 20; y++) {  // Concentrated vertical area (8 rows)
                        if (rand() % 3 == 0) {  // 33% density for guaranteed cells
                            col |= (1UL << y);
                        }
                    }
                    board[x] = col;
                    board[BOARD_WIDTH - 1 - x] = col;  // Mirror
                }
                
                // Ensure minimum cells by adding a few guaranteed ones
                board[halfWidth - 4] |= (1UL << 14) | (1UL << 16);
                board[BOARD_WIDTH - 1 - (halfWidth - 4)] |= (1UL << 14) | (1UL << 16);
                board[halfWidth - 3] |= (1UL << 15);
                board[BOARD_WIDTH - 1 - (halfWidth - 3)] |= (1UL << 15);
            }
            break;
            
        case 2: // Horizontal symmetry - Small (guaranteed 20-30 cells)
            {
                int startX = BOARD_WIDTH/2 - 12;  // Tighter horizontal range
                int endX = BOARD_WIDTH/2 + 12;
                startX = (startX < 0) ? 0 : startX;
                endX = (endX >= BOARD_WIDTH) ? BOARD_WIDTH : endX;
                
                for (int x = startX; x < endX; x++) {
                    uint32_t col = 0;
                    int halfHeight = BOARD_HEIGHT / 2;
                    
                    for (int y = 4; y < halfHeight - 2; y++) {  // Concentrated area
                        if (rand() % 4 == 0) { // 25% density
                            col |= (1UL << y);
                            col |= (1UL << (BOARD_HEIGHT - 1 - y));
                        }
                    }
                    board[x] = col;
                }
                
                // Add guaranteed core cells
                int centerX = BOARD_WIDTH / 2;
                board[centerX - 2] |= (1UL << 6) | (1UL << (BOARD_HEIGHT - 1 - 6));
                board[centerX] |= (1UL << 7) | (1UL << (BOARD_HEIGHT - 1 - 7));
                board[centerX + 2] |= (1UL << 6) | (1UL << (BOARD_HEIGHT - 1 - 6));
            }
            break;
            
        case 3: // 4-way rotational - Small (guaranteed 20-30 cells)
            {
                int centerX = BOARD_WIDTH / 2;
                int centerY = BOARD_HEIGHT / 2;
                
                for (int x = centerX - 8; x < centerX; x++) {  // Tighter range
                    for (int y = centerY - 6; y < centerY; y++) {
                        if (rand() % 5 == 0) { // 20% density
                            board[x] |= (1UL << y);
                            board[BOARD_WIDTH - 1 - x] |= (1UL << (BOARD_HEIGHT - 1 - y));
                            board[BOARD_WIDTH - 1 - x] |= (1UL << y);
                            board[x] |= (1UL << (BOARD_HEIGHT - 1 - y));
                        }
                    }
                }
                
                // Add guaranteed core pattern
                board[centerX - 2] |= (1UL << (centerY - 1)) | (1UL << (centerY + 1));
                board[centerX + 1] |= (1UL << (centerY - 1)) | (1UL << (centerY + 1));
                board[centerX - 1] |= (1UL << centerY);
                board[centerX] |= (1UL << centerY);
            }
            break;
            
    }
}

void generateMediumSymmetricPattern(int symmetryType) {
    clearBoard();
    
    switch(symmetryType) {
        case 1: // Vertical symmetry - Medium (wider horizontal spread)
            {
                int halfWidth = BOARD_WIDTH / 2;
                int startX = halfWidth - 18;  // Much wider range
                int endX = halfWidth - 2;
                for (int x = startX; x < endX; x++) {
                    uint32_t col = 0;
                    for (int y = 2; y < 30; y++) {  // Nearly full vertical area
                        if (rand() % 5 == 0) {  // 20% density
                            col |= (1UL << y);
                        }
                    }
                    board[x] = col;
                    board[BOARD_WIDTH - 1 - x] = col;  // Mirror
                }
            }
            break;
        case 2: // Horizontal symmetry - Medium (wider in all directions)
            {
                int startX = BOARD_WIDTH/2 - 25;  // Much wider horizontal range
                int endX = BOARD_WIDTH/2 + 25;
                // Clamp to valid bounds
                startX = (startX < 0) ? 0 : startX;
                endX = (endX >= BOARD_WIDTH) ? BOARD_WIDTH : endX;
                for (int x = startX; x < endX; x++) {
                    uint32_t col = 0;
                    int halfHeight = BOARD_HEIGHT / 2;
                    
                    for (int y = 1; y < halfHeight; y++) {  // Wider vertical area
                        if (rand() % 5 == 0) { // 20% density
                            col |= (1UL << y);
                            col |= (1UL << (BOARD_HEIGHT - 1 - y));
                        }
                    }
                    board[x] = col;
                }
            }
            break;
        case 3: // 4-way rotational - Medium (spread in all directions)
            {
                int centerX = BOARD_WIDTH / 2;
                int centerY = BOARD_HEIGHT / 2;
                
                for (int x = centerX - 18; x < centerX; x++) {  // Much wider range
                    for (int y = centerY - 12; y < centerY; y++) {
                        if (rand() % 6 == 0) { // 16% density
                            board[x] |= (1UL << y);
                            board[BOARD_WIDTH - 1 - x] |= (1UL << (BOARD_HEIGHT - 1 - y));
                            board[BOARD_WIDTH - 1 - x] |= (1UL << y);
                            board[x] |= (1UL << (BOARD_HEIGHT - 1 - y));
                        }
                    }
                }
            }
            break;
    }
}

void initializeDayNight() {
    // Start with a random sparse pattern for Day & Night
    clearBoard();
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board[x] = RAND_LONG(); // 50% density - no ANDs or ORs
    }
    cellCount = 0;
    generation = 0;
}

void updateDayNight() {
    uint32_t new_board[BOARD_WIDTH];
    
    // Initialize new board
    for (int x = 0; x < BOARD_WIDTH; x++) {
        new_board[x] = 0;
    }
    
    cellCount = 0;
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            bool isAlive = (board[x] & (1UL << y)) != 0;
            
            // Count neighbors
            int neighbors = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = (x + dx + BOARD_WIDTH) % BOARD_WIDTH;
                    int ny = (y + dy + BOARD_HEIGHT) % BOARD_HEIGHT;
                    
                    if (board[nx] & (1UL << ny)) {
                        neighbors++;
                    }
                }
            }
            
            // Day & Night rules (inverse of Conway's):
            // Birth: 3, 6, 7, 8 neighbors
            // Survival: 3, 4, 6, 7, 8 neighbors
            bool shouldLive = false;
            if (isAlive) {
                // Survival rules
                if (neighbors == 3 || neighbors == 4 || neighbors == 6 || neighbors == 7 || neighbors == 8) {
                    shouldLive = true;
                }
            } else {
                // Birth rules  
                if (neighbors == 3 || neighbors == 6 || neighbors == 7 || neighbors == 8) {
                    shouldLive = true;
                }
            }
            
            if (shouldLive) {
                new_board[x] |= (1UL << y);
                cellCount++;
            }
        }
    }
    
    // Update the global board
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board[x] = new_board[x];
    }
}

void drawDayNight() {
    display.clearDisplay();
    
    // Draw all living cells as solid pixels
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            if (board[x] & (1UL << y)) {
                drawCell(x, y, WHITE);
            }
        }
    }
    
    // Show generation counter and cell count across the top - smaller text
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("G:");
    display.print(generation);
    display.setCursor(50, 0);
    display.print("C:");
    display.print(cellCount);
  
    display.display();
}

void initializeLangtonsAnt() {
    clearBoard();
    // Start ant in center
    antX = BOARD_WIDTH / 2;
    antY = BOARD_HEIGHT / 2;
    antDirection = 0; // Start facing North
    lastAntMove = millis();
    langtonAntMode = true;
}

void updateLangtonsAnt() {
    unsigned long now = millis();
    if (now - lastAntMove < ANT_MOVE_DELAY) return;
    
    lastAntMove = now;
    
    // Check current cell state
    bool currentCellOn = (board[antX] >> antY) & 1;
    
    if (currentCellOn) {
        // On white cell: turn right, flip cell to black, move forward
        antDirection = (antDirection + 1) % 4;
        board[antX] &= ~(1ULL << antY); // Turn cell off
    } else {
        // On black cell: turn left, flip cell to white, move forward  
        antDirection = (antDirection + 3) % 4; // +3 is same as -1 mod 4
        board[antX] |= (1ULL << antY); // Turn cell on
    }
    
    // Move ant forward
    switch(antDirection) {
        case 0: // North
            antY = (antY > 0) ? antY - 1 : BOARD_HEIGHT - 1;
            break;
        case 1: // East
            antX = (antX < BOARD_WIDTH - 1) ? antX + 1 : 0;
            break;
        case 2: // South
            antY = (antY < BOARD_HEIGHT - 1) ? antY + 1 : 0;
            break;
        case 3: // West
            antX = (antX > 0) ? antX - 1 : BOARD_WIDTH - 1;
            break;
    }
    
    generation++;
}

void drawLangtonsAnt() {
    display.clearDisplay();
    
    // Draw all cells
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            if (board[x] & (1UL << y)) {
                drawCell(x, y, WHITE);
            }
        }
    }
    
    // Draw the ant as a blinking pixel
    bool blinkState = (millis() / 200) % 2;
    if (blinkState) {
        // Draw ant as inverted pixel
        bool cellState = (board[antX] & (1UL << antY)) != 0;
        drawCell(antX, antY, cellState ? BLACK : WHITE);
    }
    
    // Show generation counter
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("G:");
    display.print(generation);
    display.setCursor(70, 0);
    display.print("X:");
    display.print(antX);
    display.setCursor(100, 0);
    display.print("Y:");
    display.print(antY);
    
    display.display();
}

void initializeSeeds() {
    clearBoard();
    int centerX = BOARD_WIDTH / 2;
    int centerY = BOARD_HEIGHT / 2;
    
    // Generate symmetrical pattern in tight center area (16x8)
    int halfWidth = 8;  // Generate in 8 columns on left, mirror to right
    int halfHeight = 4; // Generate in 4 rows above center, mirror below
    
    for (int x = centerX - halfWidth; x < centerX; x++) {
        uint32_t col = 0;
        
        // Generate pattern in upper half only
        for (int y = centerY - halfHeight; y < centerY; y++) {
            if (rand() % 12 == 0) {  // 8.3% density for good coverage
                col |= (1UL << y);
                col |= (1UL << (centerY + (centerY - y - 1))); // Mirror vertically
            }
        }
        
        // Set both left and right sides
        board[x] = col;
        board[centerX + (centerX - x - 1)] = col; // Mirror horizontally
    }
    
    // Add a few guaranteed symmetrical seed pairs in the very center
    int guaranteedPairs = 2 + (rand() % 2); // 2-3 symmetrical pairs
    for (int i = 0; i < guaranteedPairs; i++) {
        int offsetX = 1 + (rand() % 4);  // 1-4 columns from center
        int offsetY = 1 + (rand() % 3);  // 1-3 rows from center
        
        // Create 4-way symmetrical pattern
        board[centerX - offsetX] |= (1UL << (centerY - offsetY));
        board[centerX + offsetX - 1] |= (1UL << (centerY - offsetY));
        board[centerX - offsetX] |= (1UL << (centerY + offsetY - 1));
        board[centerX + offsetX - 1] |= (1UL << (centerY + offsetY - 1));
    }
    
    cellCount = 0;
    generation = 0;
    seedsMode = true;
}

void updateSeeds() {
    uint32_t new_board[BOARD_WIDTH];
    
    // Initialize new board - all cells die each generation
    for (int x = 0; x < BOARD_WIDTH; x++) {
        new_board[x] = 0;
    }
    
    cellCount = 0;
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            // Count neighbors
            int neighbors = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = (x + dx + BOARD_WIDTH) % BOARD_WIDTH;
                    int ny = (y + dy + BOARD_HEIGHT) % BOARD_HEIGHT;
                    
                    if (board[nx] & (1UL << ny)) {
                        neighbors++;
                    }
                }
            }
            
            // Seeds rules: Birth with exactly 2 neighbors, all cells die immediately
            if (neighbors == 2) {
                new_board[x] |= (1UL << y);
                cellCount++;
            }
            // All existing cells die (no survival conditions)
        }
    }
    
    // Update the global board
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board[x] = new_board[x];
    }
}

void drawSeeds() {
    display.clearDisplay();
    
    // Draw all living cells as solid pixels
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            if (board[x] & (1UL << y)) {
                drawCell(x, y, WHITE);
            }
        }
    }
    
    // Show generation counter and cell count
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("G:");
    display.print(generation);
    display.setCursor(50, 0);
    display.print("C:");
    display.print(cellCount);
    
    display.display();
}

void updateBriansBrain() {
    uint32_t new_board[BOARD_WIDTH];
    uint32_t new_dying[BOARD_WIDTH];
    
    // Initialize new arrays
    for (int x = 0; x < BOARD_WIDTH; x++) {
        new_board[x] = 0;
        new_dying[x] = 0;
    }
    
    cellCount = 0;
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        int prev_x = (x == 0) ? BOARD_WIDTH - 1 : x - 1;
        int next_x = (x == BOARD_WIDTH - 1) ? 0 : x + 1;
        
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            int prev_y = (y == 0) ? BOARD_HEIGHT - 1 : y - 1;
            int next_y = (y == BOARD_HEIGHT - 1) ? 0 : y + 1;
            
            bool isAlive = (board[x] & (1UL << y)) != 0;
            bool isDying = (briansBrain_dying[x] & (1UL << y)) != 0;
            
            // Count living neighbors (not dying ones)
            int neighbors = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = (x + dx + BOARD_WIDTH) % BOARD_WIDTH;
                    int ny = (y + dy + BOARD_HEIGHT) % BOARD_HEIGHT;
                    
                    if ((board[nx] & (1UL << ny)) && !(briansBrain_dying[nx] & (1UL << ny))) {
                        neighbors++;
                    }
                }
            }
            
            // Brian's Brain rules:
            if (isAlive && !isDying) {
                // Living cell becomes dying
                new_dying[x] |= (1UL << y);
            } else if (!isAlive && !isDying && neighbors == 2) {
                // Dead cell with exactly 2 living neighbors becomes alive
                new_board[x] |= (1UL << y);
                cellCount++;
            }
            // Dying cells become dead (do nothing - they stay 0 in new arrays)
        }
    }
    
    // Update the global arrays
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board[x] = new_board[x];
        briansBrain_dying[x] = new_dying[x];
    }
}

void drawBriansBrain() {
    display.clearDisplay();
    
    // Get current time for blinking effect - faster blink (100ms)
    bool blinkState = (millis() / 100) % 2;  
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            bool isAlive = (board[x] & (1UL << y)) != 0;
            bool isDying = (briansBrain_dying[x] & (1UL << y)) != 0;
            
            if (isAlive && !isDying) {
                // Living cells - single white pixel
                display.drawPixel(x * SCALE + 1, y * SCALE + 1, WHITE);
            } else if (isDying) {
                // Dying cells - fast blinking single pixel
                if (blinkState) {
                    display.drawPixel(x * SCALE + 1, y * SCALE + 1, WHITE);
                }
                // When not blinking, they appear black (dead-looking)
            }
            // Dead cells - nothing (black background)
        }
    }
    
    // Show generation counter and cell count across the top
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("G: ");
    display.print(generation);
    display.setCursor(60, 0);
    display.print("C: ");
    display.print(cellCount);
    
    display.display();
}

void clearBoard() {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board[x] = 0;
      briansBrain_dying[x] = 0;  // ADD THIS LINE
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
    
   if (inSymmetricSubMenu) {
        display.println("SYMMETRIC PATTERNS");
    } else if (inPatternsSubMenu) {
        display.println("PRESET PATTERNS");
    } else if (inSeedsSubMenu) {
        display.println("SEEDS GAME");
    } else if (inAltGamesSubMenu) {
        display.println("ALTERNATIVE GAMES");
    } else {
        display.println("GAME OF LIFE");
    }
    display.setTextSize(2);
    
    // Menu options
    display.setTextSize(1);
    
    if (inSeedsSubMenu) {
        // Seeds submenu
        if (seedsGameSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Random");
        
        if (seedsGameSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Custom");
    } else if (inSymmetricSubMenu) {
        // Symmetric size submenu
        if (symmetricSizeSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Small");
        
        if (symmetricSizeSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Medium");
        
        if (symmetricSizeSelection == 3) {
            display.fillRect(0, 35, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 37);
        display.println("3: Large");
    } else if (inPatternsSubMenu) {
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
    } else if (inAltGamesSubMenu) {
        // Alt Games submenu
        if (altGameSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Brians Brain");
        
        if (altGameSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Day & Night");
        
        if (altGameSelection == 3) {
            display.fillRect(0, 35, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 37);
        display.println("3: Seeds");
        
        if (altGameSelection == 4) {
            display.fillRect(0, 45, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 47);
        display.println("4: Langton Ant");
     } else if (inBriansBrainSubMenu) {
        // Brian's Brain size submenu - match symmetric format
        if (briansBrainSizeSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Small");
        
        if (briansBrainSizeSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Medium");
        
        if (briansBrainSizeSelection == 3) {
            display.fillRect(0, 35, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 37);
        display.println("3: Large");
        
        if (briansBrainSizeSelection == 4) {
            display.fillRect(0, 45, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 47);
        display.println("4: Random");
        
        if (briansBrainSizeSelection == 5) {
            display.fillRect(0, 55, 128, 8, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 57);
        display.println("5: Custom");
    } else if (inSeedsSubMenu) {
        // Seeds submenu
        if (seedsGameSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Random");
        
        if (seedsGameSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Custom"); 
    } else {
        // Main menu
        if (menuSelection == 1) {
            display.fillRect(0, 15, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 17);
        display.println("1: Preset Patterns");

        if (menuSelection == 2) {
            display.fillRect(0, 25, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 27);
        display.println("2: Random Game");

        if (menuSelection == 3) {
            display.fillRect(0, 35, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 37);
        display.println("3: Symmetric Game");

        if (menuSelection == 4) {
            display.fillRect(0, 45, 128, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 47);
        display.println("4: Custom Game");
        
        if (menuSelection == 5) {
            display.fillRect(0, 55, 128, 8, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, 57);
        display.println("5: Alt Games");
    }
    
    display.display();
}

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
    if (inSymmetricSubMenu) {
                symmetricSizeSelection = (symmetricSizeSelection > 1) ? symmetricSizeSelection - 1 : 3;
      } else if (inAltGamesSubMenu) {
                altGameSelection = (altGameSelection > 1) ? altGameSelection - 1 : 4;
      } else if (inBriansBrainSubMenu) {
    briansBrainSizeSelection = (briansBrainSizeSelection > 1) ? briansBrainSizeSelection - 1 : 5;
        } else if (inSeedsSubMenu) {
    seedsGameSelection = (seedsGameSelection > 1) ? seedsGameSelection - 1 : 2;
} else if (!inPatternsSubMenu) {
                menuSelection = (menuSelection > 1) ? menuSelection - 1 : 5;    
            } else {
                patternSelection = (patternSelection > 1) ? patternSelection - 1 : 5;
            }   
            lastDirectionalPress = now;
            showMenu();
        }
        else if (isBtnDown(btnValue)) {
           if (inSymmetricSubMenu) {
                symmetricSizeSelection = (symmetricSizeSelection < 4) ? symmetricSizeSelection + 1 : 1;
             } else if (inAltGamesSubMenu) {
                altGameSelection = (altGameSelection < 4) ? altGameSelection + 1 : 1;
            } else if (inBriansBrainSubMenu) {
    briansBrainSizeSelection = (briansBrainSizeSelection < 5) ? briansBrainSizeSelection + 1 : 1;   
            } else if (inSeedsSubMenu) {
    seedsGameSelection = (seedsGameSelection < 2) ? seedsGameSelection + 1 : 1;
} else if (!inPatternsSubMenu) { 
                menuSelection = (menuSelection < 5) ? menuSelection + 1 : 1;
            } else {
                patternSelection = (patternSelection < 5) ? patternSelection + 1 : 1;
            }
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
    if (gameMode == 4) {  // Symmetric Game
        // Generate new random symmetry type and pattern
        currentSymmetryType = rand() % 3 + 1;  // Random 1-3
        
        switch(currentSymmetrySize) {
            case 1: // Small
                generateSmallSymmetricPattern(currentSymmetryType);
                break;
            case 2: // Medium
                generateMediumSymmetricPattern(currentSymmetryType);
                break;
            case 3: // Large
                generateSymmetricPattern(currentSymmetryType);
                break;
        }
    } else if (gameMode == 8 && briansBrainMode) {  // Brian's Brain - IMPROVED VERSION
        // Clear both arrays and regenerate pattern based on stored size
        clearBoard(); // This clears both board[] and briansBrain_dying[]
        
        switch(currentBriansBrainSize) {
            case 1: // Small
                generateSmallSymmetricPattern(1);
                break;
            case 2: // Medium
                generateMediumSymmetricPattern(1);
                break;
            case 3: // Large
                generateSymmetricPattern(1);
                break;
            case 4: // Random
                randomizeBoard(); // Generate new random pattern
                break;
            case 5: // Custom
                // Return to edit mode for custom
                editMode = true;
                cursorX = BOARD_WIDTH / 2;
                cursorY = BOARD_HEIGHT / 2;
                generation = 0;
                lastDirectionalPress = now;
                return; // Exit early, don't reset generation/frame times
        }
        
       // Reset generation counter (only for non-custom modes)
        generation = 0;
        cellCount = 0;
        frame_start_t = millis() - frameDelays[currentSpeedIndex]; // Force immediate update
        
    } else if (seedsMode) {  // Seeds mode reset
        initializeSeeds();  // Use same ultra-sparse pattern as initial screen
    } else {
        if (gameMode == 5) {  // Day & Night mode needs sparse initialization
            initializeDayNight();
        } else {
            randomizeBoard();  // Normal density for Conway's Life, Seeds, etc.
        }
    }
    generation = 0;
    start_t = micros();
    frame_start_t = millis();
    lastDirectionalPress = now;
}
else if (isBtnRight(btnValue)) {
        initializeSeeds();  // Use same ultra-sparse pattern as initial screen
    } else {
        if (gameMode == 5) {  // Day & Night mode needs sparse initialization
            initializeDayNight();
        } else {
            randomizeBoard();  // Normal density for Conway's Life, Seeds, etc.
        }
    }
    generation = 0;
        start_t = micros();
        frame_start_t = millis();
        lastDirectionalPress = now;
    }
    }

void handleSetButton() {
    unsigned long now = millis();
    static unsigned long lastSetPress = 0;
    static unsigned long setButtonPressStart = 0;
    static bool setButtonPressed = false;
    static bool justEnteredSubmenu = false;
    static unsigned long submenuEntryTime = 0;
    
    int btnValue = readButtonValue();
    bool currentlyPressed = isBtnSet(btnValue);
    
    // Debounce timing
    if (now - lastSetPress < 50) return;
    
    // Check for long press while button is still pressed
    if (currentlyPressed && setButtonPressed && editMode && !inMenuMode) {
        unsigned long pressDuration = now - setButtonPressStart;
        if (pressDuration >= 800) {  // Long press detected - start immediately
            editMode = false;
            generation = 0;
            start_t = micros();
            frame_start_t = millis();
            flashLED();
            setButtonPressed = false;
            
            // Ensure mode flags are set for all custom games
            if (gameMode == 2) {
                // Regular custom game - no special flags needed
            } else if (gameMode == 6) {
                seedsMode = true;
            } else if (gameMode == 8) {
                briansBrainMode = true;  // Force set the mode
            }
            
            return;
        }
    }
    
    if (currentlyPressed && !setButtonPressed) {
        // Button just pressed
        setButtonPressed = true;
        setButtonPressStart = now;
        lastSetPress = now;
        
        if (inMenuMode) {
            if (buttonProcessingInProgress) return;
            buttonProcessingInProgress = true;
            
            if (!inPatternsSubMenu && !inSymmetricSubMenu && !inAltGamesSubMenu && !inBriansBrainSubMenu && !inSeedsSubMenu) {
                // Main menu selections
                switch(menuSelection) {
                    case 1: // Preset Patterns
                        inPatternsSubMenu = true;
                        patternSelection = 1;
                        justEnteredSubmenu = true;
                        submenuEntryTime = now;
                        showMenu();
                        return;
                        
                    case 2: // Random Game
                        gameMode = 1;
                        randomizeBoard();
                        editMode = false;
                        inMenuMode = false;
                        generation = 0;
                        start_t = micros();
                        frame_start_t = millis();
                        buttonProcessingInProgress = false;
                        break;
                        
                    case 3: // Symmetric Game  
                        inSymmetricSubMenu = true;
                        symmetricSizeSelection = 1;
                        justEnteredSubmenu = true;
                        submenuEntryTime = now;
                        showMenu();
                        return;
                        
                    case 4: // Custom Game
                        gameMode = 2;
                        clearBoard();
                        cursorX = BOARD_WIDTH / 2;
                        cursorY = BOARD_HEIGHT / 2;
                        editMode = true;
                        inMenuMode = false;
                        generation = 0;
                        buttonProcessingInProgress = false;
                        break;
                        
                    case 5: // Alt Games
                        inAltGamesSubMenu = true;
                        altGameSelection = 1;
                        justEnteredSubmenu = true;        
                        submenuEntryTime = now;          
                        showMenu();
                        return;
                }
            } 
            else if (inPatternsSubMenu && (!justEnteredSubmenu || (now - submenuEntryTime > 400))) {
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
                buttonProcessingInProgress = false;
            } 
            else if (inAltGamesSubMenu && (!justEnteredSubmenu || (now - submenuEntryTime > 400))) {
                // Handle Alt Games submenu selections
                justEnteredSubmenu = false;  // Clear the flag
                switch(altGameSelection) {
                    case 1: // Brian's Brain
                        inBriansBrainSubMenu = true;
                        briansBrainSizeSelection = 1;
                        inAltGamesSubMenu = false;  // Exit Alt Games menu
                        justEnteredSubmenu = true;
                        submenuEntryTime = now;
                        showMenu();
                        return;
                    case 2: // Day & Night
                        gameMode = 5;
                        dayNightMode = true;
                        initializeDayNight();
                        editMode = false;
                        inMenuMode = false;
                        inAltGamesSubMenu = false;
                        generation = 0;
                        frame_start_t = now;
                        buttonProcessingInProgress = false;
                        break;
                    case 3: // Seeds
                        inSeedsSubMenu = true;
                        seedsGameSelection = 1;
                        inAltGamesSubMenu = false;
                        justEnteredSubmenu = true;
                        submenuEntryTime = now;
                        showMenu();
                        return;
                    case 4: // Langton's Ant
                        gameMode = 7;
                        langtonAntMode = true;
                        initializeLangtonsAnt();
                        editMode = false;
                        inMenuMode = false;
                        inAltGamesSubMenu = false;
                        generation = 0;
                        frame_start_t = now;
                        buttonProcessingInProgress = false;
                        break;
                }
            } 
            else if (inBriansBrainSubMenu && (!justEnteredSubmenu || (now - submenuEntryTime > 400))) {
    // Brian's Brain size selection
    justEnteredSubmenu = false;
    gameMode = 8;
    briansBrainMode = true;
    
    // STORE the current size selection for later resets
    currentBriansBrainSize = briansBrainSizeSelection;
    
    // Exit all menu states
    inBriansBrainSubMenu = false;
    inAltGamesSubMenu = false;
    
    // Handle Custom option differently
    if (briansBrainSizeSelection == 5) {
        // Custom - go to edit mode
        inMenuMode = false;
        editMode = true;
        clearBoard();
        cursorX = BOARD_WIDTH / 2;
        cursorY = BOARD_HEIGHT / 2;
        generation = 0;
        buttonProcessingInProgress = false;
        return;
    }
    
    // For all other options (Small, Medium, Large, Random)
    inMenuMode = false;
    editMode = false;
    
    // Clear both board arrays
    clearBoard();
    
    // Generate initial pattern based on selected size
    switch(briansBrainSizeSelection) {
        case 1: // Small
            generateSmallSymmetricPattern(1);
            break;
        case 2: // Medium
            generateMediumSymmetricPattern(1);
            break;
        case 3: // Large
            generateSymmetricPattern(1);
            break;
        case 4: // Random
            randomizeBoard();
            break;
    }
    
    // Initialize Brian's Brain specific variables
    cellCount = 0;
    generation = 0;
    frame_start_t = now - frameDelays[currentSpeedIndex]; // Force immediate first update
    start_t = micros();
    buttonProcessingInProgress = false;  // Clear this flag
    
    // Force immediate display update
    drawBriansBrain();
    return;
}
            else if (inSymmetricSubMenu && (!justEnteredSubmenu || (now - submenuEntryTime > 400))) {
                // Handle symmetric size selection
                justEnteredSubmenu = false;
                gameMode = 4;
                currentSymmetrySize = symmetricSizeSelection; // Store selected size
                currentSymmetryType = rand() % 3 + 1; // Random symmetry type (1-3)
                
                switch(currentSymmetrySize) {
                    case 1: // Small
                        generateSmallSymmetricPattern(currentSymmetryType);
                        break;
                    case 2: // Medium
                        generateMediumSymmetricPattern(currentSymmetryType);
                        break;
                    case 3: // Large
                        generateSymmetricPattern(currentSymmetryType);
                        break;
                }
                
                inMenuMode = false;
                inSymmetricSubMenu = false;
                editMode = false;
                generation = 0;
                start_t = micros();
                frame_start_t = millis();
                buttonProcessingInProgress = false;
            }
        else if (inSeedsSubMenu && (!justEnteredSubmenu || (now - submenuEntryTime > 400))) {
    justEnteredSubmenu = false;
    gameMode = 6;
    seedsMode = true;
    
    switch(seedsGameSelection) {
        case 1: // Random Seeds
            seedsWasCustom = false;  // Track that this was random
            initializeSeeds();
            editMode = false;
            break;
        case 2: // Custom Seeds
            seedsWasCustom = true;   // Track that this was custom
            clearBoard();
            cursorX = BOARD_WIDTH / 2;
            cursorY = BOARD_HEIGHT / 2;
            editMode = true;
            break;
    }
    
    inSeedsSubMenu = false;
    inMenuMode = false;
    generation = 0;
    frame_start_t = now;
    buttonProcessingInProgress = false;
}    
        }
    } 
    else if (!currentlyPressed && setButtonPressed) {
        // Button released
        unsigned long pressDuration = now - setButtonPressStart;
        setButtonPressed = false;
        
        if (editMode && !inMenuMode) {
            if (pressDuration < 800 && pressDuration >= 50) {  // Only short press - toggle cell
                toggleCellAtCursor();
                flashLED();
            }
        }
        
        // Clear processing flag when button is released
        if (buttonProcessingInProgress && pressDuration > 100) {
            buttonProcessingInProgress = false;
        }
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
         
        // Handle Brian's Brain submenu specifically
        if (inBriansBrainSubMenu) {
            inBriansBrainSubMenu = false;
            inAltGamesSubMenu = true;  // Return to Alt Games menu
            altGameSelection = 1;     // Reset to Brian's Brain
            showMenu();
            return;
        }
        // Handle Seeds submenu specifically
        // Handle Seeds submenu specifically
        if (inSeedsSubMenu) {
            inSeedsSubMenu = false;
            inAltGamesSubMenu = true;  // Return to Alt Games menu
            altGameSelection = 3;     // Reset to Seeds
            showMenu();
            return;
        }
        // Reset all states and return to main menu
        inMenuMode = true;
        inPatternsSubMenu = false;
        inSymmetricSubMenu = false;
        inAltGamesSubMenu = false;
        inBriansBrainSubMenu = false;  // ADD THIS LINE
        briansBrainMode = false;
        inSeedsSubMenu = false;
        dayNightMode = false;
        seedsMode = false;
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
            
               switch (gameMode) {
                case 1:
                case 2:
                case 3:
                    // Conway's Game of Life (existing code below)
                    break;
                    
                case 4: // Symmetric Game (keep this for regular symmetric patterns)
                    break;
                    
                case 5: // Day & Night
                    if (dayNightMode) {
                        updateDayNight();
                        drawDayNight();
                        return; // Skip Conway's logic
                    }
                    break;
                    
                case 6: // Seeds
    if (seedsMode) {
        if (editMode) {
            // Custom Seeds - show edit interface
            display.clearDisplay();
            for (int x = 0; x < BOARD_WIDTH; x++) {
                for (int y = 0; y < BOARD_HEIGHT; y++) {
                    if (board[x] & (1UL << y)) {
                        drawCell(x, y, WHITE);
                    }
                }
            }
            drawCursor();
            display.display();
            return;
        } else {
            // Running Seeds simulation
            updateSeeds();
            drawSeeds();
            
            // Auto-reset based on original mode
            if (cellCount == 0 || (cellCount > 500 && generation > 150)) {
                if (seedsWasCustom) {
                    // Return to edit mode for custom Seeds
                    clearBoard();
                    editMode = true;
                    cursorX = BOARD_WIDTH / 2;
                    cursorY = BOARD_HEIGHT / 2;
                } else {
                    // Generate new random pattern for random Seeds
                    initializeSeeds();
                }
                generation = 0;
            }
            return;
        }
    }
    break;
                    
                case 7: // Langton's Ant
                    if (langtonAntMode) {
                        updateLangtonsAnt();
                        drawLangtonsAnt();
                        return; // Skip Conway's logic
                    }
                    break;
                    
                case 8: // Brian's Brain - ADD THIS CASE
                    if (briansBrainMode) {
                        updateBriansBrain();
                        drawBriansBrain();
                        return; // Skip Conway's logic
                    }
                    break;
            }

            
            // Conway's Game of Life logic (only runs for cases 1, 2, 3)
            if (gameMode >= 1 && gameMode <= 3) {
            } else if (dayNightMode) {
                updateDayNight();
                drawDayNight();
                return; // Skip Conway's logic
           }

            cellCount = 0; // Reset cell count before each generation

            // Clear display for Conway's Game of Life
            display.clearDisplay();
            
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
                            drawCell(x - 1, y, WHITE);
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
                    drawCell(0, y, WHITE);
                }
                first_col >>= 1;
                
                if (last_col & 1) {
                    drawCell(BOARD_WIDTH - 1, y, WHITE);
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

      if (!briansBrainMode) {
            display.display();
        }
        // Brian's Brain handles its own display in drawBriansBrain()
      
    }

      else if (editMode && (gameMode == 2 || gameMode == 6 || (gameMode == 8 && briansBrainMode))) {  // Show cursor for custom games, custom Seeds, and custom Brian's Brain
       // In edit mode, just draw the current state and cursor
        display.clearDisplay();
        
        if (gameMode == 8 && briansBrainMode) {
            // Brian's Brain edit mode - show living cells as single pixels
            for (int x = 0; x < BOARD_WIDTH; x++) {
                for (int y = 0; y < BOARD_HEIGHT; y++) {
                    bool isAlive = (board[x] & (1UL << y)) != 0;
                    if (isAlive) {
                        // Show as single white pixel like in running mode
                        display.drawPixel(x * SCALE + 1, y * SCALE + 1, WHITE);
                    }
                }
            }
        } else {
            // Regular edit mode for other games
            for (int x = 0; x < BOARD_WIDTH; x++) {
                uint32_t col = board[x];
                for (int y = 0; y < BOARD_HEIGHT; y++) {
                    if (col & (1UL << y)) {
                        drawCell(x, y, WHITE);
                    }
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

