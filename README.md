![OOLO0046](https://github.com/user-attachments/assets/da0e76cf-106c-456c-9749-726d07561c74)        ![QAZB0208](https://github.com/user-attachments/assets/6ac36b5f-e18f-4702-b923-bd1952dfdfd4)

YouTube Vid
Short - https://youtube.com/shorts/Atr1aoWWVLA

Long - https://youtu.be/dLQRpCf72iw


The Game of Life is a cellular automation created by mathematician John Conway. It's what is known as a zero player game, meaning that its evolution and game play is determined by its initial state and requires no further input. You interact with the Game of Life by creating an initial configuration and observing how it evolves.

The game itself is based on a few, simple, mathematical rules consisting of a grid of cells that can either live, die or multiply. When the game is run, the cells can give the illusion that they are alive which is what makes this game so interesting.
Now you know what it is – what does my hand held game version do?


Core Game


Conways 'Game of Life' based on toroidal world: Edges wrap around (top connects to bottom, left to right).  Game includes the following menus:

1. Preset patterns - 5 famous Conway patterns including Coe Ship, Gosper Glider Gun, 4-8-12 Diamond, Achim's p144, 56P6H1V
   
2. Random generation - Starts with random cell pattern, auto-resets when pattern dies/repeats
   
3. Symmetric pattern generator - Generate symmetric patterns with 3 sizes Small, Medium, Large & Random. Symmetry type: Vertical, Horizontal, or 4-way Rotational
   
4. Custom builder - Interactive editor to design your own starting patterns
   
5. Alt Games - see below




Alternative Games

Four Alternative cellular automata available to play in the Alt Menu.  These include: 

1. Brian's Brain - 3-state automaton (alive/dying/dead) 5 modes: Small/Medium/Large/Random/Custom Visual: solid pixels (alive), blinking pixels (dying)

2. Day & Night - Inverse Conway rules creating dense patterns 2 density options: Medium Start (~35%), Dense Start (~50%) Birth: 3,6,7,8 neighbors | Survival: 3,4,6,7,8 neighbors

3. Seeds - Birth-only automaton creating explosive patterns 2 modes: Random (symmetric center pattern), Custom Birth with exactly 2 neighbors, all cells die each generation

4. Cyclic CA - Multi-state cycling automaton 3 pattern types: Vertical Symmetry, 4-Way Rotational, Random 6 visual states with different blink/display patterns



Features

1. Interactive pattern editor with blinking cursor

2. Real-time generation counter and cell statistics (some modes)

3. Comprehensive menu system with multiple submenus

4. Smart Detection: Automatically detects when patterns die out or start repeating

5. Automatic pattern detection and game reset (some modes)

6. Menu System: Clean interface with pattern submenu


   
HARDWARE REQUIREMENTS:

1. Arduino-compatible microcontroller (tested on Trinket M0)
   
2. SSD1306 128x64 OLED display (I2C)
 
3. 5-button resistor ladder on analog pin A3
  
4.  Button B on analog pin A4 (menu return/navigation)

   

BUTTON MAPPING (Resistor Ladder on A3):

 * • UP: ~39kΩ to 5V    (Navigation/Cursor up/Speed up)
 * • DOWN: ~33kΩ to 5V  (Navigation/Cursor down/Speed down) 
 * • LEFT: ~20kΩ to 5V  (Cursor left/New pattern/Back in menu)
 * • RIGHT: ~10kΩ to 5V (Cursor right/New pattern)
 * • SET: ~5.1kΩ to 5V  (Select/Toggle cell/Start simulation)

 * BUTTON B (on A4):
   
 * • ~15K to 5V    (Return to appropriate menu from anywhere)



  CONTROLS:

 * • Menu: UP/DOWN navigate, SET selects, Button B goes back
 * • Edit Mode: Arrow keys move cursor, SET toggles cell, long-press SET starts
 * • Simulation: UP/DOWN changes speed (12 levels)
 * • Simulation: LEFT/RIGHT regenerates pattern (game-specific behavior)
 * • Any mode: Button B returns to appropriate menu/submenu



   DISPLAY:

 * • 64x32 cell grid displayed at 2x scale on 128x64 OLED
 * • Real-time generation counter and cell statistics (most modes)
 * • Blinking cursor in edit mode
 * • Automatic game over detection and statistics display
 * • Game-specific visual indicators:
 *   - Brian's Brain: Blinking dying cells
 *   - Cyclic CA: Multiple blink patterns for different states (no text overlay)
 *   - Seeds/Day & Night: Standard cell display with counters

 
   ALGORITHM:

   
 * • Conway's Life: Optimized bit-manipulation with 3-bit rotation
 * • Toroidal (wraparound) boundary conditions
 * • Hash-based pattern repetition detection for auto-reset
 * • Efficient neighbor counting using lookup table
 * • Game-specific rule engines for each cellular automaton
 * • Pattern detection prevents infinite loops in Random/Custom modes
