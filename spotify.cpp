#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>

// --- Helpers ---
float FadeValue(float from, float to, float speed) {
    if (from < to) {
        from += speed;
        if (from > to) from = to;
    }
    else if (from > to) {
        from -= speed;
        if (from < to) from = to;
    }
    return from;
}

Color CustomColorLerp(Color c1, Color c2, float t) {
    Color result;
    result.r = (unsigned char)Clamp((int)Lerp((float)c1.r, (float)c2.r, t), 0, 255);
    result.g = (unsigned char)Clamp((int)Lerp((float)c1.g, (float)c2.g, t), 0, 255);
    result.b = (unsigned char)Clamp((int)Lerp((float)c1.b, (float)c2.b, t), 0, 255);
    result.a = (unsigned char)Clamp((int)Lerp((float)c1.a, (float)c2.a, t), 0, 255);
    return result;
}

float easeOutCubic(float t) { return 1 - powf(1 - t, 3); }

// --- Track struct ---
struct Track {
    std::string file;
    std::string title;
    std::string artist;
    std::string cover;
    std::vector<std::string> playlistTags; // Tags for playlist filtering
    Music music;
    Texture2D coverTex;
    bool loaded = false;
    std::string errorMessage;
};

static std::vector<Track> playlist;
static int currentIndex = -1;
static float volume = 0.8f;
static float seekPos = 0.0f; // 0..1
static bool draggingSeek = false;
static std::string selectedPlaylist = "All"; // Current playlist ("All", "Favourites", "Chill", "Workout")

// Animation state
static float globalTime = 0.0f;
static float playPulse = 0.0f;
static float hoverPulse = 0.0f;

// --- Playlist ---
bool LoadPlaylistHardcoded() {
    Track a = {
        "Assets/Music/BabyBoy.mp3", // Jamendo: "Morning" by Stevia Sphere
        "Oh My Little Baby Boy",
        "Babe",
        "https://imgjam3.jamendo.com/albums/a0/57/576/cover_500.jpg",
        {"Favourites", "Chill"},
        {0},
        {0},
        false,
        ""
    };
    Track b = {
        "assets/music/Sailor-Song.mp3", // Replace with a valid local MP3
        "Sailor Song",
        "Gigi Perez",
        "assets/art/sample_cover.png",
        {"Favourites", "Workout"},
        {0},
        {0},
        false,
        ""
    };
    Track c = {
        "assets/music/download.mp3", // Replace with a valid local MP3
        "Sample Track",
        "Local Artist",
        "assets/art/sample_cover2.png",
        {"Chill", "Workout"},
        {0},
        {0},
        false,
        ""
    };
    playlist.push_back(a);
    playlist.push_back(b);
    playlist.push_back(c);
    return true;
}

void UnloadAll() {
    for (auto& t : playlist) {
        if (t.loaded) {
            UnloadMusicStream(t.music);
            UnloadTexture(t.coverTex);
            t.loaded = false;
            t.errorMessage.clear();
        }
    }
}

bool LoadTrack(int idx) {
    if (idx < 0 || idx >= (int)playlist.size()) return false;
    Track& t = playlist[idx];
    if (t.loaded) return true;

    // Check if local file exists
    if (t.file.find("assets/") == 0 && !FileExists(t.file.c_str())) {
        t.errorMessage = "Local file not found: " + t.file;
        TraceLog(LOG_WARNING, "%s", t.errorMessage.c_str());
        return false;
    }

    t.music = LoadMusicStream(t.file.c_str());
    if (t.music.ctxData == nullptr) {
        t.errorMessage = "Audio load failed: " + t.file;
        TraceLog(LOG_WARNING, "%s", t.errorMessage.c_str());
        return false;
    }

    t.coverTex = FileExists(t.cover.c_str()) ? LoadTexture(t.cover.c_str()) : LoadTextureFromImage(GenImageColor(512, 512, DARKGRAY));
    if (t.coverTex.id == 0) {
        t.errorMessage = "Failed to load cover: " + t.cover;
        TraceLog(LOG_WARNING, "%s", t.errorMessage.c_str());
        t.coverTex = LoadTextureFromImage(GenImageColor(512, 512, DARKGRAY));
    }
    t.loaded = true;
    t.errorMessage.clear();
    return true;
}

