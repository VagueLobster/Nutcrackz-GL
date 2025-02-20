#include "nzpch.hpp"
#include "ScriptGlue.hpp"

#include "Nutcrackz/ImGui/ImGui.hpp"

#include "Nutcrackz/Asset/AssetManager.hpp"

#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Core/Hash.hpp"
#include "Nutcrackz/Scene/Entity.hpp"
#include "Nutcrackz/Scene/Components.hpp"
#include "Nutcrackz/Physics/ContactListener2D.hpp"

#include "Nutcrackz/Scripting/ScriptEngine.hpp"

#include "Nutcrackz/Utils/TypeInfo.hpp"

#include "Nutcrackz/Core/KeyCodes.hpp"

//#include <Coral/ManagedObject.hpp>
//#include <Coral/HostInstance.hpp>

#include <functional>

namespace Nutcrackz {

#ifdef NZ_PLATFORM_WINDOWS
	#define NZ_FUNCTION_NAME __func__
#else
	#define NZ_FUNCTION_NAME __FUNCTION__
#endif

#define NZ_ADD_INTERNAL_CALL(icall) coreAssembly.AddInternalCall("Nutcrackz.InternalCalls", #icall, (void*)InternalCalls::icall)

#ifdef NZ_DIST
	#define HZ_ICALL_VALIDATE_PARAM(param) NZ_CORE_VERIFY(param, "{} called with an invalid value ({}) for parameter '{}'", NZ_FUNCTION_NAME, param, #param)
	#define HZ_ICALL_VALIDATE_PARAM_V(param, value) NZ_CORE_VERIFY(param, "{} called with an invalid value ({}) for parameter '{}'.\nStack Trace: {}", NZ_FUNCTION_NAME, value, #param, ScriptUtils::GetCurrentStackTrace())
#else
	#define HZ_ICALL_VALIDATE_PARAM(param) { if (!(param)) { NZ_CONSOLE_LOG_ERROR("{} called with an invalid value ({}) for parameter '{}'", NZ_FUNCTION_NAME, param, #param); } }
	#define HZ_ICALL_VALIDATE_PARAM_V(param, value) { if (!(param)) { NZ_CONSOLE_LOG_ERROR("{} called with an invalid value ({}) for parameter '{}'.", NZ_FUNCTION_NAME, value, #param); } }
#endif

	bool ScriptGlue::s_CalledSetCursor = false;
	bool ScriptGlue::s_ChangedCursor = false;
	rtmcpp::Vec2 ScriptGlue::s_CursorHotSpot = rtmcpp::Vec2(0.0f, 0.0f);
	std::string ScriptGlue::s_SetCursorPath = "";

	Entity s_HoveredEntity;
	Entity s_SelectedEntity;

	std::unordered_map<Coral::TypeId, std::function<void(Entity&)>> s_CreateComponentFuncs;
	std::unordered_map<Coral::TypeId, std::function<bool(Entity&)>> s_HasComponentFuncs;
	std::unordered_map<Coral::TypeId, std::function<void(Entity&)>> s_RemoveComponentFuncs;

	template<typename TComponent>
	static void RegisterManagedComponent(Coral::ManagedAssembly& coreAssembly)
	{
		// NOTE(Peter): Get the demangled type name of TComponent
		const TypeNameString& componentTypeName = TypeInfo<TComponent, true>().Name();
		std::string componentName = fmt::format("Nutcrackz.{}", componentTypeName);

		auto& type = coreAssembly.GetType(componentName);

		if (type)
		{
			s_CreateComponentFuncs[type.GetTypeId()] = [](Entity& entity) { entity.AddComponent<TComponent>(); };
			s_HasComponentFuncs[type.GetTypeId()] = [](Entity& entity) { return entity.HasComponent<TComponent>(); };
			s_RemoveComponentFuncs[type.GetTypeId()] = [](Entity& entity) { entity.RemoveComponent<TComponent>(); };
		}
		else
		{
			NZ_CORE_ERROR("No C# component class found for {}!", componentName);
			NZ_CORE_VERIFY(false, "No C# component class found!");
		}
	}

	template<typename TComponent>
	static void RegisterManagedComponent(std::function<void(Entity&)>&& addFunction, Coral::ManagedAssembly& coreAssembly)
	{
		// NOTE(Peter): Get the demangled type name of TComponent
		const TypeNameString& componentTypeName = TypeInfo<TComponent, true>().Name();
		std::string componentName = fmt::format("Nutcrackz.{}", componentTypeName);

		auto& type = coreAssembly.GetType(componentName);

		if (type)
		{
			s_CreateComponentFuncs[type.GetTypeId()] = std::move(addFunction);
			s_HasComponentFuncs[type.GetTypeId()] = [](Entity& entity) { return entity.HasComponent<TComponent>(); };
			s_RemoveComponentFuncs[type.GetTypeId()] = [](Entity& entity) { entity.RemoveComponent<TComponent>(); };
		}
		else
		{
			NZ_CORE_ERROR("No C# component class found for {}!", componentName);
			NZ_CORE_VERIFY(false, "No C# component class found!");
		}
	}

	template<typename... TArgs>
	static void WarnWithTrace(const std::string& inFormat, TArgs&&... inArgs)
	{
		/*auto stackTrace = ScriptUtils::GetCurrentStackTrace();
		std::string formattedMessage = fmt::format(inFormat, std::forward<TArgs>(inArgs)...);
		Log::GetEditorConsoleLogger()->warn("{}\nStack Trace: {}", formattedMessage, stackTrace);*/
	}

	template<typename... TArgs>
	static void ErrorWithTrace(const std::string& inFormat, TArgs&&... inArgs)
	{
		/*auto stackTrace = ScriptUtils::GetCurrentStackTrace();
		std::string formattedMessage = fmt::format(inFormat, std::forward<TArgs>(inArgs)...);
		Log::GetEditorConsoleLogger()->error("{}\nStack Trace: {}", formattedMessage, stackTrace);*/
	}

	void ScriptGlue::RegisterGlue(Coral::ManagedAssembly& coreAssembly)
	{
		if (!s_CreateComponentFuncs.empty())
		{
			s_CreateComponentFuncs.clear();
			s_HasComponentFuncs.clear();
			s_RemoveComponentFuncs.clear();
		}

		RegisterComponentTypes(coreAssembly);
		RegisterInternalCalls(coreAssembly);

		coreAssembly.UploadInternalCalls();
	}

	void ScriptGlue::RegisterComponentTypes(Coral::ManagedAssembly& coreAssembly)
	{
		RegisterManagedComponent<TagComponent>(coreAssembly);
		RegisterManagedComponent<TransformComponent>(coreAssembly);
		RegisterManagedComponent<SpriteRendererComponent>(coreAssembly);
		RegisterManagedComponent<CircleRendererComponent>(coreAssembly);
		RegisterManagedComponent<TriangleRendererComponent>(coreAssembly);
		RegisterManagedComponent<LineRendererComponent>(coreAssembly);
		//RegisterManagedComponent<FauxLightRendererComponent>();
		RegisterManagedComponent<TextComponent>(coreAssembly);
		RegisterManagedComponent<ParticleSystemComponent>(coreAssembly);
		//RegisterManagedComponent<CubeRendererComponent>(coreAssembly);
		//RegisterManagedComponent<PyramidRendererComponent>(coreAssembly);
		//RegisterManagedComponent<TriangularPrismRendererComponent>(coreAssembly);
		//RegisterManagedComponent<PlaneRendererComponent>(coreAssembly);
		//RegisterManagedComponent<OBJRendererComponent>(coreAssembly);
		//RegisterManagedComponent<SkyboxComponent>();
		RegisterManagedComponent<ButtonWidgetComponent>(coreAssembly);
		RegisterManagedComponent<CircleWidgetComponent>(coreAssembly);
		RegisterManagedComponent<CameraComponent>(coreAssembly);
		RegisterManagedComponent<Rigidbody2DComponent>(coreAssembly);
		RegisterManagedComponent<BoxCollider2DComponent>(coreAssembly);
		RegisterManagedComponent<CircleCollider2DComponent>(coreAssembly);
		RegisterManagedComponent<TriangleCollider2DComponent>(coreAssembly);
		RegisterManagedComponent<CapsuleCollider2DComponent>(coreAssembly);
		RegisterManagedComponent<MeshCollider2DComponent>(coreAssembly);
		RegisterManagedComponent<ScriptComponent>(coreAssembly);
		RegisterManagedComponent<AudioListenerComponent>(coreAssembly);
		RegisterManagedComponent<AudioSourceComponent>(coreAssembly);
		RegisterManagedComponent<VideoRendererComponent>(coreAssembly);
	}

