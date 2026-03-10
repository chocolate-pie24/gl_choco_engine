#version 330 core

layout(location = 0) in vec3 in_position;

uniform mat4 g_model_matrix;
uniform mat4 g_view_matrix;
uniform mat4 g_projection_matrix;

void main() {
	gl_Position = g_projection_matrix * g_view_matrix * g_model_matrix * vec4(in_position, 1.0);
}
