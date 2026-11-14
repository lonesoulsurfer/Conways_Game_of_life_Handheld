# Game of Life Handheld

This is a handheld Conway's Game of Life with an OLED screen and 6 buttons. It has 5 famous patterns, random and symmetric patterns, a pattern editor and 4 other cellular automata. The same hardware also plays my [Star Wars game](https://github.com/lonesoulsurfer/Star_Wars_Handheld_Game).

![Game of Life Handheld](https://github.com/user-attachments/assets/da0e76cf-106c-456c-9749-726d07561c74)
![Game of Life Handheld screen](https://github.com/user-attachments/assets/6ac36b5f-e18f-4702-b923-bd1952dfdfd4)

## Build guide and video

- Step-by-step build guide on Instructables: not available yet.
- Videos on YouTube: [short video](https://youtube.com/shorts/Atr1aoWWVLA) and [long video](https://youtu.be/dLQRpCf72iw)

## Features

- Conway's Game of Life on a 64 × 32 cell grid, with edges that wrap around
- 5 famous patterns, random patterns and symmetric patterns
- A pattern editor, to draw your own start pattern
- 4 other cellular automata: Brian's Brain, Day & Night, Seeds and Cyclic CA
- Automatic detection when a pattern dies or repeats
- 12 speed levels

The [user guide](docs/MANUAL.md) tells you how to use all the menus and controls.

## What you need

### Hardware

- Adafruit Trinket M0
- SSD1306 128 × 64 OLED display (I2C)
- 5 buttons on a resistor ladder (pin A3), and button B (pin A4)
- All parts are in the parts list ([PDF](Parts%20List.pdf) or Excel).
- The schematic is [Schematic.png](Schematic.png).
- The Gerber files for the PCB and the front panel are in [Gerber Files](Gerber%20Files/). You can send them to a PCB maker.

### Software

- Arduino IDE

## Upload the firmware

1. In the Arduino IDE, click **File → Preferences**.
2. In **Additional boards manager URLs**, add: `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
3. Open the **Boards Manager**. Search for **Adafruit SAMD Boards** and install it.
4. Open the **Library Manager**. Install **Adafruit GFX Library** and **Adafruit SSD1306**.
5. Download this repository. Click **Code → Download ZIP**, then extract the ZIP file.
6. Open `IDE_Sketch_Game_of_Life/Game_of_life_V3/Game_of_life_V3.ino`. This is the newest version.
7. Connect the Trinket M0 to your computer with a USB cable.
8. In the **Tools** menu, set **Board** to **Adafruit Trinket M0**, and set **Port** to the port of the Trinket.
9. Click **Upload**.

## Files in this repository

| Folder or file | Contents |
|----------------|----------|
| `IDE_Sketch_Game_of_Life/` | Firmware, versions 1, 2 and 3 |
| `Eagle Files/` | Eagle files for the PCB |
| `Game of Life - Front Panel Design/` | KiCad, SVG and DXF files for the front panel |
| `Gerber Files/` | Gerber files for the PCB and the front panel, ready to order |
| `Parts List.pdf`, `Parts List.xlsx` | Parts list |
| `Schematic.png`, `PCB.png` | Schematic and PCB image |
| `docs/MANUAL.md` | User guide |

## Support

Do you like this project? You can support my work on [GitHub Sponsors](https://github.com/sponsors/lonesoulsurfer).

## Licence

- **Code:** [MIT License](LICENSE). You can use, change and sell it.
- **Hardware** (PCB, panels and Gerber files): [CERN Open Hardware Licence v2, Strongly Reciprocal](LICENSE-HARDWARE). You can make, change and sell boards. If you share or sell a changed design, you must also share the changed design files under the same licence.

Copyright (c) 2025 Marcus (lonesoulsurfer).
