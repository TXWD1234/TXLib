// This file is a backend wrapper of TXLib for glad
// Copyright@GLAD

#pragma once
#include <cstddef>
#include <cstdint>

namespace gl {

// Basic OpenGL types mapped to standard C++ types to avoid including glad.h
using enum_t = unsigned int; // GLenum
using uint_t = unsigned int; // GLuint
using int_t = int; // GLint
using sizei_t = int; // GLsizei
using boolean_t = unsigned char; // GLboolean
using bitfield_t = unsigned int; // GLbitfield
using char_t = char; // GLchar
using sizeiptr_t = ptrdiff_t; // GLsizeiptr
using intptr_t = ptrdiff_t; // GLintptr
using sync_t = void*; // GLsync

// --- Initialization ---
bool init(void* loadProc);

// --- General State ---
void clear(bitfield_t mask);
void clearColor(float red, float green, float blue, float alpha);
void enable(enum_t cap);
void disable(enum_t cap);
void blendFunc(enum_t sfactor, enum_t dfactor);

// --- Buffers (DSA) ---
void createBuffers(sizei_t n, uint_t* buffers);
void deleteBuffers(sizei_t n, const uint_t* buffers);
void namedBufferStorage(uint_t buffer, sizeiptr_t size, const void* data, bitfield_t flags);
void* mapNamedBufferRange(uint_t buffer, intptr_t offset, sizeiptr_t length, bitfield_t access);
boolean_t unmapNamedBuffer(uint_t buffer);

// --- Vertex Arrays (DSA) ---
void createVertexArrays(sizei_t n, uint_t* arrays);
void deleteVertexArrays(sizei_t n, const uint_t* arrays);
void vertexArrayAttribFormat(uint_t vaobj, uint_t attribindex, int_t size, enum_t type, boolean_t normalized, uint_t relativeoffset);
void vertexArrayAttribIFormat(uint_t vaobj, uint_t attribindex, int_t size, enum_t type, uint_t relativeoffset);
void enableVertexArrayAttrib(uint_t vaobj, uint_t index);
void vertexArrayAttribBinding(uint_t vaobj, uint_t attribindex, uint_t bindingindex);
void vertexArrayVertexBuffer(uint_t vaobj, uint_t bindingindex, uint_t buffer, intptr_t offset, sizei_t stride);
void vertexArrayBindingDivisor(uint_t vaobj, uint_t bindingindex, uint_t divisor);

// --- Textures (DSA) ---
void createTextures(enum_t target, sizei_t n, uint_t* textures);
void deleteTextures(sizei_t n, const uint_t* textures);
void textureStorage3D(uint_t texture, sizei_t levels, enum_t internalformat, sizei_t width, sizei_t height, sizei_t depth);
void textureSubImage3D(uint_t texture, int_t level, int_t xoffset, int_t yoffset, int_t zoffset, sizei_t width, sizei_t height, sizei_t depth, enum_t format, enum_t type, const void* pixels);
void textureParameteri(uint_t texture, enum_t pname, int_t param);

// --- Shaders & Programs ---
uint_t createShader(enum_t type);
void deleteShader(uint_t shader);
void shaderSource(uint_t shader, sizei_t count, const char_t* const* string, const int_t* length);
void compileShader(uint_t shader);
void getShaderiv(uint_t shader, enum_t pname, int_t* params);
void getShaderInfoLog(uint_t shader, sizei_t bufSize, sizei_t* length, char_t* infoLog);

uint_t createProgram();
void deleteProgram(uint_t program);
void attachShader(uint_t program, uint_t shader);
void linkProgram(uint_t program);
void getProgramiv(uint_t program, enum_t pname, int_t* params);
void getProgramInfoLog(uint_t program, sizei_t bufSize, sizei_t* length, char_t* infoLog);
void programParameteri(uint_t program, enum_t pname, int_t value);

// --- Program Pipelines ---
void createProgramPipelines(sizei_t n, uint_t* pipelines);
void deleteProgramPipelines(sizei_t n, const uint_t* pipelines);
void useProgramStages(uint_t pipeline, bitfield_t stages, uint_t program);
void validateProgramPipeline(uint_t pipeline);
void getProgramPipelineiv(uint_t pipeline, enum_t pname, int_t* params);
void getProgramPipelineInfoLog(uint_t pipeline, sizei_t bufSize, sizei_t* length, char_t* infoLog);

// --- Draw & Bind ---
void drawArrays(enum_t mode, int_t first, sizei_t count);
void drawArraysInstanced(enum_t mode, int_t first, sizei_t count, sizei_t instancecount);
void bindVertexArray(uint_t array);
void useProgram(uint_t program);
void bindProgramPipeline(uint_t pipeline);
void bindTextureUnit(uint_t unit, uint_t texture);
sync_t fenceSync(enum_t condition, bitfield_t flags);
void deleteSync(sync_t sync);
enum_t clientWaitSync(sync_t sync, bitfield_t flags, uint64_t timeout);

} // namespace gl
