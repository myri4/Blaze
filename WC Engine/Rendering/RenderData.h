#pragma once

#include <wc/vk/Buffer.h>
#include <wc/vk/Commands.h>
#include <wc/vk/Image.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <wc/Math/Splines.h>
#include <wc/Utils/Image.h>
#include "Font.h"
#include "AssetManager.h"

#undef LoadImage

namespace wc
{
	struct Vertex
	{
		glm::vec3 Position;
		uint32_t TextureID = 0;
		glm::vec2 TexCoords;
		float Fade = 0.f;
		float Thickness = 0.f;
		uint64_t EntityID = 0;

		glm::vec4 Color;
		Vertex() = default;
		Vertex(const glm::vec3& pos, glm::vec2 texCoords, uint32_t texID, const glm::vec4& color, uint64_t eid) : Position(pos), TexCoords(texCoords), TextureID(texID), Color(color), EntityID(eid) {}
		Vertex(const glm::vec3& pos, glm::vec2 texCoords, float thickness, float fade, const glm::vec4& color, uint64_t eid) : Position(pos), TexCoords(texCoords), Thickness(thickness), Fade(fade), Color(color), EntityID(eid) {}
	};

	struct LineVertex
	{
		glm::vec3 Position;

		glm::vec4 Color;
		uint64_t EntityID = 0;

		LineVertex() = default;
		LineVertex(const glm::vec3& pos, const glm::vec4& color, uint64_t eid) : Position(pos), Color(color), EntityID(eid) {}
	};

	struct RenderData
	{
	private:
		vk::DBufferManager<Vertex, vk::DEVICE_ADDRESS> m_VertexBuffer;
		vk::DBufferManager<uint32_t, vk::INDEX_BUFFER> m_IndexBuffer;
		vk::DBufferManager<LineVertex, vk::DEVICE_ADDRESS> m_LineVertexBuffer;
	public:

		auto GetVertexBuffer() const { return m_VertexBuffer.GetBuffer(); }
		auto GetIndexBuffer() const { return m_IndexBuffer.GetBuffer(); }

		auto GetLineVertexBuffer() const { return m_LineVertexBuffer.GetBuffer(); }

		auto GetIndexCount() const { return m_IndexBuffer.GetSize(); }
		auto GetVertexCount() const { return m_VertexBuffer.GetSize(); }

		auto GetLineVertexCount() const { return m_LineVertexBuffer.GetSize(); }

		void UploadVertexData()
		{
			vk::SyncContext::ImmediateSubmit([this](VkCommandBuffer cmd) {
				m_IndexBuffer.Update(cmd);
				m_VertexBuffer.Update(cmd);
				});
		}

		void UploadLineVertexData()
		{
			vk::SyncContext::ImmediateSubmit([this](VkCommandBuffer cmd) {
				m_LineVertexBuffer.Update(cmd);
				});
		}

		void Allocate()
		{
		}

		void Reset()
		{
			m_IndexBuffer.Reset();
			m_VertexBuffer.Reset();
			m_LineVertexBuffer.Reset();
		}

		void Free()
		{
			m_VertexBuffer.Free();
			m_IndexBuffer.Free();
			m_LineVertexBuffer.Free();
		}

		void DrawQuad(const glm::mat4& transform, uint32_t texID, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			auto vertCount = m_VertexBuffer.GetSize();

			m_VertexBuffer.Push({transform * glm::vec4( 0.5f,  0.5f, 0.f, 1.f), { 1.f, 0.f }, texID, color, entityID});
			m_VertexBuffer.Push({transform * glm::vec4(-0.5f,  0.5f, 0.f, 1.f), { 0.f, 0.f }, texID, color, entityID});
			m_VertexBuffer.Push({transform * glm::vec4(-0.5f, -0.5f, 0.f, 1.f), { 0.f, 1.f }, texID, color, entityID});
			m_VertexBuffer.Push({transform * glm::vec4( 0.5f, -0.5f, 0.f, 1.f), { 1.f, 1.f }, texID, color, entityID});

			m_IndexBuffer.Push(0 + vertCount);
			m_IndexBuffer.Push(1 + vertCount);
			m_IndexBuffer.Push(2 + vertCount);

			m_IndexBuffer.Push(2 + vertCount);
			m_IndexBuffer.Push(3 + vertCount);
			m_IndexBuffer.Push(0 + vertCount);
		}

