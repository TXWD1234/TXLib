#version 460 core
#extension GL_ARB_gpu_shader_int64 : require

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

layout(location = 0) in vec2 squarePos;
layout(location = 1) in vec2 squareUV;
layout(location = 2) in uint in_animState;
layout(location = 3) in vec2 instancePos;
layout(location = 4) in float scale;
layout(location = 5) in uvec2 in_textureHandleRaw;

out vec2 uv;
out flat uint animState;
out flat uint64_t v_textureHandle;

void main() {
	uv = squareUV;
	animState = in_animState;
	v_textureHandle = packUint2x32(in_textureHandleRaw);
    gl_Position = vec4(scale * squarePos + instancePos, 0.0, 1.0);
}