![OOLO0046](https://github.com/user-attachments/assets/da0e76cf-106c-456c-9749-726d07561c74)        ![QAZB0208](https://github.com/user-attachments/assets/6ac36b5f-e18f-4702-b923-bd1952dfdfd4)

YouTube Vid
Short - https://youtube.com/shorts/Atr1aoWWVLA



I’ve been fascinated with Conway’s Game of Life ever since I read ‘The Recursive Universe’ way back in 2012. Since then, I wanted a way that I could play the game just like you would play a Nintendo ‘Game & Watch’ – a pocket-sized version that I could whip out anytime and start to explore & build my own patterns.

Fast-forward to 2025 and that idea has become reality! Arduino AI Assistant has been a massive help in building the code and there is absolutely no way that I could of done it without AI’s assistance. If you haven’t used it before and don’t know much about coding, then I strongly recommend that you give it a try.

So, what is Conway’s Game of Life (or GOL for short!). I recently gave the below description in another GOL project which might help those that haven’t heard of it before:

The Game of Life is a cellular automation created by mathematician John Conway. It's what is known as a zero player game, meaning that its evolution and game play is determined by its initial state and requires no further input. You interact with the Game of Life by creating an initial configuration and observing how it evolves.

The game itself is based on a few, simple, mathematical rules consisting of a grid of cells that can either live, die or multiply. When the game is run, the cells can give the illusion that they are alive which is what makes this game so interesting.
Now you know what it is – what does my hand held game version do?



Core Game

• Cellular Automaton: Simulates Conway's famous "zero-player game" where cells live/die based on neighbor count 
• Displayed on a 128x64 OLED screen 
• Toroidal World: Edges wrap around (top connects to bottom, left to right)



5 Game Modes (4 & 5 added in V2)
1. Random Game: Starts with random cell pattern, auto-resets when pattern dies/repeats
2. Custom Builder: Interactive editor to design your own starting patterns
3. Preset Patterns: 5 famous Conway patterns including Gosper Glider Gun
4. Custom Game: Build your own pattern with cursor, long-press SET to start
5. Alt Games: Alternative cellular automata collection
     • Brian's Brain: 3-state automaton with birth/dying/dead phases
     • Day & Night: Inverse Conway rules creating dense stable patterns
     • Seeds: Birth-only automaton creating explosive wave patterns
     • Langton's Ant: Single ant creating emergent highway patterns



Controls (5-button resistor ladder)

• Menu Navigation: UP/DOWN navigate, SET selects, LEFT goes back 
• Pattern Editor: Arrow keys move cursor, SET toggles cells, long-press SET starts simulation 
• During Simulation: UP/DOWN changes speed (12 levels), LEFT/RIGHT generates new random



Features

• Real-time Statistics: Generation counter, live cell count, max cells reached 
• Smart Detection: Automatically detects when patterns die out or start repeating 
• Game Over Screen: Shows final statistics for 4 seconds 
• Battery Optimized: Efficient bit-manipulation algorithms 
• Menu System: Clean interface with pattern submenu





Heres some more ideas to implement:


Pattern Management


• Pattern Save/Load: Store custom patterns in EEPROM with names 

• Pattern Library: More famous patterns (pulsar, pentadecathlon, etc.) 

• Random Seeds: Different randomization algorithms (sparse, dense, clusters) 

• Symmetrical Patterns: Generate symmetric starting conditions

Game Modes


• Survival Mode: Try to keep population above threshold for X generations 

• Target Mode: Reach specific cell count goals 

• Time Challenge: Fastest to stabilization 

• Multi-Rule Mode: Different cellular automaton rules (B36/S23, etc.)




Advanced Controls


• Step Mode: Advance one generation at a time with button press 

• Rewind: Store last N generations for backward stepping 

• Pause/Resume: Freeze simulation mid-run 

• Bookmark Positions: Mark interesting generations to return to



Statistics & Analysis


• Population Oscillation Detection: Identify period-N oscillators 

• Stability Analysis: Time to stabilization metrics 

• High Score System: Longest-lived patterns, highest populations 

• Pattern Classification: Auto-detect gliders, oscillators, still lifes



Hardware Enhancements


• Sound Effects: Beeps for births/deaths, musical tones for populations 

 • Color Coding: Different colors for cell age or generation born
