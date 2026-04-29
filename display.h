#ifndef DISPLAY_H
#define DISPLAY_H

#include <epoxy/gl.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    // Fullscreen quad — position generated from gl_VertexID, no VBO needed.
    // UV v-axis is flipped: OpenGL stores row 0 at the bottom of a texture,
    // but our framebuffer stores row 0 at the top (standard image convention).
    const char* vert_src = R"glsl(
#version 330 core
out vec2 uv;
void main() {
    vec2 pos[4] = vec2[](vec2(-1,-1), vec2(1,-1), vec2(-1,1), vec2(1,1));
    vec2 uvs[4] = vec2[](vec2(0,1),   vec2(1,1),  vec2(0,0),  vec2(1,0));
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    uv = uvs[gl_VertexID];
}
)glsl";

    // Gamma correction lives here (linear → gamma 2.0 via sqrt).
    // The CPU framebuffer stays in linear light all the way to the GPU.
    const char* frag_src = R"glsl(
#version 330 core
in  vec2 uv;
out vec4 frag_colour;
uniform sampler2D frame_tex;
void main() {
    vec3 linear = texture(frame_tex, uv).rgb;
    frag_colour = vec4(sqrt(clamp(linear, 0.0, 1.0)), 1.0);
}
)glsl";
} // anonymous namespace

class Display {
public:
    int          width, height;
    GLFWwindow*  window  = nullptr;  // public so ImGui and the main loop can use it directly

    Display(int w, int h, const std::string& title) : width(w), height(h) {
        if (!glfwInit())
            throw std::runtime_error("GLFW init failed");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Window creation failed");
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(0);  // uncapped — renderer controls frame pacing

        build_shader();
        build_texture();
        glGenVertexArrays(1, &vao);  // core profile requires a bound VAO for attribute-less draws

        glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int action, int) {
            if (action == GLFW_PRESS && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q))
                glfwSetWindowShouldClose(w, GLFW_TRUE);
        });
    }

    ~Display() {
        glDeleteTextures(1, &texture);
        glDeleteProgram(shader);
        glDeleteVertexArrays(1, &vao);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    Display(const Display&)            = delete;
    Display& operator=(const Display&) = delete;

    bool should_close() const { return glfwWindowShouldClose(window); }

    // Upload linear-float RGB data (width * height * 3 floats, row 0 = top of image).
    // Does not draw or swap — call draw_quad() after to render the updated texture.
    void upload(const float* linear_rgb) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        GL_RGB, GL_FLOAT, linear_rgb);
    }

    // Draw the fullscreen textured quad. Does not clear or swap.
    void draw_quad() {
        glUseProgram(shader);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Swap front/back buffers and process pending window events.
    void swap_and_poll() {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Convenience: upload + draw + swap + poll in one call (used by render_in_window).
    void present(const float* linear_rgb) {
        upload(linear_rgb);
        glClear(GL_COLOR_BUFFER_BIT);
        draw_quad();
        swap_and_poll();
    }

private:
    GLuint texture = 0;
    GLuint shader  = 0;
    GLuint vao     = 0;

    void build_shader() {
        auto compile = [](GLenum type, const char* src) -> GLuint {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) throw std::runtime_error("Shader compile failed");
            return s;
        };

        GLuint vs = compile(GL_VERTEX_SHADER,   vert_src);
        GLuint fs = compile(GL_FRAGMENT_SHADER, frag_src);
        shader = glCreateProgram();
        glAttachShader(shader, vs);
        glAttachShader(shader, fs);
        glLinkProgram(shader);
        glDeleteShader(vs);
        glDeleteShader(fs);

        glUseProgram(shader);
        glUniform1i(glGetUniformLocation(shader, "frame_tex"), 0);
    }

    void build_texture() {
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        // GL_RGB32F: full 32-bit float, linear light, matches double-precision framebuffer
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0,
                     GL_RGB, GL_FLOAT, nullptr);
        // Zero-initialise so the window shows black before the first render pass
        std::vector<float> zeros(width * height * 3, 0.0f);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        GL_RGB, GL_FLOAT, zeros.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
};

#endif
