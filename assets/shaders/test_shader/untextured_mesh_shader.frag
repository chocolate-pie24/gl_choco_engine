#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

in vec3 view_normal;
in vec3 view_position;

out vec4 out_color;

uniform float g_directional_light_intensity;
uniform vec3 g_directional_light_color;
uniform vec3 g_directional_light_direction;

uniform float g_ambient_light_intensity;
uniform vec3 g_ambient_light_color;

uniform Material g_material;

void main() {
    vec3 normal = normalize(view_normal);
    vec3 directional_light_direction = normalize(g_directional_light_direction);

    // 内積によってnormalをdirectional_light_directionに射影した長さを計算
    // 値が大きいほど法線と光源方向が平行であり、光が強く当たる
    float diffuse_factor = max(dot(normal, directional_light_direction), 0.0);

    // g_directional_material.diffuse: RGB各成分に対する拡散反射係数（0.0～1.0）
    // g_directional_light_color: 光源のRGB各成分の相対強度（0.0～1.0）
    // g_directional_light_intensity: 光源の強度
    // diffuse_factor: 面が光源に対してどれだけ正面を向いているか
    vec3 diffuse_color = g_material.diffuse * g_directional_light_color * g_directional_light_intensity * diffuse_factor;

    vec3 ambient_color = g_material.ambient * g_ambient_light_color * g_ambient_light_intensity;

    vec3 specular_color = vec3(0.0);
    if(diffuse_factor > 0.0 && g_material.shininess > 0.0) {
        vec3 view_direction = normalize(-view_position);    // view座標系なのでカメラが原点にあるため、-1をかけるとview_directionになる
        vec3 reflect_direction = reflect(-directional_light_direction, normal); // reflect(I, N)は、入射方向Iを法線Nに対して反射した方向を返す

        // まず内積によって、反射方向と視線方向の一致度を求め、その一致度をshininessで累乗することで、ハイライトの広がりを制御する
        float specular_factor = pow(max(dot(reflect_direction, view_direction), 0.0), g_material.shininess);

        specular_color = g_material.specular * g_directional_light_color * g_directional_light_intensity * specular_factor;
    }

    vec3 color = diffuse_color + ambient_color + specular_color;

    out_color = vec4(color, 1.0);
}
