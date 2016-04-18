# Arduboy minesweeper

## A simple minesweeper for arduboy.
- A grid of 15 * 9,
- Three levels of difficulty: easy (20 mines), medium (30) and hard (40),
- Awesome old-school mutable sound effects (because I can't do sound design),
- Vastly uncommented code (because apparently, I can't really code),
- Incredible art!
- Fast mode!
  - Flagging an open tile will click all non-flagged surrounding tiles. It's faster, it's also deadlier. If you're scared, you can deactivate it in the menu.

## Controls
- Move with the D-Pad
- Left button opens a tile
- Right button flags a tile
- I can't remember what I did in the menus. Was it B goes back, and A validates? A and B are confusing on the Arduboy... Why not keep them inverted, like on the Gameboy?!


### technical stuff
Each tile is represented by a byte:
```
00000000
1....... tile has a mine
.1...... tile has been opened
..1..... tile has been flagged
...x.... unused, waste of valuable space
....1111 number of surrounding mines (0 to 8)
```

###TODO
- [ ] Make the menus not as rubbish,
- [ ] High scores on the EEPROM?

Note: great thanks to http://fuopy.github.io/arduboy-image-converter/