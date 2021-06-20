CD Player
=========

Just goofing about with `libvlc` to make some simple CD player software for a Raspberry Pi & external CD drive & DAC setup.
I need to add GPIO buttons and an LCD and configure those.

So far, this auto-plays an inserted CD and allow the user to skip tracks, play, pause and eject the disc.
The endgame is that I'll add in a rip feature to rip the data of discs to a NAS too.

# Requirements

1. `libvlc`
2. `ncurses` (when I've got GPIO buttons set up I'll remove this)

# Installation

1. Navigate to a build directory of your choice (can be `cdplayer/build` or just the repo root dir).
2. Run `cmake [path to src]`.
3. Run `make`.
