#include <SDL.h>
#include <cmath>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <string>
#include <chrono>
#include "MathFunctions.cpp"
#include "RenderFunctions.cpp"
#include "InputHandling.cpp"
#include "BotHandling.cpp"

const int SCREEN_WIDTH = 1500;
const int SCREEN_HEIGHT = 800;
const int FPS = 60;
const int FRAME_DELAY = 1000 / FPS;

const int WORLD_X_OFFSET = 400;
const int WORLD_Y_OFFSET = 100;
const int WORLD_PIXEL_SIZE = 2;

const std::vector<std::vector<std::string>> ASSET_NAMES{{
        "assets/BLOCKS/dirt_0",
            "assets/BLOCKS/stone_0",
            "assets/BLOCKS/dev_grid"}, {
            "assets/BLUE/BLUE BASIC",
            "assets/BLUE/BLUE ALCHEMIST",
            "assets/BLUE/BLUE ARCHER",
            "assets/BLUE/BLUE BUILDER",
            "assets/BLUE/BLUE INFINITE",
            "assets/BLUE/BLUE MAGE",
            "assets/BLUE/BLUE MEDIC",
            "assets/BLUE/BLUE MINER",
            "assets/BLUE/BLUE SMITH",
            "assets/BLUE/BLUE SOLDIER",
            "assets/BLUE/BLUE TANK",
            "assets/BLUE/BLUE NEXUS" }, {
            "assets/RED/RED BASIC",
            "assets/RED/RED ALCHEMIST",
            "assets/RED/RED ARCHER",
            "assets/RED/RED BUILDER",
            "assets/RED/RED INFINITE",
            "assets/RED/RED MAGE",
            "assets/RED/RED MEDIC",
            "assets/RED/RED MINER",
            "assets/RED/RED SMITH",
            "assets/RED/RED SOLDIER",
            "assets/RED/RED TANK",
            "assets/RED/RED NEXUS" }, {

            }
};

void initialize_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {

        exit(1);
    }
}

SDL_Window* create_window() {
    SDL_Window* window = SDL_CreateWindow("MapRender", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {

        SDL_Quit();
        exit(1);
    }
    return window;
}

SDL_Surface* get_window_surface(SDL_Window* window) {
    SDL_Surface* screen = SDL_GetWindowSurface(window);
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
    return screen;
}

input handle_events(bool& running, input current_input) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (current_input.mouse_pressed)current_input.mouse_reset = false;
        current_input.mouse_x = event.button.x;
        current_input.mouse_y = event.button.y;
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                current_input.mouse_pressed = true;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                current_input.mouse_pressed = false;
                current_input.mouse_reset = true;
            }
        }
        else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            SDL_Keycode keyCode = event.key.keysym.sym;
            auto it = key_map.find(keyCode);
            if (it != key_map.end()) {
                int index = it->second;
                current_input.key_pressed[index] = (event.type == SDL_KEYDOWN);
                if (SDL_KEYUP) {
                    current_input.key_reset[index] = true;
                }
            }
        }
    }
    return current_input;
}

void load_textures(std::vector<std::vector<std::vector<Uint32>>>* texture) {
    std::string file_name;
    int line_counter;
    (*texture).resize(ASSET_NAMES.size());
    for (int i = 0; i < ASSET_NAMES.size(); i++) {
        (*texture)[i].resize(ASSET_NAMES[i].size());
        for (int j = 0; j < ASSET_NAMES[i].size(); j++) {
            (*texture)[i][j].resize(256);
            file_name = ASSET_NAMES[i][j];
            file_name += ".txt";
            std::ifstream inputFile(file_name);
            if (inputFile.is_open()) {
                std::string line;
                line_counter = 0;
                while (std::getline(inputFile, line)) {
                    (*texture)[i][j][line_counter] = std::stoi(line);
                    line_counter++;
                }
                inputFile.close();
            }
        }
    }
}