void PlayTrack(int idx) {
    if (idx < 0 || idx >= (int)playlist.size()) return;
    if (currentIndex >= 0 && currentIndex < (int)playlist.size() && playlist[currentIndex].loaded) {
        StopMusicStream(playlist[currentIndex].music);
    }
    currentIndex = idx;
    if (LoadTrack(currentIndex)) {
        PlayMusicStream(playlist[currentIndex].music);
        SetMusicVolume(playlist[currentIndex].music, volume);
        playPulse = 0.9f;
    }
    else {
        TraceLog(LOG_ERROR, "Could not play track: %s", playlist[currentIndex].title.c_str());
    }
}

void TogglePlayPause() {
    if (currentIndex < 0) {
        if (!playlist.empty()) PlayTrack(0);
        return;
    }
    Music& m = playlist[currentIndex].music;
    if (IsMusicStreamPlaying(m)) {
        PauseMusicStream(m);
    }
    else {
        ResumeMusicStream(m);
    }
    playPulse = 0.9f;
}

void NextTrack() {
    if (!playlist.empty()) PlayTrack((currentIndex + 1) % (int)playlist.size());
}

void PrevTrack() {
    if (!playlist.empty()) PlayTrack((currentIndex - 1 + (int)playlist.size()) % (int)playlist.size());
}

