// Libraries
#include <muffin/muffin.hpp>
#include <json/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cmath>
#include <ctime>
using namespace std;

// Main
namespace game {
    bool restart = false, music = true, sfx = true;

    unsigned char state = 0;
    void init();

    namespace utils {
        bool aabb(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    }
    namespace world {
        unsigned int sprite, clouds, step = 0, cycles = 0;
        unsigned short w, h, chunk;
        unsigned char size;

        struct item {
            unsigned char id;
            bool item;
        };
        vector<world::item> bg, map;
        vector<unsigned char> hash, perlin;

        /*
        struct entity {
            unsigned char id;

            float x = 0, y = 0, dy = 0;
            bool motion, flip, physics, collisionu, collisiond, collisionl, collisionr;
        };
        vector<world::entity> entities;
        */

        json indexing[3];
        unsigned char name;
        string seed;

        void init();
        void loop(unsigned char layer);
        void save();
        void load();
        void index();
        void generate();
        void reset();

        unsigned char get(int x, int y);
        void          set(int x, int y, unsigned char id);
        unsigned char getbg(int x, int y);
        void          setbg(int x, int y, unsigned char id);
    };
    namespace player {
        unsigned int sprite;

        float x = 0, y = 0, dy = 0, speed = 0.85, camx = 0, camy = 0;
        bool motion, flip, jump, flying, underwater, physics, click, collisionu, collisiond, collisionl, collisionr;

        world::item items[9];
        unsigned char idx = 0;

        void init();
        void loop(unsigned char layer);
        void reset();
    };
    namespace ui {
        unsigned int font, gui, intro, panorama, logo, click;
        unsigned char sel = 0, substate = 0, tab = 0;
        bool debug = false, minimap = false, inventory = false, menu = false, input = false;

        vector<vector<unsigned char>> creative;

        float fps, prev, count;

        void init();
        void loop();
        void reset();
    };
    namespace sound {
        vector<unsigned int> songs;
        unsigned int playing = 0, count = 0;

        void init();
        void loop();
    };
};

void game::init() {
    // Inits muffin
    muffin::init("Minecraft: 2D Edition", 1024, 620, "assets/icon.png", MUFFIN_FLAGS_ACCELERATED | MUFFIN_FLAGS_VSYNC | MUFFIN_FLAGS_HIGHDPI);
    muffin::graphics::setscale(3);

    // Inits game
    world::init();
    player::init();
    ui::init();
    sound::init();

    // Loops
    while (muffin::poll() && !restart) {
        // Clears buffers
        muffin::graphics::setcolor(0, 0, 0, 255);
        muffin::graphics::clear();
        
        // Update game
        switch (state) {
            case 0: {
                ui::loop();
                break;
            }
            case 1: {
                world::loop(0);
                player::loop(0);
                world::loop(1);
                player::loop(1);
                ui::loop();
                break;
            }
        }

        sound::loop();

        muffin::update();
    }

    muffin::close();
}

// Utils
bool game::utils::aabb(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return x1 < x2 + w2 &&
           x1 + w1 > x2 &&
           y1 < y2 + h2 &&
           y1 + h1 > y2;
}

// World
void game::world::init() {
    sprite = muffin::graphics::loadimage("assets/blocks/terrain.png");

    world::index();
}

void game::world::loop(unsigned char layer) {
    switch (layer) {
        case 0: {
            // Day/night cycle
            cycles += 1;
            muffin::graphics::setcolor(192, 216, 255, 255);
            muffin::graphics::drawfillrect(0, 0, 343, 206);
            for (unsigned int i = 0; i < 128; i++) {
                muffin::graphics::setcolor(142, 178, 255, 255 - i * 2);
                muffin::graphics::drawline(0, i, 342, i);
            }

            chunk = 0;

            // Map drawing
            for (unsigned int y = 0; y < h; y++) {
                for (unsigned int x = 0; x < w; x++) {
                    // Screen-space chunk processing
                    if (utils::aabb(player::x - 171, player::y - 103, 364, 228, x * 16, y * 16, 16, 16)) {
                        chunk++;
                        // Background
                        if (getbg(x, y)) {
                            muffin::graphics::drawimage(sprite, x * 16 + floor(player::camx), y * 16 + floor(player::camy), 16, 16, false, 0, getbg(x, y) % 16 * 16, getbg(x, y) / 16 * 16, 16, 16);
                            
                            muffin::graphics::setcolor(0, 0, 0, 85);
                            muffin::graphics::drawfillrect(x * 16 + floor(player::camx), y * 16 + floor(player::camy), 16, 16);
                        }
                    }
                }
            }

            break;
        }
        case 1: {
            bool selection = false;
            player::underwater = false, player::collisionu = false, player::collisiond = false, player::collisionl = false, player::collisionr = false;

            // Map
            for (unsigned int y = 0; y < h; y++) {
                for (unsigned int x = 0; x < w; x++) {
                    // Screen-space chunk processing
                    if (utils::aabb(player::x - 171, player::y - 103, 364, 228, x * 16, y * 16, 16, 16)) {
                        chunk++;

                        // Main
                        if (get(x, y)) {
                            muffin::graphics::setcolor(255, 0, 0, 255);
                            if (get(x, y) >= 38 && get(x, y) <= 39) 
                                muffin::graphics::setcolormod(sprite, 131, 185, 124, 255);
                            muffin::graphics::drawimage(sprite, x * 16 + floor(player::camx), y * 16 + floor(player::camy), 16, 16, false, 0, get(x, y) % 16 * 16, get(x, y) / 16 * 16, 16, 16);
                            if (get(x, y) == 3) {
                                muffin::graphics::setcolormod(sprite, 131, 185, 124, 255);
                                muffin::graphics::drawimage(sprite, x * 16 + floor(player::camx), y * 16 + floor(player::camy), 16, 16, false, 0, 38 % 16 * 16, 38 / 16 * 16, 16, 16);
                            }
                            muffin::graphics::setcolormod(sprite, 255, 255, 255, 255);

                            if ((get(x, y) < 11 or get(x, y) > 15) && get(x, y) != 39 && 
                                (get(x, y) < 55 or get(x, y) > 56) && get(x, y) != 205) {
                                // Player
                                if (!player::collisionu) player::collisionu = utils::aabb(player::x + 6,  player::y - 1,  18, 2,  x * 16, y * 16, 16, 16);
                                if (!player::collisiond) player::collisiond = utils::aabb(player::x + 6,  player::y + 31, 18, 2,  x * 16, y * 16, 16, 16);
                                if (!player::collisionl) player::collisionl = utils::aabb(player::x + 4,  player::y + 1,  2,  30, x * 16, y * 16, 16, 16);
                                if (!player::collisionr) player::collisionr = utils::aabb(player::x + 24, player::y + 1,  2,  30, x * 16, y * 16, 16, 16);

                                // Entities
                                /*
                                for (auto & i : entities) {
                                    if (!i.collisionu) i.collisionu = utils::aabb(i.x + 6,  i.y - 1,  18, 2,  x * 16, y * 16, 16, 16);
                                    if (!i.collisiond) i.collisiond = utils::aabb(i.x + 6,  i.y + 31, 18, 2,  x * 16, y * 16, 16, 16);
                                    if (!i.collisionl) i.collisionl = utils::aabb(i.x + 4,  i.y + 1,  2,  30, x * 16, y * 16, 16, 16);
                                    if (!i.collisionr) i.collisionr = utils::aabb(i.x + 24, i.y + 1,  2,  30, x * 16, y * 16, 16, 16);
                                }
                                */
                            } else if (get(x, y) == 205) {
                                if (!player::underwater) player::underwater = utils::aabb(player::x,      player::y,      32, 32, x * 16, y * 16, 16, 16);
                            }
                        }

                        // Block physics
                        if (get(x, y) == 205) {
                            if (!get(x, y + 1)) set(x, y + 1, 205);
                        }

                        // Block place/destroy
                        if (!selection && !ui::inventory && !ui::menu && utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, x * 16 + floor(player::camx), y * 16 + floor(player::camy), 16, 16)) {
                            selection = true;

                            muffin::graphics::setcolor(255, 255, 255, 85);
                            muffin::graphics::drawfillrect(x * 16 + floor(player::camx), y * 16 + floor(player::camy), 16, 16);
                            if (muffin::input::keyboard(MUFFIN_KEY_LALT)) {
                                if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                    player::click = true;
                                    setbg(x, y, 0);
                                } else if (muffin::input::mouse(MUFFIN_BUTTON_RIGHT) && !utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, player::x, player::y, 32, 32)) {
                                    player::click = true;
                                    if (player::items[player::idx].id != 0) 
                                        setbg(x, y, player::items[player::idx].id);
                                } else {
                                    player::click = false;
                                }
                            } else {
                                if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                    player::click = true;
                                    set(x, y, 0);
                                } else if (muffin::input::mouse(MUFFIN_BUTTON_RIGHT) && !utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, player::x, player::y, 32, 32)) {
                                    player::click = true;
                                    if (player::items[player::idx].id != 0) 
                                        set(x, y, player::items[player::idx].id);
                                } else {
                                    player::click = false;
                                }
                            }
                        }
                    }
                }
            }

            // Entities
            /*
            for (auto & i : entities) {
                if (i.physics) {
                    if (!i.collisiond) {
                        i.dy += 0.075f, i.y += i.dy;
                    } else {
                        i.dy  = 0,      i.y--;
                    }
                }

                if (cycles % 500 > 250) i.flip = rand() % 2;
                i.motion = true;

                if (!i.flip && !i.collisionr && i.x >= 0)     i.x += 0.15f;
                if (i.flip  && !i.collisionl && i.x < w * 16) i.x -= 0.15f;

                muffin::graphics::drawimage(cow, i.x + floor(player::camx), i.y + floor(player::camy), 32, 32, i.flip, 0, (i.motion ? floor(muffin::ticksms() % 400 / 200) : 0) * 32, 0, 32, 32);

                i.collisionu = false, i.collisiond = false, i.collisionl = false, i.collisionr = false;
            }
            */

            break;
        }
    }
}