	void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& coreAssembly)
	{
		// Testing Only!
		//NZ_ADD_INTERNAL_CALL(NativeLog);
		//NZ_ADD_INTERNAL_CALL(NativeLog_Vector);
		//NZ_ADD_INTERNAL_CALL(NativeLog_VectorDot);

		// AssetHandle
		NZ_ADD_INTERNAL_CALL(AssetHandle_IsValid);

		// ConsoleLog
		//NZ_ADD_INTERNAL_CALL(ConsoleLog_Trace);
		//NZ_ADD_INTERNAL_CALL(ConsoleLog_Info);
		//NZ_ADD_INTERNAL_CALL(ConsoleLog_Warning);
		//NZ_ADD_INTERNAL_CALL(ConsoleLog_Error);
		//NZ_ADD_INTERNAL_CALL(ConsoleLog_Critical);

		// Input
		NZ_ADD_INTERNAL_CALL(Input_IsKeyDown);
		NZ_ADD_INTERNAL_CALL(Input_IsKeyUp);
		NZ_ADD_INTERNAL_CALL(Input_IsMouseButtonPressed);
		NZ_ADD_INTERNAL_CALL(Input_PressMouseButton);
		NZ_ADD_INTERNAL_CALL(Input_ReleaseMouseButton);
		NZ_ADD_INTERNAL_CALL(Input_GetMousePositionX);
		NZ_ADD_INTERNAL_CALL(Input_GetMousePositionY);
		NZ_ADD_INTERNAL_CALL(Input_GetMouseWorldPositionX);
		NZ_ADD_INTERNAL_CALL(Input_GetMouseWorldPositionY);

		// Application
		//NZ_ADD_INTERNAL_CALL(Application_GetFPS);
		//NZ_ADD_INTERNAL_CALL(Application_GetFrameTime);
		//NZ_ADD_INTERNAL_CALL(Application_GetMinFrameTime);
		//NZ_ADD_INTERNAL_CALL(Application_GetMaxFrameTime);

		// Gamepad
		NZ_ADD_INTERNAL_CALL(Input_IsControllerPresent);
		NZ_ADD_INTERNAL_CALL(Input_GetConnectedControllerIDs);
		NZ_ADD_INTERNAL_CALL(Input_GetControllerName);
		NZ_ADD_INTERNAL_CALL(Input_IsControllerButtonPressed);
		NZ_ADD_INTERNAL_CALL(Input_IsControllerButtonHeld);
		NZ_ADD_INTERNAL_CALL(Input_IsControllerButtonDown);
		NZ_ADD_INTERNAL_CALL(Input_IsControllerButtonReleased);
		NZ_ADD_INTERNAL_CALL(Input_GetControllerAxis);
		NZ_ADD_INTERNAL_CALL(Input_GetControllerHat);
		NZ_ADD_INTERNAL_CALL(Input_GetControllerDeadzone);
		NZ_ADD_INTERNAL_CALL(Input_SetControllerDeadzone);

		// Scene
		NZ_ADD_INTERNAL_CALL(Scene_LoadScene);
		NZ_ADD_INTERNAL_CALL(Scene_GetCursor);
		NZ_ADD_INTERNAL_CALL(Scene_SetCursor);
		NZ_ADD_INTERNAL_CALL(Scene_GetMouseHotSpotX);
		NZ_ADD_INTERNAL_CALL(Scene_GetMouseHotSpotY);
		NZ_ADD_INTERNAL_CALL(Scene_SetMouseHotSpot);
		NZ_ADD_INTERNAL_CALL(Scene_ChangeCursor);
		//NZ_ADD_INTERNAL_CALL(Scene_CloseApplication);
		NZ_ADD_INTERNAL_CALL(Scene_GetName);
		NZ_ADD_INTERNAL_CALL(Scene_SetName);
		NZ_ADD_INTERNAL_CALL(Scene_IsGamePaused);
		NZ_ADD_INTERNAL_CALL(Scene_SetPauseGame);
		NZ_ADD_INTERNAL_CALL(Scene_CreateEntity);
		NZ_ADD_INTERNAL_CALL(Scene_IsEntityValid);
		NZ_ADD_INTERNAL_CALL(Scene_GetHoveredEntity);
		NZ_ADD_INTERNAL_CALL(Scene_GetSelectedEntity);
		NZ_ADD_INTERNAL_CALL(Scene_SetSelectedEntity);
		NZ_ADD_INTERNAL_CALL(Scene_RenderHoveredEntityOutline);
		NZ_ADD_INTERNAL_CALL(Scene_RenderSelectedEntityOutline);
		//NZ_ADD_INTERNAL_CALL(Scene_GetEntityComponent);

		// Entity
		NZ_ADD_INTERNAL_CALL(Entity_CreateComponent);
		NZ_ADD_INTERNAL_CALL(Entity_HasComponent);
		NZ_ADD_INTERNAL_CALL(Entity_RemoveComponent);
		NZ_ADD_INTERNAL_CALL(Entity_RemoveComponent);
		NZ_ADD_INTERNAL_CALL(Entity_DestroyEntity);
		NZ_ADD_INTERNAL_CALL(Entity_FindEntityByTag);
		NZ_ADD_INTERNAL_CALL(Entity_FindEntityByName);

		// Tag
		NZ_ADD_INTERNAL_CALL(TagComponent_GetTag);
		NZ_ADD_INTERNAL_CALL(TagComponent_SetTag);

		// Transform
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetIsEnabled);
		NZ_ADD_INTERNAL_CALL(TransformComponent_SetIsEnabled);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetTransform);
		NZ_ADD_INTERNAL_CALL(TransformComponent_SetTransform);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetTranslationX);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetTranslationY);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetTranslationZ);
		NZ_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetRotationX);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetRotationY);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetRotationZ);
		NZ_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetScaleX);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetScaleY);
		NZ_ADD_INTERNAL_CALL(TransformComponent_GetScaleZ);
		NZ_ADD_INTERNAL_CALL(TransformComponent_SetScale);

		// CameraComponent
		NZ_ADD_INTERNAL_CALL(CameraComponent_GetIsPrimary);
		NZ_ADD_INTERNAL_CALL(CameraComponent_SetPrimary);
		NZ_ADD_INTERNAL_CALL(CameraComponent_GetFixedAspectRatio);
		NZ_ADD_INTERNAL_CALL(CameraComponent_SetFixedAspectRatio);

		// SpriteRenderer
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetOffsetX);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetOffsetY);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetOffsetZ);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetOffset);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetUVX);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetUVY);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetUV);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetUseParallax);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetUseParallax);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetParallaxSpeedX);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetParallaxSpeedY);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetParallaxSpeed);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetParallaxDivision);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetParallaxDivision);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetUseTextureAtlasAnimation);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetUseTextureAtlasAnimation);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetAnimationSpeed);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetAnimationSpeed);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetNumTiles);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetNumTiles);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetStartIndexX);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetStartIndexX);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetStartIndexY);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetStartIndexY);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColumn);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColumn);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetRow);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetRow);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetSaturation);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetSaturation);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTextureAssetHandle);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTextureAssetHandle);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTextureAssetID);
		NZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTextureAssetID);

		// CircleRenderer
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetUVX);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetUVY);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetUV);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetUseParallax);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetUseParallax);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetParallaxSpeedX);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetParallaxSpeedY);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetParallaxSpeed);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetParallaxDivision);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetParallaxDivision);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetUseTextureAtlasAnimation);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetUseTextureAtlasAnimation);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetAnimationSpeed);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetAnimationSpeed);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetNumTiles);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetNumTiles);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetStartIndexX);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetStartIndexX);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetStartIndexY);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetStartIndexY);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetColumn);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetColumn);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_GetRow);
		NZ_ADD_INTERNAL_CALL(CircleRendererComponent_SetRow);

		// TriangleRenderer
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetUVX);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetUVY);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_SetUV);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_GetSaturation);
		NZ_ADD_INTERNAL_CALL(TriangleRendererComponent_SetSaturation);

		// LineRenderer
		NZ_ADD_INTERNAL_CALL(LineRendererComponent_GetLineThickness);
		NZ_ADD_INTERNAL_CALL(LineRendererComponent_SetLineThickness);

		// TextRenderer
		NZ_ADD_INTERNAL_CALL(TextComponent_GetText);
		NZ_ADD_INTERNAL_CALL(TextComponent_SetText);
		NZ_ADD_INTERNAL_CALL(TextComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(TextComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(TextComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(TextComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(TextComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(TextComponent_GetKerning);
		NZ_ADD_INTERNAL_CALL(TextComponent_SetKerning);
		NZ_ADD_INTERNAL_CALL(TextComponent_GetLineSpacing);
		NZ_ADD_INTERNAL_CALL(TextComponent_SetLineSpacing);

		// VideoRenderer
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetVideoHandle);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetVideoHandle);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetSaturation);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetSaturation);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetUseBillboard);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetUseBillboard);
		//NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetUseExternalAudio);
		//NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetUseExternalAudio);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetPlayVideo);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetPlayVideo);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetRepeatVideo);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetRepeatVideo);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetPauseVideo);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetPauseVideo);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_GetVolume);
		NZ_ADD_INTERNAL_CALL(VideoRendererComponent_SetVolume);

		// ParticleSystem
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetVelocityX);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetVelocityY);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetVelocityZ);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetVelocity);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetVelocityVariationX);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetVelocityVariationY);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetVelocityVariationZ);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetVelocityVariation);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorBeginX);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorBeginY);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorBeginZ);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorBeginW);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetColorBegin);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorEndX);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorEndY);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorEndZ);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetColorEndW);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetColorEnd);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetSizeBegin);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetSizeBegin);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetSizeEnd);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetSizeEnd);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetSizeVariation);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetSizeVariation);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetLifeTime);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetLifeTime);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetParticleSize);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetParticleSize);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetUseLinear);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetUseLinear);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetTextureHandle);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetTextureHandle);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_GetUseBillboard);
		NZ_ADD_INTERNAL_CALL(ParticleSystemComponent_SetUseBillboard);

		// ButtonWidgetComponent
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetUVX);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetUVY);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_SetUV);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetRadius);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_SetRadius);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetDimensionsX);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetDimensionsY);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_SetDimensions);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetInvertCorners);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_SetInvertCorners);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_GetUseLinear);
		NZ_ADD_INTERNAL_CALL(ButtonWidgetComponent_SetUseLinear);

		// CircleWidgetComponent
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetColorX);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetColorY);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetColorZ);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetColorW);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_SetColor);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetUVX);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetUVY);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_SetUV);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetThickness);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_SetThickness);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetFade);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_SetFade);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetRadius);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_SetRadius);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_GetUseLinear);
		NZ_ADD_INTERNAL_CALL(CircleWidgetComponent_SetUseLinear);

		// Rigidbody2D
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulse);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulseToCenter);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetLinearVelocityX);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetLinearVelocityY);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetLinearVelocity);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetType);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetType);
		//NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetGravity);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetGravityX);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetGravityY);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetGravity);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetEnabled);
		NZ_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetEnabled);

		// BoxCollider2D
		//NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetOffset);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetOffsetX);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetOffsetY);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetOffset);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetSizeX);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetSizeY);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetSize);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetDensity);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetDensity);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetFriction);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetFriction);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetRestitution);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetRestitution);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetCollisionRayX);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetCollisionRayY);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetCollisionRay);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetAwake);
		NZ_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetAwake);

		// CircleCollider2D
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetOffsetX);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetOffsetY);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetOffset);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRadius);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRadius);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetDensity);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetDensity);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetFriction);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetFriction);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRestitution);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRestitution);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetCollisionRayX);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetCollisionRayY);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetCollisionRay);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetAwake);
		NZ_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetAwake);

		// TriangleCollider2D
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetOffsetX);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetOffsetY);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetOffset);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetSizeX);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetSizeY);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetSize);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetDensity);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetDensity);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetFriction);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetFriction);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetRestitution);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetRestitution);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetCollisionRayX);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetCollisionRayY);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetCollisionRay);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_GetAwake);
		NZ_ADD_INTERNAL_CALL(TriangleCollider2DComponent_SetAwake);

		// CapsuleCollider2D
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetOffsetX);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetOffsetY);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetOffset);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetSizeX);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetSizeY);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetSize);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetDensity);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetDensity);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetFriction);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetFriction);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetRestitution);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetRestitution);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetCollisionRayX);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetCollisionRayY);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetCollisionRay);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_GetAwake);
		NZ_ADD_INTERNAL_CALL(CapsuleCollider2DComponent_SetAwake);

		// MeshCollider2D
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetOffsetX);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetOffsetY);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetOffset);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetSizeX);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetSizeY);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetSize);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetDensity);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetDensity);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetFriction);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetFriction);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetRestitution);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetRestitution);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetRestitutionThreshold);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetCollisionRayX);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetCollisionRayY);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetCollisionRay);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_GetAwake);
		NZ_ADD_INTERNAL_CALL(MeshCollider2DComponent_SetAwake);

		// AudioListener
		NZ_ADD_INTERNAL_CALL(AudioListenerComponent_GetActive);
		NZ_ADD_INTERNAL_CALL(AudioListenerComponent_SetActive);

		// AudioSource
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetAssetHandle);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetAssetHandle);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetVolume);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetVolume);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetPitch);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetPitch);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetPlayOnAwake);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetPlayOnAwake);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetLooping);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetLooping);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetSpatialization);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetSpatialization);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetAttenuationModel);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetAttenuationModel);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetRollOff);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetRollOff);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetMinGain);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetMinGain);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetMaxGain);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetMaxGain);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetMinDistance);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetMinDistance);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetMaxDistance);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetMaxDistance);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetConeInnerAngle);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetConeInnerAngle);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetConeOuterAngle);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetConeOuterAngle);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetConeOuterGain);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetConeOuterGain);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetCone);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_GetDopplerFactor);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_SetDopplerFactor);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_IsPlaying);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_Play);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_Pause);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_UnPause);
		NZ_ADD_INTERNAL_CALL(AudioSourceComponent_Stop);
	}

	Entity ScriptGlue::GetHoveredEntity()
	{
		return s_HoveredEntity;
	}

	void ScriptGlue::SetHoveredEntity(Entity entity)
	{
		s_HoveredEntity = entity;
	}

	Entity ScriptGlue::GetSelectedEntity()
	{
		return s_SelectedEntity;
	}

	void ScriptGlue::SetSelectedEntity(Entity entity)
	{
		s_SelectedEntity = entity;
	}

	namespace InternalCalls {

#pragma region TestingOnly

		/*void NativeLog(Coral::NativeString string, int parameter)
		{
			std::string str = Coral::NativeString(string);
			std::cout << str << ", " << parameter << std::endl;
		}

		void NativeLog_Vector(glm::vec3* parameter, glm::vec3* outResult)
		{
			NZ_CORE_WARN("Value: {0}", *parameter);
			*outResult = glm::normalize(*parameter);
		}

		float NativeLog_VectorDot(glm::vec3* parameter)
		{
			NZ_CORE_WARN("Value: {0}", *parameter);
			return glm::dot(*parameter, *parameter);
		}*/

#pragma endregion

#pragma region AssetHandle

		bool AssetHandle_IsValid(AssetHandle assetHandle)
		{
			return AssetManager::IsAssetHandleValid(assetHandle);
		}

#pragma endregion

#pragma region Input

		bool Input_IsKeyDown(KeyCodeHazel keycode)
		{
			return Input::IsKeyPressed(keycode);
		}

		bool Input_IsKeyUp(KeyCodeHazel keycode)
		{
			return Input::IsKeyReleased(keycode);
		}

		bool Input_IsMouseButtonPressed(MouseCodeHazel button)
		{
			if (ScriptGlue::s_IsCursorInViewport)
				return Input::IsMouseButtonPressed(button);

			return false;
		}

		bool Input_PressMouseButton(MouseCodeHazel button)
		{
			if (ScriptGlue::s_IsCursorInViewport)
				return Input::PressMouseButton(button);

			return false;
		}

		bool Input_ReleaseMouseButton(MouseCodeHazel button)
		{
			if (ScriptGlue::s_IsCursorInViewport)
				return Input::ReleaseMouseButton(button);

			return false;
		}

		float Input_GetMousePositionX()
		{
			float mousePositionX = Input::GetMousePosition().X;
			return mousePositionX;
		}

		float Input_GetMousePositionY()
		{
			float mousePositionY = Input::GetMousePosition().Y;
			return mousePositionY;
		}

		float Input_GetMouseWorldPositionX()
		{
			rtmcpp::Vec2 mousePosition = Input::GetViewportMousePosition();
			return mousePosition.X;
		}

		float Input_GetMouseWorldPositionY()
		{
			rtmcpp::Vec2 mousePosition = Input::GetViewportMousePosition();
			return mousePosition.Y;
		}

#pragma endregion

#pragma region Application

		//float Application_GetFPS()
		//{
		//	return Application::Get().GetFPS();
		//}
		//
		//float Application_GetFrameTime()
		//{
		//	return Application::Get().GetFrameTime();
		//}
		//
		//float Application_GetMinFrameTime()
		//{
		//	return Application::Get().GetMinFrameTime();
		//}
		//
		//float Application_GetMaxFrameTime()
		//{
		//	return Application::Get().GetMaxFrameTime();
		//}

#pragma endregion

#pragma region Gamepad

		bool Input_IsControllerPresent(int id) { return Input::IsControllerPresent(id); }

		Coral::Array<int32_t> Input_GetConnectedControllerIDs()
		{
			return Coral::Array<int32_t>::New(Input::GetConnectedControllerIDs());
		}

		Coral::String Input_GetControllerName(int id)
		{
			auto name = Input::GetControllerName(id);
			if (name.empty())
				return {};

			return Coral::String::New(name);
		}

		bool Input_IsControllerButtonPressed(int id, int button) { return Input::IsControllerButtonPressed(id, button); }
		bool Input_IsControllerButtonHeld(int id, int button) { return Input::IsControllerButtonHeld(id, button); }
		bool Input_IsControllerButtonDown(int id, int button) { return Input::IsControllerButtonDown(id, button); }
		bool Input_IsControllerButtonReleased(int id, int button) { return Input::IsControllerButtonReleased(id, button); }

		float Input_GetControllerAxis(int id, int axis) { return Input::GetControllerAxis(id, axis); }
		uint8_t Input_GetControllerHat(int id, int hat) { return Input::GetControllerHat(id, hat); }

		float Input_GetControllerDeadzone(int id, int axis) { return Input::GetControllerDeadzone(id, axis); }
		void Input_SetControllerDeadzone(int id, int axis, float deadzone) { return Input::SetControllerDeadzone(id, axis, deadzone); }

#pragma endregion

#pragma region Scene

		void Scene_LoadScene(AssetHandle assetHandle)
		{
			RefPtr<Scene> activeScene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(activeScene, "No active scene!");
			//NZ_ICALL_VALIDATE_PARAM_V(assetHandle, "nullptr");
			//NZ_ICALL_VALIDATE_PARAM(AssetManager::IsAssetHandleValid(assetHandle));
			if(AssetManager::IsAssetHandleValid(assetHandle))
				activeScene->OnSceneTransition(assetHandle);
		}

		Coral::String Scene_GetCursor()
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			return Coral::String::New(ScriptGlue::s_SetCursorPath);
		}

		void Scene_SetCursor(Coral::String filepath)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			ScriptGlue::s_SetCursorPath = filepath;
			ScriptGlue::s_ChangedCursor = true;
		}

		float Scene_GetMouseHotSpotX()
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			rtmcpp::Vec2 mouseHotSpot = ScriptGlue::s_CursorHotSpot;
			return mouseHotSpot.X;
		}

		float Scene_GetMouseHotSpotY()
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			rtmcpp::Vec2 mouseHotSpot = ScriptGlue::s_CursorHotSpot;
			return mouseHotSpot.Y;
		}

		void Scene_SetMouseHotSpot(float hotSpotX, float hotSpotY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			ScriptGlue::s_CursorHotSpot = rtmcpp::Vec2(hotSpotX, hotSpotY);
		}

		//void Scene_ChangeCursor(Coral::NativeString filepath, glm::vec2* hotSpot)
		//{
		//	RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
		//	NZ_CORE_ASSERT(scene, "No active scene!");
		//
		//	std::string path = ConvertCoralStringToCppString(filepath);
		//
		//	if (ScriptGlue::s_SetCursorPath != path)
		//		ScriptGlue::s_SetCursorPath = path;
		//
		//	if (ScriptGlue::s_CursorHotSpot.x != (int)hotSpot->x && ScriptGlue::s_CursorHotSpot.y != (int)hotSpot->y)
		//		ScriptGlue::s_CursorHotSpot = glm::ivec2((int)hotSpot->x, (int)hotSpot->y);
		//
		//	ScriptGlue::s_ChangedCursor = true;
		//}

		void Scene_ChangeCursor(Coral::String filepath, float hotSpotX, float hotspotY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			if (ScriptGlue::s_SetCursorPath != filepath)
				ScriptGlue::s_SetCursorPath = filepath;

			if (ScriptGlue::s_CursorHotSpot.X != hotSpotX || ScriptGlue::s_CursorHotSpot.Y != hotspotY)
				ScriptGlue::s_CursorHotSpot = rtmcpp::Vec2(hotSpotX, hotspotY);

			ScriptGlue::s_ChangedCursor = true;
		}

		//static void Scene_CloseApplication()
		//{
		//	if (ScriptEngine::s_IsRuntime == true)
		//	{
		//		Application::Get().Close();
		//	}
		//}

		Coral::String Scene_GetCurrentFilename()
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			return Coral::String::New(scene->GetName());
		}

		Coral::String Scene_GetName()
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			return Coral::String::New(scene->GetName());
		}

		void Scene_SetName(Coral::String path)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			scene->SetName(path);
		}

		bool Scene_IsGamePaused()
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			return scene->IsGamePaused();
		}

		void Scene_SetPauseGame(bool shouldPause)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			scene->ShouldGameBePaused(shouldPause);
		}

		uint64_t Scene_CreateEntity(Coral::String tag)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene, "No active scene!");

			return scene->CreateEntity(tag).GetEntityHandle();
		}

		bool Scene_IsEntityValid(uint64_t entityID)
		{
			if (entityID == 0)
				return false;
		
			return (bool)(ScriptEngine::GetInstance().GetCurrentScene()->TryGetEntityWithID(entityID));
		}

		uint64_t Scene_GetHoveredEntity()
		{
			if (!s_HoveredEntity)
				return 0;

			return s_HoveredEntity.GetEntityHandle();
		}

		uint64_t Scene_GetSelectedEntity()
		{
			if (!s_SelectedEntity)
				return 0;

			return s_SelectedEntity.GetEntityHandle();
		}

		void Scene_SetSelectedEntity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			s_SelectedEntity = entity;
		}

		void Scene_RenderHoveredEntityOutline(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);

			static uint64_t finalEntityID = 0;

			if (entityID > 500)
				finalEntityID = entityID;

			Entity entity = scene->GetEntityByID(finalEntityID);
			NZ_CORE_ASSERT(entity);

			scene->RenderHoveredEntityOutline(entity, rtmcpp::Vec4(colorX, colorY, colorZ, colorW));
		}

		void Scene_RenderSelectedEntityOutline(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);

			static uint64_t finalEntityID = 0;

			if (entityID > 500)
				finalEntityID = entityID;

			Entity entity = scene->GetEntityByID(finalEntityID);
			NZ_CORE_ASSERT(entity);

			scene->RenderSelectedEntityOutline(entity, rtmcpp::Vec4(colorX, colorY, colorZ, colorW));
		}

		//void Scene_GetEntityComponent(uint64_t entityID, void* component)
		//{
		//	RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
		//	NZ_CORE_ASSERT(scene);
		//	Entity entity = scene->GetEntityByID(entityID);
		//	NZ_CORE_ASSERT(entity);
		//
		//	Coral::ManagedType* managedType = reinterpret_cast<Coral::ManagedType*>(component);
		//	NZ_CORE_ASSERT(s_GetComponentFuncs.find(managedType) != s_GetComponentFuncs.end());
		//	s_GetComponentFuncs.at(managedType)(entity);
		//}

