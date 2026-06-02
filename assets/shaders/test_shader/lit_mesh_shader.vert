#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

uniform mat4 g_model_matrix;
uniform mat4 g_view_matrix;
uniform mat4 g_projection_matrix;

out vec4 frag_color;

void main() {
    gl_Position = g_projection_matrix * g_view_matrix * g_model_matrix * vec4(in_position, 1.0);
    frag_color = vec4(0.0, 1.0, 0.0, 1.0);
}
