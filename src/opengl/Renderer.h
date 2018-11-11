#pragma once

#include <vector>

#include "../IRenderer.h"
#include "opengl/VAO.h"
#include "opengl/Program.h"
#include "opengl/Buffer.h"

namespace render::opengl {
	class Renderer : public IRenderer {
	public:
		Renderer();
		~Renderer();

		virtual void resizeViewport(int width, int height) override;
		virtual void clear() override;

		virtual auto createTexture(const std::vector<Image>& mipmaps) const -> std::unique_ptr<ITexture> override;
		virtual auto createCubeTexture(const std::array<Image, 6>& sides) const -> std::unique_ptr<ITexture> override;
		virtual auto createBuffer(std::size_t size, const void* data) const -> std::unique_ptr<IBuffer> override;
		virtual auto createInputLayout(IBuffer& buffer, const std::vector<AttributeLayout>& layout) const -> std::unique_ptr<IInputLayout> override;

		virtual void renderCoords(const glm::mat4& matrix) override;
		virtual void renderSkyBox(ITexture& cubemap, const glm::mat4& matrix) override;
		virtual void renderStatic(std::vector<EntityData> entities, const std::vector<Decal>& decals, IInputLayout& staticLayout, IInputLayout& decalLayout, std::vector<std::unique_ptr<render::ITexture>>& textures, render::ITexture& lightmapAtlas, const RenderSettings& settings) override;
		virtual void renderImgui(ImDrawData* data) override;

	private:
		void renderBrushEntity(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas, const RenderSettings& settings, glm::vec3 origin, float alpha, bsp30::RenderMode renderMode);
		void renderFri(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas);
		void renderDecals(const std::vector<Decal>& decals, std::vector<std::unique_ptr<render::ITexture>>& textures);

		struct Glew {
			Glew();
		} m_glew;

		gl::VAO m_emptyVao;
		gl::Program m_skyboxProgram;
		gl::Program m_shaderProgram;
		gl::Program m_coordsProgram;
	};

	class Platform : public IPlatform {
	public:
		virtual auto createWindowAndContext(int width, int height, const char * title, GLFWmonitor * monitor) -> GLFWwindow* override;
		virtual auto createRenderer() -> std::unique_ptr<IRenderer> override;
		virtual void swapBuffers() override;

	private:
		GLFWwindow* m_window;
	};
}