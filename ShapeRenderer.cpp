#include "ShapeRenderer.hpp"

#include "ColorProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <cmath>
#include <vector>

ShapeRenderer::ShapeRenderer() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(
		color_program->Position_vec4,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		reinterpret_cast<void *>(offsetof(Vertex, position))
	);
	glEnableVertexAttribArray(color_program->Position_vec4);
	glVertexAttribPointer(
		color_program->Color_vec4,
		4,
		GL_UNSIGNED_BYTE,
		GL_TRUE,
		sizeof(Vertex),
		reinterpret_cast<void *>(offsetof(Vertex, color))
	);
	glEnableVertexAttribArray(color_program->Color_vec4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

ShapeRenderer::~ShapeRenderer() {
	if (vbo != 0) glDeleteBuffers(1, &vbo);
	if (vao != 0) glDeleteVertexArrays(1, &vao);
}

void ShapeRenderer::draw_rectangle(
	glm::mat4 const &world_to_clip,
	glm::vec2 const &min,
	glm::vec2 const &max,
	glm::u8vec4 const &color
) {
	std::array< Vertex, 6 > vertices = {
		Vertex(glm::vec3(min.x, min.y, 0.0f), color),
		Vertex(glm::vec3(max.x, min.y, 0.0f), color),
		Vertex(glm::vec3(max.x, max.y, 0.0f), color),
		Vertex(glm::vec3(min.x, min.y, 0.0f), color),
		Vertex(glm::vec3(max.x, max.y, 0.0f), color),
		Vertex(glm::vec3(min.x, max.y, 0.0f), color)
	};
	draw(world_to_clip, vertices.data(), vertices.size());
}

void ShapeRenderer::draw_circle(
	glm::mat4 const &world_to_clip,
	glm::vec2 const &center,
	float radius,
	glm::u8vec4 const &color
) {
	constexpr uint32_t Segments = 24;
	std::vector< Vertex > vertices;
	vertices.reserve(Segments * 3);
	for (uint32_t i = 0; i < Segments; ++i) {
		float angle1 = float(i) / float(Segments) * 2.0f * float(M_PI);
		float angle2 = float(i + 1) / float(Segments) * 2.0f * float(M_PI);
		vertices.emplace_back(glm::vec3(center, 0.0f), color);
		vertices.emplace_back(glm::vec3(center + radius * glm::vec2(std::cos(angle1), std::sin(angle1)), 0.0f), color);
		vertices.emplace_back(glm::vec3(center + radius * glm::vec2(std::cos(angle2), std::sin(angle2)), 0.0f), color);
	}
	draw(world_to_clip, vertices.data(), vertices.size());
}

void ShapeRenderer::draw(glm::mat4 const &world_to_clip, Vertex const *vertices, size_t count) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), vertices, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(color_program->program);
	glUniformMatrix4fv(color_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(count));
	glBindVertexArray(0);
	glUseProgram(0);
}
