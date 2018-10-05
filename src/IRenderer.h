#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <array>
#include <vector>

#include "Image.h"
#include "bspdef.h"

class Camera;
struct Decal;
struct ImDrawData;
struct RenderSettings;

namespace render {
	class ITexture {
	public:
		~ITexture() = default;
	};

	class IBuffer {
	public:
		~IBuffer() = default;
	};

	class IInputLayout {
	public:
		~IInputLayout() = default;
	};

	struct FaceRenderInfo {
		render::ITexture* tex;
		unsigned int offset;
		unsigned int count;
	};

	struct AttributeLayout {
		int size;
		enum class Type {
			Float
		} type;
		unsigned int stride;
		unsigned int offset;
	};

	struct EntityData {
		std::vector<FaceRenderInfo> fri;
		glm::vec3 origin;
		float alpha;
		bsp30::RenderMode renderMode;
	};

	class IRenderer {
	public:
		~IRenderer() = default;

		virtual void resizeViewport(int width, int height) = 0;
		virtual void clear() = 0;

		virtual auto createTexture(const std::vector<Image>& mipmaps) const -> std::unique_ptr<ITexture> = 0;
		virtual auto createCubeTexture(const std::array<Image, 6>& sides) const -> std::unique_ptr<ITexture> = 0;
		virtual auto createBuffer(std::size_t size, const void* data) const -> std::unique_ptr<IBuffer> = 0;
		virtual auto createInputLayout(IBuffer& buffer, const std::vector<AttributeLayout>& layout) const -> std::unique_ptr<IInputLayout> = 0;

		virtual void renderCoords(const glm::mat4& matrix) = 0;
		virtual void renderSkyBox(ITexture& cubemap, const glm::mat4& matrix) = 0;
		virtual void renderStatic(std::vector<EntityData> entities, const std::vector<Decal>& decals, IInputLayout& staticLayout, IInputLayout& decalLayout, std::vector<std::unique_ptr<render::ITexture>>& textures, render::ITexture& lightmapAtlas, const RenderSettings& settings) = 0;
		virtual void renderImgui(ImDrawData* data) = 0;
	};
}
