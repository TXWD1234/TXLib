#version 460 core

layout(location = 0) in vec2 squarePos;
layout(location = 1) in vec2 squareUV;
layout(location = 2) in uint in_animState;
layout(location = 3) in vec2 instancePos;

out vec2 uv;
flat out uint animState;

void main() {
	uv = squareUV;
	animState = in_animState;
    gl_Position = vec4(squarePos + instancePos, 0.0, 1.0);
}