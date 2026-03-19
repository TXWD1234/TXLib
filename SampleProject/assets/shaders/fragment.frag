#version 460 core

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2DArray u_tex;

in vec2 uv;
flat in uint animState;

void main() {
    vec4 texColor = texture(u_tex, vec3(uv, animState));
	//texColor.x *= 0.5;
	//texColor.y *= 1.5;
	//texColor.z *= 1.5;
	FragColor = texColor;
}