// --- MAIN ---
int main() {
    const int screenW = 1280;
    const int screenH = 760;
    InitWindow(screenW, screenH, "Spotify Clone");
    InitAudioDevice();
    SetTargetFPS(60);

    Color bg = { 12, 14, 20, 255 };
    Color panel = { 18, 20, 26, 220 };
    Color neon = { 30, 215, 96, 255 };
    Color soft = { 40, 44, 52, 200 };
    Color text = WHITE;

    Rectangle left = { 18, 18, 300, screenH - 36 };
    Rectangle center = { left.x + left.width + 16, 18, screenW - left.width - 52, screenH - 140 };
    Rectangle bottom = { 16, screenH - 108, screenW - 32, 90 };
    Rectangle btnPrev = { bottom.x + 80, bottom.y + 18, 52, 52 };
    Rectangle btnPlay = { bottom.x + 150, bottom.y + 10, 76, 76 };
    Rectangle btnNext = { bottom.x + 240, bottom.y + 18, 52, 52 };
    Rectangle seekBar = { bottom.x + 340, bottom.y + 36, bottom.width - 420, 18 };
    Rectangle playlistsButton = { left.x + 16, left.y + 60, left.width - 32, 36 };
    Rectangle favouritesButton = { left.x + 16, left.y + 104, left.width - 32, 36 };
    Rectangle chillButton = { left.x + 16, left.y + 148, left.width - 32, 36 };
    Rectangle workoutButton = { left.x + 16, left.y + 192, left.width - 32, 36 };

    LoadPlaylistHardcoded();

    int listHover = -1;
    int selectedListIndex = -1;
    float listScroll = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        globalTime += dt;
        playPulse = FadeValue(playPulse, 0.0f, dt * 1.8f);
        hoverPulse = FadeValue(hoverPulse, 0.0f, dt * 1.4f);

        if (currentIndex >= 0 && currentIndex < (int)playlist.size() && playlist[currentIndex].loaded) {
            UpdateMusicStream(playlist[currentIndex].music);
            float len = GetMusicTimeLength(playlist[currentIndex].music);
            if (len > 0) seekPos = GetMusicTimePlayed(playlist[currentIndex].music) / len;
        }

        Vector2 mouse = GetMousePosition();

        // Handle playlist button clicks
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, playlistsButton)) selectedPlaylist = "All";
            if (CheckCollisionPointRec(mouse, favouritesButton)) selectedPlaylist = "Favourites";
            if (CheckCollisionPointRec(mouse, chillButton)) selectedPlaylist = "Chill";
            if (CheckCollisionPointRec(mouse, workoutButton)) selectedPlaylist = "Workout";
            if (CheckCollisionPointRec(mouse, btnPrev)) { PrevTrack(); playPulse = 0.5f; }
            if (CheckCollisionPointRec(mouse, btnPlay)) { TogglePlayPause(); playPulse = 0.9f; }
            if (CheckCollisionPointRec(mouse, btnNext)) { NextTrack(); playPulse = 0.5f; }
            if (CheckCollisionPointRec(mouse, seekBar)) { draggingSeek = true; }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) draggingSeek = false;
        if (draggingSeek && currentIndex >= 0 && playlist[currentIndex].loaded) {
            float x = Clamp((mouse.x - seekBar.x) / seekBar.width, 0.0f, 1.0f);
            seekPos = x;
            SeekMusicStream(playlist[currentIndex].music, seekPos * GetMusicTimeLength(playlist[currentIndex].music));
        }

        BeginDrawing();
        ClearBackground(bg);

        // Background animated subtle bands
        for (int i = 0; i < 6; i++) {
            float alpha = 0.03f + i * 0.01f;
            DrawRectangleGradientH(
                0, i * (screenH / 6), screenW, screenH / 6,
                ColorAlpha(DARKBLUE, alpha),
                ColorAlpha(BLACK, alpha * 0.6f)
            );
        }

        // Sidebar
        DrawRectangleRounded(left, 0.14f, 6, panel);
        DrawText("Your Library", left.x + 18, left.y + 18, 20, text);

        // Playlist buttons
        const char* lists[] = { "All", "Favourites", "Chill", "Workout" };
        Rectangle listButtons[] = { playlistsButton, favouritesButton, chillButton, workoutButton };
        for (int i = 0; i < 4; i++) {
            Rectangle r = listButtons[i];
            bool h = CheckCollisionPointRec(mouse, r);
            if (h) { listHover = i; hoverPulse = 1.0f; }
            float f = (i == listHover || selectedPlaylist == lists[i]) ? easeOutCubic(hoverPulse) : 0.0f;
            Color back = CustomColorLerp(panel, neon, f * 0.06f);
            DrawRectangleRounded(r, 0.12f, 4, back);
            if (i == listHover || selectedPlaylist == lists[i]) DrawRectangleRoundedLines(r, 0.12f, 4, ColorAlpha(neon, f * 0.6f));
            DrawText(lists[i], r.x + 14, r.y + 8, 16, text);
        }

        // Center Panel (Track List)
        DrawRectangleRounded(center, 0.14f, 6, panel);
        DrawText(("Tracks - " + selectedPlaylist).c_str(), center.x + 18, center.y + 18, 20, text);

        float rowY = center.y + 60 - listScroll;
        const float rowH = 80;
        int displayIndex = 0;
        for (size_t i = 0; i < playlist.size(); ++i) {
            // Filter tracks by selected playlist
            if (selectedPlaylist != "All" &&
                std::find(playlist[i].playlistTags.begin(), playlist[i].playlistTags.end(), selectedPlaylist) == playlist[i].playlistTags.end()) {
                continue;
            }
            Rectangle rowRect = { center.x + 12, rowY + displayIndex * (rowH + 8), center.width - 24, rowH };
            bool h = CheckCollisionPointRec(mouse, rowRect);
            float f = (displayIndex == selectedListIndex || h) ? easeOutCubic(hoverPulse) : 0.0f;
            Color rowBack = CustomColorLerp(panel, neon, f * 0.06f);
            DrawRectangleRounded(rowRect, 0.12f, 4, rowBack);
            if (h || displayIndex == selectedListIndex) DrawRectangleRoundedLines(rowRect, 0.12f, 4, ColorAlpha(neon, f * 0.6f));

            Rectangle coverDest = { rowRect.x + 8, rowRect.y + 8, 64, 64 };
            if (playlist[i].cover.size() && playlist[i].loaded) {
                Rectangle source = { 0, 0, (float)playlist[i].coverTex.width, (float)playlist[i].coverTex.height };
                DrawTexturePro(playlist[i].coverTex, source, coverDest, { 0, 0 }, 0, ColorAlpha(text, 1.0f - f * 0.2f));
            }
            else {
                DrawRectangleRounded(coverDest, 0.2f, 4, ColorAlpha(DARKGRAY, 1.0f - f * 0.2f));
                if (!playlist[i].errorMessage.empty()) {
                    DrawText(playlist[i].errorMessage.c_str(), (int)rowRect.x + 80, (int)rowRect.y + 50, 12, RED);
                }
            }

            DrawText(playlist[i].title.c_str(), (int)rowRect.x + 80, (int)rowRect.y + 12, 18, text);
            DrawText(playlist[i].artist.c_str(), (int)rowRect.x + 80, (int)rowRect.y + 34, 14, ColorAlpha(text, 0.7f));

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rowRect)) {
                selectedListIndex = displayIndex;
                PlayTrack((int)i);
            }
            displayIndex++;
        }

        // Bottom Bar
        DrawRectangleRounded(bottom, 0.14f, 6, panel);
        if (currentIndex >= 0 && currentIndex < (int)playlist.size()) {
            Track& cur = playlist[currentIndex];
            Rectangle coverDest = { bottom.x + 18, bottom.y + 12, 64, 64 };
            if (cur.loaded) {
                Rectangle source = { 0, 0, (float)cur.coverTex.width, (float)cur.coverTex.height };
                DrawTexturePro(cur.coverTex, source, coverDest, { 0, 0 }, 0, ColorAlpha(text, 1.0f - playPulse * 0.2f));
            }
            DrawText(cur.title.c_str(), bottom.x + 90, bottom.y + 18, 20, text);
            DrawText(cur.artist.c_str(), bottom.x + 90, bottom.y + 46, 16, ColorAlpha(text, 0.7f));
            if (!cur.errorMessage.empty()) {
                DrawText(cur.errorMessage.c_str(), bottom.x + 90, bottom.y + 68, 14, RED);
            }

            // Playback Controls
            Color btnBack = CustomColorLerp(soft, neon, playPulse * 0.1f);
            DrawRectangleRounded(btnPrev, 0.3f, 4, btnBack);
            DrawText("<", btnPrev.x + 20, btnPrev.y + 16, 20, text);
            DrawRectangleRounded(btnPlay, 0.3f, 4, btnBack);
            DrawText(IsMusicStreamPlaying(playlist[currentIndex].music) ? "||" : ">", btnPlay.x + 28, btnPlay.y + 20, 28, text);
            DrawRectangleRounded(btnNext, 0.3f, 4, btnBack);
            DrawText(">", btnNext.x + 20, btnNext.y + 16, 20, text);

            // Seek Bar
            DrawRectangleRounded(seekBar, 0.3f, 4, soft);
            Rectangle progress = { seekBar.x, seekBar.y, seekBar.width * seekPos, seekBar.height };
            DrawRectangleRounded(progress, 0.3f, 4, ColorAlpha(neon, 0.8f + playPulse * 0.2f));
        }
        else {
            DrawText("No track loaded. Add assets/music/Sailor-Song.mp3 or download.mp3", bottom.x + 18, bottom.y + 36, 16, ColorAlpha(text, 0.7f));
        }

        EndDrawing();
    }

    UnloadAll();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}