void game::world::load() {
    // Path
    string path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\0a.dat";
    ifstream f;
    f.unsetf(ios::skipws);

    // Map data
    f.open(path, ios::in | ios::binary);
    if (f.is_open()) {
        f.seekg(0, ios::end);
        unsigned int size = f.tellg();
        f.seekg(0, ios::beg);

        map.clear();
        map.resize(size);
        f.read((char *)map.data(), size);
    }
    f.close();

    // Background data
    path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\0b.dat";
    f.open(path, ios::in | ios::binary);
    if (f.is_open()) {
        f.seekg(0, ios::end);
        unsigned int size = f.tellg();
        f.seekg(0, ios::beg);

        bg.clear();
        bg.resize(size);
        f.read((char *)bg.data(), size);
    }
    f.close();

    // Player data
    path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\0c.dat";
    f.open(path, ios::in | ios::binary);
    if (f.is_open()) {
        f.seekg(0, ios::end);
        unsigned int size = f.tellg();
        f.seekg(0, ios::beg);

        f.read((char *)&player::items[0], size);
    }
    f.close();

    // JSON data
    path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\conf.json";

    f.open(path);
    if (f.is_open()) {
        json j;
        f >> j;

        if (j.contains("width"))  w         = j["width"].get<unsigned int>();
        if (j.contains("height")) h         = j["height"].get<unsigned int>();
        if (j.contains("seed"))   seed      = j["seed"].get<unsigned long>();
        if (j.contains("x"))      player::x = j["x"].get<float>();
        if (j.contains("y"))      player::y = j["y"].get<float>();
    }
    f.close();
}

