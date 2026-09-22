# Pac-Man Deluxe (Raylib Edition)

[![Language](https://img.shields.io/badge/Language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Library](https://img.shields.io/badge/Library-Raylib%205.0-red.svg)](https://www.raylib.com/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg)](#)

A feature-rich 2D arcade recreation of classic Pac-Man built in C using **Raylib**. Developed as a Fundamentals of Programming (BP) course project at Amirkabir University of Technology.

---

## Key Features

* **Multi-Level Campaign Architecture:**
  * **3 Distinct Mazes:** *Classic Labyrinth*, *The Citadel*, and *The Twin Vaults*.
  * **Continuous Progression:** Clearing all food pellets triggers celebratory particle fireworks, grants a level completion bonus, and seamlessly advances to the next maze while respawning all items.
* **Corridor-Assisted Movement Engine:**
  * Waypoint tile-navigation system eliminating wall-clipping and corner snags.
  * Turn buffering allowing responsive directional changes before reaching alley openings.
* **Distinct Ghost Personalities & States:**
  * **Blinky (Red):** Direct target chaser using Manhattan-distance waypoint routing.
  * **Pinky (Pink):** Ambush predator anticipating Pac-Man's path three tiles ahead.
  * **Inky (Sky Blue):** Dynamic flanker alternating between aggressive pincer moves and corner retreats.
  * **Clyde (Orange):** Territorial patrol ghost that scatters when Pac-Man gets too close.
  * **Frightened & Recovery Mode:** Eating power berries turns ghosts vulnerable, causing them to flee and flash white before returning to normal.
  * **Ghost House Respawn:** Eaten ghosts return to the central bullpen before resuming pursuit.
* **Items, Hazards & Power-Ups:**
  * **Pizza Pellets:** Core objective points scattered throughout corridors.
  * **Power Berry:** Activates Ghost Hunt mode for 8 seconds, enabling Pac-Man to eat vulnerable ghosts for bonus score multipliers.
  * **Chili Pepper:** Provides an immediate 7-second speed boost.
  * **Golden Apple:** Restores 1 life (up to a 3-heart maximum).
  * **Poison Mushroom:** Dangerous hazard tile that deducts 1 life on contact.
* **Visual Polish & Procedural Fallbacks:**
  * Built-in vector graphics fallbacks: renders crisp procedural animations (animated chomping mouth, ghost skirts, tracking eyes) even if `.png` assets are absent.
  * Screen-shake trauma effects upon taking damage.
  * Particle explosions on eating pellets, picking up fruits, and catching ghosts.
  * Floating floating-point score popups (`+100`, `+200`, `POWER UP!`).
* **Game Management & Leaderboard:**
  * Custom callsign/pilot tag input screen with real-time text editing.
  * Persistent local leaderboard stored in `records.txt`, sorted dynamically by high score.
  * In-game pause menu preserving state with quick resume or safe exit to main menu.

---

## Controls

| Key | Action | Context |
| :--- | :--- | :--- |
| **W / Up Arrow** | Move Up | In-Game |
| **S / Down Arrow** | Move Down | In-Game |
| **A / Left Arrow** | Move Left | In-Game |
| **D / Right Arrow** | Move Right | In-Game |
| **P / ESC** | Pause Game / Open Pause Menu | In-Game |
| **M** | Toggle Sound / Music Mute | In-Game |
| **1, 2, 3 / Enter** | Select Menu Options | Main Menu |
| **Backspace** | Delete Character | Pilot Tag Input |
| **Enter** | Confirm Selection / Launch Mission | Global |

---

## Collectibles & Entities

| Sprite / Item | Effect | Score |
| :--- | :--- | :--- |
| **Pizza Pellet** (`pizza.png`) | Standard maze pellet. Clearing all respawns the board. | +10 |
| **Power Berry** (`berry.png`) | Triggers 8-second Ghost Hunt mode. | +50 |
| **Chili Pepper** (`pepper.png`) | Accelerates movement speed for 7 seconds. | +100 |
| **Apple** (`apple.png`) | Grants +1 Extra Life (max 3 lives). | +100 |
| **Mushroom** (`Mushroom.png`) | Hazard: Deducts 1 life and causes screen shake. | 0 |
| **Scared Ghost** | Eaten during Power Berry mode; sends ghost home. | +200 |

---

## Project Structure

```text
PACMAN/
├── main.c                  # Core state machine, game loop, AI, and renderer
├── records.txt             # High score file (auto-generated)
├── pacman.png              # Optional texture: Pac-Man
├── ghost.png               # Optional texture: Normal Ghost
├── ghostc.png              # Optional texture: Frightened Ghost
├── pizza.png               # Optional texture: Food pellet
├── pepper.png              # Optional texture: Speed booster
├── berry.png               # Optional texture: Power pellet
├── apple.png               # Optional texture: Extra life
├── Mushroom.png            # Optional texture: Hazard
├── heart.png               # Optional texture: HUD lives
├── background_menu.png     # Optional texture: Main menu backdrop
├── game.mp3                # In-game music track
└── menu.mp3                # Menu ambient music

```

---

## Compilation & Installation

### Prerequisites

* C99-compliant compiler (`gcc` or `clang`)
* [Raylib 5.0+](https://www.raylib.com/?utm_source=gemini) installed on your system

### Linux (Arch / Ubuntu / Debian / Fedora)

```bash
gcc main.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o pacman
./pacman

```

### Windows (MinGW-w64 / MSYS2 / w64devkit)

```bash
gcc main.c -lraylib -lopengl32 -lgdi32 -lwinmm -o pacman.exe
pacman.exe

```

### macOS

```bash
gcc main.c -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o pacman
./pacman

```

---

## Author & Academic Notice

Developed as a Base Programming (BP) project at **Amirkabir University of Technology (Tehran Polytechnic)**. Provided for educational, reference, and demonstration purposes.

```
