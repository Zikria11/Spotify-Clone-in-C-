# 🎵 Spotify Clone (Raylib)

A simple **Spotify-inspired UI clone** built using [raylib](https://www.raylib.com/).  
This project demonstrates how to use **raylib** for UI/graphics rendering, smooth animations, and creating an interactive music player interface.

---

## 🚀 Features
- 🎨 Spotify-like modern UI layout (sidebar, header, and content area).
- 🎶 Dummy song/playlist display.
- 🌈 Smooth hover animations using color interpolation.
- ⚡ Built with **pure C++ and raylib** — no external GUI frameworks.

---

## 🛠️ Tech Stack
- **Language:** C++  
- **Graphics Library:** [raylib](https://www.raylib.com/)  

---


---

## 🔧 Installation & Setup

### 1. Install raylib
Follow the [raylib installation guide](https://github.com/raysan5/raylib#installation) for your platform.  
## on Linux (Debian/Ubuntu):
```bash
sudo apt install libraylib-dev
```
## On Windows (MinGW + MSYS2):
``` bash
pacman -S mingw-w64-x86_64-raylib
```
Or build from source using CMake.

## 2. Clone this repository
``` bash
git clone https://github.com/your-username/spotify-clone-raylib.git
cd spotify-clone-raylib
```
## 3. Build & Run

Using g++:
``` bash
g++ main.cpp -o spotify-clone -lraylib -lopengl32 -lgdi32 -lwinmm
./spotify-clone
```

### On Linux:
``` bash
g++ main.cpp -o spotify-clone -lraylib -lm -ldl -lpthread -lGL -lX11
./spotify-clone
```

📜 License

This project is licensed under the MIT License — free to use, modify, and distribute.

🙌 Acknowledgments

raylib
 by raysan5

Inspired by the UI of Spotify
