#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

in vec3 view_normal;

out vec4 out_color;

uniform float g_light_intensity;
uniform vec3 g_light_color;
uniform vec3 g_light_direction;
uniform Material g_material;

void main() {
    vec3 normal = normalize(view_normal);
    vec3 light_direction = normalize(g_light_direction);

    // 内積によってnormalをlight_directionに射影した長さを計算
    // 値が大きいほど法線と光源方向が平行であり、光が強く当たる
    float diffuse_factor = max(dot(normal, light_direction), 0.0);

    vec3 diffuse_color = g_material.diffuse * g_light_color * g_light_intensity * diffuse_factor;

    out_color = vec4(diffuse_color, 1.0);
}
