#include "nzpch.hpp"
#include "Scene.hpp"

#include "Entity.hpp"
#include "Components.hpp"
#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Core/Audio/AudioListener.hpp"
#include "Nutcrackz/Core/Audio/AudioSource.hpp"
#include "Nutcrackz/Core/Audio/AudioEngine.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"
#include "Nutcrackz/Renderer/Grid.hpp"
#include "Nutcrackz/Renderer/Renderer2D.hpp"
#include "Nutcrackz/Renderer/Renderer3D.hpp"
#include "Nutcrackz/Renderer/VideoRenderer.hpp"
#include "Nutcrackz/Core/Input.hpp"
#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Physics/ContactListener2D.hpp"
#include "Nutcrackz/UndoRedo/CommandHistory.hpp"
#include "Nutcrackz/Math/Math.hpp"
#include "Nutcrackz/Utils/PlatformUtils.hpp"

#include <glad/glad.h>

namespace Nutcrackz {

	bool Scene::s_SetPaused = false;
	rtmcpp::Vec2 Scene::s_Gravity = { 0.0f, -9.81f };

	int g_PauseTimer = 0;
	int g_ResumeTimer = 0;
	bool g_ResetTimer = false;

	static ContactListener2D s_Box2DContactListener;

	Scene::Scene()
	{
		if (m_PhysicsWorld == nullptr)
			m_PhysicsWorld = new b2World({ s_Gravity.X, s_Gravity.Y });

		CommandHistory::ResetCommandsHistory();
	}

	Scene::~Scene()
	{
		if (m_Physics2DBodyEntityBuffer)
		{
			delete m_Physics2DBodyEntityBuffer;
			m_Physics2DBodyEntityBuffer = nullptr;
		}

		if (m_PhysicsWorld)
		{
			delete m_PhysicsWorld;
			m_PhysicsWorld = nullptr;
		}

		if (m_EntityMap.size() > 0)
			m_EntityMap.erase(m_EntityMap.begin(), m_EntityMap.end());
	}

