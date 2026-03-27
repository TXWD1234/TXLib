#version 460 core
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_ARB_bindless_texture : require

layout(location = 0) out vec4 FragColor;

in vec2 uv;
in flat uint64_t textureHandle;
in flat float textureIndex;

void main() {
	#if defined(GL_ARB_bindless_texture)
        if (textureHandle != 0ul) {
            sampler2DArray s = sampler2DArray(textureHandle);
            FragColor = texture(s, vec3(uv, textureIndex));
        } else {
            FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta for null
        }
    #else
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Yellow fallback for Intel
    #endif
}