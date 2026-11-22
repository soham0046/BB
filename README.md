# BOUNCE BACK - Every Fall Has a Comeback (Unreal Engine 5, simple version)

You are Noor, who inherits her grandfather's old bus in the small town of Pelham Hollow. Drive around, pick up passengers,
and talk to the neighbors as the story happens. 18 days, 8 lessons, 4 endings.

This is the **simple** version. The whole game is about 700 lines of C++ (plus the story text in `Content/Story`).

## How to run
1. Install Unreal Engine 5.4+ and (Windows) Visual Studio 2022 with the "Game development with C++" workload.
2. Right-click `BounceBack.uproject` -> Generate Visual Studio project files, then double-click it and say Yes to building.
3. File -> New Level -> **Empty Level**, save as `Content/Maps/Main`.
4. Press Play.

Not compiled in Unreal yet (made without the engine), so there may be a few small compile errors on the first build - paste them to get them fixed.

## Controls
W/S gas and brake, A/D steer, Space handbrake, E pick up passengers at a bus stop, Enter to read/continue and choose, 1-4 pick an answer.

## What each file does
| File | What it does |
|---|---|
| BBCore | Save data, reading the story text files, the conversation runner |
| BBLayout.h | Where the buildings and bus stops are (the town map) |
| BBUtil.h | Helpers for making things out of the engine's built-in boxes/spheres |
| BBTown | Builds the town: ground, roads, buildings, trees, lake, sun |
| BBBus | The bus: drive, turn, wobble, camera |
| BBGameMode | The game: clock, story, passengers, day end, screens |
| BBHUD | Draws all text and boxes |

## Not in this version
No walking around, no weather, no upgrade shop, no sound, passengers are just money (no people models).

## Story text
`Content/Story/*.txt` - scenes use a simple format. `@name` starts a scene, `Name: text` is a line, `? choice | effects | label` is a choice.
MIT license.
