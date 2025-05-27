#type vertex
#version 430 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;

out vec3 v_normal;
out vec3 v_position;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_position = world_pos.xyz;
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;
    gl_Position = u_view * world_pos;
}

#type fragment
#version 430 core

in vec3 v_normal;
in vec3 v_position;

uniform vec3 u_color;

out vec4 frag_color;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(vec3(0.25, 1.0, 0.5));
    float diffuse = max(dot(N, L), 0.1);
    frag_color = vec4(u_color * diffuse, 1.0);
}