#pragma endregion

#pragma region Entity

		static inline Entity GetEntity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_VERIFY(scene, "No active scene!");
			return scene->TryGetEntityWithID(entityID);
		};

		void Entity_CreateComponent(uint64_t entityID, Coral::ReflectionType componentType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return;

			Coral::Type& type = componentType;

			if (!type)
				return;

			//Coral::ScopedString typeName = type.GetFullName();

			if (!s_CreateComponentFuncs.contains(type.GetTypeId()))
			{
				//ErrorWithTrace("Cannot check if entity '{}' has a component of type '{}'. That component hasn't been registered with the engine.", entity.Name(), std::string(typeName));
				return;
			}

			if (s_HasComponentFuncs.at(type.GetTypeId())(entity))
			{
				//WarnWithTrace("Attempting to add duplicate component '{}' to entity '{}', ignoring.", std::string(typeName), entity.Name());
				return;
			}

			return s_CreateComponentFuncs.at(type.GetTypeId())(entity);
		}

		bool Entity_HasComponent(uint64_t entityID, Coral::ReflectionType componentType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return false;

			Coral::Type& type = componentType;

			if (!type)
				return false;

			//Coral::ScopedString typeName = type.GetFullName();

			if (!s_HasComponentFuncs.contains(type.GetTypeId()))
			{
				//ErrorWithTrace("Cannot check if entity '{}' has a component of type '{}'. That component hasn't been registered with the engine.", entity.Name(), std::string(typeName));
				return false;
			}

			return s_HasComponentFuncs.at(type.GetTypeId())(entity);
		}

		bool Entity_RemoveComponent(uint64_t entityID, Coral::ReflectionType componentType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return false;

			Coral::Type& type = componentType;

			if (!type)
				return false;

			//Coral::ScopedString typeName = type.GetFullName();

			if (!s_RemoveComponentFuncs.contains(type.GetTypeId()))
			{
				//ErrorWithTrace("Cannot remove a component of type '{}' from entity '{}'. That component hasn't been registered with the engine.", std::string(typeName), entity.Name());
				return false;
			}

			if (!s_HasComponentFuncs.at(type.GetTypeId())(entity))
			{
				//WarnWithTrace("Tried to remove component '{}' from entity '{}' even though it doesn't have that component.", std::string(typeName), entity.Name());
				return false;
			}

			s_RemoveComponentFuncs.at(type.GetTypeId())(entity);
			return true;
		}

		void Entity_DestroyEntity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.DestroyEntity();
		}

		uint64_t Entity_FindEntityByName(Coral::String name)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);

			std::string nameCStr = name;
			NZ_CORE_WARN("Entity name: {}", nameCStr);

			Entity entity = scene->FindEntityByName(nameCStr);

			if (entity)
				return entity.GetEntityHandle();

			return 0;
		}

		uint64_t Entity_FindEntityByTag(Coral::String tag)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);

			std::string nameCStr = tag;
			NZ_CORE_WARN("Entity name: {}", nameCStr);

			Entity entity = scene->FindEntityByTag(nameCStr);

			if (entity)
				return entity.GetComponent<IDComponent>().ID;

			return 0;
		}

