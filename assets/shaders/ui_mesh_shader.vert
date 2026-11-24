#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;

uniform mat4 g_model_matrix;
uniform mat4 g_view_matrix;
uniform mat4 g_projection_matrix;

out vec2 tex_coords;

void main() {
	tex_coords = vec2(in_tex_coord.x, in_tex_coord.y);

	// TODO: uiカメラ実装後、z座標を0にする
	gl_Position = g_projection_matrix * g_view_matrix * g_model_matrix * vec4(in_position, -1.0, 1.0);
}
