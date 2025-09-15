
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


Three Game Modes
1. Random Game: Starts with random cell pattern, auto-resets when pattern dies/repeats
2. Custom Builder: Interactive editor to design your own starting patterns
3. Preset Patterns: 5 famous Conway patterns including Gosper Glider Gun


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
