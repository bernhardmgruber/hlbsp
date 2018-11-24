#include "Renderer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "opengl/Texture.h"
#include "../IRenderable.h"
#include "../Camera.h"
#include "../Entity.h"
#include "../mathlib.h"
#include "../Bsp.h"

namespace render::opengl {
	namespace {
		void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			const char* sourceStr = [&] {
				switch (source) {
				case GL_DEBUG_SOURCE_API_ARB: return "API";
				case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return "shader compiler";
				case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: return "window system";
				case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: return "third party";
				case GL_DEBUG_SOURCE_APPLICATION_ARB: return "application";
				case GL_DEBUG_SOURCE_OTHER_ARB: return "other";
				default: return "unknown";
				}
			}();

			const char* typeStr = [&] {
				switch (type) {
				case GL_DEBUG_TYPE_ERROR_ARB: return "error";
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "deprecated behavior";
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return "undefined behavior";
				case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "performance";
				case GL_DEBUG_TYPE_PORTABILITY_ARB: return "portability";
				case GL_DEBUG_TYPE_OTHER_ARB: return "other";
				default: return "unknown";
				}
			}();

			const char* severityStr = [&] {
				switch (severity) {
				case GL_DEBUG_SEVERITY_HIGH_ARB: return "high";
				case GL_DEBUG_SEVERITY_MEDIUM_ARB: return "medium";
				case GL_DEBUG_SEVERITY_LOW_ARB: return "low";
				default: return "unknown";
				}
			}();

			std::cerr << "OpenGL debug callback: [" << severityStr << "|" << sourceStr << "|" << typeStr << "] " << std::string(message, length) << '\n';

#ifdef _WIN32
			if (type == GL_DEBUG_TYPE_ERROR_ARB)
				__debugbreak();
#endif
		}

