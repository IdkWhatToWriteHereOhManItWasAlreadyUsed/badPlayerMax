#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <libs/robin_hood.h>

class Shader
{
private:
    GLuint Program;
    robin_hood::unordered_flat_map<std::string, GLint> uniformCache;

public:
    ~Shader() {
        if (Program != 0) {
            glDeleteProgram(Program);
        }
    }

    Shader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath = "");
    //Shader(const std::string& vertexPath, const std::string& fragmentPath);
    // Shader(const std::string vertexPath, const std::string fragmentPath);

    void Use() const {  
        glUseProgram(Program);
    }

    GLint GetUniformLocation(const std::string& name) {
        auto it = uniformCache.find(name);
        if (it != uniformCache.end()) {
            return it->second;
        }

        GLint location = glGetUniformLocation(Program, name.c_str());
        uniformCache[name] = location;
        return location;
    }

    void SetUniform1i(const std::string& name, int value) {
        GLint loc = GetUniformLocation(name);
        if (loc != -1) {
            glUniform1i(loc, value);
        }
    }

    void SetUniform1f(const std::string& name, float value) {
        GLint loc = GetUniformLocation(name);
        if (loc != -1) {
            glUniform1f(loc, value);
        }
    }

    void SetUniform3f(const std::string& name, float x, float y, float z) {
        GLint loc = GetUniformLocation(name);
        if (loc != -1) {
            glUniform3f(loc, x, y, z);
        }
    }

    void SetUniform3f(const std::string& name, const glm::vec3& value) {
        SetUniform3f(name, value.x, value.y, value.z);
    }

    void SetUniform4f(const std::string& name, float x, float y, float z, float w) {
        GLint loc = GetUniformLocation(name);
        if (loc != -1) {
            glUniform4f(loc, x, y, z, w);
        }
    }

    void SetUniform4f(const std::string& name, const glm::vec4& value) {
        SetUniform4f(name, value.x, value.y, value.z, value.w);
    }

    void SetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) {
        GLint loc = GetUniformLocation(name);
        if (loc != -1) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
        }
    }

    void SetUniformTexture(const std::string& name, int textureUnit) {
        SetUniform1i(name, textureUnit);
    }

    GLuint GetID() const { return Program; }
    bool IsValid() const { return Program != 0; }

private:
    void CheckCompileErrors(GLuint shader, const std::string& type);
    void CheckLinkErrors(GLuint program);
    static std::string ReadFileFast(const std::string& filePath);
};

#endif