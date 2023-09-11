#include <SDL.h>
#include <vector>

class nexus;
class bot;
class nexus {

public:
	int
		x_pos,
		y_pos,
		health,
		team,
		size;

	Uint32 texture_id;

	bool alive;

	void init(int x_pos_in, int y_pos_in, int size_in, int team_in) {

		x_pos = x_pos_in;
		y_pos = y_pos_in;
		team = team_in;
		health = 100;
		alive = true;
		size = size_in;
		texture_id = 11;

	}

	void draw(SDL_Surface* surface, std::vector<std::vector<std::vector<Uint32>>> texture, int grid_x, int grid_y) {

		draw_asset(surface, texture[team][texture_id], x_pos + grid_x, y_pos + grid_y, size);

	}

};
class bot {

public:

	float x_pos, y_pos;
	Uint32 size;
	Uint32 texture_id;

	Uint32 speed;

	std::vector<point> waypoints;

	int team;
	int type;
	int health;
	int damage;
	int rotation;

	void init(float x_pos_in, float y_pos_in, Uint32 size_in, Uint32 speed_in, int rotation_in, int team_in, int type_in) {

		x_pos = x_pos_in;
		y_pos = y_pos_in;
		size = size_in;
		speed = speed_in;
		rotation = rotation_in;
		team = team_in;
		type = type_in;
		texture_id = type;
		health = 10;
		damage = 4;

	}

	void respawn(std::vector<std::vector<nexus>> nexus_vec) {
		int resawn_index = int(random_float(0, nexus_vec[team].size() - 0.0000001));
		x_pos = nexus_vec[team][resawn_index].x_pos;
		y_pos = nexus_vec[team][resawn_index].y_pos;
		health = 100;
		waypoints.clear();
	}

	void draw(SDL_Surface* surface, std::vector<std::vector<std::vector<Uint32>>> texture, int grid_x, int grid_y) {

		draw_asset_rotated(surface, texture[team][texture_id], x_pos + grid_x, y_pos + grid_y, size, rotation);

	}

	void set_waypoints(std::vector<std::vector<cell>> world, float target_x, float target_y, int cell_size, int grid_width, int grid_height) {

		target_x *= cell_size * 16;
		target_y *= cell_size * 16;

		waypoints.clear();

		point target_index = pos_to_index(target_x, target_y, cell_size);

		point current_index = pos_to_index(x_pos, y_pos, cell_size);

		waypoints.push_back(current_index);

		std::vector<std::vector<int>> directions(world.size(), std::vector<int>(world[0].size()));

		point trace_index;
		trace_index.x = current_index.x;
		trace_index.y = current_index.y;

		for (int i = 0; i < world[0].size(); i++) {
			for (int j = 0; j < world.size(); j++) {

				directions[j][i] = 0;

			}
		}

		directions[target_index.x][target_index.y] = 5;

		bool running = true;

		while (running) {

			running = false;

			for (int i = 0; i < world[0].size(); i++) {
				for (int j = 0; j < world.size(); j++) {

					if (directions[j][i] == 0 && world[j][i].walkable) {

						if (j > 0) {
							if (directions[j - 1][i] != 0) {
								directions[j][i] = 4;
								running = true;
							}
						}
						if (j < world.size() - 1) {
							if (directions[j + 1][i] != 0) {
								directions[j][i] = 2;
								running = true;
							}
						}
						if (i > 0) {
							if (directions[j][i - 1] != 0) {
								directions[j][i] = 1;
								running = true;
							}
						}
						if (i < world[0].size() - 1) {
							if (directions[j][i + 1] != 0) {
								directions[j][i] = 3;
								running = true;
							}
						}


					}

				}
			}

			if (directions[current_index.x][current_index.y] != 0) {
				running = false;
			}

		}


		int last_direction;

		if (directions[current_index.x][current_index.y] == 0) {
			//unreachable

		}
		else {

			while (trace_index.x != target_index.x || trace_index.y != target_index.y) {

				running = true;

				while (running) {

					last_direction = directions[trace_index.x][trace_index.y];

					if (last_direction == 1) {

						trace_index.y--;

					}
					else if (last_direction == 2) {

						trace_index.x++;

					}
					else if (last_direction == 3) {

						trace_index.y++;

					}
					else if (last_direction == 4) {

						trace_index.x--;

					}

					if (directions[trace_index.x][trace_index.y] != last_direction) {
						running = false;
					}

				}

				waypoints.push_back(trace_index);

			}

		}



	}