		void DrawLineQuad(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			glm::vec3 vertices[4];
			vertices[0] = transform * glm::vec4(0.5f, 0.5f, 0.f, 1.f);
			vertices[1] = transform * glm::vec4(-0.5f, 0.5f, 0.f, 1.f);
			vertices[2] = transform * glm::vec4(-0.5f, -0.5f, 0.f, 1.f);
			vertices[3] = transform * glm::vec4(0.5f, -0.5f, 0.f, 1.f);
			DrawLine(vertices[0], vertices[1], color, entityID);
			DrawLine(vertices[1], vertices[2], color, entityID);
			DrawLine(vertices[2], vertices[3], color, entityID);
			DrawLine(vertices[3], vertices[0], color, entityID);
		}

		void DrawLineQuad(glm::vec2 start, glm::vec2 end, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			glm::vec3 vertices[4];
			vertices[0] = glm::vec4(end.x, end.y, 0.f, 1.f);
			vertices[1] = glm::vec4(start.x, end.y, 0.f, 1.f);
			vertices[2] = glm::vec4(start.x, start.y, 0.f, 1.f);
			vertices[3] = glm::vec4(end.x, start.y, 0.f, 1.f);
			DrawLine(vertices[0], vertices[1], color, entityID);
			DrawLine(vertices[1], vertices[2], color, entityID);
			DrawLine(vertices[2], vertices[3], color, entityID);
			DrawLine(vertices[3], vertices[0], color, entityID);
		}

		void DrawQuad(const glm::vec3& position, glm::vec2 size, uint32_t texID = 0, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position) * glm::scale(glm::mat4(1.f), { size.x, size.y, 1.f });
			DrawQuad(transform, texID, color, entityID);
		}