#pragma endregion

#pragma region Tag

		Coral::String TagComponent_GetTag(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& tagComponent = entity.GetComponent<TagComponent>();

			return Coral::String::New(tagComponent.Tag.c_str());
		}

		void TagComponent_SetTag(uint64_t entityID, Coral::String tag)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& tagComponent = entity.GetComponent<TagComponent>();

			tagComponent.Tag = tag;
		}

#pragma endregion

#pragma region Transform

		bool TransformComponent_GetIsEnabled(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Enabled;
		}

		void TransformComponent_SetIsEnabled(uint64_t entityID, bool isEnabled)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TransformComponent>().Enabled = isEnabled;
		}

		void TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			*outTransform = entity.GetComponent<TransformComponent>();
		}

		void TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TransformComponent>() = *inTransform;
		}

		float TransformComponent_GetTranslationX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Translation.X;
		}

		float TransformComponent_GetTranslationY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Translation.Y;
		}

		float TransformComponent_GetTranslationZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Translation.Z;
		}

		void TransformComponent_SetTranslation(uint64_t entityID, float translationX, float translationY, float translationZ)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TransformComponent>().Translation = rtmcpp::Vec4(translationX, translationY, translationZ, 1.0f);
		}

		float TransformComponent_GetRotationX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Rotation.X;
		}

		float TransformComponent_GetRotationY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Rotation.Y;
		}

		float TransformComponent_GetRotationZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Rotation.Z;
		}

		void TransformComponent_SetRotation(uint64_t entityID, float rotationX, float rotationY, float rotationZ)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TransformComponent>().Rotation = rtmcpp::Vec3(rotationX, rotationY, rotationZ);
		}

		float TransformComponent_GetScaleX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Scale.X;
		}

		float TransformComponent_GetScaleY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Scale.Y;
		}

		float TransformComponent_GetScaleZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TransformComponent>().Scale.Z;
		}

		void TransformComponent_SetScale(uint64_t entityID, float scaleX, float scaleY, float scaleZ)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TransformComponent>().Scale = rtmcpp::Vec3(scaleX, scaleY, scaleZ);
		}

#pragma endregion

#pragma region CameraComponent

		bool CameraComponent_GetIsPrimary(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CameraComponent>().Primary;
		}

		void CameraComponent_SetPrimary(uint64_t entityID, bool primary)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CameraComponent>().Primary = primary;
		}

		bool CameraComponent_GetFixedAspectRatio(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CameraComponent>().FixedAspectRatio;
		}

		void CameraComponent_SetFixedAspectRatio(uint64_t entityID, bool fixedAspectRatio)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CameraComponent>().FixedAspectRatio = fixedAspectRatio;
		}

