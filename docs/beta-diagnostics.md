# Beta tester reports

The beta displays `BETA BUILD - WORK IN PROGRESS` in the top-left corner.
Its build number remains in the top-right of menus. The optional FPS counter
uses a smaller face.

Ask testers to copy this file after reproducing a problem:

`/data/homebrew/PPSA98743/userdata/logs/encore-test.log`

If present, include `encore-test.log.1` and `encore-test.log.2` from the same
folder. Use their existing FTP connection; the files are in writable storage
for folder, PKG and exFAT installations. Do not delete their songs or profiles.
Include the build number, controller model, steps to reproduce, and whether
the issue happened during launch, browsing, loading, gameplay or text entry.
A video can help with timing or display issues.

The log records session start/clean end, screen transitions, output size,
controller IDs and stream setup, song loading/streaming failures, and restarts.
It does not continuously record button presses or audio, account credentials,
profile names or networking credentials. It uses a fixed event queue and a
background writer; gameplay and audio callbacks do not wait for disk writes.
Three rotating files bound the storage footprint to approximately 1.6 MiB.

A PS5 system crash can terminate the process before its final write. A missing
clean-end record is useful evidence but does not by itself prove a game crash:
closing the application from the PS menu can have the same result. The log
is not a substitute for a PS5 crash dump or for physically testing controllers.

## Early startup checkpoints (Alpha v.39)

`userdata/logs/encore-startup.log` is written synchronously before platform,
controller and Vulkan initialization. Its last completed stage narrows an early
exit that occurs before `encore-test.log` starts. One previous launch is retained
as `encore-startup.log.previous`. Writes stop before the render loop. The log
contains stage labels and timestamps, without input or account data. No file
can also mean the title never reached main or could not access writable storage;
it is not proof of a particular firmware or GPU defect.

The first base-console investigation found a mandatory startup reference to a
development song's album cover. That file is intentionally absent from the
release song bundle. The corrected build reserves its preview descriptor from
an existing menu texture, keeps it hidden, and loads the selected song's cover
on demand. The fix passed main-menu, Quickplay and Settings launches from an
external USB folder on a base PS5 (reported firmware 11.20), at 3840x2160 and
about 59.9 FPS. Firmware 13.00 still needs tester confirmation.
