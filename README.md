# Arduboy minesweeper

## A simple minesweeper for arduboy.
- A grid of 15 * 9,
- Three levels of difficulty: easy (mines), medium (30) and hard (40),
- Awesome old-school mutable sound effects (because I can't do sound design),
- Vastly uncommented code (because apparently, I can't really code),
- Incredible art!
- Fast mode!
  - Flagging an open tile will click all non-flagged surrounding tiles. It's faster, it's also deadlier. If you're scared, you can deactivate it in the menu.

## Controls
- Move with the D-Pad
- Left button opens a tile
- Right button flags a tile


### technical stuff
Each tile is represented by a byte:
- bits 0-3 store the number of surrounding mines
- bit 4 is useless. I should add stuff, just so I don't waste time.
- bit 5 is 1 if the tile has been flagged
- bit 6 is 1 if the tile has been opened
- bit 7 is 1 if the tile has a mine

###TODO
- [ ] Make the menus not as rubbish (e.g. remember position, not look like they've been done by a 5 year old),
- [ ] Make extra sure I'm not doing anything stupid with the tile bytes,
- [ ] High scores on the EEPROM?

Note: great thanks to http://fuopy.github.io/arduboy-image-converter/