void game::world::save() {
    // Path
    string path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\0a.dat";
    filesystem::create_directories(filesystem::path(path).parent_path());
    ofstream f;
    f.unsetf(ios::skipws);

    // Map file
    f.open(path, ios::out | ios::binary);
    if (f.is_open()) 
        f.write((char *)map.data(), world::map.size() * sizeof(world::item));
    f.close();

    // Background file
    path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\0b.dat";

    f.open(path, ios::out | ios::binary);
    if (f.is_open()) 
        f.write((char *)bg.data(), world::bg.size() * sizeof(world::item));
    f.close();

    // Player data
    path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\0c.dat";

    f.open(path, ios::out | ios::binary);
    if (f.is_open()) 
        f.write((char *)&player::items[0], 9 * sizeof(world::item));
    f.close();

    // JSON data
    path = string(muffin::system::prefpath("lipx", "minecraft")) + "worlds\\" + to_string(name) + "\\conf.json";

    json j({
        { "width",  w                  },
        { "height", h                  },
        { "seed",   atoi(seed.c_str()) },
        { "date",   time(NULL)         },
        { "x",      player::x          },
        { "y",      player::y          }
    });

    f.open(path);
    if (f.is_open()) 
        f << setw(4) << j;
    f.close();

    //system(string("explorer " + filesystem::path(path).parent_path().string()).c_str());
}

void game::world::index() {
    // Creates game data directory in case it doesn't exist
    if (!filesystem::exists(muffin::system::prefpath("lipx", "minecraft") + string("worlds")))
        filesystem::create_directories(muffin::system::prefpath("lipx", "minecraft") + string("worlds"));

    unsigned int idx = 0;
    for (auto & i : filesystem::directory_iterator(filesystem::path(muffin::system::prefpath("lipx", "minecraft") + string("worlds")))) {
        ifstream f(i.path().string() + "\\conf.json");
        if (f.is_open()) {
            f >> indexing[idx];
            f.close();
        }

        idx++;
    }
}

void game::world::generate() {
    if (seed != "0") srand(atoi(seed.c_str()));

    if (step < 8) {
        switch (step) {
            case 0: {
                w = (size == 3 ? 512 : 
                     size == 2 ? 256 : 
                     size == 1 ? 128 : 64);
                h = (size == 3 ? 128 : 
                     size == 2 ? 64  : 
                     size == 1 ? 32  : 16);
                map.resize(w * h, { 0 });
                bg.resize(w * h,  { 0 });
                break;
            }
            case 1: {
                hash.resize(256);
                for (auto & i : hash) 
                    i = rand() % 255;
                break;
            }
            case 2: {
                auto noise2   = [&](int x, int y) {
                    return hash[(hash[(y + time(NULL)) % 256] + x) % 256];
                };
                auto smooth   = [&](float x, float y, float s) {
                    return x + (s * s * (3 - 2 * s)) * (y - x);
                };
                auto perlin2d = [&](float x, float y, float f, unsigned int d) {
                    double xa = x * f, ya = y * f, amp = 1, fin = 0, div = 0;
                    for (unsigned int i = 0; i < d; i++) {
                        div += 256 * amp, fin += smooth(smooth(noise2((int)xa, (int)ya), noise2((int)xa + 1, (int)ya),     xa - (int)xa), 
                                                 smooth(noise2((int)xa, (int)ya + 1),    noise2((int)xa + 1, (int)ya + 1), xa - (int)xa), ya - (int)ya) * amp, amp /= 2, xa *= 2, ya *= 2;
                    }
                    return fin / div;
                };

                for (unsigned int y = 0; y < h; y++) {
                    for (unsigned int x = 0; x < w; x++) {
                        perlin.push_back(perlin2d(x, y, 0.1, 4) * 10 - 1);
                    }
                }
                break;
            }
            case 3: {
                for (unsigned int y = 0; y < h; y++) {
                    for (unsigned int x = perlin[y] + h / 2; x < w; x++) {
                        map[y + x * h].id    = 2;
                        bg[y + x * h - 1].id = 2;
                    }
                }
                break;
            }
            case 4: {
                for (unsigned int y = 0; y < h; y++) {
                    bool up = false;
                    for (unsigned int x = perlin[y] + h / 2 + h / 6; x < w; x++) {
                        if (!up) {
                            map[y + x * h].id = rand() % 2 + 1;
                            bg[y + x * h].id  = rand() % 2 + 1;
                            up = true;
                        } else {
                            bg[y + x * h].id  = 1;
                        }
                    }

                    for (unsigned int x = perlin[y] + h / 2 + h / 5; x < w; x++) {
                        map[y + x * h].id = rand() % 18 == 17 ? 34 : 1;
                    }

                    for (unsigned int x = perlin[y] + h / 2 + h / 3; x < w; x++) {
                        map[y + x * h].id = rand() % 18 == 17 ? 33 : 1;

                        if (perlin[y + x * h] == 5) map[y + x * h].id = 0;
                        if (perlin[y + x * h] == 4) map[y + x * h].id = 19;
                    }
                }
                break;
            }
            case 5: {
                for (unsigned int x = 0; x < w; x++) {
                    for (unsigned int y = 0; y < h; y++) {
                        if (map[x + y * w].id == 2) {
                            map[x + y * w].id       = 3;
                            map[x + (y - 1) * w].id = rand() % 2 ? 0 : rand() % 2 ? 12 : 39;
                            break;
                        }
                    }
                }
                break;
            }
            case 6: {
                for (unsigned int y = h - 4; y < h; y++) {
                    for (unsigned int x = 0; x < w; x++) {
                        if (y == (unsigned int)h - 1) {
                            map[x + y * w].id = 17;
                        } else {
                            map[x + y * w].id = rand() % 2 ? 17 : map[x + y * w].id;
                        }
                    }
                }

                /*
                for (unsigned int i = 0; i < 15; i++) {
                    entities.push_back({ });
                    entities.back().x  = rand() % (w * 16);
                    entities.back().id = rand() % 3;
                }
                */

                break;
            }
            case 7: {
                break;
            }
        }

        step++;
        muffin::delay(250);
    } else {
        ui::minimap  = false;
        step         = 0;
        state        = 1;
        sound::count = muffin::ticksms();
    }
}

void game::world::reset() {
    cycles = 0;
    chunk  = 0;
    name   = 0;

    bg.clear();
    map.clear();
    hash.clear();
    perlin.clear();
    seed.clear();
    //entities.clear();
}

void game::world::set(int x, int y, unsigned char id) {
    if (x >= 0 && x < w && 
        y >= 0 && y < h) {
        world::map[x + y * w].id = id;
    }
}