#pragma endregion

#pragma region SpriteRenderer

		float SpriteRendererComponent_GetOffsetX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().SpriteTranslation.X;
		}

		float SpriteRendererComponent_GetOffsetY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().SpriteTranslation.Y;
		}

		float SpriteRendererComponent_GetOffsetZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().SpriteTranslation.Z;
		}

		void SpriteRendererComponent_SetOffset(uint64_t entityID, float offsetX, float offsetY, float offsetZ)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().SpriteTranslation = rtmcpp::Vec4(offsetX, offsetY, offsetZ, 1.0f);
		}

		float SpriteRendererComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().Color.X;
		}

		float SpriteRendererComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().Color.Y;
		}

		float SpriteRendererComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().Color.Z;
		}

		float SpriteRendererComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().Color.W;
		}

		void SpriteRendererComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float SpriteRendererComponent_GetUVX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().UV.X;
		}

		float SpriteRendererComponent_GetUVY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().UV.Y;
		}

		void SpriteRendererComponent_SetUV(uint64_t entityID, float uvX, float uvY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().UV = rtmcpp::Vec2(uvX, uvY);
		}

		bool SpriteRendererComponent_GetUseParallax(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.UseParallaxScrolling;
		}

		void SpriteRendererComponent_SetUseParallax(uint64_t entityID, bool useParallax)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.UseParallaxScrolling = useParallax;
		}

		float SpriteRendererComponent_GetParallaxSpeedX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.ParallaxSpeed.X;
		}

		float SpriteRendererComponent_GetParallaxSpeedY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.ParallaxSpeed.Y;
		}

		void SpriteRendererComponent_SetParallaxSpeed(uint64_t entityID, float speedX, float speedY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.ParallaxSpeed = rtmcpp::Vec2(speedX, speedY);
		}

		float SpriteRendererComponent_GetParallaxDivision(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.ParallaxDivision;
		}

		void SpriteRendererComponent_SetParallaxDivision(uint64_t entityID, float division)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.ParallaxDivision = division;
		}

		bool SpriteRendererComponent_GetUseTextureAtlasAnimation(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.UseTextureAtlasAnimation;
		}

		void SpriteRendererComponent_SetUseTextureAtlasAnimation(uint64_t entityID, bool useTextureAtlasAnimation)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.UseTextureAtlasAnimation = useTextureAtlasAnimation;
		}

		float SpriteRendererComponent_GetAnimationSpeed(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.AnimationSpeed;
		}

		void SpriteRendererComponent_SetAnimationSpeed(uint64_t entityID, float animSpeed)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.AnimationSpeed = animSpeed;
		}

		int SpriteRendererComponent_GetNumTiles(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().m_AnimationData.NumberOfTiles;
		}

		void SpriteRendererComponent_SetNumTiles(uint64_t entityID, int numTiles)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.NumberOfTiles = numTiles;
		}

		int SpriteRendererComponent_GetStartIndexX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<SpriteRendererComponent>().m_AnimationData.StartIndexX);
		}

		void SpriteRendererComponent_SetStartIndexX(uint64_t entityID, int startIndex)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.StartIndexX = (int)(static_cast<float>(startIndex));
		}

		int SpriteRendererComponent_GetStartIndexY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<SpriteRendererComponent>().m_AnimationData.StartIndexY);
		}

		void SpriteRendererComponent_SetStartIndexY(uint64_t entityID, int startIndex)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.StartIndexY = (int)(static_cast<float>(startIndex));
		}

		int SpriteRendererComponent_GetColumn(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<SpriteRendererComponent>().m_AnimationData.Rows);
		}

		void SpriteRendererComponent_SetColumn(uint64_t entityID, int column)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.Rows = (int)(static_cast<float>(column));
		}

		int SpriteRendererComponent_GetRow(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<SpriteRendererComponent>().m_AnimationData.Columns);
		}

		void SpriteRendererComponent_SetRow(uint64_t entityID, int row)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().m_AnimationData.Columns = (int)(static_cast<float>(row));
		}

		float SpriteRendererComponent_GetSaturation(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().Saturation;
		}

		void SpriteRendererComponent_SetSaturation(uint64_t entityID, float saturation)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().Saturation = saturation;
		}

		AssetHandle SpriteRendererComponent_GetTextureAssetHandle(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<SpriteRendererComponent>().TextureHandle;
		}

		void SpriteRendererComponent_SetTextureAssetHandle(uint64_t entityID, AssetHandle textureHandle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().TextureHandle = textureHandle;
		}

		uint64_t SpriteRendererComponent_GetTextureAssetID(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return (uint64_t)entity.GetComponent<SpriteRendererComponent>().TextureHandle;
		}

		void SpriteRendererComponent_SetTextureAssetID(uint64_t entityID, uint64_t textureHandle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<SpriteRendererComponent>().TextureHandle = (AssetHandle)textureHandle;
		}

#pragma endregion

#pragma region CircleRenderer

		float CircleRendererComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().Color.X;
		}

		float CircleRendererComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().Color.Y;
		}

		float CircleRendererComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().Color.Z;
		}

		float CircleRendererComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().Color.W;
		}

		void CircleRendererComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float CircleRendererComponent_GetUVX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().UV.X;
		}

		float CircleRendererComponent_GetUVY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().UV.Y;
		}

		void CircleRendererComponent_SetUV(uint64_t entityID, float uvX, float uvY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().UV = rtmcpp::Vec2(uvX, uvY);
		}

		bool CircleRendererComponent_GetUseParallax(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.UseParallaxScrolling;
		}

		void CircleRendererComponent_SetUseParallax(uint64_t entityID, bool useParallax)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.UseParallaxScrolling = useParallax;
		}

		float CircleRendererComponent_GetParallaxSpeedX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.ParallaxSpeed.X;
		}

		float CircleRendererComponent_GetParallaxSpeedY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.ParallaxSpeed.Y;
		}

		void CircleRendererComponent_SetParallaxSpeed(uint64_t entityID, float speedX, float speedY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.ParallaxSpeed = rtmcpp::Vec2(speedX, speedY);
		}

		float CircleRendererComponent_GetParallaxDivision(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.ParallaxDivision;
		}

		void CircleRendererComponent_SetParallaxDivision(uint64_t entityID, float division)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.ParallaxDivision = division;
		}

		bool CircleRendererComponent_GetUseTextureAtlasAnimation(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.UseTextureAtlasAnimation;
		}

		void CircleRendererComponent_SetUseTextureAtlasAnimation(uint64_t entityID, bool useTextureAtlasAnimation)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.UseTextureAtlasAnimation = useTextureAtlasAnimation;
		}

		float CircleRendererComponent_GetAnimationSpeed(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.AnimationSpeed;
		}

		void CircleRendererComponent_SetAnimationSpeed(uint64_t entityID, float animSpeed)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.AnimationSpeed = animSpeed;
		}

		int CircleRendererComponent_GetNumTiles(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleRendererComponent>().m_AnimationData.NumberOfTiles;
		}

		void CircleRendererComponent_SetNumTiles(uint64_t entityID, int numTiles)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.NumberOfTiles = numTiles;
		}

		int CircleRendererComponent_GetStartIndexX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<CircleRendererComponent>().m_AnimationData.StartIndexX);
		}

		void CircleRendererComponent_SetStartIndexX(uint64_t entityID, int startIndex)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.StartIndexX = (int)(static_cast<float>(startIndex));
		}

		int CircleRendererComponent_GetStartIndexY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<CircleRendererComponent>().m_AnimationData.StartIndexY);
		}

		void CircleRendererComponent_SetStartIndexY(uint64_t entityID, int startIndex)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.StartIndexY = (int)(static_cast<float>(startIndex));
		}

		int CircleRendererComponent_GetColumn(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<CircleRendererComponent>().m_AnimationData.Rows);
		}

		void CircleRendererComponent_SetColumn(uint64_t entityID, int column)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.Rows = (int)(static_cast<float>(column));
		}

		int CircleRendererComponent_GetRow(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<CircleRendererComponent>().m_AnimationData.Columns);
		}

		void CircleRendererComponent_SetRow(uint64_t entityID, int row)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleRendererComponent>().m_AnimationData.Columns = (int)(static_cast<float>(row));
		}

#pragma endregion

#pragma region TriangleRenderer

		float TriangleRendererComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().Color.X;
		}

		float TriangleRendererComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().Color.Y;
		}

		float TriangleRendererComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().Color.Z;
		}

		float TriangleRendererComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().Color.W;
		}

		void TriangleRendererComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TriangleRendererComponent>().Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float TriangleRendererComponent_GetUVX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().UV.X;
		}

		float TriangleRendererComponent_GetUVY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().UV.Y;
		}

		void TriangleRendererComponent_SetUV(uint64_t entityID, float uvX, float uvY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TriangleRendererComponent>().UV = rtmcpp::Vec2(uvX, uvY);
		}

		float TriangleRendererComponent_GetSaturation(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<TriangleRendererComponent>().Saturation;
		}

		void TriangleRendererComponent_SetSaturation(uint64_t entityID, float saturation)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TriangleRendererComponent>().Saturation = saturation;
		}

#pragma endregion

#pragma region LineRendererComponent

		float LineRendererComponent_GetLineThickness(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<LineRendererComponent>().LineThickness;
		}

		void LineRendererComponent_SetLineThickness(uint64_t entityID, float lineThickness)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<LineRendererComponent>().LineThickness = lineThickness;
		}

#pragma endregion

