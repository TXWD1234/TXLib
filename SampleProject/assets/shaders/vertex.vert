#version 460 core
#extension GL_ARB_gpu_shader_int64 : require

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

layout(location = 0) in vec2 meshPosition;
layout(location = 1) in vec2 meshUV;
layout(location = 2) in vec2 instancePosition;
layout(location = 3) in uvec2 instanceTextureHandle;
layout(location = 4) in float instanceTextureIndex;

out vec2 uv;
out flat uint64_t textureHandle;
out flat float textureIndex;

void main() {
	uv = meshUV;
	textureHandle = packUint2x32(instanceTextureHandle);
	textureIndex = instanceTextureIndex;
    gl_Position = vec4(meshPosition + instancePosition, 0.0, 1.0);
}