unsigned char game::world::get(int x, int y) {
    if (x >= 0 && x < w && 
        y >= 0 && y < h) {
        return world::map[x + y * w].id;
    }

    return 0;
}

void game::world::setbg(int x, int y, unsigned char id) {
    if (x >= 0 && x < w && 
        y >= 0 && y < h) {
        world::bg[x + y * w].id = id;
    }
}

unsigned char game::world::getbg(int x, int y) {
    if (x >= 0 && x < w && 
        y >= 0 && y < h) {
        return world::bg[x + y * w].id;
    }

    return 0;
}

// Player
void game::player::init() {
    sprite  = muffin::graphics::loadimage("assets/entity/steve.png");

    x       = world::w * 8, y = world::h * 4;
    physics = true;
}

void game::player::loop(unsigned char layer) {
    switch (layer) {
        case 0: {
            // Item selection
            if (muffin::input::keyboardevent(MUFFIN_KEY_1)) idx = 0;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_2)) idx = 1;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_3)) idx = 2;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_4)) idx = 3;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_5)) idx = 4;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_6)) idx = 5;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_7)) idx = 6;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_8)) idx = 7;
            else if (muffin::input::keyboardevent(MUFFIN_KEY_9)) idx = 8;

            // Drawing
            muffin::graphics::drawimage(sprite, floor(x + camx), floor(y + camy), 32, 32, flip, 0, (motion ?     floor(muffin::ticksms() % 400 / 200)         : 0) * 32, 0, 32, 32);
            muffin::graphics::drawimage(sprite, floor(x + camx), floor(y + camy), 32, 32, flip, 0, (click  ?     floor(muffin::ticksms() % 400 / 200) ? 2 : 4 : 
                                                                                                    motion ? 2 + floor(muffin::ticksms() % 400 / 200)         : 2) * 32, 0, 32, 32);

            // Debug
            if (ui::debug) {
                muffin::graphics::setcolor(255, 0, 0, 255);
                muffin::graphics::drawfillrect(floor(player::x + 6  + camx), floor(player::y - 1  + camy), 18, 2);
                muffin::graphics::drawfillrect(floor(player::x + 6  + camx), floor(player::y + 31 + camy), 18, 2);
                muffin::graphics::drawfillrect(floor(player::x + 4  + camx), floor(player::y + 1  + camy),  2, 30);
                muffin::graphics::drawfillrect(floor(player::x + 24 + camx), floor(player::y + 1  + camy),  2, 30);

                muffin::graphics::setcolor(0, 255, 0, 255);
                muffin::graphics::drawrect(floor(player::x + camx), floor(player::y + camy), 32, 32);
            }

            break;
        }
        case 1: {
            motion = false;

            // Physics
            if (physics) {
                if (underwater && !collisionu && !collisiond && !flying) {
                    y += speed * 0.85;
                    dy   = 0;
                    jump = false;
                } else {
                    if (jump && !collisionu && !flying) y -= 1.55f;

                    if (!collisiond && !flying) {
                        dy += 0.075f, y += dy;
                    } else {
                        dy   = 0;
                        jump = false;
                    }
                }
            }
            
            if (!ui::inventory && !ui::menu) {
                // Movement
                if (muffin::input::keyboard(MUFFIN_KEY_W)) {
                    if (underwater) {
                        y -= speed * 1.45;
                    } else {
                        if (flying && !collisionu) {
                            y -= speed * 1.5;
                        } else if (!jump) jump = true;
                    }
                }
                if (muffin::input::keyboard(MUFFIN_KEY_S) && flying && !collisiond) {
                    y += speed * 1.5;
                }
                if (muffin::input::keyboard(MUFFIN_KEY_A) && !collisionl && x >= 0) {
                    x -= speed;
                    
                    motion = true, flip = true;
                }
                if (muffin::input::keyboard(MUFFIN_KEY_D) && !collisionr && x < world::w * 16) {
                    x += speed;
                    
                    motion = true, flip = false;
                }
                if (muffin::input::keyboardevent(MUFFIN_KEY_SPACE)) {
                    flying = !flying;
                }

                speed = muffin::input::keyboard(MUFFIN_KEY_LCTRL) ? 1.55 : 0.85;
            }
            camx = -x + 156, camy = -y + 82;
            break;
        }
    }
}

void game::player::reset() {
    x   = world::w * 8, y = world::h * 8 / 4;

    idx = 0;
}

// UI
void game::ui::init() {
    font     = muffin::graphics::loadfont("assets/fonts/regular.otf", 10);
    gui      = muffin::graphics::loadimage("assets/gui/gui.png");
    intro    = muffin::graphics::loadimage("assets/gui/logo.png");
    logo     = muffin::graphics::loadimage("assets/gui/title.png");
    panorama = muffin::graphics::loadimage("assets/gui/panorama.png");
    click    = muffin::audio::loadaudio("assets/sfx/click.ogg");

    count    = muffin::ticksms();
    creative.resize(3, vector<unsigned char>(5 * 9));
    creative = {
        {
            1,   2,   3,   4,   5,   6,   7,   10,  11,  12,  14,  15,  16,  17,  18,  19, 
            20,  22,  23,  27,  28,  32,  33,  34,  35,  36,  44,  46,  48,  49,  50,  51, 
            53,  54,  55,  56,  60,  63,  64,  65,  66,  67,  68,  70,  72,  73,  74,  79, 
        }, 
        {
            80,  81,  82,  83,  84,  85,  88,  89,  90,  91,  92,  93,  94,  95,  96,  99, 
            100, 101, 102, 103, 104, 105, 108, 113, 114, 115, 116, 117, 119, 120, 122, 123, 
            125, 126, 129, 130, 133, 136, 141, 142, 143, 144, 145, 146, 148, 150, 151, 160, 
        }, 
        {
            161, 162, 177, 178, 192, 193, 194, 205, 209, 210, 
        }
    };
}

