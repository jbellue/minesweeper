# Arduboy minesweeper

## A simple minesweeper for arduboy.
- A grid of 15 * 9,
- Three levels of difficulty: easy (15 mines), medium (25) and hard (35),
- Awesome old-school mutable sound effects (because I can't do sound design),
- Vastly uncommented code (because apparently, I can't really code),
- Incredible art!
- Fast mode!
  - Flagging an open tile will click all non-flagged surrounding tiles. It's faster, it's also deadlier. If you're scared, you can deactivate it in the menu.
- Best times saved on the EEPROM, yay!
  - Press LEFT or RIGHT on the main menu to see them. In there, press both UP and DOWN to erase them.
  
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

### What's new
Updated to latest (?) version of the library.

###TODO
- [ ] Fix the speed. Latest version of the library fixed the speed, and it's too fast now. Lowering the framerate makes things unresponsive...
- [ ] Comments. Comments, comments, and comments.
- [X] Maybe get some kind of debug mode?

Note: great thanks to http://fuopy.github.io/arduboy-image-converter/