	template<typename... Component>
	static void CopyComponent(flecs::world& dst, flecs::world& src, const std::unordered_map<uint64_t, flecs::entity>& entityMap)
	{
		([&]()
		{
			src.each([&](flecs::entity srcEntity, Component& comp)
			{
				flecs::entity dstEntity = entityMap.at(uint64_t(srcEntity));
				dstEntity.add(srcEntity);
				dstEntity.set(comp);
				dst.add<Component>(dstEntity);
			});
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, flecs::world& dst, flecs::world& src, const std::unordered_map<uint64_t, flecs::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	static void CopyAllComponents(flecs::world& dstSceneRegistry, flecs::world& srcSceneRegistry, const std::unordered_map<uint64_t, flecs::entity>& enttMap)
	{
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);
	}

	static void CopyAllExistingComponents(Entity dst, Entity src)
	{
		CopyComponentIfExists(AllComponents{}, dst, src);
	}

	RefPtr<Scene> Scene::Copy(RefPtr<Scene> other)
	{
		RefPtr<Scene> newScene = RefPtr<Scene>::Create();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcWorld = other->m_ECS;
		auto& dstWorld = newScene->m_ECS;

		std::unordered_map<uint64_t, flecs::entity> enttMap;

		// Create entities in new scene
		auto idFilter = srcWorld.filter<IDComponent>();
		idFilter.each([&](flecs::entity srcEntity, IDComponent& e)
		{
			uint64_t id = (uint64_t)srcEntity;
			const auto& name = srcEntity.get<TagComponent>()->Tag;
			Entity newEntity = newScene->CreateEntityWithID(id, name);
			enttMap[id] = (flecs::entity)newEntity;
		});

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstWorld, srcWorld, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithID(uint64_t(), name);
	}

	Entity Scene::CreateEntityWithID(uint64_t id, const std::string& name)
	{
		Entity entity = { m_ECS.entity() };
		entity.AddComponent<IDComponent>(id);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[id] = entity;
		
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetEntityHandle());
		entity.DestroyEntity();
	}

	void Scene::OnRuntimeStart()
	{
		RefPtr<Scene> _this = this;

		OnPhysics2DStart();

		ContactListener2D::m_IsPlaying = true;
		Renderer2D::s_AnimateInRuntime = true;

		{
			auto filter = m_ECS.filter<TransformComponent, AudioListenerComponent>();
			filter.each([&](TransformComponent& transform, AudioListenerComponent& ac)
			{
				ac.Listener = RefPtr<AudioListener>::Create();
				if (ac.Active)
				{
					const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
					const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);
					ac.Listener->SetConfig(ac.Config);
					ac.Listener->SetPosition(transform.Translation);
					ac.Listener->SetDirection(rtmcpp::Vec3{ -forward.X, -forward.Y, -forward.Z });
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, AudioSourceComponent>();
			filter.each([&](TransformComponent& transform, AudioSourceComponent& ac)
			{
				if (AssetManager::IsAssetHandleValid(ac.Audio))
				{
					if (ac.Audio && !ac.AudioSourceData.UsePlaylist)
					{
						RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(ac.Audio);
						const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
						const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);

						if (audioSource != nullptr)
						{
							audioSource->SetConfig(ac.Config);
							audioSource->SetPosition(transform.Translation);
							audioSource->SetDirection(forward);
							if (ac.Config.PlayOnAwake)
								audioSource->Play();
						}
					}
					else if (ac.Audio && ac.AudioSourceData.UsePlaylist)
					{
						if (ac.AudioSourceData.CurrentIndex >= ac.AudioSourceData.Playlist.size())
							ac.AudioSourceData.CurrentIndex = 0;

						if (ac.AudioSourceData.CurrentIndex < ac.AudioSourceData.Playlist.size())
						{
							RefPtr<AudioSource> playingSourceIndex = AssetManager::GetAsset<AudioSource>(ac.AudioSourceData.Playlist[ac.AudioSourceData.CurrentIndex]);
							const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
							const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);

							if (playingSourceIndex != nullptr)
							{
								playingSourceIndex->SetConfig(ac.Config);
								playingSourceIndex->SetPosition(transform.Translation);
								playingSourceIndex->SetDirection(forward);
								if (ac.Config.PlayOnAwake)
									playingSourceIndex->Play();

								ac.AudioSourceData.PlayingCurrentIndex = true;
								ac.AudioSourceData.CurrentIndex++;
							}
						}
					}
				}
			});
		}

		// Scripting
		{
			auto& scriptEngine = ScriptEngine::GetMutable();
			scriptEngine.SetCurrentScene(_this);

			auto filter = m_ECS.filter<ScriptComponent>();
			filter.each([&](flecs::entity entity, ScriptComponent& sc)
			{
				sc.Instance = scriptEngine.Instantiate(uint64_t(entity), m_ScriptStorage, uint64_t(entity));
			});

			filter.each([&](flecs::entity entity, ScriptComponent& sc)
			{
				sc.Instance.Invoke("OnCreate");
			});
		}
	}

	void Scene::OnRuntimeStop()
	{
		ContactListener2D::m_IsPlaying = false;
		Renderer2D::s_AnimateInRuntime = false;
		
		OnPhysics2DStop();

		{
			auto filter = m_ECS.filter<AudioSourceComponent>();
			filter.each([&](AudioSourceComponent& asc)
			{
				auto& ac = asc;
				if (AssetManager::IsAssetHandleValid(ac.Audio))
				{
					if (ac.Audio && !ac.AudioSourceData.UsePlaylist)
					{
						RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(ac.Audio);

						if (audioSource != nullptr && audioSource->IsPlaying())
							audioSource->Stop();
					}
					else if (ac.Audio && ac.AudioSourceData.UsePlaylist)
					{
						ac.AudioSourceData.CurrentIndex = ac.AudioSourceData.StartIndex;
						ac.AudioSourceData.PlayingCurrentIndex = false;

						for (auto audio : ac.AudioSourceData.Playlist)
						{
							RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(audio);

							if (audioSource != nullptr && audioSource->IsPlaying())
								audioSource->Stop();
						}
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<VideoRendererComponent>();
			filter.each([&](VideoRendererComponent& vsc)
			{
				auto& vc = vsc;

				if (vc.Texture)
				{
					if (vc.Texture->HasLoadedAudio())
					{
						if (!vc.m_VideoData.UseExternalAudio)
						{
							if (!vc.Texture->AVReaderSeekFrame(&vc.Texture->GetVideoState(), 0, true))
							{
								NZ_CORE_WARN("Could not seek a/v back to start frame!");
								return;
							}

							vc.Texture->CloseAudio(&vc.Texture->GetVideoState());
						}
						else
						{
							if (!vc.Texture->VideoReaderSeekFrame(&vc.Texture->GetVideoState(), 0))
							{
								NZ_CORE_WARN("Could not seek video back to start frame!");
								return;
							}
						}

						vc.m_VideoData.SeekAudio = true;
					}

					vc.Texture->DeleteRendererID(vc.m_VideoData.VideoRendererID);
					vc.Texture->CloseVideo(&vc.Texture->GetVideoState());

					std::filesystem::path filepath = Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(vc.Video);
					vc.m_VideoData.VideoRendererID = vc.Texture->GetIDFromTexture(vc.m_VideoData.VideoFrameData, &vc.m_VideoData.PresentationTimeStamp, vc.m_VideoData.PauseVideo, filepath);

					vc.Texture->SetRendererID(vc.m_VideoData.VideoRendererID);

					vc.m_VideoData.PresentationTimeStamp = 0;
					vc.m_VideoData.RestartPointFromPause = 0.0;

					if (vc.m_VideoData.IsRenderingVideo)
						vc.m_VideoData.IsRenderingVideo = false;
				}
			});
		}

		{
			auto& scriptEngine = ScriptEngine::GetMutable();

			auto filter = m_ECS.filter<IDComponent, ScriptComponent>();
			filter.each([&](flecs::entity scriptEntity, IDComponent& id, ScriptComponent& sc)
			{
				sc.Instance.Invoke("OnDestroy");
				scriptEngine.DestroyInstance(id.ID, m_ScriptStorage);
			});

			filter.each([&](flecs::entity scriptEntity, IDComponent& id, ScriptComponent& sc)
			{
				{
					Entity e = { scriptEntity };

					sc.HasInitializedScript = false;

					if (!m_ScriptStorage.EntityStorage.contains(e.GetEntityHandle()))
						return;

					m_ScriptStorage.ShutdownEntityStorage(sc.ScriptHandle, e.GetEntityHandle());
				}
			});
		}
	}

	void Scene::OnSimulationStart()
	{
		OnPhysics2DStart();
	}

	void Scene::OnSimulationStop()
	{
		OnPhysics2DStop();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		NZ_PROFILE_FUNCTION_COLOR("Scene::OnUpdateRuntime");

		if ((!m_IsPaused && !s_SetPaused) || m_StepFrames-- > 0)
		{
			if (!Renderer2D::s_AnimateInRuntime)
				Renderer2D::s_AnimateInRuntime = true;

			// Box2D physics
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::Rigidbody2DComponent Scope", 0xFF7200);

				// Copy transform from Nutcrackz to box2d
				if (m_PhysicsWorld != nullptr)
				{
					auto filter = m_ECS.filter<Rigidbody2DComponent>();
					filter.each([&](flecs::entity rb2dEntity, Rigidbody2DComponent& r2d)
					{
						Entity e = { rb2dEntity };
						auto& transform = e.GetComponent<TransformComponent>();
						auto& rb2d = e.GetComponent<Rigidbody2DComponent>();

						b2Body* body = (b2Body*)rb2d.RuntimeBody;

						if (e.HasComponent<BoxCollider2DComponent>())
						{
							auto& bc2d = e.GetComponent<BoxCollider2DComponent>();

							if (body->IsAwake() != bc2d.Awake)
								body->SetAwake(bc2d.Awake);

							bc2d.CollisionRay = rtmcpp::Vec2(transform.Translation.X, transform.Translation.Y);

							if (bc2d.BoxScaleSize.X != (bc2d.Size.X * transform.Scale.X) || bc2d.BoxScaleSize.Y != (bc2d.Size.Y * transform.Scale.Y) ||
								bc2d.BoxOffset.X != bc2d.Offset.X || bc2d.BoxOffset.Y != bc2d.Offset.Y)
							{
								bc2d.BoxScaleSize = rtmcpp::Vec2(bc2d.Size.X * transform.Scale.X, bc2d.Size.Y * transform.Scale.Y);

								if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
								{
									b2Fixture* fixture = body->GetFixtureList();
									body->DestroyFixture(fixture);
								}

								b2PolygonShape boxShape;
								boxShape.SetAsBox(bc2d.BoxScaleSize.X, bc2d.BoxScaleSize.Y, b2Vec2(bc2d.Offset.X, bc2d.Offset.Y), 0.0f);

								b2FixtureDef fixtureDef;
								fixtureDef.shape = &boxShape;
								fixtureDef.density = bc2d.Density;
								fixtureDef.friction = bc2d.Friction;
								fixtureDef.restitution = bc2d.Restitution;
								fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;

								body->CreateFixture(&fixtureDef);
								bc2d.BoxOffset = bc2d.Offset;
							}
						}

						if (e.HasComponent<CircleCollider2DComponent>())
						{
							auto& cc2d = e.GetComponent<CircleCollider2DComponent>();

							if (body->IsAwake() != cc2d.Awake)
								body->SetAwake(cc2d.Awake);

							cc2d.CollisionRay = rtmcpp::Vec2(transform.Translation.X, transform.Translation.Y);

							if (cc2d.CircleScaleRadius != transform.Scale.X * cc2d.Radius)
							{
								transform.Scale.Y = transform.Scale.X;
								cc2d.CircleScaleRadius = cc2d.Radius * transform.Scale.X;

								if (body->GetFixtureList()->GetType() == b2Shape::e_circle)
								{
									b2Fixture* fixture = body->GetFixtureList();
									body->DestroyFixture(fixture);
								}

								b2CircleShape circleShape;
								circleShape.m_p.Set(cc2d.Offset.X, cc2d.Offset.Y);
								circleShape.m_radius = cc2d.CircleScaleRadius;

								b2FixtureDef fixtureDef;
								fixtureDef.shape = &circleShape;
								fixtureDef.density = cc2d.Density;
								fixtureDef.friction = cc2d.Friction;
								fixtureDef.restitution = cc2d.Restitution;
								fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

								body->CreateFixture(&fixtureDef);
							}
							else if (cc2d.CircleScaleRadius != transform.Scale.Y * cc2d.Radius)
							{
								transform.Scale.X = transform.Scale.Y;
								cc2d.CircleScaleRadius = cc2d.Radius * transform.Scale.Y;

								if (body->GetFixtureList()->GetType() == b2Shape::e_circle)
								{
									b2Fixture* fixture = body->GetFixtureList();
									body->DestroyFixture(fixture);
								}

								b2CircleShape circleShape;
								circleShape.m_p.Set(cc2d.Offset.X, cc2d.Offset.Y);
								circleShape.m_radius = cc2d.CircleScaleRadius;

								b2FixtureDef fixtureDef;
								fixtureDef.shape = &circleShape;
								fixtureDef.density = cc2d.Density;
								fixtureDef.friction = cc2d.Friction;
								fixtureDef.restitution = cc2d.Restitution;
								fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

								body->CreateFixture(&fixtureDef);
							}
						}

						if (e.HasComponent<TriangleCollider2DComponent>())
						{
							auto& tc2d = e.GetComponent<TriangleCollider2DComponent>();

							if (body->IsAwake() != tc2d.Awake)
								body->SetAwake(tc2d.Awake);

							tc2d.CollisionRay = rtmcpp::Vec2(transform.Translation.X, transform.Translation.Y);

							if (tc2d.TriangleScaleSize.X != (tc2d.Size.X * transform.Scale.X) || tc2d.TriangleScaleSize.Y != (tc2d.Size.Y * transform.Scale.Y) ||
								tc2d.TriangleOffset.X != tc2d.Offset.X || tc2d.TriangleOffset.Y != tc2d.Offset.Y)
							{
								tc2d.TriangleScaleSize = rtmcpp::Vec2(tc2d.Size.X * transform.Scale.X, tc2d.Size.Y * transform.Scale.Y);

								if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
								{
									b2Fixture* fixture = body->GetFixtureList();
									body->DestroyFixture(fixture);
								}

								b2Vec2 vertices[3];
								vertices[0].Set((-1.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (-1.0f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
								vertices[1].Set((1.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (-1.0f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
								vertices[2].Set((0.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (1.0f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);

								b2PolygonShape triangleShape;
								triangleShape.Set(vertices, 3);

								b2FixtureDef fixtureDef;
								fixtureDef.shape = &triangleShape;
								fixtureDef.density = tc2d.Density;
								fixtureDef.friction = tc2d.Friction;
								fixtureDef.restitution = tc2d.Restitution;
								fixtureDef.restitutionThreshold = tc2d.RestitutionThreshold;

								body->CreateFixture(&fixtureDef);
								tc2d.TriangleOffset = tc2d.Offset;
							}
						}

						if (e.HasComponent<CapsuleCollider2DComponent>())
						{
							auto& cc2d = e.GetComponent<CapsuleCollider2DComponent>();

							if (body->IsAwake() != cc2d.Awake)
								body->SetAwake(cc2d.Awake);

							cc2d.CollisionRay = rtmcpp::Vec2(transform.Translation.X, transform.Translation.Y);

							if (cc2d.CapsuleScaleSize.X != (cc2d.Size.X * transform.Scale.X) || cc2d.CapsuleScaleSize.Y != (cc2d.Size.Y * transform.Scale.Y) ||
								cc2d.CapsuleOffset.X != cc2d.Offset.X || cc2d.CapsuleOffset.Y != cc2d.Offset.Y)
							{
								cc2d.CapsuleScaleSize = rtmcpp::Vec2(cc2d.Size.X * transform.Scale.X, cc2d.Size.Y * transform.Scale.Y);

								if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
								{
									b2Fixture* fixture = body->GetFixtureList();
									body->DestroyFixture(fixture);
								}

								b2Vec2 vertices[8];
								vertices[0].Set((-0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[1].Set((-1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[2].Set((-1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[3].Set((-0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[4].Set((0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[5].Set((1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[6].Set((1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
								vertices[7].Set((0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);

								b2PolygonShape capsuleShape;
								capsuleShape.Set(vertices, 8);

								b2FixtureDef fixtureDef;
								fixtureDef.shape = &capsuleShape;
								fixtureDef.density = cc2d.Density;
								fixtureDef.friction = cc2d.Friction;
								fixtureDef.restitution = cc2d.Restitution;
								fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

								body->CreateFixture(&fixtureDef);
								cc2d.CapsuleOffset = cc2d.Offset;
							}
						}

						if (e.HasComponent<MeshCollider2DComponent>())
						{
							auto& mc2d = e.GetComponent<MeshCollider2DComponent>();

							if (body->IsAwake() != mc2d.Awake)
								body->SetAwake(mc2d.Awake);

							mc2d.CollisionRay = rtmcpp::Vec2(transform.Translation.X, transform.Translation.Y);

							if (mc2d.MeshScaleSize.X != (mc2d.Size.X * transform.Scale.X) || mc2d.MeshScaleSize.Y != (mc2d.Size.Y * transform.Scale.Y) ||
								mc2d.MeshOffset.X != mc2d.Offset.X || mc2d.MeshOffset.Y != mc2d.Offset.Y)
							{
								mc2d.MeshScaleSize = rtmcpp::Vec2(mc2d.Size.X * transform.Scale.X, mc2d.Size.Y * transform.Scale.Y);

								if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
								{
									b2Fixture* fixture = body->GetFixtureList();
									body->DestroyFixture(fixture);
								}

								b2Vec2 vertices[8];
								for (uint32_t i = 0; i < mc2d.Positions.size(); i++)
								{
									if (mc2d.Positions.size() >= 3)
									{
										if (i == mc2d.Positions.size() - 1)
										{
											vertices[i].Set((mc2d.Positions[i].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
											vertices[0].Set((mc2d.Positions[0].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[0].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
										}

										if ((i + 1) < mc2d.Positions.size())
										{
											vertices[i].Set((mc2d.Positions[i].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
											vertices[i + 1].Set((mc2d.Positions[i + 1].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i + 1].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
										}
									}
								}

								b2PolygonShape meshShape;
								meshShape.Set(vertices, (int32)mc2d.Positions.size());

								b2FixtureDef fixtureDef;
								fixtureDef.shape = &meshShape;
								fixtureDef.density = mc2d.Density;
								fixtureDef.friction = mc2d.Friction;
								fixtureDef.restitution = mc2d.Restitution;
								fixtureDef.restitutionThreshold = mc2d.RestitutionThreshold;

								body->CreateFixture(&fixtureDef);
								mc2d.MeshOffset = mc2d.Offset;
							}
						}

						bool awake = body->GetPosition().x != transform.Translation.X || body->GetPosition().x != transform.Translation.Y || body->GetAngle() != transform.Rotation.Z;

						if (!awake)
						{
							body->SetTransform({ transform.Translation.X, transform.Translation.Y }, transform.Rotation.Z);
							body->SetAwake(true);
						}
					});
				}
			}

			if (m_PhysicsWorld != nullptr)
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::Rigidbody2DComponent2 Scope", 0xFF7200);

				m_PhysicsWorld->Step(ts, 6, 2);

				auto filter = m_ECS.filter<Rigidbody2DComponent>();
				filter.each([&](flecs::entity rb2dEntity, Rigidbody2DComponent& r2d)
				{
					Entity e = { rb2dEntity };
					if (e.HasComponent<TransformComponent>())
					{
						if (e.GetComponent<TransformComponent>().Enabled)
						{
							if (e.HasComponent<Rigidbody2DComponent>())
							{
								auto& rb2d = e.GetComponent<Rigidbody2DComponent>();
								b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
								body->SetEnabled(rb2d.SetEnabled);

								auto& position = body->GetPosition();
								auto& transform = e.GetComponent<TransformComponent>();
								transform.Translation.X = position.x;
								transform.Translation.Y = position.y;
								transform.Rotation.Z = body->GetAngle();
							}
						}
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioListenerComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<AudioListenerComponent>();
				filter.each([&](flecs::entity acEntity, AudioListenerComponent& alc)
				{
					Entity e = { acEntity };
					auto& ac = e.GetComponent<AudioListenerComponent>();
					auto& transform = e.GetComponent<TransformComponent>();

					if (ac.Active)
					{
						const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
						const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);
						ac.Listener->SetPosition(transform.Translation);
						ac.Listener->SetDirection(rtmcpp::Vec3{ -forward.X, -forward.Y, -forward.Z });
						//break;
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, AudioSourceComponent>();
				filter.each([&](TransformComponent& transform, AudioSourceComponent& asc)
				{
					//Entity e = { acEntity, this };
					//auto& transform = e.GetComponent<TransformComponent>();

					//const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
					//const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);

					if (asc.Audio && !asc.AudioSourceData.UsePlaylist)
					{
						RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(asc.Audio);
						if (!audioSource->IsPlaying() && asc.Paused)
						{
							audioSource->SetConfig(asc.Config);
							audioSource->Play();
							asc.Paused = false;
						}

						if (audioSource != nullptr)
						{
							audioSource->SetConfig(asc.Config);
							audioSource->SetPosition(transform.Translation);
							//audioSource->SetDirection(forward);
						}
					}
					else if (asc.Audio && asc.AudioSourceData.UsePlaylist)
					{
						NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent 2 Scope", 0xEE3AFF);

						RefPtr<AudioSource> audioSourceIndex = AssetManager::GetAsset<AudioSource>(asc.AudioSourceData.Playlist[asc.AudioSourceData.OldIndex]);

						//if (ac.AudioSourceData.OldIndex <= ac.AudioSourceData.Playlist.size() - 1)
						if (asc.AudioSourceData.CurrentIndex < asc.AudioSourceData.Playlist.size() && audioSourceIndex != nullptr && asc.Config.PlayOnAwake && !audioSourceIndex->IsPlaying() && !asc.Paused)
						{
							NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent 3 Scope", 0xFF8E68);

							audioSourceIndex = AssetManager::GetAsset<AudioSource>(asc.AudioSourceData.Playlist[asc.AudioSourceData.CurrentIndex]);

							if (!audioSourceIndex->IsLooping())
							{
								NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent 4 Scope", 0xFF2F68);

								audioSourceIndex->SetConfig(asc.Config);
								audioSourceIndex->Play();
								asc.AudioSourceData.PlayingCurrentIndex = true;
								asc.Paused = false;

								//const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
								//const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);

								audioSourceIndex->SetConfig(asc.Config);
								audioSourceIndex->SetPosition(transform.Translation);
								//audioSourceIndex->SetDirection(forward);

								if (asc.AudioSourceData.RepeatAfterSpecificTrackPlays && asc.AudioSourceData.CurrentIndex == asc.AudioSourceData.StartIndex)
								{
									NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent 5 Scope", 0xA191FF);

									audioSourceIndex->SetLooping(true);
								}

								if (asc.AudioSourceData.OldIndex != asc.AudioSourceData.CurrentIndex)
								{
									NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent 6 Scope", 0x8CCBFF);

									asc.AudioSourceData.OldIndex = asc.AudioSourceData.CurrentIndex;
								}

								asc.AudioSourceData.CurrentIndex++;
							}
						}
						else if (asc.AudioSourceData.CurrentIndex < asc.AudioSourceData.Playlist.size() && audioSourceIndex != nullptr && asc.Config.PlayOnAwake && asc.Paused)
						{
							audioSourceIndex->SetConfig(asc.Config);
							audioSourceIndex->Play();
							asc.AudioSourceData.PlayingCurrentIndex = true;
							asc.Paused = false;
						}

						if (asc.AudioSourceData.RepeatPlaylist && !asc.AudioSourceData.RepeatAfterSpecificTrackPlays && asc.AudioSourceData.CurrentIndex >= asc.AudioSourceData.Playlist.size())
						{
							if (audioSourceIndex != nullptr && !audioSourceIndex->IsPlaying())
								asc.AudioSourceData.CurrentIndex = 0;
						}

						if (asc.AudioSourceData.RepeatAfterSpecificTrackPlays && !asc.AudioSourceData.RepeatPlaylist && asc.AudioSourceData.CurrentIndex > asc.AudioSourceData.StartIndex)
						{
							if (audioSourceIndex != nullptr && !audioSourceIndex->IsPlaying())
								asc.AudioSourceData.CurrentIndex = asc.AudioSourceData.StartIndex;
						}
					}
				});
			}
		}
		else if (m_IsPaused || s_SetPaused)
		{
			if (Renderer2D::s_AnimateInRuntime)
				Renderer2D::s_AnimateInRuntime = false;

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioListenerComponent 2 Scope", 0xFF7200);

				auto filter = m_ECS.filter<AudioListenerComponent>();
				filter.each([&](flecs::entity acEntity, AudioListenerComponent& alc)
				{
					Entity e = { acEntity };
					auto& ac = e.GetComponent<AudioListenerComponent>();
					auto& transform = e.GetComponent<TransformComponent>();

					if (ac.Active)
					{
						const rtmcpp::Mat4 inverted = rtmcpp::Inverse(transform.GetTransform());
						const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);
						ac.Listener->SetPosition(transform.Translation);
						ac.Listener->SetDirection(rtmcpp::Vec3{ -forward.X, -forward.Y, -forward.Z });
						//break;
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::AudioSourceComponent 2 Scope", 0xFF7200);

				auto filter = m_ECS.filter<AudioSourceComponent>();
				filter.each([&](flecs::entity acEntity, AudioSourceComponent& asc)
				{
					Entity e = { acEntity };
					auto& transform = e.GetComponent<TransformComponent>();

					if (asc.Audio)
					{
						if (!asc.AudioSourceData.UsePlaylist)
						{
							RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(asc.Audio);
							if (audioSource->IsPlaying())
							{
								audioSource->SetConfig(asc.Config);
								audioSource->Pause();
								asc.Paused = true;
							}
						}
						else if (asc.AudioSourceData.UsePlaylist)
						{
							if (asc.AudioSourceData.OldIndex == 0)
							{
								RefPtr<AudioSource> audioSourceIndex = AssetManager::GetAsset<AudioSource>(asc.Audio);

								if (audioSourceIndex->IsPlaying())
								{
									audioSourceIndex->SetConfig(asc.Config);
									audioSourceIndex->Pause();
									//ac.AudioSourceData.PlayingCurrentIndex = false;
									asc.Paused = true;
								}
							}
							else if (asc.AudioSourceData.OldIndex > 0)
							{
								RefPtr<AudioSource> audioSourceIndex = AssetManager::GetAsset<AudioSource>(asc.AudioSourceData.Playlist[asc.AudioSourceData.OldIndex]);

								if (asc.AudioSourceData.OldIndex < asc.AudioSourceData.Playlist.size())
								{
									if (audioSourceIndex->IsPlaying())
									{
										audioSourceIndex->SetConfig(asc.Config);
										audioSourceIndex->Pause();
										//ac.AudioSourceData.PlayingCurrentIndex = false;
										asc.Paused = true;
									}
								}
							}
						}
					}
				});
			}
		}

		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::ScriptComponent Scope", 0xFF7200);

			// Update Scripts
			auto filter = m_ECS.filter<IDComponent, ScriptComponent>();
			filter.each([&](IDComponent& id, ScriptComponent& sc)
			{
				sc.Instance.Invoke<float>("OnUpdate", ts);
			});
		}

		// Render 2D
		Camera* mainCamera = nullptr;
		rtmcpp::Mat4 cameraTransform;
		{
			NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::CameraComponent Scope", 0xFF7200);

			auto filter = m_ECS.filter<TransformComponent, CameraComponent>();
			filter.each([&](TransformComponent& transform, CameraComponent& camera)
			{
				if (transform.Enabled)
				{
					if (camera.Primary)
					{
						mainCamera = &(*camera.Camera.Raw());

						if (m_SceneCameraPosition.X != transform.Translation.X && m_SceneCameraPosition.Y != transform.Translation.Y && m_SceneCameraPosition.Z != transform.Translation.Z)
							m_SceneCameraPosition = transform.Translation;

						if (m_SceneCameraSize.X != transform.Scale.X && m_SceneCameraSize.Y != transform.Scale.Y && m_SceneCameraSize.Z != transform.Scale.Z)
							m_SceneCameraSize = transform.Scale;

						cameraTransform = transform.GetTransform();
						//break;
					}
				}
			});
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			rtmcpp::Vec4 cameraWAxis = cameraTransform.Value.w_axis;
			
			//SortQuadEntitiesDepth(cameraWAxis.Z);
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::ButtonWidgetComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, ButtonWidgetComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, ButtonWidgetComponent& sprite)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							Renderer2D::DrawSpriteWidget(transform.GetTransform(), sprite, (int)entity);
						}
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::CircleWidgetComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, CircleWidgetComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, CircleWidgetComponent& circle)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							Renderer2D::DrawSpriteWidget(transform.GetTransform(), circle, (int)entity);
						}
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::SpriteRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, SpriteRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, SpriteRendererComponent& sprite)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							rtmcpp::Mat4 spriteTransform = sprite.GetTransform() * transform.GetTransform();
							Renderer2D::DrawSprite(spriteTransform, sprite, ts, sprite.m_AnimationData.UsePerTextureAnimation, cameraTransform, (int)entity);
						}
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::CircleRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, CircleRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, CircleRendererComponent& circle)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							Renderer2D::DrawCircle(transform.GetTransform(), circle, ts, cameraTransform, (int)entity);
						}
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::TriangleRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, TriangleRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, TriangleRendererComponent& triangle)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							rtmcpp::Mat4 triangleTransform = triangle.GetTransform() * transform.GetTransform();
							Renderer2D::DrawTriangle(triangleTransform, triangle, (int)entity);
						}
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::LineRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, LineRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, LineRendererComponent& lineRenderer)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							for (uint32_t i = 0; i < lineRenderer.Translations.size(); i++)
							{
								if ((i + 1) < lineRenderer.Translations.size() && lineRenderer.Translations.size() >= 2)
								{
									rtmcpp::Vec4 pos1 = lineRenderer.Translations[i];
									rtmcpp::Vec4 pos2 = lineRenderer.Translations[i + 1];

									Renderer2D::SetLineWidth(lineRenderer.LineThickness);
									Renderer2D::DrawLine(transform.GetTransform(), pos1, pos2, lineRenderer.Colors[i + 1], (int)entity);
								}
							}
						}
					}
				});
			}

			// Draw text
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::TextComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, TextComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, TextComponent& text)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						Renderer2D::DrawString(text.TextString, transform.GetTransform(), text, (int)entity);
					}
				});
			}
			Renderer2D::EndScene();

			VideoRenderer::BeginScene(*mainCamera, cameraTransform);
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::VideoRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, VideoRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, VideoRendererComponent& video)
				{
					if ((!m_IsPaused && !s_SetPaused) || m_StepFrames-- > 0)
						video.m_VideoData.PauseVideo = false;
					else
						video.m_VideoData.PauseVideo = true;

					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							if (!video.m_VideoData.PlayVideo)
								video.m_VideoData.PlayVideo = true;

							VideoRenderer::DrawVideoSprite(transform, video, video.m_VideoData, (int)entity);
						}
					}
				});
			}
			VideoRenderer::EndScene();

			ParticleSystem::BeginScene(*mainCamera, cameraTransform);
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::ParticleSystemComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, ParticleSystemComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, ParticleSystemComponent& psc)
				{
					if (IsSpriteVisibleToCamera(false, transform.Translation, transform.Scale, m_SceneCameraPosition, m_SceneCameraSize))
					{
						if (transform.Enabled)
						{
							psc.UpdateParticleProps();

							for (int i = 0; i < psc.ParticleSize; i++)
							{
								psc.m_ParticleSystem.Emit(psc.Particle);
							}

							psc.m_ParticleSystem.OnUpdate(ts);
							psc.m_ParticleSystem.DrawParticles(transform, psc, psc.UseBillboard, (int)entity);
						}
					}
				});
			}
			ParticleSystem::EndScene();

			Renderer3D::BeginScene(*mainCamera, cameraTransform);
			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::CubeRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, CubeRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, CubeRendererComponent& cube)
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 cubeTransform = cube.GetTransform() * transform.GetTransform();

						Renderer3D::DrawCubeMesh(cubeTransform, cube, ts, cameraTransform, (int)entity, cube.m_AnimationData.UsePerTextureAnimation, false);
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::PyramidRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, PyramidRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, PyramidRendererComponent& pyramid)
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 pyramidTransform = pyramid.GetTransform() * transform.GetTransform();

						Renderer3D::DrawPyramidMesh(pyramidTransform, pyramid, (int)entity, false);
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::TriangularPrismRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, TriangularPrismRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, TriangularPrismRendererComponent& triangle)
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 pyramidTransform = triangle.GetTransform() * transform.GetTransform();

						Renderer3D::DrawTriangularPrismMesh(pyramidTransform, triangle, (int)entity, false);
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::PlaneRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, PlaneRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, PlaneRendererComponent& plane)
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 planeTransform = plane.GetTransform() * transform.GetTransform();

						Renderer3D::DrawPlaneMesh(planeTransform, plane, (int)entity, false);
					}
				});
			}

			{
				NZ_PROFILE_SCOPE_COLOR("Scene::OnUpdateRuntime::OBJRendererComponent Scope", 0xFF7200);

				auto filter = m_ECS.filter<TransformComponent, OBJRendererComponent>();
				filter.each([&](flecs::entity entity, TransformComponent& transform, OBJRendererComponent& obj)
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 objTransform = obj.GetTransform() * transform.GetTransform();

						Renderer3D::DrawOBJMesh(objTransform, obj, (int)entity, false);
					}
				});
			}
			Renderer3D::EndScene();

			m_ECS.progress();
		}

