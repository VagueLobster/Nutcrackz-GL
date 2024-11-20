#include "nzpch.hpp"

#include <misc/cpp/imgui_stdlib.cpp>

#ifdef glGetError
#undef glGetError
#endif

#ifdef glActiveTexture
#undef glActiveTexture
#endif

#ifdef glAttachShader
#undef glAttachShader
#endif

#ifdef glBindBuffer
#undef glBindBuffer
#endif

#ifdef glBindSampler
#undef glBindSampler
#endif

#ifdef glBindTexture
#undef glBindTexture
#endif

#ifdef glBindVertexArray
#undef glBindVertexArray
#endif

#ifdef glBlendEquation
#undef glBlendEquation
#endif

#ifdef glBlendEquationSeparate
#undef glBlendEquationSeparate
#endif

#ifdef glBlendFuncSeparate
#undef glBlendFuncSeparate
#endif

#ifdef glBufferData
#undef glBufferData
#endif

#ifdef glBufferSubData
#undef glBufferSubData
#endif

#ifdef glClear
#undef glClear
#endif

#ifdef glClearColor
#undef glClearColor
#endif

#ifdef glCompileShader
#undef glCompileShader
#endif

#ifdef glCreateProgram
#undef glCreateProgram
#endif

#ifdef glCreateShader
#undef glCreateShader
#endif

#ifdef glDeleteBuffers
#undef glDeleteBuffers
#endif

#ifdef glDeleteProgram
#undef glDeleteProgram
#endif

#ifdef glDeleteShader
#undef glDeleteShader
#endif

#ifdef glDeleteTextures
#undef glDeleteTextures
#endif

#ifdef glDeleteVertexArrays
#undef glDeleteVertexArrays
#endif

#ifdef glDetachShader
#undef glDetachShader
#endif

#ifdef glDisable
#undef glDisable
#endif

#ifdef glDrawElements
#undef glDrawElements
#endif

#ifdef glDrawElementsBaseVertex
#undef glDrawElementsBaseVertex
#endif

#ifdef glEnable
#undef glEnable
#endif

#ifdef glEnableVertexAttribArray
#undef glEnableVertexAttribArray
#endif

#ifdef glGenBuffers
#undef glGenBuffers
#endif

#ifdef glGenTextures
#undef glGenTextures
#endif

#ifdef glGenVertexArrays
#undef glGenVertexArrays
#endif

#ifdef glGetAttribLocation
#undef glGetAttribLocation
#endif

#ifdef glGetIntegerv
#undef glGetIntegerv
#endif

#ifdef glGetProgramInfoLog
#undef glGetProgramInfoLog
#endif

#ifdef glGetProgramiv
#undef glGetProgramiv
#endif

#ifdef glGetShaderInfoLog
#undef glGetShaderInfoLog
#endif

#ifdef glGetShaderiv
#undef glGetShaderiv
#endif

#ifdef glGetString
#undef glGetString
#endif

#ifdef glGetStringi
#undef glGetStringi
#endif

#ifdef glGetUniformLocation
#undef glGetUniformLocation
#endif

#ifdef glIsEnabled
#undef glIsEnabled
#endif

#ifdef glLinkProgram
#undef glLinkProgram
#endif

#ifdef glPixelStorei
#undef glPixelStorei
#endif

#ifdef glPolygonMode
#undef glPolygonMode
#endif

#ifdef glReadPixels
#undef glReadPixels
#endif

#ifdef glScissor
#undef glScissor
#endif

#ifdef glShaderSource
#undef glShaderSource
#endif

#ifdef glTexImage2D
#undef glTexImage2D
#endif

#ifdef glTexParameteri
#undef glTexParameteri
#endif

#ifdef glUniform1i
#undef glUniform1i
#endif

#ifdef glUniformMatrix4fv
#undef glUniformMatrix4fv
#endif

#ifdef glUseProgram
#undef glUseProgram
#endif

#ifdef glVertexAttribPointer
#undef glVertexAttribPointer
#endif

#ifdef glViewport
#undef glViewport
#endif

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>