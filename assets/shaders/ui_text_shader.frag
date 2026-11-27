#version 330 core

in vec2 tex_coords;

out vec4 color;

uniform usampler2D tex0;
uniform vec4 g_text_color;

void main() {
    ivec2 pixel_coords = ivec2(floor(tex_coords));
    ivec2 packed_coords = ivec2(pixel_coords.x / 8, pixel_coords.y);

    uint packed_byte = texelFetch(tex0, packed_coords, 0).r;

    int bit_index = 7 - (pixel_coords.x % 8);   // 左端bitは7bit目なので、0bitから始まるように変換
    uint pixel_bit = (packed_byte >> bit_index) & 1u;

    color = vec4(g_text_color.rgb, g_text_color.a * float(pixel_bit));
}