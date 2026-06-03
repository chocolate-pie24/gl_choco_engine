#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

uniform mat4 g_model_matrix;
uniform mat4 g_view_matrix;
uniform mat4 g_projection_matrix;

out vec4 frag_color;

void main() {
    gl_Position = g_projection_matrix * g_view_matrix * g_model_matrix * vec4(in_position, 1.0);
    frag_color = in_color;
    gl_PointSize = 10.0;    // TODO: 仮(将来的にはCPU側から設定可能にする)
}
