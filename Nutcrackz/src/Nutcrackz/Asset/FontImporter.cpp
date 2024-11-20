#include "nzpch.hpp"
#include "FontImporter.hpp"

#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Scene/SceneSerializer.hpp"

#include "Nutcrackz/Renderer/Font.hpp"

#undef INFINITE
#include "msdf-atlas-gen.h"
#include "FontGeometry.h"
#include "GlyphGeometry.h"

#include "Nutcrackz/Renderer/MSDFData.hpp"

namespace Nutcrackz {

	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static RefPtr<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
		const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height, bool useLinear)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
		generator.setThreadCount(8);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		TextureSpecification spec;
		spec.Width = bitmap.width;
		spec.Height = bitmap.height;

		// If using MSDF
		//spec.Format = ImageFormat::RGB8;

		// If using MTSDF
		spec.Format = ImageFormat::RGBA8;

		spec.GenerateMips = false;
		spec.UseLinear = useLinear;

		RefPtr<Texture2D> texture = Texture2D::Create(spec);

		// If using MSDF
		//texture->SetData(Buffer((void*)bitmap.pixels, bitmap.width * bitmap.height * 3));

		// If using MTSDF
		texture->SetData(Buffer((void*)bitmap.pixels, bitmap.width * bitmap.height * 4));
		return texture;
	}

	RefPtr<Font> FontImporter::ImportFont(AssetHandle handle, const AssetMetadata& metadata)
	{
		//NZ_PROFILE_FUNCTION();

		return LoadFont(Project::GetActiveAssetDirectory() / metadata.FilePath);
	}

	RefPtr<Font> FontImporter::LoadFont(const std::filesystem::path& path)
	{
		//NZ_PROFILE_FUNCTION();

		RefPtr<Font> fontTexture = RefPtr<Font>::Create();
		MSDFData* data = new MSDFData();
		RefPtr<Texture2D> atlasTexture;

		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		NZ_CORE_ASSERT(ft);

		msdfgen::FontHandle* font = msdfgen::loadFont(ft, path.string().c_str());
		if (!font)
		{
			NZ_CORE_ERROR("Failed to load font: {}", path.string());
			return RefPtr<Font>();
		}

		struct CharsetRange
		{
			uint32_t Begin, End;
		};

		// From imgui_draw.cpp
		static const CharsetRange charsetRanges[] =
		{
			{ 0x0020, 0x00FF }
		};

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		double fontScale = 1.0;
		data->FontGeometry = msdf_atlas::FontGeometry(&data->Glyphs);
		int glyphsLoaded = data->FontGeometry.loadCharset(font, fontScale, charset);
		NZ_CORE_INFO("Loaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(data->Glyphs.data(), (int)data->Glyphs.size());
		NZ_CORE_ASSERT(remaining == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
		// if MSDF || MTSDF

		uint64_t coloringSeed = 0;
		bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs = data->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
			}, (int)data->Glyphs.size()).finish(THREAD_COUNT);
		}
		else
		{
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : data->Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		// If using MSDF
		//atlasTexture = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("Test", (float)emSize, data->Glyphs, data->FontGeometry, width, height, useLinear);

		// If using MTSDF
		atlasTexture = CreateAndCacheAtlas<uint8_t, float, 4, msdf_atlas::mtsdfGenerator>("Test", (float)emSize, data->Glyphs, data->FontGeometry, width, height, true);

#if 0
		msdfgen::Shape shape;
		if (msdfgen::loadGlyph(shape, font, 'C'))
		{
			shape.normalize();
			//                      max. angle
			msdfgen::edgeColoringSimple(shape, 3.0);
			//           image width, height
			msdfgen::Bitmap<float, 3> msdf(32, 32);
			//                     range, scale, translation
			msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(4.0, 4.0));
			msdfgen::savePng(msdf, "output.png");
		}
#endif

		fontTexture->SetMSDFData(data);
		fontTexture->SetAtlasTexture(atlasTexture);

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);

		return fontTexture;
	}

	RefPtr<Font> FontImporter::ReloadFont(const AssetMetadata& metadata, MSDFData* data, RefPtr<Texture2D>& atlasTexture, bool useLinear)
	{
		//NZ_PROFILE_FUNCTION();

		RefPtr<Font> fontTexture = RefPtr<Font>::Create();

		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		NZ_CORE_ASSERT(ft);

		std::string fileString = (Project::GetActiveAssetDirectory() / metadata.FilePath).string();

		msdfgen::FontHandle* font = msdfgen::loadFont(ft, fileString.c_str());
		if (!font)
		{
			NZ_CORE_ERROR("Failed to reload font: {}, getting default font instead", fileString);
			return LoadFont("assets/fonts/opensans/OpenSans-Regular.ttf");
		}

		struct CharsetRange
		{
			uint32_t Begin, End;
		};

		// From imgui_draw.cpp
		static const CharsetRange charsetRanges[] =
		{
			{ 0x0020, 0x00FF }
		};

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		double fontScale = 1.0;
		data->FontGeometry = msdf_atlas::FontGeometry(&data->Glyphs);
		int glyphsLoaded = data->FontGeometry.loadCharset(font, fontScale, charset);
		NZ_CORE_INFO("Reloaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(data->Glyphs.data(), (int)data->Glyphs.size());
		NZ_CORE_ASSERT(remaining == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
		// if MSDF || MTSDF

		uint64_t coloringSeed = 0;
		bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs = data->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
				}, (int)data->Glyphs.size()).finish(THREAD_COUNT);
		}
		else {
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : data->Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		// If using MSDF
		//atlasTexture = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("Test", (float)emSize, data->Glyphs, data->FontGeometry, width, height, useLinear);

		// If using MTSDF
		atlasTexture = CreateAndCacheAtlas<uint8_t, float, 4, msdf_atlas::mtsdfGenerator>("Test", (float)emSize, data->Glyphs, data->FontGeometry, width, height, true);

#if 0
		msdfgen::Shape shape;
		if (msdfgen::loadGlyph(shape, font, 'C'))
		{
			shape.normalize();
			//                      max. angle
			msdfgen::edgeColoringSimple(shape, 3.0);
			//           image width, height
			msdfgen::Bitmap<float, 3> msdf(32, 32);
			//                     range, scale, translation
			msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(4.0, 4.0));
			msdfgen::savePng(msdf, "output.png");
		}
#endif

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);

		return fontTexture;
	}

}