		// Note: Rotation should be in radians
		void DrawQuad(const glm::vec3& position, glm::vec2 size, float rotation, uint32_t texID = 0, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position) * glm::rotate(glm::mat4(1.f), rotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f), { size.x, size.y, 1.f });
			DrawQuad(transform, texID, color, entityID);
		}

		void DrawTriangle(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, uint32_t texID, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			auto vertCount = m_VertexBuffer.GetSize();
			m_VertexBuffer.Push({glm::vec4(v1, 0.f, 1.f), { 1.f, 0.f }, texID, color, entityID});
			m_VertexBuffer.Push({glm::vec4(v2, 0.f, 1.f), { 0.f, 0.f }, texID, color, entityID});
			m_VertexBuffer.Push({glm::vec4(v3, 0.f, 1.f), { 0.f, 1.f }, texID, color, entityID});

			m_IndexBuffer.Push(0 + vertCount);
			m_IndexBuffer.Push(1 + vertCount);
			m_IndexBuffer.Push(2 + vertCount);
		}

		void DrawCircle(const glm::mat4& transform, float thickness = 1.f, float fade = 0.05f, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			auto vertCount = m_VertexBuffer.GetSize();

			m_VertexBuffer.Push({transform * glm::vec4( 1.f, 1.f, 0.f, 1.f),  {  1.f, 1.f }, thickness, fade, color, entityID});
			m_VertexBuffer.Push({transform * glm::vec4(-1.f, 1.f, 0.f, 1.f),  { -1.f, 1.f }, thickness, fade, color, entityID});
			m_VertexBuffer.Push({transform * glm::vec4(-1.f, -1.f, 0.f, 1.f), { -1.f,-1.f }, thickness, fade, color, entityID});
			m_VertexBuffer.Push({transform * glm::vec4( 1.f, -1.f, 0.f, 1.f), {  1.f,-1.f }, thickness, fade, color, entityID});

			m_IndexBuffer.Push(0 + vertCount);
			m_IndexBuffer.Push(1 + vertCount);
			m_IndexBuffer.Push(2 + vertCount);

			m_IndexBuffer.Push(2 + vertCount);
			m_IndexBuffer.Push(3 + vertCount);
			m_IndexBuffer.Push(0 + vertCount);
		}

		void DrawCircle(glm::vec3 position, float radius, float thickness = 1.f, float fade = 0.05f, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			auto transform = glm::translate(glm::mat4(1.f), position) * glm::scale(glm::mat4(1.f), { radius, radius, 1.f });
			DrawCircle(transform, thickness, fade, color, entityID);
		}

		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& startColor, const glm::vec4& endColor, uint64_t entityID = 0)
		{
			m_LineVertexBuffer.Push({ glm::vec4(start, 1.f), startColor, entityID} );
			m_LineVertexBuffer.Push({ glm::vec4(end, 1.f), endColor, entityID});
		}

		//void DrawLines(const LineVertex* vertices, uint32_t count)
		//{
		//	memcpy(m_LineVertexBuffer + m_LineVertexBuffer.Counter * sizeof(LineVertex), vertices, count * sizeof(LineVertex));
		//	m_LineVertexBuffer.Counter += count;
		//}

		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& startColor, const glm::vec3& endColor, uint64_t entityID = 0) { DrawLine(start, end, glm::vec4(startColor, 1.f), glm::vec4(endColor, 1.f), entityID); }
		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0) { DrawLine(start, end, color, color, entityID); }
		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, uint64_t entityID = 0) { DrawLine(start, end, color, color, entityID); }


		void DrawLine(glm::vec2 start, glm::vec2 end, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0) { DrawLine(glm::vec3(start, 0.f), glm::vec3(end, 0.f), color, color, entityID); }
		void DrawLine(glm::vec2 start, glm::vec2 end, const glm::vec3& startColor, const glm::vec3& endColor, uint64_t entityID = 0) { DrawLine(glm::vec3(start, 0.f), glm::vec3(end, 0.f), glm::vec4(startColor, 1.f), glm::vec4(endColor, 1.f), entityID); }
		void DrawLine(glm::vec2 start, glm::vec2 end, const glm::vec3& color, uint64_t entityID = 0) { DrawLine(glm::vec3(start, 0.f), glm::vec3(end, 0.f), color, color, entityID); }

		// @TODO:
		// Add support for color gradient and specifying vec3s instead of vec4s for colors
		// Add support for 3D
		void DrawBezierCurve(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, const glm::vec4& color = glm::vec4(1.f), uint32_t steps = 30, uint64_t entityID = 0)
		{
			glm::vec2 prevPointOnCurve = p0;
			for (uint32_t i = 0; i < steps; i++)
			{
				float t = (i + 1.f) / steps;
				glm::vec2 nextOnCurve = bezierLerp(p0, p1, p2, t);
				DrawLine(prevPointOnCurve, nextOnCurve, color, entityID);
				prevPointOnCurve = nextOnCurve;
			}
		}

		void DrawBezierCurve(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, const glm::vec4& color = glm::vec4(1.f), uint32_t steps = 30, uint64_t entityID = 0)
		{
			glm::vec2 prevPointOnCurve = p0;
			for (uint32_t i = 0; i < steps; i++)
			{
				float t = (i + 1.f) / steps;
				glm::vec2 nextOnCurve = bezierLerp(p0, p1, p2, p3, t);
				DrawLine(prevPointOnCurve, nextOnCurve, color, entityID);
				prevPointOnCurve = nextOnCurve;
			}
		}

		void DrawString(const std::string& string, const Font& font, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.f), uint64_t entityID = 0)
		{
			auto vertCount = m_VertexBuffer.GetSize();

			const auto& fontGeometry = font.FontGeometry;
			const auto& metrics = fontGeometry.getMetrics();
			uint32_t texID = font.TextureID;
			auto& fontAtlas = font.Tex;
			float texelWidth = 1.f / fontAtlas.GetImage().GetSize().x;
			float texelHeight = 1.f / fontAtlas.GetImage().GetSize().y;

			double x = 0.0;
			double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
			double y = 0.0;

			const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

			for (uint32_t i = 0; i < string.size(); i++)
			{
				char character = string[i];

				if (character == '\r')
					continue;

				if (character == '\n')
				{
					x = 0;
					y -= fsScale * metrics.lineHeight + font.LineSpacing;
					continue;
				}

				if (character == ' ')
				{
					double advance = spaceGlyphAdvance;
					if (i < string.size() - 1)
					{
						char nextCharacter = string[i + 1];
						double dAdvance;
						fontGeometry.getAdvance(dAdvance, character, nextCharacter);
						advance = dAdvance;
					}

					x += fsScale * advance + font.Kerning;
					continue;
				}

				if (character == '\t')
				{
					x += 4.f * (fsScale * spaceGlyphAdvance + font.Kerning);
					continue;
				}

				auto glyph = fontGeometry.getGlyph(character);

				if (!glyph)
					glyph = fontGeometry.getGlyph('?');

				if (!glyph) return;

				double al, ab, ar, at;
				glyph->getQuadAtlasBounds(al, ab, ar, at);
				glm::dvec2 texCoordMin(al, ab);
				glm::dvec2 texCoordMax(ar, at);

				double pl, pb, pr, pt;
				glyph->getQuadPlaneBounds(pl, pb, pr, pt);
				glm::dvec2 quadMin(pl, pb);
				glm::dvec2 quadMax(pr, pt);

				quadMin = quadMin * fsScale + glm::dvec2(x, y);
				quadMax = quadMax * fsScale + glm::dvec2(x, y);

				texCoordMin *= glm::dvec2(texelWidth, texelHeight);
				texCoordMax *= glm::dvec2(texelWidth, texelHeight);

				Vertex vertices[] = {
					Vertex(transform * glm::vec4(quadMax, 0.f, 1.f), texCoordMax, texID, color, entityID),
					Vertex(transform * glm::vec4(quadMin.x, quadMax.y, 0.f, 1.f), { texCoordMin.x, texCoordMax.y }, texID, color, entityID),
					Vertex(transform * glm::vec4(quadMin, 0.f, 1.f), texCoordMin, texID, color, entityID),
					Vertex(transform * glm::vec4(quadMax.x, quadMin.y, 0.f, 1.f), { texCoordMax.x, texCoordMin.y }, texID, color, entityID),
				};

				for (uint32_t i = 0; i < 4; i++)
				{
					vertices[i].Thickness = -1.f;
					m_VertexBuffer.Push(vertices[i]);
				}

				m_IndexBuffer.Push(0 + vertCount);
				m_IndexBuffer.Push(1 + vertCount);
				m_IndexBuffer.Push(2 + vertCount);

				m_IndexBuffer.Push(2 + vertCount);
				m_IndexBuffer.Push(3 + vertCount);
				m_IndexBuffer.Push(0 + vertCount);

				if (i < string.size() - 1)
				{
					double advance = glyph->getAdvance();
					char nextCharacter = string[i + 1];
					fontGeometry.getAdvance(advance, character, nextCharacter);

					x += fsScale * advance + font.Kerning;
				}
			}
		}

		void DrawString(const std::string& string, const Font& font, glm::vec2 position, glm::vec2 scale, float rotation, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), { position.x, position.y, 0.f }) * glm::rotate(glm::mat4(1.f), rotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f), { scale.x, scale.y, 1.f });
			DrawString(string, font, transform, color);
		}

		void DrawString(const std::string& string, const Font& font, glm::vec2 position, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), { position.x, position.y, 0.f });
			DrawString(string, font, transform, color);
		}
	};
}