void game::ui::loop() {
    // FPS calculation
    count++;
    if (prev < muffin::ticksms() - 1000) {
        fps = count, prev = muffin::ticksms(), count = 0;
    }

    // Draw text with shading
    auto drawtext = [&](string text, unsigned int x, unsigned int y, bool fg, unsigned long color = 0xffffffff) {
        muffin::graphics::setcolor(0, 0, 0, 85);
        if (fg) muffin::graphics::drawfillrect(x - 2, y - 2, text.length() * 8, 16);

        muffin::graphics::setcolorhex(color - 0xbfbfbf5f);
        muffin::graphics::drawtext(font, text.c_str(), x + 1, y + 1, 1, 0, 0);
        muffin::graphics::setcolorhex(color);
        muffin::graphics::drawtext(font, text.c_str(), x,     y,     1, 0, 0);
    };

    // Draw according to state
    switch (state) {
        case 0: {
            switch (substate) {
                case 0: {
                    // Intro
                    muffin::graphics::drawimage(intro, 343 / 2 - 512 / 4, 207 - 256 / 2, 512 / 2, 256 / 2);

                    if (muffin::ticksms() - count > 5000) {
                        count    = muffin::ticksms();
                        substate = 1;
                    }

                    break;
                }
                case 1: {
                    // Panorama
                    muffin::graphics::drawimage(panorama, muffin::ticksms() / 100 % 1024,        0, 1024, 256);
                    muffin::graphics::drawimage(panorama, muffin::ticksms() / 100 % 1024 - 1024, 0, 1024, 256);

                    // Logo
                    muffin::graphics::drawimage(logo, 343 / 2 - 2000 / 16, 0, 2000 / 8, 506 / 8);
                    drawtext("Singleplayer", 343 / 2 - 30, 206 / 2, false, 0xffff00ff);

                    // Buttons
                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 - 5, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 5, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Singleplayer", 343 / 2 - 30, 206 / 2, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            count    = muffin::ticksms();
                            substate = 2;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 5, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Singleplayer", 343 / 2 - 30, 206 / 2, false);
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 20, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 20, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Options...", 343 / 2 - 20, 206 / 2 + 25, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT))  {
                            count    = muffin::ticksms();
                            substate = 5;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 20, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Options...", 343 / 2 - 20, 206 / 2 + 25, false);
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 45, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("About & help", 343 / 2 - 30, 206 / 2 + 50, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT))  {
                            count    = muffin::ticksms();
                            substate = 6;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("About & help", 343 / 2 - 30, 206 / 2 + 50, false);
                    }

                    drawtext("Minecraft: 2D Edition 1.0.0", 2, 206 - 12, false);

                    break;
                }
                case 2: {
                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        if (y > 0 && y < 4) muffin::graphics::setcolormod(world::sprite, 45,  45,  45,  255);
                        else                muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);
                    drawtext("Select World", 343 / 2 - 32, 206 / 12, false);

                    // Buttons
                    for (unsigned int i = 0; i < 3; i++) {
                        time_t time;
                        if (!world::indexing[i].empty()) {
                            time = world::indexing[i]["date"].get<time_t>();
                            if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 4 + i * 25, 200, 20)) {
                                muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 4 + i * 25, 200, 20, false, 0, 0, 222, 200, 20);
                                drawtext(string("World " + to_string(i + 1) + " - " + string(ctime(&time))).c_str(), 343 / 2 - 90, 206 / 4 + i * 25 + 5, false, 0xffff00ff);

                                if (muffin::ticksms() - count > 1000 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                    count       = muffin::ticksms();
                                    substate    = 8;
                                    world::name = i;
                                    muffin::audio::stopaudio();
                                    muffin::audio::playaudio(click);
                                }
                            } else {
                                muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 4 + i * 25, 200, 20, false, 0, 0, 202, 200, 20);
                                drawtext(string("World " + to_string(i + 1) + " - " + string(ctime(&time))).c_str(), 343 / 2 - 90, 206 / 4 + i * 25 + 5, false);
                            }
                        } else {
                            if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 4 + i * 25, 200, 20)) {
                                muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 4 + i * 25, 200, 20, false, 0, 0, 222, 200, 20);
                                drawtext(string("Empty slot " + to_string(i + 1)).c_str(), 343 / 2 - 90, 206 / 4 + i * 25 + 5, false, 0xffff00ff);

                                if (muffin::ticksms() - count > 1000 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                    count       = muffin::ticksms();
                                    substate    = 3;
                                    world::name = i;
                                    muffin::audio::stopaudio();
                                    muffin::audio::playaudio(click);
                                }
                            } else {
                                muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 4 + i * 25, 200, 20, false, 0, 0, 202, 200, 20);
                                drawtext(string("Empty slot " + to_string(i + 1)).c_str(), 343 / 2 - 90, 206 / 4 + i * 25 + 5, false);
                            }
                        }
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 45, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Refresh", 343 / 2 - 22, 206 / 2 + 50, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            world::index();
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Refresh", 343 / 2 - 22, 206 / 2 + 50, false);
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 70, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Cancel", 343 / 2 - 16, 206 / 2 + 75, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            count    = muffin::ticksms();
                            substate = 1;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Cancel", 343 / 2 - 16, 206 / 2 + 75, false);
                    }

                    break;
                }
                case 3: {
                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        if (y > 0 && y < 4) muffin::graphics::setcolormod(world::sprite, 45,  45,  45,  255);
                        else                muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);
                    drawtext("Creating World", 343 / 2 - 32, 206 / 12, false);

                    // Buttons
                    if (input) {
                        if (muffin::input::keyboardevent(MUFFIN_KEY_0))              world::seed.push_back('0');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_1))         world::seed.push_back('1');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_2))         world::seed.push_back('2');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_3))         world::seed.push_back('3');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_4))         world::seed.push_back('4');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_5))         world::seed.push_back('5');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_6))         world::seed.push_back('6');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_7))         world::seed.push_back('7');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_8))         world::seed.push_back('8');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_9))         world::seed.push_back('9');
                        else if (muffin::input::keyboardevent(MUFFIN_KEY_BACKSPACE)) world::seed.pop_back();
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 - 64, 200, 24)) {
                        if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) input = true;
                        muffin::graphics::setcolor(155, 155, 155, 255);
                    } else {
                        if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) input = false;
                        muffin::graphics::setcolor(125, 125, 125, 255);
                    }
                    if (input) muffin::graphics::setcolor(185, 185, 185, 255);
                    muffin::graphics::drawfillrect(343 / 2 - 100, 206 / 2 - 64, 200, 24);
                    muffin::graphics::setcolor(0,   0,   0,   255);
                    muffin::graphics::drawfillrect(343 / 2 - 99,  206 / 2 - 63, 198, 22);

                    // Buttons
                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 - 35, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 35, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext(world::size == 3 ? "World size: B" : 
                                 world::size == 2 ? "World size: M" : 
                                 world::size == 1 ? "World size: S" : "World size: T", 343 / 2 - 34, 206 / 2 - 30, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            if (world::size < 4) {
                                count       = muffin::ticksms();
                                world::size++;
                                muffin::audio::stopaudio();
                                muffin::audio::playaudio(click);
                            } else {
                                count       = muffin::ticksms();
                                world::size = 0;
                                muffin::audio::stopaudio();
                                muffin::audio::playaudio(click);
                            }
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 35, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext(world::size == 3 ? "World size: B" : 
                                 world::size == 2 ? "World size: M" : 
                                 world::size == 1 ? "World size: S" : "World size: T", 343 / 2 - 34, 206 / 2 - 30, false);
                    }

                    if (!world::seed.empty()) {
                        drawtext(world::seed.c_str(), 343 / 2 - 94, 206 / 2 - 58, false);

                        if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 45, 200, 20)) {
                            muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 222, 200, 20);
                            drawtext("Create", 343 / 2 - 16, 206 / 2 + 50, false, 0xffff00ff);

                            if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                count    = muffin::ticksms();
                                substate = 4;
                                muffin::audio::stopaudio();
                                muffin::audio::playaudio(click);
                                muffin::audio::stopmusic();
                            }
                        } else {
                            muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 202, 200, 20);
                            drawtext("Create", 343 / 2 - 16, 206 / 2 + 50, false);
                        }
                    } else {
                        muffin::graphics::setcolor(185, 185, 185, 255);
                        drawtext("Insert a seed, leave 0 for random.", 343 / 2 - 94, 206 / 2 - 58, false);

                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 182, 200, 20);
                        drawtext("Create", 343 / 2 - 16, 206 / 2 + 50, false);
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 70, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Cancel", 343 / 2 - 16, 206 / 2 + 75, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            count    = muffin::ticksms();
                            substate = 2;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                            world::seed.clear();
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Cancel", 343 / 2 - 16, 206 / 2 + 75, false);
                    }

                    break;
                }
                case 4: {
                    world::generate();

                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        if (y > 0 && y < 5) muffin::graphics::setcolormod(world::sprite, 45,  45,  45,  255);
                        else                muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);

                    drawtext(world::step == 0 ? "Initializing world"      : 
                             world::step == 1 ? "Hash-mapping"            : 
                             world::step == 2 ? "Generating perlin noise" : 
                             world::step == 3 ? "Placing mountains"       : 
                             world::step == 4 ? "Digging caves"           : 
                             world::step == 5 ? "Planting nature"         : 
                             world::step == 6 ? "Doing final touches"     : "Done!", 343 / 2 - world::w / 2, 206 / 4, false);

                    // Minimap
                    for (unsigned int y = 0; y < world::h; y++) {
                        for (unsigned int x = 0; x < world::w; x++) {
                            muffin::graphics::setcolor(world::map[x + y * world::w].id * 35, world::map[x + y * world::w].id * 35, world::map[x + y * world::w].id * 35, 255);
                            muffin::graphics::drawdot(343 / 2 - world::w / 2 + x, 206 / 2 - world::h / 2 + y);
                        }
                    }

                    break;
                }
                case 5: {
                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        if (y > 0 && y < 4) muffin::graphics::setcolormod(world::sprite, 45,  45,  45,  255);
                        else                muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);
                    drawtext("Options", 343 / 2 - 20, 206 / 12, false);

                    // Buttons
                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 - 55, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 55, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext(music ? "Music: [x]" : "Music: [ ]", 343 / 2 - 24, 206 / 2 - 50, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            music = !music;
                            count = muffin::ticksms();
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 55, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext(music ? "Music: [x]" : "Music: [ ]", 343 / 2 - 24, 206 / 2 - 50, false);
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 - 30, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 30, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext(sfx ? "SFX: [x]" : "SFX: [ ]", 343 / 2 - 18, 206 / 2 - 25, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            sfx   = !sfx;
                            count = muffin::ticksms();
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 30, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext(sfx ? "SFX: [x]" : "SFX: [ ]", 343 / 2 - 18, 206 / 2 - 25, false);
                    }

                    if (muffin::input::mouse(MUFFIN_BUTTON_RIGHT)) {
                        if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 45, 200, 20)) {
                            muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 222, 200, 20);
                            drawtext("Reset data", 343 / 2 - 28, 206 / 2 + 50, false, 0xffff00ff);

                            if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                count   = muffin::ticksms();
                                restart = true;
                                muffin::audio::stopaudio();
                                muffin::audio::playaudio(click);
                                filesystem::remove_all(filesystem::path(muffin::system::prefpath("lipx", "minecraft")));
                            }
                        } else {
                            muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 202, 200, 20);
                            drawtext("Reset data", 343 / 2 - 28, 206 / 2 + 50, false);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 182, 200, 20);
                        drawtext("Reset data (MOUSE_R to turn on)", 343 / 2 - 85, 206 / 2 + 50, false);
                    }

                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 70, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Cancel", 343 / 2 - 16, 206 / 2 + 75, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            count    = muffin::ticksms();
                            substate = 1;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Cancel", 343 / 2 - 16, 206 / 2 + 75, false);
                    }

                    break;
                }
                case 6: {
                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        if (y > 0 && y < 5) muffin::graphics::setcolormod(world::sprite, 45,  45,  45,  255);
                        else                muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);
                    drawtext("About", 343 / 2 - 18, 206 / 12, false);

                    // About
                    drawtext("Minecraft: 2D Edition",       343 / 8, 206 / 5,      false);
                    drawtext("(almost) everything by lipx", 343 / 8, 206 / 5 + 16, false);
                    drawtext("Aknowledgements:",            343 / 8, 206 / 5 + 40, false);
                    drawtext("Skydev - playtesting",        343 / 8, 206 / 5 + 56, false);
                    drawtext("Mojang - for existing",       343 / 8, 206 / 5 + 72, false);
                    drawtext("Thanks :3",                   343 / 8, 206 / 5 + 96, false);

                    // Buttons
                    if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 70, 200, 20)) {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 222, 200, 20);
                        drawtext("Back", 343 / 2 - 12, 206 / 2 + 75, false, 0xffff00ff);

                        if (muffin::ticksms() - count > 500 && muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                            count    = muffin::ticksms();
                            substate = 1;
                            muffin::audio::stopaudio();
                            muffin::audio::playaudio(click);
                        }
                    } else {
                        muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 70, 200, 20, false, 0, 0, 202, 200, 20);
                        drawtext("Back", 343 / 2 - 12, 206 / 2 + 75, false);
                    }

                    break;
                }
                case 7: {
                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);

                    drawtext("Saving...", 343 / 2 - 30, 206 / 12, false);
                    if (muffin::ticksms() - count >= 1000) {
                        world::save();
                        world::reset();
                        player::reset();
                        reset();
                    }

                    break;
                }
                case 8: {
                    // Background
                    for (unsigned int y = 0; y < 206; y++) {
                        muffin::graphics::setcolormod(world::sprite, 105, 105, 105, 255);
                        
                        for (unsigned int x = 0; x < 343; x++) {
                            muffin::graphics::drawimage(world::sprite, x * 32, y * 32, 32, 32, false, 0, 32, 0, 16, 16);
                        }
                    }
                    muffin::graphics::setcolormod(world::sprite, 255, 255, 255, 255);

                    drawtext("Loading...", 343 / 2 - 30, 206 / 12, false);
                    if (muffin::ticksms() - count >= 1000) {
                        world::load();
                        state = 1;
                    }

                    break;
                }
            }
            
            break;
        }
        case 1: {
            // Debug screen (yes, I even made this lmaaoo)
            if (muffin::input::keyboardevent(MUFFIN_KEY_F3)) debug   = !debug;
            if (debug) {
                unsigned int pos = 4;
                drawtext(to_string((int)fps) + " FPS", 2, pos, true);
                pos += 16;
                drawtext("XY: " + to_string(player::x) + " / " + to_string(player::y), 2, pos, true);
                pos += 16;
                drawtext(to_string(world::w * world::w) + " blocks (" + to_string(world::chunk) + " by chunk)", 2, pos, true);
                pos += 16;
                drawtext(to_string(world::cycles) + " cycles", 2, pos, true);
            }

            // Slot bar
            muffin::graphics::drawimage(gui, 82, 180, 182, 22, false, 0, 0, 0, 182, 22);
            for (unsigned int i = 0; i < 9; i++) {
                if (player::items[i].id != 0) 
                    muffin::graphics::drawimage(world::sprite, 85 + i * 20, 183, 16, 16, false, 0, player::items[i].id % 16 * 16, player::items[i].id / 16 * 16, 16, 16);
            }
            muffin::graphics::drawimage(gui, 81 + player::idx * 20, 179, 24, 24, false, 0, 0, 22, 24, 24);

            // Inventory
            if (muffin::input::keyboardevent(MUFFIN_KEY_E) && !menu) inventory = !inventory;
            if (inventory && !menu) {
                muffin::graphics::setcolor(0, 0, 0, 85);
                muffin::graphics::drawfillrect(0, 0, 343, 206);

                for (unsigned int i = 0; i < creative.size(); i++) {
                    if (tab == i) muffin::graphics::drawimage(gui, 343 / 2 - 195 / 2 + (i * 26), 206 / 2 - 136 / 2 - 26, 26, 30, false, 0, 195, 76, 26, 30);
                    else          muffin::graphics::drawimage(gui, 343 / 2 - 195 / 2 + (i * 26), 206 / 2 - 136 / 2 - 26, 26, 30, false, 0, 195, 46, 26, 30);

                    if (muffin::input::mouse(MUFFIN_BUTTON_LEFT) && utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 195 / 2 + (i * 26), 206 / 2 - 136 / 2 - 26, 26, 30))
                        tab = i;
                }

                muffin::graphics::drawimage(gui, 343 / 2 - 195 / 2, 206 / 2 - 136 / 2, 195, 136, false, 0, 0,   46, 195, 136);
                muffin::graphics::setcolorhex(0x373737ff);
                muffin::graphics::drawtext(font, string("Section " + to_string(tab + 1)).c_str(), 343 / 2 - 195 / 2 + 8, 206 / 2 - 136 / 2 + 6, 1, 0, 0);

                for (unsigned int y = 0; y < 5; y++) {
                    for (unsigned int x = 0; x < 9; x++) {
                        if (creative[tab][x + y * 9] != 0) {
                            muffin::graphics::drawimage(world::sprite, (343 / 2 - 195 / 2 + 9) + x * 18, (206 / 2 - 136 / 2 + 18) + y * 18, 16, 16, false, 0, (creative[tab][(x + y * 9)] % 16) * 16, (creative[tab][(x + y * 9)] / 16) * 16, 16, 16);
                            if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, (343 / 2 - 195 / 2 + 9) + x * 18, (206 / 2 - 136 / 2 + 18) + y * 18, 16, 16)) {
                                muffin::graphics::setcolor(255, 255, 255, 85);
                                muffin::graphics::drawfillrect((343 / 2 - 195 / 2 + 9) + x * 18, (206 / 2 - 136 / 2 + 18) + y * 18, 16, 16);
                                if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                                    sel = creative[tab][x + y * 9];
                                }
                            }
                            if (debug) drawtext(to_string(creative[tab][x + y * 9]), (343 / 2 - 195 / 2 + 9) + x * 18, (206 / 2 - 136 / 2 + 18) + y * 18, true);
                        }
                    }
                }

                for (unsigned int i = 0; i < 9; i++) {
                    if (player::items[i].id != 0) 
                        muffin::graphics::drawimage(world::sprite, (343 / 2 - 195 / 2 + 9) + i * 18, 206 / 2 - 136 / 2 + 112, 16, 16, false, 0, player::items[i].id % 16 * 16, player::items[i].id / 16 * 16, 16, 16);
                }

                if (sel != 0) muffin::graphics::drawimage(world::sprite, muffin::input::mousex() / 3, muffin::input::mousey() / 3, 16, 16, false, 0, sel % 16 * 16, sel / 16 * 16, 16, 16);
                if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 195 / 2 + 9, (206 / 2 - 136 / 2 + 18) + 94, 9 * 18, 16)) {
                    if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                        if (sel) {
                            player::items[(muffin::input::mousex() / 3 - (343 / 2 - 195 / 2 + 9)) / 18].id = sel;
                            sel = 0;
                        }
                    } else if (muffin::input::mouse(MUFFIN_BUTTON_RIGHT)) {
                        if (!sel) {
                            sel = player::items[(muffin::input::mousex() / 3 - (343 / 2 - 195 / 2 + 9)) / 18].id;
                            player::items[(muffin::input::mousex() / 3 - (343 / 2 - 195 / 2 + 9)) / 18].id = 0;
                        }
                    }
                }
            } else {
                sel = 0;
            }

            // Menu
            if (muffin::input::keyboardevent(MUFFIN_KEY_ESCAPE)) menu = !menu;
            if (menu) {
                inventory = false;
                muffin::graphics::setcolor(0, 0, 0, 85);
                muffin::graphics::drawfillrect(0, 0, 343, 206);

                // Buttons
                if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 - 5, 200, 20)) {
                    muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 5, 200, 20, false, 0, 0, 222, 200, 20);
                    drawtext("Resume", 343 / 2 - 20, 206 / 2, false, 0xffff00ff);

                    if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                        menu = false;
                        muffin::audio::stopaudio();
                        muffin::audio::playaudio(click);
                    }
                } else {
                    muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 - 5, 200, 20, false, 0, 0, 202, 200, 20);
                    drawtext("Resume", 343 / 2 - 20, 206 / 2, false);
                }

                if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 20, 200, 20)) {
                    muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 20, 200, 20, false, 0, 0, 222, 200, 20);
                    drawtext("Save", 343 / 2 - 14, 206 / 2 + 25, false, 0xffff00ff);

                    if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                        world::save();
                        muffin::audio::stopaudio();
                        muffin::audio::playaudio(click);
                    }
                } else {
                    muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 20, 200, 20, false, 0, 0, 202, 200, 20);
                    drawtext("Save", 343 / 2 - 14, 206 / 2 + 25, false);
                }

                if (utils::aabb(muffin::input::mousex() / 3, muffin::input::mousey() / 3, 1, 1, 343 / 2 - 100, 206 / 2 + 45, 200, 20)) {
                    muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 222, 200, 20);
                    drawtext("Save and quit", 343 / 2 - 34, 206 / 2 + 50, false, 0xffff00ff);

                    if (muffin::input::mouse(MUFFIN_BUTTON_LEFT)) {
                        count = muffin::ticksms();
                        state = 0, substate = 7;
                        muffin::audio::stopaudio();
                        muffin::audio::playaudio(click);
                    }
                } else {
                    muffin::graphics::drawimage(gui, 343 / 2 - 100, 206 / 2 + 45, 200, 20, false, 0, 0, 202, 200, 20);
                    drawtext("Save and quit", 343 / 2 - 34, 206 / 2 + 50, false);
                }
            }

            // Minimap
            if (muffin::input::keyboardevent(MUFFIN_KEY_F4)) minimap = !minimap;
            if (minimap) {
                for (unsigned int y = 0; y < world::h; y++) {
                    for (unsigned int x = 0; x < world::w; x++) {
                        muffin::graphics::setcolor(world::map[x + y * world::w].id * 35, world::map[x + y * world::w].id * 35, world::map[x + y * world::w].id * 35, 255);
                        muffin::graphics::drawdot(343 / 2 - world::w / 2 + x, 206 / 2 - world::h / 2 + y);
                    }
                }

                muffin::graphics::setcolor(255, 0, 0, 255);
                muffin::graphics::drawline(343 / 2 - world::w / 2 + player::x / 16,     206 / 2 - world::h / 2 + player::y / 16, 
                                           343 / 2 - world::w / 2 + player::x / 16, 1 + 206 / 2 - world::h / 2 + player::y / 16);

                muffin::graphics::setcolor(0, 255, 0, 255);
                /*
                for (auto & i : world::entities) {
                    muffin::graphics::drawline(343 / 2 - world::w / 2 + i.x / 16,     206 / 2 - world::h / 2 + i.y / 16, 
                                               343 / 2 - world::w / 2 + i.x / 16, 1 + 206 / 2 - world::h / 2 + i.y / 16);
                }
                */
            }

            break;
        }
    }
}

