#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>

#include <cstddef>

struct ShapeRenderer {
	ShapeRenderer();
	~ShapeRenderer();

	void draw_rectangle(
		glm::mat4 const &world_to_clip,
		glm::vec2 const &min,
		glm::vec2 const &max,
		glm::u8vec4 const &color
	);
	void draw_circle(
		glm::mat4 const &world_to_clip,
		glm::vec2 const &center,
		float radius,
		glm::u8vec4 const &color
	);

	struct Vertex {
		Vertex(glm::vec3 const &position_, glm::u8vec4 const &color_)
			: position(position_), color(color_) {
		}

		glm::vec3 position;
		glm::u8vec4 color;
	};

	GLuint vao = 0;
	GLuint vbo = 0;

	void draw(glm::mat4 const &world_to_clip, Vertex const *vertices, size_t count);
};