#pragma region TextComponent

		Coral::String TextComponent_GetText(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return Coral::String::New(entity.GetComponent<TextComponent>().TextString.c_str());
		}

		void TextComponent_SetText(uint64_t entityID, Coral::String textString)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<TextComponent>().TextString = textString;
		}

		float TextComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			return tc.Color.X;
		}

		float TextComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			return tc.Color.Y;
		}

		float TextComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			return tc.Color.Z;
		}

		float TextComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			return tc.Color.W;
		}

		void TextComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			tc.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float TextComponent_GetKerning(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			return tc.Kerning;
		}

		void TextComponent_SetKerning(uint64_t entityID, float kerning)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			tc.Kerning = kerning;
		}

		float TextComponent_GetLineSpacing(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			return tc.LineSpacing;
		}

		void TextComponent_SetLineSpacing(uint64_t entityID, float lineSpacing)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);
			NZ_CORE_ASSERT(entity.HasComponent<TextComponent>());

			auto& tc = entity.GetComponent<TextComponent>();
			tc.LineSpacing = lineSpacing;
		}

#pragma endregion

#pragma region VideoRenderer

		float VideoRendererComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().Color.X;
		}

		float VideoRendererComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().Color.Y;
		}

		float VideoRendererComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().Color.Z;
		}

		float VideoRendererComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().Color.W;
		}

		void VideoRendererComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		uint64_t VideoRendererComponent_GetVideoHandle(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return (uint64_t)entity.GetComponent<VideoRendererComponent>().Video;
		}

		void VideoRendererComponent_SetVideoHandle(uint64_t entityID, uint64_t videoHandle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().Video = (AssetHandle)videoHandle;
		}

		float VideoRendererComponent_GetSaturation(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().Saturation;
		}

		void VideoRendererComponent_SetSaturation(uint64_t entityID, float saturation)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().Saturation = saturation;
		}

		bool VideoRendererComponent_GetUseBillboard(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().m_VideoData.UseBillboard;
		}

		void VideoRendererComponent_SetUseBillboard(uint64_t entityID, bool useBillboard)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().m_VideoData.UseBillboard = useBillboard;
		}

		/*bool VideoRendererComponent_GetUseExternalAudio(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().m_VideoData.UseExternalAudio;
		}

		void VideoRendererComponent_SetUseExternalAudio(uint64_t entityID, bool useExternalAudio)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().m_VideoData.UseExternalAudio = useExternalAudio;
		}*/

		bool VideoRendererComponent_GetPlayVideo(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().m_VideoData.PlayVideo;
		}

		void VideoRendererComponent_SetPlayVideo(uint64_t entityID, bool playVideo)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().m_VideoData.PlayVideo = playVideo;
		}

		bool VideoRendererComponent_GetRepeatVideo(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().m_VideoData.RepeatVideo;
		}

		void VideoRendererComponent_SetRepeatVideo(uint64_t entityID, bool repeatVideo)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().m_VideoData.RepeatVideo = repeatVideo;
		}

		bool VideoRendererComponent_GetPauseVideo(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().m_VideoData.VideoPaused;
		}

		void VideoRendererComponent_SetPauseVideo(uint64_t entityID, bool pauseVideo)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().m_VideoData.VideoPaused = pauseVideo;
		}

		float VideoRendererComponent_GetVolume(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<VideoRendererComponent>().m_VideoData.Volume;
		}

		void VideoRendererComponent_SetVolume(uint64_t entityID, float volume)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<VideoRendererComponent>().m_VideoData.Volume = volume;
		}

#pragma endregion

#pragma region ParticleSystem

		float ParticleSystemComponent_GetVelocityX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().Velocity.X;
		}

		float ParticleSystemComponent_GetVelocityY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().Velocity.Y;
		}

		float ParticleSystemComponent_GetVelocityZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().Velocity.Z;
		}

		void ParticleSystemComponent_SetVelocity(uint64_t entityID, float velocityX, float velocityY, float velocityZ)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().Velocity = rtmcpp::Vec3(velocityX, velocityY, velocityZ);
		}

		float ParticleSystemComponent_GetVelocityVariationX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().VelocityVariation.X;
		}

		float ParticleSystemComponent_GetVelocityVariationY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().VelocityVariation.Y;
		}

		float ParticleSystemComponent_GetVelocityVariationZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().VelocityVariation.Z;
		}

		void ParticleSystemComponent_SetVelocityVariation(uint64_t entityID, float velocityVariationX, float velocityVariationY, float velocityVariationZ)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().VelocityVariation = rtmcpp::Vec3(velocityVariationX, velocityVariationY, velocityVariationZ);
		}

		float ParticleSystemComponent_GetColorBeginX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorBegin.X;
		}

		float ParticleSystemComponent_GetColorBeginY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorBegin.Y;
		}

		float ParticleSystemComponent_GetColorBeginZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorBegin.Z;
		}

		float ParticleSystemComponent_GetColorBeginW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorBegin.W;
		}

		void ParticleSystemComponent_SetColorBegin(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().ColorBegin = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float ParticleSystemComponent_GetColorEndX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorEnd.X;
		}

		float ParticleSystemComponent_GetColorEndY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorEnd.Y;
		}

		float ParticleSystemComponent_GetColorEndZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorEnd.Z;
		}

		float ParticleSystemComponent_GetColorEndW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ColorEnd.W;
		}

		void ParticleSystemComponent_SetColorEnd(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().ColorEnd = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float ParticleSystemComponent_GetSizeBegin(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().SizeBegin;
		}

		void ParticleSystemComponent_SetSizeBegin(uint64_t entityID, float sizeBegin)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().SizeBegin = sizeBegin;
		}

		float ParticleSystemComponent_GetSizeEnd(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().SizeEnd;
		}

		void ParticleSystemComponent_SetSizeEnd(uint64_t entityID, float sizeEnd)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().SizeEnd = sizeEnd;
		}

		float ParticleSystemComponent_GetSizeVariation(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().SizeVariation;
		}

		void ParticleSystemComponent_SetSizeVariation(uint64_t entityID, float sizeVariation)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().SizeVariation = sizeVariation;
		}

		float ParticleSystemComponent_GetLifeTime(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().LifeTime;
		}

		void ParticleSystemComponent_SetLifeTime(uint64_t entityID, float lifeTime)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().LifeTime = lifeTime;
		}

		int ParticleSystemComponent_GetParticleSize(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().ParticleSize;
		}

		void ParticleSystemComponent_SetParticleSize(uint64_t entityID, int particleSize)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().ParticleSize = particleSize;
		}

		bool ParticleSystemComponent_GetUseLinear(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().UseLinear;
		}

		void ParticleSystemComponent_SetUseLinear(uint64_t entityID, bool useLinear)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().UseLinear = useLinear;
		}

		uint64_t ParticleSystemComponent_GetTextureHandle(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return (uint64_t)entity.GetComponent<ParticleSystemComponent>().TextureHandle;
		}

		void ParticleSystemComponent_SetTextureHandle(uint64_t entityID, uint64_t textureHandle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().TextureHandle = (AssetHandle)textureHandle;
		}

		bool ParticleSystemComponent_GetUseBillboard(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ParticleSystemComponent>().UseBillboard;
		}

		void ParticleSystemComponent_SetUseBillboard(uint64_t entityID, bool useBillboard)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ParticleSystemComponent>().UseBillboard = useBillboard;
		}

#pragma endregion

#pragma region ButtonWidgetComponent

		float ButtonWidgetComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Color.X;
		}

		float ButtonWidgetComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Color.Y;
		}

		float ButtonWidgetComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Color.Z;
		}

		float ButtonWidgetComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Color.W;
		}

		void ButtonWidgetComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ButtonWidgetComponent>().Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float ButtonWidgetComponent_GetUVX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().UV.X;
		}

		float ButtonWidgetComponent_GetUVY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().UV.Y;
		}

		void ButtonWidgetComponent_SetUV(uint64_t entityID, float uvX, float uvY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ButtonWidgetComponent>().UV = rtmcpp::Vec2(uvX, uvY);
		}

		float ButtonWidgetComponent_GetRadius(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Radius;
		}

		void ButtonWidgetComponent_SetRadius(uint64_t entityID, float radius)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ButtonWidgetComponent>().Radius = radius;
		}

		float ButtonWidgetComponent_GetDimensionsX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Dimensions.X;
		}

		float ButtonWidgetComponent_GetDimensionsY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().Dimensions.Y;
		}

		void ButtonWidgetComponent_SetDimensions(uint64_t entityID, float dimsX, float dimsY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ButtonWidgetComponent>().Dimensions = rtmcpp::Vec2(dimsX, dimsY);
		}

		bool ButtonWidgetComponent_GetInvertCorners(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().InvertCorners;
		}

		void ButtonWidgetComponent_SetInvertCorners(uint64_t entityID, bool invertCorners)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ButtonWidgetComponent>().InvertCorners = invertCorners;
		}

		bool ButtonWidgetComponent_GetUseLinear(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<ButtonWidgetComponent>().UseLinear;
		}

		void ButtonWidgetComponent_SetUseLinear(uint64_t entityID, bool useLinear)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<ButtonWidgetComponent>().UseLinear = useLinear;
		}

#pragma endregion