void game::ui::reset() {
    sel = 0, substate = 1, tab = 0;
    minimap = false, inventory = false, menu = false, input = false;

    prev = 0, count = 0;
    muffin::audio::volumemusic(0);
}

// Sound
void game::sound::init() {
    for (unsigned int i = 1; i < 23; i++)
        songs.push_back(muffin::audio::loadmusic(string("assets/music/" + to_string(i) + ".mp3").c_str()));

    muffin::audio::playmusic(songs[rand() % 2 ? 21 : 5]);
}

void game::sound::loop() {
    if (state == 1) {
        if (!muffin::audio::playing()) {
            if (!count) {
                muffin::audio::stopmusic();
                count = muffin::ticksms();
            } else if (muffin::ticksms() - count >= 15000) {
                muffin::audio::playmusic(songs[rand() % 22]);
                count = 0;
            }
        }
    }

    muffin::audio::volumemusic(music * 64);
    muffin::audio::volumeaudio(sfx   * 128);
}

int main(int argc, char * argv[]) {
    // Creates game data directory in case it doesn't exist
    if (!filesystem::exists(muffin::system::prefpath("lipx", "minecraft")))
        filesystem::create_directories(muffin::system::prefpath("lipx", "minecraft"));

    do {
        game::restart = false;
        game::init();
        game::player::reset();
        game::world::reset();
        game::ui::reset();
    } while (game::restart);
    
    return 0;
}