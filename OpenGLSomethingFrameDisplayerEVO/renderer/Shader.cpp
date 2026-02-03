#include "Shader.h"
#include <fstream>
#include <iostream>
#include <sstream>

std::string Shader::ReadFileFast(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "ERROR::SHADER: Failed to open file: " << filePath << std::endl;
        return "";
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(static_cast<size_t>(size));
    file.read(&buffer[0], size);

    return buffer;
}

// Конструктор с геометрическим шейдером
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath)
    : Program(0)
{
    uniformCache.reserve(32);

    std::string vertexCode = ReadFileFast(vertexPath);
    std::string fragmentCode = ReadFileFast(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "ERROR::SHADER: Failed to load shader files" << std::endl;
        return;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    GLuint geometry = 0;
    if (!geometryPath.empty()) {
        std::string geometryCode = ReadFileFast(geometryPath);
        if (!geometryCode.empty()) {
            const char* gShaderCode = geometryCode.c_str();

            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, nullptr);
            glCompileShader(geometry);
            CheckCompileErrors(geometry, "GEOMETRY");
        }
    }

    Program = glCreateProgram();
    glAttachShader(Program, vertex);
    glAttachShader(Program, fragment);
    if (geometry != 0) {
        glAttachShader(Program, geometry);
    }

    glLinkProgram(Program);
    CheckLinkErrors(Program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geometry != 0) {
        glDeleteShader(geometry);
    }
}
/*
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
    : Shader(vertexPath, fragmentPath, "")
{
}*/

/*

Shader::Shader(const std::string vertexPath, const std::string fragmentPath)
    : Shader(vertexPath, fragmentPath, "")
{
}*/


void Shader::CheckCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
            << infoLog << "\n" << std::endl;
    }
}

void Shader::CheckLinkErrors(GLuint program) {
    GLint success;
    GLchar infoLog[1024];

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n"
            << infoLog << "\n" << std::endl;
    }
}