	void move(int pixel_size) {

		if (!waypoints.empty()) {

			if (x_pos > index_to_pos(waypoints[0].x, waypoints[0].y, pixel_size).x + 1) {
				x_pos -= speed;
				rotation = 3;
			}
			else if (x_pos < index_to_pos(waypoints[0].x, waypoints[0].y, pixel_size).x - 1) {
				x_pos += speed;
				rotation = 1;
			}
			else if (y_pos > index_to_pos(waypoints[0].x, waypoints[0].y, pixel_size).y + 1) {
				y_pos -= speed;
				rotation = 0;
			}
			else if (y_pos < index_to_pos(waypoints[0].x, waypoints[0].y, pixel_size).y - 1) {
				y_pos += speed;
				rotation = 2;
			}
			else {
				waypoints.erase(waypoints.begin());
			}
		}

	}

	void tick(std::vector<std::vector<cell>> world, std::vector<std::vector<bot>>& bot_vec, std::vector<std::vector<nexus>>& nexus_vec, int pixel_size) {
		int target_index;
		bool enemy_in_range = false;

		for (int i = 0; i < bot_vec.size(); i++) {
			for (int j = 0; j < bot_vec[i].size(); j++) {

				if (i != team) {

					if (std::sqrt(std::pow(std::abs(bot_vec[i][j].x_pos - x_pos), 2) + std::pow(std::abs(bot_vec[i][j].y_pos - y_pos), 2)) < 10) {
						enemy_in_range = true;
						attack(0, i, j, damage, bot_vec, nexus_vec);
					}

				}

			}
		}
		for (int i = 0; i < nexus_vec.size(); i++) {
			for (int j = 0; j < nexus_vec[i].size(); j++) {

				if (i != team) {

					if (std::sqrt(std::pow(std::abs(nexus_vec[i][j].x_pos - x_pos), 2) + std::pow(std::abs(nexus_vec[i][j].y_pos - y_pos), 2)) < 10) {
						enemy_in_range = true;
						attack(1, i, j, damage, bot_vec, nexus_vec);
					}

				}

			}
		}
		if (enemy_in_range == false) {
			if (!waypoints.empty()) {
				move(pixel_size);
			}
			else {
				if (type == 0) {
					if (team > 0) {
						if (!nexus_vec[team * -1 + 3].empty()) {
							target_index = random_float(0, nexus_vec[team * -1 + 3].size() - 0.00001);
							set_waypoints(world, nexus_vec[team * -1 + 3][target_index].x_pos / 32, nexus_vec[team * -1 + 3][target_index].y_pos / 32, pixel_size, world.size(), world[0].size());
						}else if (!bot_vec[team * -1 + 3].empty()) {
							target_index = random_float(0, bot_vec[team * -1 + 3].size() - 0.00001);
							set_waypoints(world, bot_vec[team * -1 + 3][target_index].x_pos / 32, bot_vec[team * -1 + 3][target_index].y_pos / 32, pixel_size, world.size(), world[0].size());
						}
					}
				}

			}
		}

	}

	void attack(int type, int index0, int index1, int damage, std::vector<std::vector<bot>>& bot_vec, std::vector<std::vector<nexus>>& nexus_vec) {

		if (type == 0) {
			bot_vec[index0][index1].health -= damage;
			if (bot_vec[index0][index1].health <= 0) {
				if (nexus_vec[index0].empty()) {
					remove(0, index0, index1, bot_vec, nexus_vec);
				}
				else {
					bot_vec[index0][index1].respawn(nexus_vec);
				}
			}
		}
		else {
			nexus_vec[index0][index1].health -= damage;
			if (nexus_vec[index0][index1].health <= 0) {
				remove(1, index0, index1, bot_vec, nexus_vec);
			}
		}
	}

	void remove(int type, int index0, int index1, std::vector<std::vector<bot>>& bot_vec, std::vector<std::vector<nexus>>& nexus_vec) {
		if (type == 0) {
			bot_vec[index0].erase(bot_vec[index0].begin() + index1);
		}
		else {
			nexus_vec[index0].erase(nexus_vec[index0].begin() + index1);
		}
	}

};