void clean_up(SDL_Window* window) {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool on_button(input current_input, button button) {
    return current_input.mouse_reset && current_input.mouse_x > button.x && current_input.mouse_x < button.x_bound && current_input.mouse_y > button.y && current_input.mouse_y < button.y_bound;
}

int main(int argc, char* argv[]) {

    //srand(time(0));
    srand(0);

    Uint32 frameStart;
    int frameTime;

    input current_input;

    initialize_SDL();

    SDL_Window* window = create_window();

    SDL_Surface* screen = get_window_surface(window);

    //init start

    std::vector<std::vector<cell>> world(30, std::vector<cell>(20));

    for (int i = 0; i < world[0].size(); i++) {
        for (int j = 0; j < world.size(); j++) {
            //world[j][i].texture = int(random_float(0,3.9999));
            world[j][i].texture = 0;
            world[j][i].walkable = true;
            world[j][i].occupied = false;
            if (j > 5 && j < world.size() - 5) {
                if (random_float(0, 3) > 2) {
                    world[j][i].texture = 1;
                    world[j][i].walkable = false;
                }
            }
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds duration, last_duration;

    int active_textbox = -1;
    bool using_textbox = false;

    std::vector<textbox> textbox(std::vector<textbox>(4));

    std::vector<button> button(std::vector<button>(14));

    button[0].init(10, 10, 3, "SAVE", 0xCCCCCC, 0x444444, 0x999999);
    button[1].init(100, 10, 3, "LOAD", 0xCCCCCC, 0x444444, 0x999999);
    textbox[0].init(10, 50, 3, 9, 0xCCCCCC, 0x444444, 0x999999);
    button[2].init(190, 10, 3, "SEED", 0xCCCCCC, 0x444444, 0x999999);
    textbox[1].init(280, 10, 3, 6, 0xCCCCCC, 0x444444, 0x999999);
    button[3].init(190, 50, 3, "START", 0xCCCCCC, 0x444444, 0x999999);
    button[4].init(300, 50, 3, "RESET", 0xCCCCCC, 0x444444, 0x999999);
    button[5].init(410, 10, 3, "TEAM 1", 0xCCCCCC, 0x444444, 0x999999);
    button[6].init(410, 50, 3, "ERASER", 0xCCCCCC, 0x444444, 0x999999);
    button[7].init(540, 10, 3, "PLACE", 0xCCCCCC, 0x444444, 0x999999);
    button[8].init(540, 50, 3, "MONEY", 0xCCCCCC, 0x444444, 0x999999);
    button[9].init(650, 10, 3, "  PEASANT", 0xCCCCCC, 0x444444, 0x999999);
    button[10].init(650, 50, 3, "         ", 0xCCCCCC, 0x444444, 0x999999);
    button[11].init(830, 10, 3, " NORMAL", 0xCCCCCC, 0x444444, 0x999999);
    button[12].init(830, 50, 3, "EDIT MONEY", 0xCCCCCC, 0x444444, 0x999999);
    textbox[2].init(1030, 50, 3, 10, 0xCCCCCC, 0x444444, 0x999999);


    auto now = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    std::vector<std::vector<std::vector<Uint32>>> texture;// (ASSET_NAMES.size(), std::vector<Uint32>(256));

    load_textures(&texture);

    std::vector<std::vector<bot>> bot_vec(3, std::vector<bot>(0));
    std::vector<std::vector<nexus>> nexus_vec(3, std::vector <nexus>(0));

    /* for (int i = 0; i < bot.size(); i++) {

           bot[i].init(16 * WORLD_PIXEL_SIZE * 2, 16 * WORLD_PIXEL_SIZE * 16, WORLD_PIXEL_SIZE, 7, 2, 0);

     }*/

     /*
     bot_vec[current_team][bot_vec[current_team].size() - 1].set_waypoints(
     world,
     16 * WORLD_PIXEL_SIZE * 27,
     16 * WORLD_PIXEL_SIZE * 2,
     WORLD_PIXEL_SIZE, world.size(),
     world[0].size());
     */
    int current_team = 1;
    int current_class = 0;
    int current_type = 0;

    int
        pallet_x = 30,
        pallet_y = 100,
        pallet_size = 2;

    draw_world(screen, texture[0], world, WORLD_X_OFFSET, WORLD_Y_OFFSET, WORLD_PIXEL_SIZE);
    draw_assets(screen, texture, pallet_x, pallet_y, pallet_size, current_team);

    bool playing = false;

    point active_cell;

    std::vector<int> money(3);
    int starting_money = int(random_float(100, 5000));
    for (int i = 0; i < money.size(); i++) {
        money[i] = starting_money;
    }

    //init end

    SDL_UpdateWindowSurface(window);

    bool running = true;
    while (running) {

        frameStart = SDL_GetTicks();

        //loop start

        auto now = std::chrono::high_resolution_clock::now();
        last_duration = duration;
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        //std::cout << duration.count() - last_duration.count() << "ms" << std::endl;

        //draw_rectangle(screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x555555);

        draw_world(screen, texture[0], world, WORLD_X_OFFSET, WORLD_Y_OFFSET, WORLD_PIXEL_SIZE);
        draw_assets(screen, texture, pallet_x, pallet_y, pallet_size, current_team);

        if (playing) {
            for (int j = 0; j < 3; j++) {
                for (int i = 0; i < bot_vec[j].size(); i++) {


                    bot_vec[j][i].tick(world, bot_vec, nexus_vec, WORLD_PIXEL_SIZE);


                }
            }
        }

        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < nexus_vec[j].size(); i++) {

                nexus_vec[j][i].draw(screen, texture, WORLD_X_OFFSET, WORLD_Y_OFFSET);

            }
        }

        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < bot_vec[j].size(); i++) {

                bot_vec[j][i].draw(screen, texture, WORLD_X_OFFSET, WORLD_Y_OFFSET);

            }
        }


        if (current_input.mouse_pressed && current_input.mouse_reset) {
            if (current_input.mouse_x > WORLD_X_OFFSET &&
                current_input.mouse_x < WORLD_PIXEL_SIZE * world.size() * 16 + WORLD_X_OFFSET &&
                current_input.mouse_y > WORLD_Y_OFFSET &&
                current_input.mouse_y < WORLD_PIXEL_SIZE * world[0].size() * 16 + WORLD_Y_OFFSET) {
                active_cell.x = (current_input.mouse_x - WORLD_X_OFFSET) / (16 * WORLD_PIXEL_SIZE);
                active_cell.y = (current_input.mouse_y - WORLD_Y_OFFSET) / (16 * WORLD_PIXEL_SIZE);
                if (current_class == 2) {
                    if (current_type < 11) {
                        if (world[active_cell.x][active_cell.y].occupied == false && world[active_cell.x][active_cell.y].walkable == true) {
                            world[active_cell.x][active_cell.y].occupied = true;
                            money[current_team] -= 30;
                            current_input.mouse_reset = false;
                            bot new_bot;
                            new_bot.init(
                                (current_input.mouse_x - WORLD_X_OFFSET) - (current_input.mouse_x - WORLD_X_OFFSET) % (16 * WORLD_PIXEL_SIZE),
                                (current_input.mouse_y - WORLD_Y_OFFSET) - (current_input.mouse_y - WORLD_Y_OFFSET) % (16 * WORLD_PIXEL_SIZE),
                                WORLD_PIXEL_SIZE, 2, current_team * 2 - 1, current_team, current_type);
                            bot_vec[current_team].push_back(new_bot);

                            std::cout << bot_vec.size() << " spawned" << std::endl;
                        }
                    }
                    else if (current_type == 11) {
                        if (world[active_cell.x][active_cell.y].occupied == false && world[active_cell.x][active_cell.y].walkable == true) {
                            world[active_cell.x][active_cell.y].occupied = true;
                            money[current_team] -= 100;
                            current_input.mouse_reset = false;
                            nexus new_nexus;
                            new_nexus.init(
                                (current_input.mouse_x - WORLD_X_OFFSET) - (current_input.mouse_x - WORLD_X_OFFSET) % (16 * WORLD_PIXEL_SIZE),
                                (current_input.mouse_y - WORLD_Y_OFFSET) - (current_input.mouse_y - WORLD_Y_OFFSET) % (16 * WORLD_PIXEL_SIZE),
                                WORLD_PIXEL_SIZE, current_team);
                            nexus_vec[current_team].push_back(new_nexus);
                            std::cout << nexus_vec.size() << " spawned" << std::endl;
                        }
                    }
                }
            }
            else if (current_input.mouse_x > pallet_x &&
                current_input.mouse_x < pallet_size * 4 * 16 + pallet_x &&
                current_input.mouse_y > pallet_y &&
                current_input.mouse_y < pallet_size * 6 * 16 + pallet_y) {

                if (current_input.mouse_x > pallet_x && current_input.mouse_x < pallet_x + pallet_size * 2 * 16) {
                    current_class = 1;
                }
                else if (current_input.mouse_x > pallet_x + pallet_size * 2 * 16 && current_input.mouse_x < pallet_x + pallet_size * 4 * 16) {
                    current_class = 2;
                    current_type = int((current_input.mouse_y - pallet_y) / (pallet_size * 16)) * 2 + int((current_input.mouse_x - pallet_x) / (pallet_size * 16)) - 2;
                    std::cout << current_type << std::endl;
                }

            }
            else if (on_button(current_input, button[3])) {
                current_input.mouse_reset = false;
                if (button[3].text == "START") {
                    button[3].text = "STOP";
                    playing = true;
                }
                else {
                    button[3].text = "START";
                    playing = false;
                }

            }
            else if (on_button(current_input, button[4])) {
                current_input.mouse_reset = false;
                for (int j = 0; j < 3; j++) {
                    bot_vec[j].clear();
                    nexus_vec[j].clear();
                    money[j] = starting_money;
                }
                button[3].text = "START";
                playing = false;
                for (int i = 0; i < world[0].size(); i++) {
                    for (int j = 0; j < world.size(); j++) {
                        world[j][i].occupied = false;
                        
                    }
                }
            }
            else if (on_button(current_input, button[5])) {
                current_input.mouse_reset = false;

                if (button[5].text == "TEAM 1") {
                    button[5].text = "TEAM 2";
                    current_team = 2;
                }
                else {
                    button[5].text = "TEAM 1";
                    current_team = 1;
                }

            }


        }

        button[10].text = std::to_string(money[current_team]);

        handle_buttons(screen, &button);

        handle_textboxes(screen, &textbox, &active_textbox, &using_textbox, &current_input);

        //loop end

        SDL_UpdateWindowSurface(window);

        frameTime = SDL_GetTicks() - frameStart;
        if (FRAME_DELAY > frameTime) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }

        current_input = handle_events(running, current_input);
    }

    clean_up(window);

    return EXIT_SUCCESS;
}