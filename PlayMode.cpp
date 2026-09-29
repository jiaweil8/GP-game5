#include "PlayMode.hpp"

#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <cmath>

PlayMode::PlayMode(Client &client_)
	: client(client_), text_renderer(data_path("PaytoneOne-Regular.ttf")) {
	boom_text = text_renderer.make_text("BOOM!");
}

PlayMode::~PlayMode() {
	text_renderer.destroy_text(status_text);
	text_renderer.destroy_text(boom_text);
	for (auto &entry : player_name_textures) {
		text_renderer.destroy_text(entry.second);
	}
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.key == SDLK_A) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			controls.jump.downs += 1;
			controls.jump.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			controls.jump.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	controls.send_controls_message(&client.connection);

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.jump.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	glClearColor(0.05f, 0.07f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	//figure out view transform to center the arena:
	float aspect = float(drawable_size.x) / float(drawable_size.y);
	float scale = std::min(
		2.0f * aspect / (Game::ArenaMax.x - Game::ArenaMin.x + 2.0f * Game::PlayerRadius),
		2.0f / (Game::ArenaMax.y - Game::ArenaMin.y + 2.0f * Game::PlayerRadius)
	);
	glm::vec2 offset = -0.5f * (Game::ArenaMax + Game::ArenaMin);

	glm::mat4 world_to_clip = glm::mat4(
		scale / aspect, 0.0f, 0.0f, offset.x,
		0.0f, scale, 0.0f, offset.y,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	auto world_to_pixel = [&](glm::vec2 const &world) {
		glm::vec4 clip = world_to_clip * glm::vec4(world, 0.0f, 1.0f);
		return glm::vec2(
			(clip.x / clip.w * 0.5f + 0.5f) * float(drawable_size.x),
			(0.5f - clip.y / clip.w * 0.5f) * float(drawable_size.y)
		);
	};

	//Main game graphics use filled triangles 
	shape_renderer.draw_rectangle(
		world_to_clip,
		Game::ArenaMin,
		Game::ArenaMax,
		glm::u8vec4(0x32, 0xd1, 0xc6, 0xff)
	);
	shape_renderer.draw_rectangle(
		world_to_clip,
		Game::ArenaMin + glm::vec2(0.02f),
		Game::ArenaMax - glm::vec2(0.02f),
		glm::u8vec4(0x17, 0x24, 0x35, 0xff)
	);

	//Draw the explosion behind the losing player.
	if (game.round_loser != nullptr) {
		shape_renderer.draw_circle(
			world_to_clip,
			game.round_loser->position,
			0.18f,
			glm::u8vec4(0xff, 0x45, 0x20, 0xff)
		);
		shape_renderer.draw_circle(
			world_to_clip,
			game.round_loser->position,
			0.11f,
			glm::u8vec4(0xff, 0xd1, 0x36, 0xff)
		);
	}

	for (auto const &player : game.players) {
		glm::u8vec4 color = glm::u8vec4(player.color.x * 255, player.color.y * 255, player.color.z * 255, 0xff);
		shape_renderer.draw_circle(
			world_to_clip,
			player.position + glm::vec2(0.012f, -0.012f),
			Game::PlayerRadius,
			glm::u8vec4(0x08, 0x0c, 0x12, 0xff)
		);
		shape_renderer.draw_circle(world_to_clip, player.position, Game::PlayerRadius, color);

		if (&player == &game.players.front()) {
			shape_renderer.draw_circle(
				world_to_clip,
				player.position,
				0.016f,
				glm::u8vec4(0xff, 0xff, 0xff, 0xff)
			);
		}
		if (&player == game.bomb_holder) {
			glm::vec2 bomb_position = player.position + glm::vec2(0.0f, Game::PlayerRadius + 0.035f);
			shape_renderer.draw_circle(
				world_to_clip,
				bomb_position,
				0.035f,
				glm::u8vec4(0x08, 0x0c, 0x12, 0xff)
			);
			shape_renderer.draw_circle(
				world_to_clip,
				bomb_position,
				0.015f,
				glm::u8vec4(0xff, 0x6b, 0x35, 0xff)
			);
			shape_renderer.draw_circle(
				world_to_clip,
				bomb_position + glm::vec2(0.022f, 0.025f),
				0.010f,
				glm::u8vec4(0xff, 0xd1, 0x36, 0xff)
			);
		}
	}

	//Player name textures are created once and reused.
	for (auto const &player : game.players) {
		auto found = player_name_textures.find(player.name);
		if (found == player_name_textures.end()) {
			TextTexture texture = text_renderer.make_text(player.name);
			found = player_name_textures.emplace(player.name, texture).first;
		}
		float name_scale = 0.38f;
		glm::vec2 name_position = world_to_pixel(
			player.position + glm::vec2(0.0f, -Game::PlayerRadius - 0.035f)
		);
		name_position.x -= 0.5f * float(found->second.width) * name_scale;
		text_renderer.draw_text(
			found->second,
			name_position,
			name_scale,
			glm::vec3(0.9f, 0.94f, 1.0f),
			drawable_size
		);
	}
	if (game.round_loser != nullptr) {
		float boom_scale = 0.55f;
		glm::vec2 boom_position = world_to_pixel(
			game.round_loser->position + glm::vec2(0.0f, 0.20f)
		);
		boom_position.x -= 0.5f * float(boom_text.width) * boom_scale;
		text_renderer.draw_text(
			boom_text,
			boom_position,
			boom_scale,
			glm::vec3(1.0f, 0.85f, 0.2f),
			drawable_size
		);
	}

	std::string status = "Waiting for another player";
	glm::vec3 status_color(1.0f, 0.85f, 0.2f);
	if (game.round_state == Game::RoundState::Playing) {
		status = "Playing  Bomb: " + std::to_string(int(std::ceil(game.bomb_timer)));
		status_color = glm::vec3(1.0f);
	} else if (game.round_state == Game::RoundState::RoundOver) {
		status = "Round over";
		if (game.round_loser != nullptr) status = game.round_loser->name + " lost!";
		status_color = glm::vec3(1.0f, 0.25f, 0.15f);
	}
	if (status != current_status) {
		text_renderer.destroy_text(status_text);
		status_text = text_renderer.make_text(status);
		current_status = status;
	}
	text_renderer.draw_text(
		status_text,
		glm::vec2(40.0f, 24.0f),
		0.7f,
		status_color,
		drawable_size
	);
	GL_ERRORS();
}
