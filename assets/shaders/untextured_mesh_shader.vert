#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

uniform mat4 g_model_matrix;
uniform mat4 g_view_matrix;
uniform mat4 g_projection_matrix;

out vec3 view_normal;   // ビュー座標系に変換された法線情報
out vec3 view_position;

void main() {
    vec4 position = g_view_matrix * g_model_matrix * vec4(in_position, 1.0);

    view_position = position.xyz;
    gl_Position = g_projection_matrix * position;

    // 当面はx, y, z方向への拡大/縮小倍率が均等なmodel_matrixであることを前提とする
    // 将来均等ではないmodel_matrixが出てきた際に法線変換行列の導入を検討する
    view_normal = (g_view_matrix * g_model_matrix * vec4(in_normal, 0.0)).xyz;    // 平行移動は適用しないため第四成分は0
}
