Hello.

Quake 3 Movie Maker's Edition is a special version of Quake 3 engine that aids
the creation of game movies, making it possible to simplify required tasks and
adds new features to the engine that are related to the movie making.

This version - 1.33.4.1.

Difference from Quake 3:
  * Ability to fill the sky with any solid color including 'space' maps,
    without touching the portal views.
  * Can force a model on a specific player number.
  * Can compress TGA's with RLE compression.
  * Can write JPEG screenshots during avidemo.
  * Can write .wav sound during avidemo.
  * AviDemo frame rate precision has been raised, now it captures exactly at
    the same FPS the avidemo was specified - 24 fps will not create output at
    24.39fps, and 300fps will not be 333fps (useful for TGA_Hook).
  * Will not stop demo playback when a random keyboard key was pressed.
  * Some engine bugs were fixed.
  * Number of screenshots is not limited to 10000.
  * Windowed mode does not disable hardware overbright support, this will help
    rendering at custom resolutions & completely avoid using fullscreen mode.
  * Screenshots are placed into subdirectory with name of a demo.


It works with OSP, Defrag, CPMA and any other existing Quake 3 mod that works
OK with original Quake 3 v1.32.

As additional plus, it works with TGA_Hook too - select TGA screenshot format
and disable TGA RLE compression. You probably need to modify TGA_Hook's exe
file path in registry at "HKEY_LOCAL_MACHINE\SOFTWARE\14K\Hooker\ExeLocation"
to point at Quake3MME executable. The screenshot naming feature will not work
for obvious reasons (TGA_Hook generates the name).

Be careful with windowed mode exporting - you still need to leave Quake3MME in
foreground to get proper screenshots.

Also, many people miss the point that avidemo in Quake 3 is not realtime, it
is rendering until it's done and frame rate of end result does not depend on
performance of your hardware.


New console commands in Quake3MME:
  * mme_skykey <number>   - If not equal zero, the sky is filled with a solid
                            color.
                  (default) 0 = disabled
                            1 = red
                            2 = green
                            3 = yellow
                            4 = blue
                            5 = cyan
                            6 = magenta
                            7 = white
                            8 = black

  * mme_playernumbers     - Shows the player number assigned to a player. Player
                            numbers are used with the command below.

  * mme_forcemodelplayerX - Force model for player X. Total of 16 players can
                            be specified which model is used. WARNING: In
                            order for this feature to work, you must disable
                            'cg_enemymodel' and 'cg_forcemodel'.
                            Default values are empty.

  * mme_wavdemo_enabled   - Controls recording of .wav sound during avidemo.
                            As a side effect, there will be no sound through
                            the sound card. Sound is exported into
                            "screenshots/<demoname>/wav" directory.
                  (default) 0 = no sound is exported
                            1 = sound is exported while screenshooting.
                            2 = screenshooting is skipped and only sound is
                                exported

  * mme_screenshot_format - Self-explanatory. The format of screenshots.
                  (default) tga  = screenshots exported in tga format
                            jpeg = screenshots exported in jpeg format

  * mme_tga_rle_compress  - Enable TGA RLE compression. Saves some disk space.
                            0 = disable TGA RLE compression
                  (default) 1 = enable TGA RLE compression

  * mme_jpeg_quality      - Specifies the level of compression of the JPEG
                            screenshots if JPEG screenshots are used.
                            0-100 = compression level
                               90 = default

  * mme_jpeg_optimize_huffman_tables
                          - Saves some disk space by optimizing some headers,
                            but slows down screenshooting a bit.
                            0 = disables optimized huffman tables
                  (default) 1 = enables optimized huffman tables

  * mme_jpeg_downsample_chroma
                          - Write the colour component at lesser resolution,
                            saves disk space in tradeoff with quality.
                            Should not be turned on unless you are shooting at
                            very high resolutions.
                  (default) 0 = disables chroma downsample
                            1 = enables chroma downsample

  * mme_anykeystopsdemo   - Prevents any key press stopping demo playback.
                            Keys which would normally stop the demo play-
                            back can be bound for use in demo exporting.
                            0 = key pressing stops demo playback
                  (default) 1 = key pressing does not stop demo playback

  * mme_roundmode         - When equal to 1, physics calculation is done at
                            higher quality because rounding mode will be done
                            as specified in C standard. Influences
                            strafejumping when running on maps (not demo
                            playback), so I left an option to turn back to old
                            q3 style.
                            0 = physics calculation is not altered
                  (default) 1 = physics calculation is done at a higher
                                quality. Effects demo recording, not demo
                                playback.

----------------------------
Contact: HMage - hmd@mail.ru

http://hmd.c58.ru/muwiki/quake3mme
http://sourceforge.net/projects/quake3mme

------
Thanks

I want to thank jrb of shaolinproductions for providing and editing English
version of this file.

I want to thank Auri for provoding and editing German version of this file.

----------
Change Log

1.33.4.1:
  * Fix TGA incompatibility with VirtualDub.

1.33.4:
  * Fixed glitches when using TGA_Hook.
  * Windowed mode doesn't disable overbright bits feature.
  * Screenshot filename is based on demo name and is placed into subdirectory.
  * WAV sound is placed into subdirectory with demo name.
  * Added bugfixes from icculus.org/quake3.

1.33.3:
  * Can write sound to .wav.
  * Raised avidemo frame rate precision.
  * Can write JPEG screenshots during avidemo.
  * Added Alpha buffer initialization code, but that is not used anywhere yet.
  * Fixed bug with "unknown client command" at the beginning of map.
  * Added english readme.

1.33.2:
  * Added russian readme.
  * Can fill the sky with a solid color.
  * Can force a model to a specific player (by number).
  * Demo playback is not stopped by pressing any key.
  * Physics rounding mode is not higher quality by default. But can be turned
    back with a console variable.

1.33.1:
  * Removed the limit of 10000 screenshots.

