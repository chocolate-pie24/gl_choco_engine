#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

in vec4 frag_color;

out vec4 out_color;

uniform Material g_material;

void main() {
    out_color = frag_color;
}