		// FPS + Frame time counters
		{
			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (m_MinFrameTimeCounter < 200)
				m_MinFrameTimeCounter += 1;

			m_Deltatime += timestep;
			m_SecondsElapsed += timestep;

			if (m_Deltatime > 0.1f)
			{
				m_Deltatime -= 0.1f;
				m_FPS = 1.0f / timestep;
				m_FrameTime = m_FPS ? ((m_SecondsElapsed / m_FPS) * 1000.0f) : 0.0f;

				if (m_SecondsElapsed >= 1.0f)
				{
					m_SecondsElapsed = 0.0f;
				}

				if (m_MaxFrameTime < m_FrameTime && m_MinFrameTimeCounter > 199)
					m_MaxFrameTime = m_FrameTime;

				if (m_MinFrameTime > m_FrameTime)
					m_MinFrameTime = m_FrameTime;
			}
		}
	}

	void Scene::OnUpdateSimulation(Timestep ts, const EditorCamera& camera)
	{
		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			// Box2D physics
			{
				// Copy transform from Nutcrackz to box2d
				auto filter = m_ECS.filter<Rigidbody2DComponent>();
				filter.each([&](flecs::entity entity, Rigidbody2DComponent& r2d)
				{
					Entity e = { entity };
					auto& transform = e.GetComponent<TransformComponent>();
					auto& rb2d = e.GetComponent<Rigidbody2DComponent>();

					b2Body* body = (b2Body*)rb2d.RuntimeBody;

					if (e.HasComponent<BoxCollider2DComponent>())
					{
						auto& bc2d = e.GetComponent<BoxCollider2DComponent>();

						if (body->IsAwake() != bc2d.Awake)
							body->SetAwake(bc2d.Awake);

						if (bc2d.BoxScaleSize.X != (bc2d.Size.X * transform.Scale.X) || bc2d.BoxScaleSize.Y != (bc2d.Size.Y * transform.Scale.Y) ||
							bc2d.BoxOffset.X != bc2d.Offset.X || bc2d.BoxOffset.Y != bc2d.Offset.Y)
						{
							bc2d.BoxScaleSize = rtmcpp::Vec2(bc2d.Size.X * transform.Scale.X, bc2d.Size.Y * transform.Scale.Y);

							if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
							{
								b2Fixture* fixture = body->GetFixtureList();
								body->DestroyFixture(fixture);
							}

							b2PolygonShape boxShape;
							boxShape.SetAsBox(bc2d.BoxScaleSize.X, bc2d.BoxScaleSize.Y, b2Vec2(bc2d.Offset.X, bc2d.Offset.Y), 0.0f);

							b2FixtureDef fixtureDef;
							fixtureDef.shape = &boxShape;
							fixtureDef.density = bc2d.Density;
							fixtureDef.friction = bc2d.Friction;
							fixtureDef.restitution = bc2d.Restitution;
							fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;

							body->CreateFixture(&fixtureDef);
						}
					}

					if (e.HasComponent<CircleCollider2DComponent>())
					{
						auto& cc2d = e.GetComponent<CircleCollider2DComponent>();

						if (cc2d.CircleScaleRadius != (cc2d.Radius * transform.Scale.X))
						{
							transform.Scale.Y = transform.Scale.X;
							cc2d.CircleScaleRadius = cc2d.Radius * transform.Scale.X;

							if (body->GetFixtureList()->GetType() == b2Shape::e_circle)
							{
								b2Fixture* fixture = body->GetFixtureList();
								body->DestroyFixture(fixture);
							}

							b2CircleShape circleShape;
							circleShape.m_p.Set(cc2d.Offset.X, cc2d.Offset.Y);
							circleShape.m_radius = cc2d.CircleScaleRadius;

							b2FixtureDef fixtureDef;
							fixtureDef.shape = &circleShape;
							fixtureDef.density = cc2d.Density;
							fixtureDef.friction = cc2d.Friction;
							fixtureDef.restitution = cc2d.Restitution;
							fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

							body->CreateFixture(&fixtureDef);
						}
						else if (cc2d.CircleScaleRadius != (cc2d.Radius * transform.Scale.Y))
						{
							transform.Scale.X = transform.Scale.Y;
							cc2d.CircleScaleRadius = cc2d.Radius * transform.Scale.Y;

							if (body->GetFixtureList()->GetType() == b2Shape::e_circle)
							{
								b2Fixture* fixture = body->GetFixtureList();
								body->DestroyFixture(fixture);
							}

							b2CircleShape circleShape;
							circleShape.m_p.Set(cc2d.Offset.X, cc2d.Offset.Y);
							circleShape.m_radius = cc2d.CircleScaleRadius;

							b2FixtureDef fixtureDef;
							fixtureDef.shape = &circleShape;
							fixtureDef.density = cc2d.Density;
							fixtureDef.friction = cc2d.Friction;
							fixtureDef.restitution = cc2d.Restitution;
							fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

							body->CreateFixture(&fixtureDef);
						}
					}

					if (e.HasComponent<TriangleCollider2DComponent>())
					{
						auto& tc2d = e.GetComponent<TriangleCollider2DComponent>();

						if (tc2d.TriangleScaleSize.X != (tc2d.Size.X * transform.Scale.X) || tc2d.TriangleScaleSize.Y != (tc2d.Size.Y * transform.Scale.Y))
						{
							tc2d.TriangleScaleSize = rtmcpp::Vec2(tc2d.Size.X * transform.Scale.X, tc2d.Size.Y * transform.Scale.Y);

							if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
							{
								b2Fixture* fixture = body->GetFixtureList();
								body->DestroyFixture(fixture);
							}

							b2Vec2 vertices[3];
							vertices[0].Set((-0.5f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (-0.5f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
							vertices[1].Set((0.5f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (-0.5f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
							vertices[2].Set((0.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (0.5f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);

							b2PolygonShape triangleShape;
							triangleShape.Set(vertices, 3);

							b2FixtureDef fixtureDef;
							fixtureDef.shape = &triangleShape;
							fixtureDef.density = tc2d.Density;
							fixtureDef.friction = tc2d.Friction;
							fixtureDef.restitution = tc2d.Restitution;
							fixtureDef.restitutionThreshold = tc2d.RestitutionThreshold;

							body->CreateFixture(&fixtureDef);
						}
					}

					if (e.HasComponent<CapsuleCollider2DComponent>())
					{
						auto& cc2d = e.GetComponent<CapsuleCollider2DComponent>();

						if (cc2d.CapsuleScaleSize.X != (cc2d.Size.X * transform.Scale.X) || cc2d.CapsuleScaleSize.Y != (cc2d.Size.Y * transform.Scale.Y))
						{
							cc2d.CapsuleScaleSize = rtmcpp::Vec2(cc2d.Size.X * transform.Scale.X, cc2d.Size.Y * transform.Scale.Y);

							if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
							{
								b2Fixture* fixture = body->GetFixtureList();
								body->DestroyFixture(fixture);
							}

							b2Vec2 vertices[8];
							vertices[0].Set((-0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[1].Set((-1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[2].Set((-1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[3].Set((-0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[4].Set((0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[5].Set((1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (-0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[6].Set((1.0f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (0.65f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);
							vertices[7].Set((0.35f * cc2d.CapsuleScaleSize.X) + cc2d.Offset.X, (1.0f * cc2d.CapsuleScaleSize.Y) + cc2d.Offset.Y);

							b2PolygonShape capsuleShape;
							capsuleShape.Set(vertices, 8);

							b2FixtureDef fixtureDef;
							fixtureDef.shape = &capsuleShape;
							fixtureDef.density = cc2d.Density;
							fixtureDef.friction = cc2d.Friction;
							fixtureDef.restitution = cc2d.Restitution;
							fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

							body->CreateFixture(&fixtureDef);
						}
					}

					if (e.HasComponent<MeshCollider2DComponent>())
					{
						auto& mc2d = e.GetComponent<MeshCollider2DComponent>();

						if (mc2d.MeshScaleSize.X != (mc2d.Size.X * transform.Scale.X) || mc2d.MeshScaleSize.Y != (mc2d.Size.Y * transform.Scale.Y))
						{
							mc2d.MeshScaleSize = rtmcpp::Vec2(mc2d.Size.X * transform.Scale.X, mc2d.Size.Y * transform.Scale.Y);

							if (body->GetFixtureList()->GetType() == b2Shape::e_polygon)
							{
								b2Fixture* fixture = body->GetFixtureList();
								body->DestroyFixture(fixture);
							}

							// Set each vertex of polygon in an array
							b2Vec2 vertices[8];
							for (uint32_t i = 0; i < mc2d.Positions.size(); i++)
							{
								if (mc2d.Positions.size() >= 3)
								{
									if (i == mc2d.Positions.size() - 1)
									{
										vertices[i].Set((mc2d.Positions[i].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
										vertices[0].Set((mc2d.Positions[0].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[0].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
									}

									if ((i + 1) < mc2d.Positions.size())
									{
										vertices[i].Set((mc2d.Positions[i].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
										vertices[i + 1].Set((mc2d.Positions[i + 1].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i + 1].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
									}
								}
							}

							b2PolygonShape meshShape;
							meshShape.Set(vertices, (int32)mc2d.Positions.size());

							b2FixtureDef fixtureDef;
							fixtureDef.shape = &meshShape;
							fixtureDef.density = mc2d.Density;
							fixtureDef.friction = mc2d.Friction;
							fixtureDef.restitution = mc2d.Restitution;
							fixtureDef.restitutionThreshold = mc2d.RestitutionThreshold;

							body->CreateFixture(&fixtureDef);
						}
					}

					bool awake = body->GetPosition().x != transform.Translation.X || body->GetPosition().y != transform.Translation.Y || body->GetAngle() != transform.Rotation.Z;

					if (awake)
					{
						body->SetTransform({ transform.Translation.X, transform.Translation.Y }, transform.Rotation.Z);
						body->SetAwake(true);
					}
				});
			}

			if (m_PhysicsWorld != nullptr)
				m_PhysicsWorld->Step(ts, 6, 2);

			{
				auto filter = m_ECS.filter<Rigidbody2DComponent>();
				filter.each([&](flecs::entity entity, Rigidbody2DComponent& r2d)
				{
					Entity e = { entity };
					if (e.HasComponent<TransformComponent>())
					{
						if (e.GetComponent<TransformComponent>().Enabled)
						{
							if (e.HasComponent<Rigidbody2DComponent>())
							{
								auto& rb2d = e.GetComponent<Rigidbody2DComponent>();
								b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
								body->SetEnabled(rb2d.SetEnabled);

								auto& position = body->GetPosition();
								auto& transform = e.GetComponent<TransformComponent>();
								transform.Translation.X = position.x;
								transform.Translation.Y = position.y;
								transform.Rotation.Z = body->GetAngle();
							}
						}
					}
				});
			}
		}

		// Render
		RenderScene(ts, camera);
	}

	void Scene::OnUpdateEditor(Timestep ts, const EditorCamera& camera)
	{
		RenderScene(ts, camera);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		// Resize our non-FixedAspectRatio cameras
		if (width > 0 && height > 0)
		{
			auto filter = m_ECS.filter<CameraComponent>();
			filter.each([&](CameraComponent& cc)
			{
				auto& cameraComponent = cc;
				if (!cameraComponent.FixedAspectRatio && cameraComponent.Camera->GetWidth() == 0 && cameraComponent.Camera->GetHeight() == 0)
					cameraComponent.Camera->SetViewportSize(width, height);
			});
		}

		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto filter = m_ECS.filter<CameraComponent>();
		filter.each([&](CameraComponent& cc)
		{
			auto& cameraComponent = cc;
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera->SetViewportSize(width, height);
		});
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		Entity e{};
		auto filter = m_ECS.filter<CameraComponent>();
		filter.each([&](flecs::entity entity, CameraComponent& cc)
		{
			const auto& camera = cc;

			if (camera.Primary)
				e = Entity{ entity };
		});

		return e;
	}

	Entity Scene::FindEntityByTag(const std::string& tag)
	{
		Entity e{};
		auto filter = m_ECS.filter<TagComponent>();
		filter.each([&](flecs::entity entity, TagComponent& tc)
		{
			const auto& candidate = tc.Tag;

			if (candidate == tag)
				e = Entity{ entity };
		});

		return e;
	}

	void Scene::OnSceneTransition(AssetHandle handle)
	{
		if (m_OnSceneTransitionCallback)
			m_OnSceneTransitionCallback(handle);

		// Debug
		if (!m_OnSceneTransitionCallback)
		{
			NZ_CORE_WARN("Cannot transition scene - no callback set!");
		}
	}

	rtmcpp::Vec2 Scene::GetPhysics2DGravity()
	{
		return s_Gravity;
	}

	void Scene::SetPhysics2DGravity(const rtmcpp::Vec2& gravity)
	{
		s_Gravity = gravity;

		if (m_PhysicsWorld)
			m_PhysicsWorld->SetGravity(b2Vec2(s_Gravity.X, s_Gravity.Y));
		else if (m_PhysicsWorld == nullptr)
			m_PhysicsWorld = new b2World({ s_Gravity.X, s_Gravity.Y });
	}

	void Scene::RenderHoveredEntityOutline(Entity entity, rtmcpp::Vec4 color)
	{
		if (entity)
		{
			Entity camera = GetPrimaryCameraEntity();

			if (!camera)
				return;

			Renderer2D::BeginScene(*camera.GetComponent<CameraComponent>().Camera.Raw(), camera.GetComponent<TransformComponent>().GetTransform());

			// Calculate z index for translation
			float zIndex = 0.001f;
			rtmcpp::Vec3 cameraForwardDirection = camera.GetComponent<CameraComponent>().Camera->GetForwardDirection();

			// Calculate z index for translation
			rtmcpp::Vec3 projectionCollider = cameraForwardDirection * rtmcpp::Vec3(zIndex, zIndex, zIndex);

			// Hovered entity outline
			auto& tc = entity.GetComponent<TransformComponent>();

			if (entity.HasComponent<SpriteRendererComponent>() ||
				entity.HasComponent<CircleRendererComponent>() ||
				entity.HasComponent<TriangleRendererComponent>() ||
				entity.HasComponent<VideoRendererComponent>())
			{
				rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
				rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

				rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
					* rotation
					* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

				Renderer2D::SetLineWidth(2.0f);
				Renderer2D::DrawRect(transform, color);
			}

			Renderer2D::EndScene();
		}
	}

	void Scene::RenderSelectedEntityOutline(Entity entity, rtmcpp::Vec4 color)
	{
		if (entity)
		{
			Entity camera = GetPrimaryCameraEntity();

			if (!camera)
				return;

			Renderer2D::BeginScene(*camera.GetComponent<CameraComponent>().Camera.Raw(), camera.GetComponent<TransformComponent>().GetTransform());

			// Calculate z index for translation
			float zIndex = 0.001f;
			rtmcpp::Vec3 cameraForwardDirection = camera.GetComponent<CameraComponent>().Camera->GetForwardDirection();

			// Calculate z index for translation
			rtmcpp::Vec3 projectionCollider = cameraForwardDirection * rtmcpp::Vec3(zIndex, zIndex, zIndex);

			// Hovered entity outline
			auto& tc = entity.GetComponent<TransformComponent>();

			if (entity.HasComponent<SpriteRendererComponent>() ||
				entity.HasComponent<CircleRendererComponent>() ||
				entity.HasComponent<TriangleRendererComponent>() ||
				entity.HasComponent<VideoRendererComponent>())
			{
				rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
				rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

				rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
					* rotation
					* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

				Renderer2D::SetLineWidth(2.0f);
				Renderer2D::DrawRect(transform, color);
			}

			Renderer2D::EndScene();
		}
	}

	bool Scene::IsSpriteVisibleToCamera(bool isEditorCamera, const rtmcpp::Vec4& spritePosition, const rtmcpp::Vec3& spriteSize, const rtmcpp::Vec4& cameraPosition, const rtmcpp::Vec3& cameraSize)
	{
		float viewportWidth = ((float)m_ViewportWidth / Application::Get().GetWindow().getDPISize()) / 2.0f;
		float viewportHeight = ((float)m_ViewportHeight / Application::Get().GetWindow().getDPISize()) / 2.0f;

		rtmcpp::Vec3 scaledCameraSize = rtmcpp::Vec3(1.0f, 1.0f, 1.0f);

		if (isEditorCamera)
			scaledCameraSize = rtmcpp::Vec3((viewportWidth * 2.0f) / ((cameraSize.X * Application::Get().GetWindow().getDPISize()) / 3.0f), (viewportHeight * 4.0f) / ((cameraSize.Y * Application::Get().GetWindow().getDPISize()) / 3.0f), (viewportWidth * 2.0f) / ((cameraSize.Z * Application::Get().GetWindow().getDPISize()) / 3.0f));
		else
			scaledCameraSize = rtmcpp::Vec3(viewportWidth / ((cameraSize.X * Application::Get().GetWindow().getDPISize()) / 2.0f), viewportHeight / ((cameraSize.Y * Application::Get().GetWindow().getDPISize()) / 2.0f), viewportWidth / ((cameraSize.Z * Application::Get().GetWindow().getDPISize()) / 2.0f));

		const float minDistanceX = spriteSize.X / 2.0f + scaledCameraSize.X / 2.0f;
		const float minDistanceY = spriteSize.Y / 2.0f + scaledCameraSize.Y / 2.0f;
		const float minDistanceZ = 1.0f / 2.0f + scaledCameraSize.Z / 2.0f;
		//const float minDistanceZ = spriteSize.z / 2.0f + scaledCameraSize.z / 2.0f; // use this for 3D Meshes

		//glm::vec3 spriteCenterPos = spritePosition + spriteSize / 2.0f; // use this for 3D Meshes
		rtmcpp::Vec3 spriteCenterPos = rtmcpp::Vec3{ spritePosition.X + spriteSize.X / 2.0f, spritePosition.Y + spriteSize.Y / 2.0f, 1.0f };
		rtmcpp::Vec3 distance = rtmcpp::Vec3{ spriteCenterPos.X - cameraPosition.X, spriteCenterPos.Y - cameraPosition.Y, spriteCenterPos.Z - cameraPosition.Z };

		float xDepth = minDistanceX - abs(distance.X);
		float yDepth = minDistanceY - abs(distance.Y);
		float zDepth = minDistanceZ - abs(distance.Z);

		if (xDepth > 0 && yDepth > 0 && zDepth > 0)
		{
			return true;
		}

		return false;
	}

	void Scene::Step(int frames)
	{
		m_StepFrames = frames;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Copy name because we're going to modify component data structure
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	void Scene::OnScriptComponentDestroy(flecs::world& world, flecs::entity entity)
	{
		// TODO: Remove redundant world! 
		Entity e = { entity };

		auto& scriptComponent = e.GetComponent<ScriptComponent>();
		scriptComponent.HasInitializedScript = false;

		if (!m_ScriptStorage.EntityStorage.contains(e.GetEntityHandle()))
			return;

		m_ScriptStorage.ShutdownEntityStorage(scriptComponent.ScriptHandle, e.GetEntityHandle());
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		Entity e{};

		auto filter = m_ECS.filter<TagComponent>();
		filter.each([&](flecs::entity entity, TagComponent& tag)
		{
			const TagComponent& tc = tag;

			if (tc.Tag == name)
				e = Entity{ entity };
		});

		return e;
	}
	
	Entity Scene::GetEntityByID(uint64_t id)
	{
		// TODO: Maybe should be assert
		if (this != nullptr && m_EntityMap.size() > 0)
		{
			if (m_EntityMap.find(id) != m_EntityMap.end())
				return { m_EntityMap.at(id) };
		}

		return {};
	}

	Entity Scene::TryGetEntityWithID(uint64_t id) const
	{
		//NZ_PROFILE_FUNC();
		if (const auto iter = m_EntityMap.find(id); iter != m_EntityMap.end())
			return iter->second;
		return Entity{};
	}

	void Scene::OnPhysics2DStart()
	{
		// Box2D physics
		m_PhysicsWorld = new b2World({ s_Gravity.X, s_Gravity.Y });
		NZ_CORE_WARN("X: {0}, Y: {1}", m_PhysicsWorld->GetGravity().x, m_PhysicsWorld->GetGravity().y);

		m_PhysicsWorld->SetContactListener(&s_Box2DContactListener);
		auto filter = m_ECS.filter<Rigidbody2DComponent>();
		uint32_t physicsBodyEntityBufferIndex = 0;

		filter.each([&](flecs::entity e, Rigidbody2DComponent& r2d)
		{
			Entity entity = { e };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			m_Physics2DBodyEntityBuffer = new Entity[filter.count()];

			b2BodyDef bodyDef;
			bodyDef.type = Utils::Rigidbody2DTypeToBox2DBody(rb2d.Type);
			bodyDef.position.Set(transform.Translation.X, transform.Translation.Y);
			bodyDef.angle = transform.Rotation.Z;

			Entity* entityStorage = &m_Physics2DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
			*entityStorage = entity;
			bodyDef.userData.pointer = (uintptr_t)entityStorage;

			b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);
			body->SetEnabled(rb2d.SetEnabled);
			rb2d.RuntimeBody = body;

			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(bc2d.Size.X * transform.Scale.X, bc2d.Size.Y * transform.Scale.Y, b2Vec2(bc2d.Offset.X, bc2d.Offset.Y), 0.0f);

				bc2d.BoxScaleSize = rtmcpp::Vec2(bc2d.Size.X * transform.Scale.X, bc2d.Size.Y * transform.Scale.Y);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;

				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

				if (cc2d.CircleScaleRadius != transform.Scale.X * cc2d.Radius)
				{
					transform.Scale.Y = transform.Scale.X;
					cc2d.CircleScaleRadius = cc2d.Radius * transform.Scale.X;
				}
				else if (cc2d.CircleScaleRadius != transform.Scale.Y * cc2d.Radius)
				{
					transform.Scale.X = transform.Scale.Y;
					cc2d.CircleScaleRadius = cc2d.Radius * transform.Scale.Y;
				}
				
				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.Offset.X, cc2d.Offset.Y);
				circleShape.m_radius = cc2d.CircleScaleRadius;


				b2FixtureDef fixtureDef;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<TriangleCollider2DComponent>())
			{
				auto& tc2d = entity.GetComponent<TriangleCollider2DComponent>();

				b2Vec2 vertices[3];
				vertices[0].Set((-1.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (-1.0f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
				vertices[1].Set((1.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (-1.0f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
				vertices[2].Set((0.0f * tc2d.Size.X * transform.Scale.X) + tc2d.Offset.X, (1.0f * tc2d.Size.Y * transform.Scale.Y) + tc2d.Offset.Y);
				tc2d.TriangleScaleSize = rtmcpp::Vec2(tc2d.Size.X * transform.Scale.X, tc2d.Size.Y * transform.Scale.Y);

				b2PolygonShape triangleShape;
				triangleShape.Set(vertices, 3);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &triangleShape;
				fixtureDef.density = tc2d.Density;
				fixtureDef.friction = tc2d.Friction;
				fixtureDef.restitution = tc2d.Restitution;
				fixtureDef.restitutionThreshold = tc2d.RestitutionThreshold;

				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<CapsuleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CapsuleCollider2DComponent>();

				// Set each vertex of polygon in an array
				b2Vec2 vertices[8];
				vertices[0].Set((-0.35f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (1.0f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[1].Set((-1.0f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (0.65f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[2].Set((-1.0f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (-0.65f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[3].Set((-0.35f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (-1.0f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[4].Set((0.35f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (-1.0f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[5].Set((1.0f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (-0.65f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[6].Set((1.0f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (0.65f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);
				vertices[7].Set((0.35f * cc2d.Size.X * transform.Scale.X) + cc2d.Offset.X, (1.0f * cc2d.Size.Y * transform.Scale.Y) + cc2d.Offset.Y);

				cc2d.CapsuleScaleSize = rtmcpp::Vec2(cc2d.Size.X * transform.Scale.X, cc2d.Size.Y * transform.Scale.Y);

				b2PolygonShape capsuleShape;
				capsuleShape.Set(vertices, 8);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &capsuleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<MeshCollider2DComponent>())
			{
				auto& mc2d = entity.GetComponent<MeshCollider2DComponent>();

				b2Vec2 vertices[8];

				for (uint32_t i = 0; i < mc2d.Positions.size(); i++)
				{
					if (mc2d.Positions.size() >= 3)
					{
						if (i == mc2d.Positions.size() - 1)
						{
							vertices[i].Set((mc2d.Positions[i].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
							vertices[0].Set((mc2d.Positions[0].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[0].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
						}
						
						if ((i + 1) < mc2d.Positions.size())
						{
							vertices[i].Set((mc2d.Positions[i].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
							vertices[i + 1].Set((mc2d.Positions[i + 1].X * mc2d.Size.X * transform.Scale.X) + mc2d.Offset.X, (mc2d.Positions[i + 1].Y * mc2d.Size.Y * transform.Scale.Y) + mc2d.Offset.Y);
						}
					}
				}				

				mc2d.MeshScaleSize = rtmcpp::Vec2(mc2d.Size.X * transform.Scale.X, mc2d.Size.Y * transform.Scale.Y);

				b2PolygonShape meshShape;
				meshShape.Set(vertices, (int32)mc2d.Positions.size());

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &meshShape;
				fixtureDef.density = mc2d.Density;
				fixtureDef.friction = mc2d.Friction;
				fixtureDef.restitution = mc2d.Restitution;
				fixtureDef.restitutionThreshold = mc2d.RestitutionThreshold;

				body->CreateFixture(&fixtureDef);
			}
		});
	}

	void Scene::OnPhysics2DStop()
	{
		if (m_Physics2DBodyEntityBuffer)
		{
			delete m_Physics2DBodyEntityBuffer;
			m_Physics2DBodyEntityBuffer = nullptr;
		}

		if (m_PhysicsWorld)
		{
			delete m_PhysicsWorld;
			m_PhysicsWorld = nullptr;
		}
	}

	void Scene::RenderScene(Timestep ts, const EditorCamera& camera)
	{
		if (m_EditorCameraPosition.X != camera.GetPosition().X || m_EditorCameraPosition.Y != camera.GetPosition().Y || m_EditorCameraPosition.Z != camera.GetPosition().Z)
			m_EditorCameraPosition = rtmcpp::Vec4{ camera.GetPosition(), 1.0f };

		Renderer2D::BeginScene(camera);
		
		//SortQuadEntitiesDepth(camera.GetPosition().Z);
		{
			auto filter = m_ECS.filter<TransformComponent, ButtonWidgetComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, ButtonWidgetComponent& sprite)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						Renderer2D::DrawSpriteWidget(transform.GetTransform(), sprite, (int)entity);
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, CircleWidgetComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, CircleWidgetComponent& circle)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						Renderer2D::DrawSpriteWidget(transform.GetTransform(), circle, (int)entity);
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, SpriteRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, SpriteRendererComponent& sprite)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 spriteTransform = transform.GetTransform() * sprite.GetTransform();
						//rtmcpp::Mat4 spriteTransform = sprite.GetTransform() * transform.GetTransform(); // From GLM!
						Renderer2D::DrawSprite(spriteTransform, sprite, ts, sprite.m_AnimationData.UsePerTextureAnimation, rtmcpp::Mat4(), (int)entity);
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, CircleRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, CircleRendererComponent& circle)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 circleTransform = transform.GetTransform() * circle.GetTransform();
						//rtmcpp::Mat4 circleTransform = circle.GetTransform() * transform.GetTransform(); // From GLM!
						Renderer2D::DrawCircle(circleTransform, circle, ts, rtmcpp::Mat4(), (int)entity);
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, TriangleRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, TriangleRendererComponent& triangle)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						rtmcpp::Mat4 triangleTransform = transform.GetTransform() * triangle.GetTransform();
						//rtmcpp::Mat4 triangleTransform = triangle.GetTransform() * transform.GetTransform(); // From GLM!
						Renderer2D::DrawTriangle(triangleTransform, triangle, (int)entity);
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, LineRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, LineRendererComponent& lineRenderer)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						for (uint32_t i = 0; i < lineRenderer.Translations.size(); i++)
						{
							if ((i + 1) < lineRenderer.Translations.size() && lineRenderer.Translations.size() >= 2)
							{
								rtmcpp::Vec4 pos1 = lineRenderer.Translations[i];
								rtmcpp::Vec4 pos2 = lineRenderer.Translations[i + 1];

								Renderer2D::SetLineWidth(lineRenderer.LineThickness);
								Renderer2D::DrawLine(transform.GetTransform(), pos1, pos2, lineRenderer.Colors[i + 1], (int)entity);
							}
						}
					}
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, TextComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, TextComponent& text)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					Renderer2D::DrawString(text.TextString, transform.GetTransform(), text, (int)entity);
				}
			});
		}

		Renderer2D::EndScene();
		
		VideoRenderer::BeginScene(camera);
		{
			auto filter = m_ECS.filter<TransformComponent, VideoRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, VideoRendererComponent& video)
			{
				if ((!m_IsPaused && !s_SetPaused) || m_StepFrames-- > 0)
					video.m_VideoData.PauseVideo = false;
				else
					video.m_VideoData.PauseVideo = true;

				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						if (video.m_VideoData.PlayVideo)
						{
							VideoRenderer::ResetPacketDuration(video);
							video.m_VideoData.PlayVideo = false;
						}

						VideoRenderer::DrawVideoSprite(transform, video, video.m_VideoData, (int)entity);
					}
				}
			});
		}
		VideoRenderer::EndScene();

		ParticleSystem::BeginScene(camera);
		{
			auto filter = m_ECS.filter<TransformComponent, ParticleSystemComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, ParticleSystemComponent& psc)
			{
				if (IsSpriteVisibleToCamera(true, transform.Translation, transform.Scale, m_EditorCameraPosition))
				{
					if (transform.Enabled)
					{
						psc.UpdateParticleProps();

						for (int i = 0; i < psc.ParticleSize; i++)
						{
							psc.m_ParticleSystem.Emit(psc.Particle);
						}

						psc.m_ParticleSystem.OnUpdate(ts);
						psc.m_ParticleSystem.DrawParticles(transform, psc, psc.UseBillboard, (int)entity);
					}
				}
			});
		}
		ParticleSystem::EndScene();

		Renderer3D::BeginScene(camera);
		{
			auto filter = m_ECS.filter<TransformComponent, CubeRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, CubeRendererComponent& cube)
			{
				if (transform.Enabled)
				{
					rtmcpp::Mat4 cubeTransform = transform.GetTransform() * cube.GetTransform();
					//rtmcpp::Mat4 cubeTransform = cube.GetTransform() * transform.GetTransform(); // From GLM!

					Renderer3D::DrawCubeMesh(cubeTransform, cube, ts, rtmcpp::Mat4(), (int)entity, cube.m_AnimationData.UsePerTextureAnimation, false);
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, PyramidRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, PyramidRendererComponent& pyramid)
			{
				if (transform.Enabled)
				{
					rtmcpp::Mat4 pyramidTransform = transform.GetTransform() * pyramid.GetTransform();
					//rtmcpp::Mat4 pyramidTransform = pyramid.GetTransform() * transform.GetTransform(); // From GLM!

					Renderer3D::DrawPyramidMesh(pyramidTransform, pyramid, (int)entity, false);
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, TriangularPrismRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, TriangularPrismRendererComponent& triangle)
			{
				if (transform.Enabled)
				{
					rtmcpp::Mat4 pyramidTransform = transform.GetTransform() * triangle.GetTransform();
					//rtmcpp::Mat4 pyramidTransform = triangle.GetTransform() * transform.GetTransform(); // From GLM!

					Renderer3D::DrawTriangularPrismMesh(pyramidTransform, triangle, (int)entity, false);
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, PlaneRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, PlaneRendererComponent& plane)
			{
				if (transform.Enabled)
				{
					rtmcpp::Mat4 planeTransform = transform.GetTransform() * plane.GetTransform();
					//rtmcpp::Mat4 planeTransform = plane.GetTransform() * transform.GetTransform(); // From GLM!

					Renderer3D::DrawPlaneMesh(planeTransform, plane, (int)entity, false);
				}
			});
		}

		{
			auto filter = m_ECS.filter<TransformComponent, OBJRendererComponent>();
			filter.each([&](flecs::entity entity, TransformComponent& transform, OBJRendererComponent& obj)
			{
				if (transform.Enabled)
				{
					rtmcpp::Mat4 objTransform = transform.GetTransform() * obj.GetTransform();
					//rtmcpp::Mat4 objTransform = obj.GetTransform() * transform.GetTransform(); // From GLM!

					Renderer3D::DrawOBJMesh(objTransform, obj, (int)entity, false);
				}
			});
		}
		Renderer3D::EndScene();

		m_ECS.progress();

		Grid::BeginScene(camera);
		{
			Grid::DrawGrid();
		}
		Grid::EndScene();

		// FPS + Frame time counters
		{
			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (m_MinFrameTimeCounter < 200)
				m_MinFrameTimeCounter += 1;

			m_Deltatime += timestep;
			m_SecondsElapsed += timestep;

			if (m_Deltatime > 0.1f)
			{
				m_Deltatime -= 0.1f;
				m_FPS = 1.0f / timestep;
				m_FrameTime = m_FPS ? ((m_SecondsElapsed / m_FPS) * 1000.0f) : 0.0f;

				if (m_SecondsElapsed >= 1.0f)
				{
					m_SecondsElapsed = 0.0f;
				}

				if (m_MaxFrameTime < m_FrameTime && m_MinFrameTimeCounter > 199)
					m_MaxFrameTime = m_FrameTime;

				if (m_MinFrameTime > m_FrameTime)
					m_MinFrameTime = m_FrameTime;
			}
		}
	}

	void Scene::SortQuadEntitiesDepth(float cameraDepth)
	{
		/*m_Registry.sort<ButtonWidgetComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					return depthA < depthB;
				}
				else
				{
					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<CircleWidgetComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					return depthA < depthB;
				}
				else
				{
					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<SpriteRendererComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					//NZ_CORE_WARN("{0} > {1}", entityA.GetComponent<TagComponent>().Tag, entityB.GetComponent<TagComponent>().Tag);
					//entityA.GetComponent<TransformComponent>().Enabled = true;
					//entityB.GetComponent<TransformComponent>().Enabled = false;

					return depthA < depthB;
				}
				else
				{
					//NZ_CORE_WARN("{0} > {1}", entityA.GetComponent<TagComponent>().Tag, entityB.GetComponent<TagComponent>().Tag);
					//entityB.GetComponent<TransformComponent>().Enabled = true;
					//entityA.GetComponent<TransformComponent>().Enabled = false;

					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<CircleRendererComponent>([this, cameraDepth](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					//NZ_CORE_WARN("{0}: {1} > {2}: {3}", entityA.GetComponent<TagComponent>().Tag, depthA, entityB.GetComponent<TagComponent>().Tag, depthB);
					//NZ_CORE_WARN("{0} < {1}", entityA.GetComponent<TagComponent>().Tag, entityB.GetComponent<TagComponent>().Tag);
					//entityA.GetComponent<TransformComponent>().Enabled = true;
					//entityB.GetComponent<TransformComponent>().Enabled = false;

					return depthA < depthB;
				}
				else
				{
					//NZ_CORE_WARN("{0}: {1} < {2}: {3}", entityA.GetComponent<TagComponent>().Tag, depthA, entityB.GetComponent<TagComponent>().Tag, depthB);
					//NZ_CORE_WARN("{0} > {1}", entityA.GetComponent<TagComponent>().Tag, entityB.GetComponent<TagComponent>().Tag);
					//entityB.GetComponent<TransformComponent>().Enabled = true;
					//entityA.GetComponent<TransformComponent>().Enabled = false;

					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<TriangleRendererComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					return depthA < depthB;
				}
				else
				{
					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<TextComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					return depthA < depthB;
				}
				else
				{
					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<VideoRendererComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					return depthA < depthB;
				}
				else
				{
					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});

		m_Registry.sort<ParticleSystemComponent>([=](const entt::entity& a, const entt::entity& b)
		{
			Entity entityA = { a, this };
			Entity entityB = { b, this };

			if (entityA && entityB)
			{
				float depthA = entityA.GetComponent<TransformComponent>().Translation.Z;
				float depthB = entityB.GetComponent<TransformComponent>().Translation.Z;

				if (cameraDepth > 0.0f)
				{
					return depthA < depthB;
				}
				else
				{
					return depthA > depthB;
				}

				return depthA < depthB;
			}

			return false;
		});*/
	}

}