#pragma region CircleWidgetComponent

		float CircleWidgetComponent_GetColorX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Color.X;
		}

		float CircleWidgetComponent_GetColorY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Color.Y;
		}

		float CircleWidgetComponent_GetColorZ(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Color.Z;
		}

		float CircleWidgetComponent_GetColorW(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Color.W;
		}

		void CircleWidgetComponent_SetColor(uint64_t entityID, float colorX, float colorY, float colorZ, float colorW)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleWidgetComponent>().Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);
		}

		float CircleWidgetComponent_GetUVX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().UV.X;
		}

		float CircleWidgetComponent_GetUVY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().UV.Y;
		}

		void CircleWidgetComponent_SetUV(uint64_t entityID, float uvX, float uvY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleWidgetComponent>().UV = rtmcpp::Vec2(uvX, uvY);
		}

		float CircleWidgetComponent_GetThickness(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Thickness;
		}

		void CircleWidgetComponent_SetThickness(uint64_t entityID, float thickness)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleWidgetComponent>().Thickness = thickness;
		}

		float CircleWidgetComponent_GetFade(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Fade;
		}

		void CircleWidgetComponent_SetFade(uint64_t entityID, float fade)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleWidgetComponent>().Fade = fade;
		}

		float CircleWidgetComponent_GetRadius(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().Radius;
		}

		void CircleWidgetComponent_SetRadius(uint64_t entityID, float radius)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleWidgetComponent>().Radius = radius;
		}

		bool CircleWidgetComponent_GetUseLinear(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<CircleWidgetComponent>().UseLinear;
		}

		void CircleWidgetComponent_SetUseLinear(uint64_t entityID, bool useLinear)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<CircleWidgetComponent>().UseLinear = useLinear;
		}

#pragma endregion

#pragma region Rigidbody2D

		void Rigidbody2DComponent_ApplyLinearImpulse(uint64_t entityID, float impulseX, float impulseY, float offsetX, float offsetY, bool wake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			body->ApplyLinearImpulse(b2Vec2(impulseX, impulseY), body->GetWorldCenter() + b2Vec2(offsetX, offsetY), wake);
		}

		void Rigidbody2DComponent_ApplyLinearImpulseToCenter(uint64_t entityID, float impulseX, float impulseY, bool wake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			body->ApplyLinearImpulseToCenter(b2Vec2(impulseX, impulseY), wake);
		}

		float Rigidbody2DComponent_GetLinearVelocityX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			const b2Vec2& linearVelocity = body->GetLinearVelocity();
			return linearVelocity.x;
		}

		float Rigidbody2DComponent_GetLinearVelocityY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			const b2Vec2& linearVelocity = body->GetLinearVelocity();
			return linearVelocity.y;
		}

		void Rigidbody2DComponent_SetLinearVelocity(uint64_t entityID, float velocityX, float velocityY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetLinearVelocity({ velocityX, velocityY });
		}

		Rigidbody2DComponent::BodyType Rigidbody2DComponent_GetType(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			return Utils::Rigidbody2DTypeFromBox2DBody(body->GetType());
		}

		void Rigidbody2DComponent_SetType(uint64_t entityID, Rigidbody2DComponent::BodyType bodyType)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			body->SetType(Utils::Rigidbody2DTypeToBox2DBody(bodyType));
		}

		float Rigidbody2DComponent_GetGravityX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& gravity = scene->GetPhysics2DGravity();
			return gravity.X;
		}

		float Rigidbody2DComponent_GetGravityY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& gravity = scene->GetPhysics2DGravity();
			return gravity.Y;
		}

		void Rigidbody2DComponent_SetGravity(uint64_t entityID, float gravityX, float gravityY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			scene->SetPhysics2DGravity({ gravityX, gravityY });
		}

		bool Rigidbody2DComponent_GetEnabled(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<Rigidbody2DComponent>().SetEnabled;
		}

		void Rigidbody2DComponent_SetEnabled(uint64_t entityID, bool setEnabled)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<Rigidbody2DComponent>().SetEnabled = setEnabled;
		}

#pragma endregion

#pragma region BoxCollider2D

		float BoxCollider2DComponent_GetOffsetX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Offset.X;

			return 0.0f;
		}

		float BoxCollider2DComponent_GetOffsetY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Offset.Y;

			return 0.0f;
		}

		void BoxCollider2DComponent_SetOffset(uint64_t entityID, float offsetX, float offsetY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().Offset = rtmcpp::Vec2(offsetX, offsetY);
		}

		float BoxCollider2DComponent_GetSizeX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Size.X;

			return 0.0f;
		}

		float BoxCollider2DComponent_GetSizeY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Size.Y;

			return 0.0f;
		}

		void BoxCollider2DComponent_SetSize(uint64_t entityID, float sizeX, float sizeY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().Size = rtmcpp::Vec2(sizeX, sizeY);
		}

		float BoxCollider2DComponent_GetDensity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Density;

			return 0.0f;
		}

		void BoxCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().Density = density;
		}

		float BoxCollider2DComponent_GetFriction(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Friction;
			
			return 0.0f;
		}

		void BoxCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().Friction = friction;
		}

		float BoxCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Restitution;

			return 0.0f;
		}

		void BoxCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().Restitution = restitution;
		}

		float BoxCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().RestitutionThreshold;

			return 0.0f;
		}

		void BoxCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().RestitutionThreshold = restitutionThreshold;
		}

		float BoxCollider2DComponent_GetCollisionRayX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().CollisionRay.X;

			return 0.0f;
		}

		float BoxCollider2DComponent_GetCollisionRayY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().CollisionRay.Y;

			return 0.0f;
		}

		void BoxCollider2DComponent_SetCollisionRay(uint64_t entityID, float rayX, float rayY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().CollisionRay = rtmcpp::Vec2(rayX, rayY);
		}

		bool BoxCollider2DComponent_GetAwake(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				return entity.GetComponent<BoxCollider2DComponent>().Awake;

			return false;
		}

		void BoxCollider2DComponent_SetAwake(uint64_t entityID, bool setAwake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<BoxCollider2DComponent>())
				entity.GetComponent<BoxCollider2DComponent>().Awake = setAwake;
		}

#pragma endregion

#pragma region CircleCollider2D

		float CircleCollider2DComponent_GetOffsetX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Offset.X;

			return 0.0f;
		}

		float CircleCollider2DComponent_GetOffsetY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Offset.Y;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetOffset(uint64_t entityID, float offsetX, float offsetY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().Offset = rtmcpp::Vec2(offsetX, offsetY);
		}

		float CircleCollider2DComponent_GetRadius(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Radius;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetRadius(uint64_t entityID, float radius)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().Radius = radius;
		}

		float CircleCollider2DComponent_GetDensity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Density;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().Density = density;
		}

		float CircleCollider2DComponent_GetFriction(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Friction;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().Friction = friction;
		}

		float CircleCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Restitution;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().Restitution = restitution;
		}

		float CircleCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().RestitutionThreshold;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().RestitutionThreshold = restitutionThreshold;
		}

		float CircleCollider2DComponent_GetCollisionRayX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().CollisionRay.X;

			return 0.0f;
		}

		float CircleCollider2DComponent_GetCollisionRayY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().CollisionRay.Y;

			return 0.0f;
		}

		void CircleCollider2DComponent_SetCollisionRay(uint64_t entityID, float rayX, float rayY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().CollisionRay = rtmcpp::Vec2(rayX, rayY);
		}

		bool CircleCollider2DComponent_GetAwake(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				return entity.GetComponent<CircleCollider2DComponent>().Awake;

			return false;
		}

		void CircleCollider2DComponent_SetAwake(uint64_t entityID, bool setAwake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CircleCollider2DComponent>())
				entity.GetComponent<CircleCollider2DComponent>().Awake = setAwake;
		}


#pragma endregion

#pragma region TriangleCollider2D

		float TriangleCollider2DComponent_GetOffsetX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Offset.X;

			return 0.0f;
		}

		float TriangleCollider2DComponent_GetOffsetY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Offset.Y;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetOffset(uint64_t entityID, float offsetX, float offsetY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().Offset = rtmcpp::Vec2(offsetX, offsetY);
		}

		float TriangleCollider2DComponent_GetSizeX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Size.X;

			return 0.0f;
		}

		float TriangleCollider2DComponent_GetSizeY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Size.Y;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetSize(uint64_t entityID, float sizeX, float sizeY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().Size = rtmcpp::Vec2(sizeX, sizeY);
		}

		float TriangleCollider2DComponent_GetDensity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Density;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().Density = density;
		}

		float TriangleCollider2DComponent_GetFriction(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Friction;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().Friction = friction;
		}

		float TriangleCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Restitution;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().Restitution = restitution;
		}

		float TriangleCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().RestitutionThreshold;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().RestitutionThreshold = restitutionThreshold;
		}

		float TriangleCollider2DComponent_GetCollisionRayX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().CollisionRay.X;

			return 0.0f;
		}

		float TriangleCollider2DComponent_GetCollisionRayY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().CollisionRay.Y;

			return 0.0f;
		}

		void TriangleCollider2DComponent_SetCollisionRay(uint64_t entityID, float rayX, float rayY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().CollisionRay = rtmcpp::Vec2(rayX, rayY);
		}

		bool TriangleCollider2DComponent_GetAwake(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				return entity.GetComponent<TriangleCollider2DComponent>().Awake;

			return false;
		}

		void TriangleCollider2DComponent_SetAwake(uint64_t entityID, bool setAwake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<TriangleCollider2DComponent>())
				entity.GetComponent<TriangleCollider2DComponent>().Awake = setAwake;
		}

#pragma endregion