		// some GLEW versions still have the last parameter mutable (looking at you travis)
		void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam) {
			debugCallback(source, type, id, severity, length, message, static_cast<const void*>(userParam));
		}

		auto channelsToTextureType(const Image& img) {
			switch (img.channels) {
				case 1: return GL_RED;
				case 2: return GL_RG;
				case 3: return GL_RGB;
				case 4: return GL_RGBA;
				default: assert(false);
			}
		}

		auto convert(AttributeLayout::Type type) {
			switch (type) {
				case AttributeLayout::Type::Float: return GL_FLOAT;
				default: assert(false);
			}
		}
	}

	struct Texture : ITexture, gl::Texture {};

	struct Buffer : IBuffer, gl::Buffer {};

	struct InputLayout : IInputLayout, gl::VAO {};

	Renderer::Glew::Glew() {
		if (glewInit() != GLEW_OK)
			throw std::runtime_error("glew failed to initialize");
	}

	Renderer::Renderer() {
		std::cout << "OpenGL version: " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << '\n';
		std::cout << "OpenGL vendor:  " << reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << '\n';

#ifndef NDEBUG
		// error callback
		if (GLEW_ARB_debug_output) {
			glDebugMessageCallback(static_cast<GLDEBUGPROC>(debugCallback), nullptr);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}
#endif

		glShadeModel(GL_SMOOTH);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		glEnable(GL_MULTISAMPLE);

		////
		//// configure lighting for flashlight
		////

		//glEnable(GL_LIGHT0);

		//GLfloat lightPos[] = {0.0f, 0.0f, 0.0f, 1.0f};
		//glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		//GLfloat spotDir[] = {0.0f, 0.0f, -1.0f};
		//glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir);
		//glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 25.0f);
		//glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.0f);
		//glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.01f);
		//glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f);
		//glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0001f);

		m_skyboxProgram = gl::Program{
			gl::Shader(GL_VERTEX_SHADER, fs::path{"../src/opengl/shader/skybox.vert"}),
			gl::Shader(GL_FRAGMENT_SHADER, fs::path{"../src/opengl/shader/skybox.frag"}),
		};

		m_shaderProgram = gl::Program{
			gl::Shader(GL_VERTEX_SHADER, fs::path{"../src/opengl/shader/main.vert"}),
			gl::Shader(GL_FRAGMENT_SHADER, fs::path{"../src/opengl/shader/main.frag"}),
		};

		m_coordsProgram = gl::Program{
			gl::Shader(GL_VERTEX_SHADER, fs::path{"../src/opengl/shader/coords.vert"}),
			gl::Shader(GL_FRAGMENT_SHADER, fs::path{"../src/opengl/shader/coords.frag"}),
		};

		ImGui_ImplOpenGL3_Init("#version 330");
		ImGui_ImplOpenGL3_NewFrame(); // trigger building of some resources
	}

	Renderer::~Renderer() {
		ImGui_ImplOpenGL3_Shutdown();
	}

	void Renderer::resizeViewport(int width, int height) {
		glViewport(0, 0, width, height);
	}

	void Renderer::clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	auto Renderer::createTexture(const std::vector<Image>& mipmaps) const -> std::unique_ptr<ITexture> {
		std::unique_ptr<Texture> t(new Texture());
		t->bind(GL_TEXTURE_2D);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipmaps.size() - 1));
		for (int i = 0; i < mipmaps.size(); i++)
			glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, mipmaps[i].width, mipmaps[i].height, 0, channelsToTextureType(mipmaps[i]), GL_UNSIGNED_BYTE, mipmaps[i].data.data());
		return t;
	}

	auto Renderer::createCubeTexture(const std::array<Image, 6>& sides) const -> std::unique_ptr<ITexture> {
		std::unique_ptr<Texture> t(new Texture());
		t->bind(GL_TEXTURE_CUBE_MAP);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		for (auto i = 0; i < 6; i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, sides[i].width, sides[i].height, 0, sides[i].channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, sides[i].data.data());
		return t;
	}

	auto Renderer::createBuffer(std::size_t size, const void* data) const -> std::unique_ptr<IBuffer> {
		std::unique_ptr<Buffer> b(new Buffer());
		b->bind(GL_ARRAY_BUFFER);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return b;
	}

	auto Renderer::createInputLayout(IBuffer& buffer, const std::vector<AttributeLayout>& layout) const -> std::unique_ptr<IInputLayout> {
		std::unique_ptr<InputLayout> l(new InputLayout());
		l->bind();
		static_cast<Buffer&>(buffer).bind(GL_ARRAY_BUFFER);
		int i = 0;
		for (const auto& al : layout) {
			glVertexAttribPointer(i, al.size, convert(al.type), false, al.stride, reinterpret_cast<void*>(al.offset));
			glEnableVertexAttribArray(i);
			i++;
		}
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return l;
	}

	void Renderer::renderCoords(const glm::mat4& matrix) {
		m_emptyVao.bind();
		m_coordsProgram.use();
		glUniformMatrix4fv(m_coordsProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));
		glDrawArrays(GL_LINES, 0, 12);
	}

	void Renderer::renderSkyBox(ITexture& cubemap, const glm::mat4& matrix) {
		m_emptyVao.bind();
		m_skyboxProgram.use();
		glUniform1i(m_skyboxProgram.uniformLocation("cubeSampler"), 0);
		glUniformMatrix4fv(m_skyboxProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

		glActiveTexture(GL_TEXTURE0);
		static_cast<Texture&>(cubemap).bind(GL_TEXTURE_CUBE_MAP);

		glDepthMask(GL_FALSE);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(GL_TRUE);
	}

	void Renderer::renderStatic(std::vector<EntityData> entities, const std::vector<Decal>& decals, IInputLayout& staticLayout, IInputLayout& decalLayout, std::vector<std::unique_ptr<render::ITexture>>& textures, render::ITexture& lightmapAtlas, const RenderSettings& settings) {
		static_cast<InputLayout&>(staticLayout).bind();
		m_shaderProgram.use();
		glUniform1i(m_shaderProgram.uniformLocation("tex1"), 0);
		glUniform1i(m_shaderProgram.uniformLocation("tex2"), 1);
		glUniform1i(m_shaderProgram.uniformLocation("nightvision"), static_cast<GLint>(settings.nightvision));
		//glUniform1i(m_shaderProgram.uniformLocation("flashlight"), static_cast<GLint>(settings.flashlight));
		glUniform1i(m_shaderProgram.uniformLocation("unit1Enabled"), static_cast<GLint>(settings.textures));
		glUniform1i(m_shaderProgram.uniformLocation("unit2Enabled"), static_cast<GLint>(settings.lightmaps));

		glUniform1i(m_shaderProgram.uniformLocation("alphaTest"), 0);

		glEnable(GL_DEPTH_TEST);

		for (auto& ent : entities)
			renderBrushEntity(std::move(ent.fri), lightmapAtlas, settings, ent.origin, ent.alpha, ent.renderMode);

		glUniform1i(m_shaderProgram.uniformLocation("unit2Enabled"), 0);

		const auto matrix = settings.projection * settings.view;
		glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

		if (settings.renderDecals) {
			static_cast<InputLayout&>(decalLayout).bind();
			renderDecals(decals, textures);
		}

		glDisable(GL_DEPTH_TEST);

		glUseProgram(0);
	}

	void Renderer::renderBrushEntity(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas, const RenderSettings& settings, glm::vec3 origin, float alpha, bsp30::RenderMode renderMode) {
		const auto matrix = glm::translate(settings.projection * settings.view, origin);
		glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

		switch (renderMode) {
		case bsp30::RENDER_MODE_NORMAL:
			break;
		case bsp30::RENDER_MODE_TEXTURE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glDepthMask(GL_FALSE);
			break;
		case bsp30::RENDER_MODE_SOLID:
			glUniform1i(m_shaderProgram.uniformLocation("alphaTest"), 1);
			break;
		case bsp30::RENDER_MODE_ADDITIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glDepthMask(GL_FALSE);
			break;
		}

		renderFri(std::move(fri), lightmapAtlas);

		switch (renderMode) {
		case bsp30::RENDER_MODE_NORMAL:
			break;
		case bsp30::RENDER_MODE_TEXTURE:
		case bsp30::RENDER_MODE_ADDITIVE:
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			break;
		case bsp30::RENDER_MODE_SOLID:
			glUniform1i(m_shaderProgram.uniformLocation("alphaTest"), 0);
			break;
		}
	}

	void Renderer::renderFri(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas) {
		// sort by texture id to avoid some rebinds
		//std::sort(begin(fri), end(fri), [](const FaceRenderInfo& a, const FaceRenderInfo& b) {
		//	return a.tex < b.tex;
		//});

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, static_cast<Texture&>(lightmapAtlas).id());
		glActiveTexture(GL_TEXTURE0);
		ITexture* curId = nullptr;
		for (const auto& i : fri) {
			if (curId != i.tex) {
				glBindTexture(GL_TEXTURE_2D, static_cast<Texture&>(*i.tex).id());
				curId = i.tex;
			}
			glDrawArrays(GL_TRIANGLES, i.offset, i.count);
		}
	}

	void Renderer::renderDecals(const std::vector<Decal>& decals, std::vector<std::unique_ptr<render::ITexture>>& textures) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, -2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glActiveTexture(GL_TEXTURE0);

		for (auto i = 0; i < decals.size(); i++) {
			glBindTexture(GL_TEXTURE_2D, static_cast<Texture&>(*textures[decals[i].texIndex]).id());
			glDrawArrays(GL_TRIANGLES, i * 4, 4);
		}

		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void Renderer::renderImgui(ImDrawData* data) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplOpenGL3_RenderDrawData(data);
	}

	auto Renderer::screenshot() const -> Image {
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		Image img(vp[2], vp[3], 3);
		glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGB, GL_UNSIGNED_BYTE, img.data.data());
		return img;
	}

	auto Platform::createWindowAndContext(int width, int height, const char * title, GLFWmonitor * monitor) -> GLFWwindow* {
		// set window hints before creating window
		// list of available hints and their defaults: http://www.glfw.org/docs/3.0/window.html#window_hints
		glfwWindowHint(GLFW_DEPTH_BITS, 32);
		glfwWindowHint(GLFW_STENCIL_BITS, 0);
		glfwWindowHint(GLFW_FOCUSED, false);
#ifndef NDEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_window = glfwCreateWindow(width, height, title, monitor, nullptr);

		return m_window;
	}

	auto Platform::createRenderer() -> std::unique_ptr<IRenderer> {
		return std::make_unique<Renderer>();
	}

	void Platform::swapBuffers() {
		glfwSwapBuffers(m_window);
	}
}
