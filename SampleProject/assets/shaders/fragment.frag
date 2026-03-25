#version 460 core
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_ARB_bindless_texture : require

layout(location = 0) out vec4 FragColor;

in vec2 uv;
in flat uint animState;
in flat uint64_t v_textureHandle;

void main() {
	#if defined(GL_ARB_bindless_texture)
        if (v_textureHandle != 0ul) {
            sampler2DArray s = sampler2DArray(v_textureHandle);
            FragColor = texture(s, vec3(uv, animState));
        } else {
            FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta for null
        }
    #else
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Yellow fallback for Intel
    #endif
}