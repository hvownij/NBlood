## NBlood Server Client Branch.  Merging master ( nukeykt )  with norender ( CL102 ) 
by Hv

# NBlood
Reverse-engineered ports of Build games using EDuke32 engine technology and development principles

## NBlood
Blood port based on EDuke32

### Installing
1. Extract NBlood to a new directory
2. Copy the following files from Blood 1.21 to NBlood folder:

   BLOOD.INI  
   BLOOD.RFF  
   BLOOD000.DEM-BLOOD003.DEM (optional)  
   CP01.MAP-CP09.MAP (optional, Cryptic Passage)  
   CPART07.AR_ (optional, Cryptic Passage)  
   CPART15.AR_ (optional, Cryptic Passage)  
   CPBB01.MAP-CPBB04.MAP (optional, Cryptic Passage)  
   CPSL.MAP (optional, Cryptic Passage)  
   CRYPTIC.INI (optional, Cryptic Passage)  
   CRYPTIC.SMK (optional, Cryptic Passage)  
   CRYPTIC.WAV (optional, Cryptic Passage)  
   GUI.RFF  
   SOUNDS.RFF  
   SURFACE.DAT  
   TILES000.ART-TILES017.ART  
   VOXEL.DAT  

3. Optionally, if you want to use CD audio tracks instead of MIDI, provide FLAC/OGG recordings in following format: bloodXX.flac/ogg, where XX is track number. Make sure to enable Redbook audio option in sound menu.
4. Optionally, if you want cutscenes and you have the original CD, copy the `movie` folder into NBlood's folder (the folder itself too). If you have the GOG version of the game, rename `game.gog` to `game.bin` and `game.inst` to `game.cue` and mount the `cue` as a virtual CD (for example with `WinCDEmu`), there you will have the `movie` folder.
5. Launch NBlood (on Linux, to play Cryptic Passage, launch with the `-ini CRYPTIC.INI` parameter)

### Notes
Demo playback is not yet working.

### Adjusting settings
We are currently working on fancy new menus for the game. In the meantime, you can edit the settings.cfg file in the game directory that's created on first run.

To invert the mouse, add the line 'in_mouseflip 0' to settings.cfg.
To change the FOV, add a new line to settings.cfg, e.g. 'fov "120"' where 120 is the desired FOV value between 60 and 140.

## Building from source
See: https://wiki.eduke32.com/wiki/Main_Page

## Building from norender source
Using msys2 and mingw gcc to compile... you need to add NORENDER=1 to the make command im order to build the nblood.exe which you can rename to nblood_server.exe as it only works with the -server #ofplayers string in cmd, bat file, or a shortcut.

## Acknowledgments
  See AUTHORS.md
