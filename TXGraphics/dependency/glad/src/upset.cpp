// This file is a backend wrapper of TXLib for glad
// Copyright@GLAD

#include "glad/glad.h"
#include "tx/upset.hpp"

namespace gl {

// --- Initialization ---
bool init(void* loadProc) { return gladLoadGLLoader((GLADloadproc)loadProc) != 0; }

// --- General State ---
void clear(bitfield_t mask) { glClear(mask); }
void clearColor(float red, float green, float blue, float alpha) { glClearColor(red, green, blue, alpha); }
void enable(enum_t cap) { glEnable(cap); }
void disable(enum_t cap) { glDisable(cap); }
void blendFunc(enum_t sfactor, enum_t dfactor) { glBlendFunc(sfactor, dfactor); }

// --- Buffers (DSA) ---
void createBuffers(sizei_t n, uint_t* buffers) { glCreateBuffers(n, buffers); }
void deleteBuffers(sizei_t n, const uint_t* buffers) { glDeleteBuffers(n, buffers); }
void namedBufferStorage(uint_t buffer, sizeiptr_t size, const void* data, bitfield_t flags) { glNamedBufferStorage(buffer, size, data, flags); }
void* mapNamedBufferRange(uint_t buffer, intptr_t offset, sizeiptr_t length, bitfield_t access) { return glMapNamedBufferRange(buffer, offset, length, access); }
boolean_t unmapNamedBuffer(uint_t buffer) { return glUnmapNamedBuffer(buffer); }

// --- Vertex Arrays (DSA) ---
void createVertexArrays(sizei_t n, uint_t* arrays) { glCreateVertexArrays(n, arrays); }
void deleteVertexArrays(sizei_t n, const uint_t* arrays) { glDeleteVertexArrays(n, arrays); }
void vertexArrayAttribFormat(uint_t vaobj, uint_t attribindex, int_t size, enum_t type, boolean_t normalized, uint_t relativeoffset) { glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset); }
void vertexArrayAttribIFormat(uint_t vaobj, uint_t attribindex, int_t size, enum_t type, uint_t relativeoffset) { glVertexArrayAttribIFormat(vaobj, attribindex, size, type, relativeoffset); }
void enableVertexArrayAttrib(uint_t vaobj, uint_t index) { glEnableVertexArrayAttrib(vaobj, index); }
void vertexArrayAttribBinding(uint_t vaobj, uint_t attribindex, uint_t bindingindex) { glVertexArrayAttribBinding(vaobj, attribindex, bindingindex); }
void vertexArrayVertexBuffer(uint_t vaobj, uint_t bindingindex, uint_t buffer, intptr_t offset, sizei_t stride) { glVertexArrayVertexBuffer(vaobj, bindingindex, buffer, offset, stride); }
void vertexArrayBindingDivisor(uint_t vaobj, uint_t bindingindex, uint_t divisor) { glVertexArrayBindingDivisor(vaobj, bindingindex, divisor); }

// --- Textures (DSA) ---
void createTextures(enum_t target, sizei_t n, uint_t* textures) { glCreateTextures(target, n, textures); }
void deleteTextures(sizei_t n, const uint_t* textures) { glDeleteTextures(n, textures); }
void textureStorage3D(uint_t texture, sizei_t levels, enum_t internalformat, sizei_t width, sizei_t height, sizei_t depth) { glTextureStorage3D(texture, levels, internalformat, width, height, depth); }
void textureSubImage3D(uint_t texture, int_t level, int_t xoffset, int_t yoffset, int_t zoffset, sizei_t width, sizei_t height, sizei_t depth, enum_t format, enum_t type, const void* pixels) { glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels); }
void textureParameteri(uint_t texture, enum_t pname, int_t param) { glTextureParameteri(texture, pname, param); }
void copyImageSubData(uint_t srcName, enum_t srcTarget, int_t srcLevel, int_t srcX, int_t srcY, int_t srcZ, uint_t dstName, enum_t dstTarget, int_t dstLevel, int_t dstX, int_t dstY, int_t dstZ, sizei_t srcWidth, sizei_t srcHeight, sizei_t srcDepth) { glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth); }

// --- Shaders & Programs ---
uint_t createShader(enum_t type) { return glCreateShader(type); }
void deleteShader(uint_t shader) { glDeleteShader(shader); }
void shaderSource(uint_t shader, sizei_t count, const char_t* const* string, const int_t* length) { glShaderSource(shader, count, string, length); }
void compileShader(uint_t shader) { glCompileShader(shader); }
void getShaderiv(uint_t shader, enum_t pname, int_t* params) { glGetShaderiv(shader, pname, params); }
void getShaderInfoLog(uint_t shader, sizei_t bufSize, sizei_t* length, char_t* infoLog) { glGetShaderInfoLog(shader, bufSize, length, infoLog); }

uint_t createProgram() { return glCreateProgram(); }
void deleteProgram(uint_t program) { glDeleteProgram(program); }
void attachShader(uint_t program, uint_t shader) { glAttachShader(program, shader); }
void linkProgram(uint_t program) { glLinkProgram(program); }
void getProgramiv(uint_t program, enum_t pname, int_t* params) { glGetProgramiv(program, pname, params); }
void getProgramInfoLog(uint_t program, sizei_t bufSize, sizei_t* length, char_t* infoLog) { glGetProgramInfoLog(program, bufSize, length, infoLog); }
void programParameteri(uint_t program, enum_t pname, int_t value) { glProgramParameteri(program, pname, value); }

// --- Program Pipelines ---
void createProgramPipelines(sizei_t n, uint_t* pipelines) { glCreateProgramPipelines(n, pipelines); }
void deleteProgramPipelines(sizei_t n, const uint_t* pipelines) { glDeleteProgramPipelines(n, pipelines); }
void useProgramStages(uint_t pipeline, bitfield_t stages, uint_t program) { glUseProgramStages(pipeline, stages, program); }
void validateProgramPipeline(uint_t pipeline) { glValidateProgramPipeline(pipeline); }
void getProgramPipelineiv(uint_t pipeline, enum_t pname, int_t* params) { glGetProgramPipelineiv(pipeline, pname, params); }
void getProgramPipelineInfoLog(uint_t pipeline, sizei_t bufSize, sizei_t* length, char_t* infoLog) { glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog); }

// --- Draw & Bind ---
void drawArrays(enum_t mode, int_t first, sizei_t count) { glDrawArrays(mode, first, count); }
void drawArraysInstanced(enum_t mode, int_t first, sizei_t count, sizei_t instancecount) { glDrawArraysInstanced(mode, first, count, instancecount); }
void bindVertexArray(uint_t array) { glBindVertexArray(array); }
void useProgram(uint_t program) { glUseProgram(program); }
void bindProgramPipeline(uint_t pipeline) { glBindProgramPipeline(pipeline); }
void bindTextureUnit(uint_t unit, uint_t texture) { glBindTextureUnit(unit, texture); }
sync_t fenceSync(enum_t condition, bitfield_t flags) { return glFenceSync(condition, flags); }
void deleteSync(sync_t sync) { glDeleteSync((GLsync)sync); }
enum_t clientWaitSync(sync_t sync, bitfield_t flags, uint64_t timeout) { return glClientWaitSync((GLsync)sync, flags, timeout); }

} // namespace gl