#pragma region CapsuleCollider2D

		float CapsuleCollider2DComponent_GetOffsetX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Offset.X;

			return 0.0f;
		}

		float CapsuleCollider2DComponent_GetOffsetY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Offset.X;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetOffset(uint64_t entityID, float offsetX, float offsetY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().Offset = rtmcpp::Vec2(offsetX, offsetY);
		}

		float CapsuleCollider2DComponent_GetSizeX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Size.X;

			return 0.0f;
		}

		float CapsuleCollider2DComponent_GetSizeY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Size.Y;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetSize(uint64_t entityID, float sizeX, float sizeY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().Size = rtmcpp::Vec2(sizeX, sizeY);
		}

		float CapsuleCollider2DComponent_GetDensity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Density;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().Density = density;
		}

		float CapsuleCollider2DComponent_GetFriction(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Friction;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().Friction = friction;
		}

		float CapsuleCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Restitution;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().Restitution = restitution;
		}

		float CapsuleCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().RestitutionThreshold;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().RestitutionThreshold = restitutionThreshold;
		}

		float CapsuleCollider2DComponent_GetCollisionRayX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().CollisionRay.X;

			return 0.0f;
		}

		float CapsuleCollider2DComponent_GetCollisionRayY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().CollisionRay.Y;

			return 0.0f;
		}

		void CapsuleCollider2DComponent_SetCollisionRay(uint64_t entityID, float rayX, float rayY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().CollisionRay = rtmcpp::Vec2(rayX, rayY);
		}

		bool CapsuleCollider2DComponent_GetAwake(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				return entity.GetComponent<CapsuleCollider2DComponent>().Awake;

			return false;
		}

		void CapsuleCollider2DComponent_SetAwake(uint64_t entityID, bool setAwake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<CapsuleCollider2DComponent>())
				entity.GetComponent<CapsuleCollider2DComponent>().Awake = setAwake;
		}

#pragma endregion

#pragma region MeshCollider2D

		float MeshCollider2DComponent_GetOffsetX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Offset.X;

			return 0.0f;
		}

		float MeshCollider2DComponent_GetOffsetY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Offset.Y;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetOffset(uint64_t entityID, float offsetX, float offsetY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().Offset = rtmcpp::Vec2(offsetX, offsetY);
		}

		float MeshCollider2DComponent_GetSizeX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Size.X;

			return 0.0f;
		}

		float MeshCollider2DComponent_GetSizeY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Size.Y;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetSize(uint64_t entityID, float sizeX, float sizeY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().Size = rtmcpp::Vec2(sizeX, sizeY);
		}

		float MeshCollider2DComponent_GetDensity(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Density;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().Density = density;
		}

		float MeshCollider2DComponent_GetFriction(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Friction;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().Friction = friction;
		}

		float MeshCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Restitution;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().Restitution = restitution;
		}

		float MeshCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().RestitutionThreshold;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().RestitutionThreshold = restitutionThreshold;
		}

		float MeshCollider2DComponent_GetCollisionRayX(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().CollisionRay.X;

			return 0.0f;
		}

		float MeshCollider2DComponent_GetCollisionRayY(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().CollisionRay.Y;

			return 0.0f;
		}

		void MeshCollider2DComponent_SetCollisionRay(uint64_t entityID, float rayX, float rayY)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().CollisionRay = rtmcpp::Vec2(rayX, rayY);
		}

		bool MeshCollider2DComponent_GetAwake(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				return entity.GetComponent<MeshCollider2DComponent>().Awake;

			return false;
		}

		void MeshCollider2DComponent_SetAwake(uint64_t entityID, bool setAwake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			if (entity.HasComponent<MeshCollider2DComponent>())
				entity.GetComponent<MeshCollider2DComponent>().Awake = setAwake;
		}

#pragma endregion

#pragma region AudioListener

		bool AudioListenerComponent_GetActive(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioListenerComponent>().Active;
		}

		void AudioListenerComponent_SetActive(uint64_t entityID, bool active)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<AudioListenerComponent>().Active = active;
		}

#pragma endregion

#pragma region AudioSource

		AssetHandle AudioSourceComponent_GetAssetHandle(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Audio;
		}

		void AudioSourceComponent_SetAssetHandle(uint64_t entityID, AssetHandle handle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<AudioSourceComponent>().Audio = handle;
		}

		float AudioSourceComponent_GetVolume(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.VolumeMultiplier;
		}

		void AudioSourceComponent_SetVolume(uint64_t entityID, float volume)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.VolumeMultiplier = volume;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetVolume(component.Config.VolumeMultiplier);
		}

		float AudioSourceComponent_GetPitch(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.PitchMultiplier;
		}

		void AudioSourceComponent_SetPitch(uint64_t entityID, float pitch)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.PitchMultiplier = pitch;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetVolume(pitch);
		}

		bool AudioSourceComponent_GetPlayOnAwake(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.PlayOnAwake;
		}

		void AudioSourceComponent_SetPlayOnAwake(uint64_t entityID, bool playOnAwake)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			entity.GetComponent<AudioSourceComponent>().Config.PlayOnAwake = playOnAwake;
		}

		bool AudioSourceComponent_GetLooping(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.Looping;
		}

		void AudioSourceComponent_SetLooping(uint64_t entityID, bool looping)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.Looping = looping;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetLooping(looping);
		}

		bool AudioSourceComponent_GetSpatialization(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.Spatialization;
		}

		void AudioSourceComponent_SetSpatialization(uint64_t entityID, bool spatialization)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.Spatialization = spatialization;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetSpatialization(spatialization);
		}

		int AudioSourceComponent_GetAttenuationModel(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return static_cast<int>(entity.GetComponent<AudioSourceComponent>().Config.AttenuationModel);
		}

		void AudioSourceComponent_SetAttenuationModel(uint64_t entityID, int attenuationModel)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.AttenuationModel = static_cast<AttenuationModelType>(attenuationModel);
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetAttenuationModel(component.Config.AttenuationModel);
		}

		float AudioSourceComponent_GetRollOff(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.RollOff;
		}

		void AudioSourceComponent_SetRollOff(uint64_t entityID, float rollOff)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.RollOff = rollOff;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetRollOff(rollOff);
		}

		float AudioSourceComponent_GetMinGain(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.MinGain;
		}

		void AudioSourceComponent_SetMinGain(uint64_t entityID, float minGain)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.MinGain = minGain;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetMinGain(minGain);
		}

		float AudioSourceComponent_GetMaxGain(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.MaxGain;
		}

		void AudioSourceComponent_SetMaxGain(uint64_t entityID, float maxGain)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.MaxGain = maxGain;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetMaxGain(maxGain);
		}

		float AudioSourceComponent_GetMinDistance(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.MinDistance;
		}

		void AudioSourceComponent_SetMinDistance(uint64_t entityID, float minDistance)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.MinDistance = minDistance;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetMinDistance(minDistance);
		}

		float AudioSourceComponent_GetMaxDistance(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.MaxDistance;
		}

		void AudioSourceComponent_SetMaxDistance(uint64_t entityID, float maxDistance)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.MaxDistance = maxDistance;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetMaxDistance(maxDistance);
		}

		float AudioSourceComponent_GetConeInnerAngle(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.ConeInnerAngle;
		}

		void AudioSourceComponent_SetConeInnerAngle(uint64_t entityID, float coneInnerAngle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.ConeInnerAngle = coneInnerAngle;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetCone(component.Config.ConeInnerAngle, component.Config.ConeOuterAngle, component.Config.ConeOuterGain);
		}

		float AudioSourceComponent_GetConeOuterAngle(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.ConeOuterAngle;
		}

		void AudioSourceComponent_SetConeOuterAngle(uint64_t entityID, float coneOuterAngle)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.ConeOuterAngle = coneOuterAngle;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetCone(component.Config.ConeInnerAngle, component.Config.ConeOuterAngle, component.Config.ConeOuterGain);
		}

		float AudioSourceComponent_GetConeOuterGain(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.ConeOuterGain;
		}

		void AudioSourceComponent_SetConeOuterGain(uint64_t entityID, float coneOuterGain)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.ConeOuterGain = coneOuterGain;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetCone(component.Config.ConeInnerAngle, component.Config.ConeOuterAngle, component.Config.ConeOuterGain);
		}

		void AudioSourceComponent_SetCone(uint64_t entityID, float coneInnerAngle, float coneOuterAngle, float coneOuterGain)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.ConeInnerAngle = coneInnerAngle;
			component.Config.ConeOuterAngle = coneOuterAngle;
			component.Config.ConeOuterGain = coneOuterGain;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetCone(component.Config.ConeInnerAngle, component.Config.ConeOuterAngle, component.Config.ConeOuterGain);
		}

		float AudioSourceComponent_GetDopplerFactor(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			return entity.GetComponent<AudioSourceComponent>().Config.DopplerFactor;
		}

		void AudioSourceComponent_SetDopplerFactor(uint64_t entityID, float dopplerFactor)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			auto& component = entity.GetComponent<AudioSourceComponent>();
			component.Config.DopplerFactor = dopplerFactor;
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->SetDopplerFactor(dopplerFactor);
		}

		bool AudioSourceComponent_IsPlaying(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& component = entity.GetComponent<AudioSourceComponent>();

			if (AssetManager::IsAssetHandleValid(component.Audio))
			{
				RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
				if (audioSource)
					return audioSource->IsPlaying();
				else
					return false;
			}
			
			return false;
		}

		void AudioSourceComponent_Play(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& component = entity.GetComponent<AudioSourceComponent>();
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
			{
				audioSource->Play();
			}
		}

		void AudioSourceComponent_Pause(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& component = entity.GetComponent<AudioSourceComponent>();
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
			{
				audioSource->Pause();
			}
		}

		void AudioSourceComponent_UnPause(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& component = entity.GetComponent<AudioSourceComponent>();
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->UnPause();
		}

		void AudioSourceComponent_Stop(uint64_t entityID)
		{
			RefPtr<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			NZ_CORE_ASSERT(scene);
			Entity entity = scene->GetEntityByID(entityID);
			NZ_CORE_ASSERT(entity);

			const auto& component = entity.GetComponent<AudioSourceComponent>();
			RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
			if (audioSource)
				audioSource->Stop();
		}

#pragma endregion

	}
}
