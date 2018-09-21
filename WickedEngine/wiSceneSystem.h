#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiImageEffects.h"
#include "wiIntersectables.h"
#include "ShaderInterop.h"
#include "wiFrustum.h"
#include "wiRenderTarget.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"

#include "wiECS.h"
#include "wiSceneSystem_Decl.h"

#include <string>
#include <vector>

class wiArchive;

namespace wiSceneSystem
{
	static const XMFLOAT4X4 IDENTITYMATRIX = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	struct NameComponent
	{
		char name[128];

		inline void operator=(const std::string& str) { strcpy_s(name, str.c_str()); }
		inline bool operator==(const std::string& str) const { return strcmp(name, str.c_str()) == 0; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct LayerComponent
	{
		uint32_t layerMask = ~0;

		inline uint32_t GetLayerMask() const { return layerMask; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};
	
	struct TransformComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
		};
		uint32_t _flags = DIRTY;

		XMFLOAT3 scale_local = XMFLOAT3(1, 1, 1);
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		// Non-serialized attributes:
		XMFLOAT4X4 world = IDENTITYMATRIX;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		XMFLOAT3 GetPosition() const;
		XMFLOAT4 GetRotation() const;
		XMFLOAT3 GetScale() const;
		XMVECTOR GetPositionV() const;
		XMVECTOR GetRotationV() const;
		XMVECTOR GetScaleV() const;
		void UpdateTransform();
		void UpdateParentedTransform(const TransformComponent& parent, const XMFLOAT4X4& inverseParentBindMatrix = IDENTITYMATRIX);
		void ApplyTransform();
		void ClearTransform();
		void Translate(const XMFLOAT3& value);
		void RotateRollPitchYaw(const XMFLOAT3& value);
		void Rotate(const XMFLOAT4& quaternion);
		void Scale(const XMFLOAT3& value);
		void MatrixTransform(const XMFLOAT4X4& matrix);
		void MatrixTransform(const XMMATRIX& matrix);
		void Lerp(const TransformComponent& a, const TransformComponent& b, float t);
		void CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t);

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct PreviousFrameTransformComponent
	{
		// Non-serialized attributes:
		XMFLOAT4X4 world_prev;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct HierarchyComponent
	{
		wiECS::Entity parentID = wiECS::INVALID_ENTITY;
		uint32_t layerMask_bind; // saved child layermask at the time of binding
		XMFLOAT4X4 world_parent_inverse_bind; // saved parent inverse worldmatrix at the time of binding

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct MaterialComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			CAST_SHADOW = 1 << 1,
			PLANAR_REFLECTION = 1 << 2,
			WATER = 1 << 3,
		};
		uint32_t _flags = DIRTY | CAST_SHADOW;

		STENCILREF engineStencilRef = STENCILREF_DEFAULT;
		uint8_t userStencilRef = 0;
		BLENDMODE blendFlag = BLENDMODE_OPAQUE;

		XMFLOAT4 baseColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
		float roughness = 0.2f;
		float reflectance = 0.02f;
		float metalness = 0.0f;
		float emissive = 0.0f;
		float refractionIndex = 0.0f;
		float subsurfaceScattering = 0.0f;
		float normalMapStrength = 1.0f;
		float parallaxOcclusionMapping = 0.0f;

		float alphaRef = 1.0f;
		
		XMFLOAT2 texAnimDirection = XMFLOAT2(0, 0);
		float texAnimFrameRate = 0.0f;
		float texAnimSleep = 0.0f;

		std::string baseColorMapName;
		std::string surfaceMapName;
		std::string normalMapName;
		std::string displacementMapName;

		// Non-serialized attributes:
		wiGraphicsTypes::Texture2D* baseColorMap = nullptr;
		wiGraphicsTypes::Texture2D* surfaceMap = nullptr;
		wiGraphicsTypes::Texture2D* normalMap = nullptr;
		wiGraphicsTypes::Texture2D* displacementMap = nullptr;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer> constantBuffer;

		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 128);
			userStencilRef = value & 0x0F;
		}
		inline UINT GetStencilRef() const
		{
			return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
		}

		wiGraphicsTypes::Texture2D* GetBaseColorMap() const;
		wiGraphicsTypes::Texture2D* GetNormalMap() const;
		wiGraphicsTypes::Texture2D* GetSurfaceMap() const;
		wiGraphicsTypes::Texture2D* GetDisplacementMap() const;

		inline float GetOpacity() const { return baseColor.w; }


		inline void SetDirty() { _flags |= DIRTY; }
		inline void SetClean() { _flags &= ~DIRTY; }
		inline bool IsDirty() const { return _flags & DIRTY; }

		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetPlanarReflections(bool value) { if (value) { _flags |= PLANAR_REFLECTION; } else { _flags &= PLANAR_REFLECTION; } }
		inline void SetWater(bool value) { if (value) { _flags |= WATER; } else { _flags &= ~WATER; } }

		inline bool IsTransparent() const { return GetOpacity() < 1.0f; }
		inline bool IsWater() const { return _flags & WATER; }
		inline bool HasPlanarReflection() const { return (_flags & PLANAR_REFLECTION) || IsWater(); }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }

		inline void SetBaseColor(const XMFLOAT4& value) { SetDirty(); baseColor = value; }
		inline void SetRoughness(float value) { SetDirty(); roughness = value; }
		inline void SetReflectance(float value) { SetDirty(); reflectance = value; }
		inline void SetMetalness(float value) { SetDirty(); metalness = value; }
		inline void SetEmissive(float value) { SetDirty(); emissive = value; }
		inline void SetRefractionIndex(float value) { SetDirty(); refractionIndex = value; }
		inline void SetSubsurfaceScattering(float value) { SetDirty(); subsurfaceScattering = value; }
		inline void SetNormalMapStrength(float value) { SetDirty(); normalMapStrength = value; }
		inline void SetParallaxOcclusionMapping(float value) { SetDirty(); parallaxOcclusionMapping = value; }
		inline void SetOpacity(float value) { SetDirty(); baseColor.w = value; }
		inline void SetAlphaRef(float value) { alphaRef = value; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct MeshComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RENDERABLE = 1 << 0,
			DOUBLE_SIDED = 1 << 1,
			DYNAMIC = 1 << 2,
		};
		uint32_t _flags = RENDERABLE;

		std::vector<XMFLOAT3>		vertex_positions;
		std::vector<XMFLOAT3>		vertex_normals;
		std::vector<XMFLOAT2>		vertex_texcoords;
		std::vector<XMUINT4>		vertex_boneindices;
		std::vector<XMFLOAT4>		vertex_boneweights;
		std::vector<XMFLOAT2>		vertex_atlas;
		std::vector<uint32_t>		vertex_colors;
		std::vector<uint32_t>		indices;

		struct MeshSubset
		{
			wiECS::Entity materialID = wiECS::INVALID_ENTITY;
			uint32_t indexOffset = 0;
			uint32_t indexCount = 0;
		};
		std::vector<MeshSubset>		subsets;

		float tessellationFactor = 0.0f;
		float impostorDistance = 100.0f;
		wiECS::Entity armatureID = wiECS::INVALID_ENTITY;

		// Non-serialized attributes:
		AABB aabb;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	indexBuffer;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_POS;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_TEX;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_BON;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_COL;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_ATL;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	streamoutBuffer_POS;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	streamoutBuffer_PRE;
		wiRenderTarget impostorTarget;


		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetDoubleSided(bool value) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }

		inline bool IsRenderable() const { return _flags & RENDERABLE; }
		inline bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }

		bool HasImpostor() const { return impostorTarget.IsInitialized(); }
		inline float GetTessellationFactor() const { return tessellationFactor; }
		inline wiGraphicsTypes::INDEXBUFFER_FORMAT GetIndexFormat() const { return vertex_positions.size() > 65535 ? wiGraphicsTypes::INDEXFORMAT_32BIT : wiGraphicsTypes::INDEXFORMAT_16BIT; }
		inline bool IsSkinned() const { return armatureID != wiECS::INVALID_ENTITY; }

		void CreateRenderData();
		void ComputeNormals(bool smooth);
		void FlipCulling();
		void FlipNormals();

		void Serialize(wiArchive& archive, uint32_t seed = 0);


		struct Vertex_POS
		{
			XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
			uint32_t normal_subsetIndex = 0;

			void FromFULL(const XMFLOAT3& _pos, const XMFLOAT3& _nor, uint32_t subsetIndex)
			{
				pos.x = _pos.x;
				pos.y = _pos.y;
				pos.z = _pos.z;
				MakeFromParams(_nor, subsetIndex);
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMLoadFloat3(&pos);
			}
			inline XMVECTOR LoadNOR() const
			{
				return XMLoadFloat3(&GetNor_FULL());
			}
			inline void MakeFromParams(const XMFLOAT3& normal)
			{
				normal_subsetIndex = normal_subsetIndex & 0xFF000000; // reset only the normals

				normal_subsetIndex |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
				normal_subsetIndex |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
				normal_subsetIndex |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
			}
			inline void MakeFromParams(const XMFLOAT3& normal, uint32_t subsetIndex)
			{
				assert(subsetIndex < 256); // subsetIndex is packed onto 8 bits

				normal_subsetIndex = 0;

				normal_subsetIndex |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
				normal_subsetIndex |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
				normal_subsetIndex |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
				normal_subsetIndex |= (subsetIndex & 0x000000FF) << 24;
			}
			inline XMFLOAT3 GetNor_FULL() const
			{
				XMFLOAT3 nor_FULL(0, 0, 0);

				nor_FULL.x = (float)((normal_subsetIndex >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
				nor_FULL.y = (float)((normal_subsetIndex >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
				nor_FULL.z = (float)((normal_subsetIndex >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;

				return nor_FULL;
			}
			inline uint32_t GetMaterialIndex() const
			{
				return (normal_subsetIndex >> 24) & 0x000000FF;
			}

			static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R32G32B32A32_FLOAT;
		};
		struct Vertex_TEX
		{
			XMHALF2 tex = XMHALF2(0.0f, 0.0f);

			void FromFULL(const XMFLOAT2& texcoords)
			{
				tex = XMHALF2(texcoords.x, texcoords.y);
			}

			static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R16G16_FLOAT;
		};
		struct Vertex_BON
		{
			uint64_t ind = 0;
			uint64_t wei = 0;

			void FromFULL(const XMUINT4& boneIndices, const XMFLOAT4& boneWeights)
			{
				ind = 0;
				wei = 0;

				ind |= (uint64_t)boneIndices.x << 0;
				ind |= (uint64_t)boneIndices.y << 16;
				ind |= (uint64_t)boneIndices.z << 32;
				ind |= (uint64_t)boneIndices.w << 48;

				wei |= (uint64_t)(boneWeights.x * 65535.0f) << 0;
				wei |= (uint64_t)(boneWeights.y * 65535.0f) << 16;
				wei |= (uint64_t)(boneWeights.z * 65535.0f) << 32;
				wei |= (uint64_t)(boneWeights.w * 65535.0f) << 48;
			}
			inline XMUINT4 GetInd_FULL() const
			{
				XMUINT4 ind_FULL(0, 0, 0, 0);

				ind_FULL.x = ((ind >> 0) & 0x0000FFFF);
				ind_FULL.y = ((ind >> 16) & 0x0000FFFF);
				ind_FULL.z = ((ind >> 32) & 0x0000FFFF);
				ind_FULL.w = ((ind >> 48) & 0x0000FFFF);

				return ind_FULL;
			}
			inline XMFLOAT4 GetWei_FULL() const
			{
				XMFLOAT4 wei_FULL(0, 0, 0, 0);

				wei_FULL.x = (float)((wei >> 0) & 0x0000FFFF) / 65535.0f;
				wei_FULL.y = (float)((wei >> 16) & 0x0000FFFF) / 65535.0f;
				wei_FULL.z = (float)((wei >> 32) & 0x0000FFFF) / 65535.0f;
				wei_FULL.w = (float)((wei >> 48) & 0x0000FFFF) / 65535.0f;

				return wei_FULL;
			}
		};

	};

	struct ObjectComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RENDERABLE = 1 << 0,
			CAST_SHADOW = 1 << 1,
			DYNAMIC = 1 << 2,
		};
		uint32_t _flags = RENDERABLE | CAST_SHADOW;

		wiECS::Entity meshID = wiECS::INVALID_ENTITY;
		uint32_t cascadeMask = 0; // which shadow cascades to skip (0: skip none, 1: skip first, etc...)
		uint32_t rendertypeMask = 0;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);

		// Non-serialized attributes:

		// occlusion result history bitfield (32 bit->32 frame history)
		uint32_t occlusionHistory = ~0;
		// occlusion query pool index
		int occlusionQueryID = -1;

		inline bool IsOccluded() const
		{
			// Perform a conservative occlusion test:
			// If it is visible in any frames in the history, it is determined visible in this frame
			// But if all queries failed in the history, it is occluded.
			// If it pops up for a frame after occluded, it is visible again for some frames
			return ((occlusionQueryID >= 0) && (occlusionHistory & 0xFFFFFFFF) == 0);
		}

		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }

		inline bool IsRenderable() const { return _flags & RENDERABLE; }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }

		inline float GetTransparency() const { return 1 - color.w; }
		inline uint32_t GetRenderTypes() const { return rendertypeMask; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct RigidBodyPhysicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLE_DEACTIVATION = 1 << 0,
			KINEMATIC = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		enum CollisionShape
		{
			BOX,
			SPHERE,
			CAPSULE,
			CONVEX_HULL,
			TRIANGLE_MESH,
			ENUM_FORCE_UINT32 = 0xFFFFFFFF
		};
		CollisionShape shape;
		bool kinematic = false;
		float mass = 1.0f;
		float friction = 1.0f;
		float restitution = 1.0f;
		float damping = 1.0f;

		// Non-serialized attributes:
		int physicsObjectID = -1;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct SoftBodyPhysicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLE_DEACTIVATION = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		float mass = 1.0f;
		float friction = 1.0f;
		std::vector<XMFLOAT3> physicsvertices;
		std::vector<uint32_t> physicsindices;

		// Non-serialized attributes:
		int physicsObjectID = -1;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct ArmatureComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		std::vector<wiECS::Entity> boneCollection;
		std::vector<XMFLOAT4X4> inverseBindMatrices;
		XMFLOAT4X4 remapMatrix = IDENTITYMATRIX; // Use this to eg. mirror the armature

		// Non-serialized attributes:
		std::vector<XMFLOAT4X4> skinningMatrices;

		GFX_STRUCT ShaderBoneType
		{
			XMFLOAT4A pose0;
			XMFLOAT4A pose1;
			XMFLOAT4A pose2;

			void Create(const XMFLOAT4X4& matIn)
			{
				pose0 = XMFLOAT4A(matIn._11, matIn._21, matIn._31, matIn._41);
				pose1 = XMFLOAT4A(matIn._12, matIn._22, matIn._32, matIn._42);
				pose2 = XMFLOAT4A(matIn._13, matIn._23, matIn._33, matIn._43);
			}

			ALIGN_16
		};
		std::vector<ShaderBoneType> boneData;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer> boneBuffer;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct LightComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			CAST_SHADOW = 1 << 0,
			VOLUMETRICS = 1 << 1,
			VISUALIZER = 1 << 2,
		};
		uint32_t _flags = EMPTY;
		XMFLOAT3 color = XMFLOAT3(1, 1, 1);

		enum LightType 
		{
			DIRECTIONAL = ENTITY_TYPE_DIRECTIONALLIGHT,
			POINT = ENTITY_TYPE_POINTLIGHT,
			SPOT = ENTITY_TYPE_SPOTLIGHT,
			SPHERE = ENTITY_TYPE_SPHERELIGHT,
			DISC = ENTITY_TYPE_DISCLIGHT,
			RECTANGLE = ENTITY_TYPE_RECTANGLELIGHT,
			TUBE = ENTITY_TYPE_TUBELIGHT,
			LIGHTTYPE_COUNT,
			ENUM_FORCE_UINT32 = 0xFFFFFFFF,
		};
		LightType type = POINT;
		float energy = 1.0f;
		float range = 10.0f;
		float fov = XM_PIDIV4;
		float shadowBias = 0.0001f;
		float radius = 1.0f; // area light
		float width = 1.0f;  // area light
		float height = 1.0f; // area light

		std::vector<std::string> lensFlareNames;

		// Non-serialized attributes:
		XMFLOAT3 position;
		XMFLOAT3 direction;
		XMFLOAT4 rotation;
		XMFLOAT3 front;
		XMFLOAT3 right;
		int shadowMap_index = -1;
		int entityArray_index = -1;

		std::vector<wiGraphicsTypes::Texture2D*> lensFlareRimTextures;

		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetVolumetricsEnabled(bool value) { if (value) { _flags |= VOLUMETRICS; } else { _flags &= ~VOLUMETRICS; } }
		inline void SetVisualizerEnabled(bool value) { if (value) { _flags |= VISUALIZER; } else { _flags &= ~VISUALIZER; } }

		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsVolumetricsEnabled() const { return _flags & VOLUMETRICS; }
		inline bool IsVisualizerEnabled() const { return _flags & VISUALIZER; }

		inline float GetRange() const { return range; }

		inline void SetType(LightType val) {
			type = val;
			switch (type)
			{
			case DIRECTIONAL:
			case SPOT:
				shadowBias = 0.0001f;
				break;
			case POINT:
			case SPHERE:
			case DISC:
			case RECTANGLE:
			case TUBE:
			case LIGHTTYPE_COUNT:
				shadowBias = 0.1f;
				break;
			}
		}
		inline LightType GetType() const { return type; }

		struct SHCAM 
		{
			XMFLOAT4X4 View, Projection;
			XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...
			XMFLOAT3 Eye, At, Up;
			float nearplane, farplane, size;

			SHCAM() {
				nearplane = 0.1f; farplane = 200, size = 0;
				Init(XMQuaternionIdentity());
				Create_Perspective(XM_PI / 2.0f);
			}
			//orthographic
			SHCAM(float size, const XMVECTOR& dir, float nearP, float farP) {
				nearplane = nearP;
				farplane = farP;
				Init(dir);
				Create_Ortho(size);
			};
			//perspective
			SHCAM(const XMFLOAT4& dir, float newNear, float newFar, float newFov) {
				size = 0;
				nearplane = newNear;
				farplane = newFar;
				Init(XMLoadFloat4(&dir));
				Create_Perspective(newFov);
			};
			void Init(const XMVECTOR& dir) {
				XMMATRIX rot = XMMatrixRotationQuaternion(dir);
				XMVECTOR rEye = XMVectorSet(0, 0, 0, 0);
				XMVECTOR rAt = XMVector3Transform(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), rot);
				XMVECTOR rUp = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rot);
				XMMATRIX rView = XMMatrixLookAtLH(rEye, rAt, rUp);

				XMStoreFloat3(&Eye, rEye);
				XMStoreFloat3(&At, rAt);
				XMStoreFloat3(&Up, rUp);
				XMStoreFloat4x4(&View, rView);
			}
			void Create_Ortho(float size) {
				XMMATRIX rProjection = XMMatrixOrthographicOffCenterLH(-size * 0.5f, size*0.5f, -size * 0.5f, size*0.5f, farplane, nearplane);
				XMStoreFloat4x4(&Projection, rProjection);
				rProjection = XMMatrixOrthographicOffCenterLH(-size * 0.5f, size*0.5f, -size * 0.5f, size*0.5f, nearplane, farplane);
				XMStoreFloat4x4(&realProjection, rProjection);
				this->size = size;
			}
			void Create_Perspective(float fov) {
				XMMATRIX rProjection = XMMatrixPerspectiveFovLH(fov, 1, farplane, nearplane);
				XMStoreFloat4x4(&Projection, rProjection);
				rProjection = XMMatrixPerspectiveFovLH(fov, 1, nearplane, farplane);
				XMStoreFloat4x4(&realProjection, rProjection);
			}
			void Update(const XMVECTOR& pos) {
				XMStoreFloat4x4(&View, XMMatrixTranslationFromVector(-pos)
					* XMMatrixLookAtLH(XMLoadFloat3(&Eye), XMLoadFloat3(&At), XMLoadFloat3(&Up))
				);
			}
			void Update(const XMMATRIX& mat) {
				XMVECTOR sca, rot, tra;
				XMMatrixDecompose(&sca, &rot, &tra, mat);

				XMMATRIX mRot = XMMatrixRotationQuaternion(rot);

				XMVECTOR rEye = XMVectorAdd(XMLoadFloat3(&Eye), tra);
				XMVECTOR rAt = XMVectorAdd(XMVector3Transform(XMLoadFloat3(&At), mRot), tra);
				XMVECTOR rUp = XMVector3Transform(XMLoadFloat3(&Up), mRot);

				XMStoreFloat4x4(&View,
					XMMatrixLookAtLH(rEye, rAt, rUp)
				);
			}
			void Update(const XMMATRIX& rot, const XMVECTOR& tra)
			{
				XMVECTOR rEye = XMVectorAdd(XMLoadFloat3(&Eye), tra);
				XMVECTOR rAt = XMVectorAdd(XMVector3Transform(XMLoadFloat3(&At), rot), tra);
				XMVECTOR rUp = XMVector3Transform(XMLoadFloat3(&Up), rot);

				XMStoreFloat4x4(&View,
					XMMatrixLookAtLH(rEye, rAt, rUp)
				);
			}
			XMMATRIX getVP() const {
				return XMMatrixTranspose(XMLoadFloat4x4(&View)*XMLoadFloat4x4(&Projection));
			}
		};

		std::vector<SHCAM> shadowCam_pointLight;
		std::vector<SHCAM> shadowCam_dirLight;
		std::vector<SHCAM> shadowCam_spotLight;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct CameraComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		float width = 0.0f;
		float height = 0.0f;
		float zNearP = 0.001f;
		float zFarP = 800.0f;
		float fov = XM_PI / 3.0f;

		// Non-serialized attributes:
		XMFLOAT3 Eye, At, Up;
		XMFLOAT3X3 rotationMatrix;
		XMFLOAT4X4 View, Projection, VP;
		Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...

		void CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV = XM_PI / 3.0f);
		void UpdateProjection();
		void UpdateCamera(const TransformComponent* transform = nullptr);
		void Reflect(const XMFLOAT4& plane = XMFLOAT4(0, 1, 0, 0));

		inline XMVECTOR GetEye() const { return XMLoadFloat3(&Eye); }
		inline XMVECTOR GetAt() const { return XMLoadFloat3(&At); }
		inline XMVECTOR GetUp() const { return XMLoadFloat3(&Up); }
		inline XMVECTOR GetRight() const { return XMVector3Cross(GetAt(), GetUp()); }
		inline XMMATRIX GetView() const { return XMLoadFloat4x4(&View); }
		inline XMMATRIX GetInvView() const { return XMLoadFloat4x4(&InvView); }
		inline XMMATRIX GetProjection() const { return XMLoadFloat4x4(&Projection); }
		inline XMMATRIX GetInvProjection() const { return XMLoadFloat4x4(&InvProjection); }
		inline XMMATRIX GetViewProjection() const { return XMLoadFloat4x4(&VP); }
		inline XMMATRIX GetInvViewProjection() const { return XMLoadFloat4x4(&InvVP); }
		inline XMMATRIX GetRealProjection() const { return XMLoadFloat4x4(&realProjection); }

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct EnvironmentProbeComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			REALTIME = 1 << 1,
		};
		uint32_t _flags = DIRTY;

		// Non-serialized attributes:
		int textureIndex = -1;
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 inverseMatrix;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline void SetRealTime(bool value) { if (value) { _flags |= REALTIME; } else { _flags &= ~REALTIME; } }

		inline bool IsDirty() const { return _flags & DIRTY; }
		inline bool IsRealTime() const { return _flags & REALTIME; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct ForceFieldComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		int type = ENTITY_TYPE_FORCEFIELD_POINT;
		float gravity = 0.0f; // negative = deflector, positive = attractor
		float range = 0.0f; // affection range

		// Non-serialized attributes:
		XMFLOAT3 position;
		XMFLOAT3 direction;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct DecalComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		// Non-serialized attributes:
		XMFLOAT4 color;
		float emissive;
		XMFLOAT3 front;
		XMFLOAT3 position;
		float range;
		XMFLOAT4 atlasMulAdd;
		XMFLOAT4X4 world;

		wiGraphicsTypes::Texture2D* texture = nullptr;
		wiGraphicsTypes::Texture2D* normal = nullptr;

		inline float GetOpacity() const { return color.w; }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct AnimationComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
		};
		uint32_t _flags = LOOPED;
		float start = 0;
		float end = 0;
		float timer = 0.0f;

		struct AnimationChannel
		{
			enum FLAGS
			{
				EMPTY = 0,
			};
			uint32_t _flags = EMPTY;

			enum Path
			{
				TRANSLATION,
				ROTATION,
				SCALE,
				UNKNOWN,
				TYPE_FORCE_UINT32 = 0xFFFFFFFF
			} path = TRANSLATION;

			wiECS::Entity target = wiECS::INVALID_ENTITY;
			uint32_t samplerIndex = 0;
		};
		struct AnimationSampler
		{
			enum FLAGS
			{
				EMPTY = 0,
			};
			uint32_t _flags = EMPTY;

			enum Mode
			{
				LINEAR,
				STEP,
				MODE_FORCE_UINT32 = 0xFFFFFFFF
			} mode = LINEAR;

			std::vector<float> keyframe_times;
			std::vector<float> keyframe_data;
		};

		std::vector<AnimationChannel> channels;
		std::vector<AnimationSampler> samplers;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }
		inline float GetLength() const { return end - start; }

		inline void Play() { _flags |= PLAYING; }
		inline void Pause() { _flags &= ~PLAYING; }
		inline void Stop() { Pause(); timer = 0.0f; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct WeatherComponent
	{
		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		float fogStart = 100;
		float fogEnd = 1000;
		float fogHeight = 0;
		float cloudiness = 0.0f;
		float cloudScale = 0.0003f;
		float cloudSpeed = 0.1f;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;

		void Serialize(wiArchive& archive, uint32_t seed = 0);
	};

	struct Scene
	{
		wiECS::ComponentManager<NameComponent> names;
		wiECS::ComponentManager<LayerComponent> layers;
		wiECS::ComponentManager<TransformComponent> transforms;
		wiECS::ComponentManager<PreviousFrameTransformComponent> prev_transforms;
		wiECS::ComponentManager<HierarchyComponent> hierarchy;
		wiECS::ComponentManager<MaterialComponent> materials;
		wiECS::ComponentManager<MeshComponent> meshes;
		wiECS::ComponentManager<ObjectComponent> objects;
		wiECS::ComponentManager<AABB> aabb_objects;
		wiECS::ComponentManager<RigidBodyPhysicsComponent> rigidbodies;
		wiECS::ComponentManager<SoftBodyPhysicsComponent> softbodies;
		wiECS::ComponentManager<ArmatureComponent> armatures;
		wiECS::ComponentManager<LightComponent> lights;
		wiECS::ComponentManager<AABB> aabb_lights;
		wiECS::ComponentManager<CameraComponent> cameras;
		wiECS::ComponentManager<EnvironmentProbeComponent> probes;
		wiECS::ComponentManager<AABB> aabb_probes;
		wiECS::ComponentManager<ForceFieldComponent> forces;
		wiECS::ComponentManager<DecalComponent> decals;
		wiECS::ComponentManager<AABB> aabb_decals;
		wiECS::ComponentManager<AnimationComponent> animations;
		wiECS::ComponentManager<wiEmittedParticle> emitters;
		wiECS::ComponentManager<wiHairParticle> hairs;
		wiECS::ComponentManager<WeatherComponent> weathers;

		// Non-serialized attributes:
		AABB bounds;
		XMFLOAT4 waterPlane = XMFLOAT4(0, 1, 0, 0);
		WeatherComponent weather;

		// Update all components by a given timestep (in seconds):
		void Update(float dt);
		// Remove everything from the scene that it owns:
		void Clear();
		// Merge with an other scene.
		void Merge(Scene& other);
		// Count how many entities there are in the scene:
		size_t CountEntities() const;

		// Removes a specific entity from the scene (if it exists):
		void Entity_Remove(wiECS::Entity entity);
		// Finds the first entity by the name (if it exists, otherwise returns INVALID_ENTITY):
		wiECS::Entity Entity_FindByName(const std::string& name);
		// Duplicates all of an entity's components and creates a new entity with them:
		wiECS::Entity Entity_Duplicate(wiECS::Entity entity);
		// Serializes entity and all of its components to archive:
		//	You can specify entity = INVALID_ENTITY when the entity needs to be created from archive
		//	You can specify seed = 0 when the archive is guaranteed to be storing persistent and unique entities
		//	propagateDeepSeed : request that entity references inside components should be seeded as well
		//	Returns either the new entity that was read, or the original entity that was written
		wiECS::Entity Entity_Serialize(wiArchive& archive, wiECS::Entity entity = wiECS::INVALID_ENTITY, uint32_t seed = 0, bool propagateSeedDeep = true);

		wiECS::Entity Entity_CreateMaterial(
			const std::string& name
		);
		wiECS::Entity Entity_CreateObject(
			const std::string& name
		);
		wiECS::Entity Entity_CreateMesh(
			const std::string& name
		);
		wiECS::Entity Entity_CreateLight(
			const std::string& name, 
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0), 
			const XMFLOAT3& color = XMFLOAT3(1, 1, 1), 
			float energy = 1, 
			float range = 10
		);
		wiECS::Entity Entity_CreateForce(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateEnvironmentProbe(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateDecal(
			const std::string& name,
			const std::string& textureName,
			const std::string& normalMapName = ""
		);
		wiECS::Entity Entity_CreateCamera(
			const std::string& name,
			float width, float height, float nearPlane = 0.01f, float farPlane = 1000.0f, float fov = XM_PIDIV4
		);
		wiECS::Entity Entity_CreateEmitter(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateHair(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);

		// Attaches an entity to a parent:
		void Component_Attach(wiECS::Entity entity, wiECS::Entity parent);
		// Detaches the entity from its parent (if it is attached):
		void Component_Detach(wiECS::Entity entity);
		// Detaches all children from an entity (if there are any):
		void Component_DetachChildren(wiECS::Entity parent);

		void Serialize(wiArchive& archive);
	};

	void RunPreviousFrameTransformUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<PreviousFrameTransformComponent>& prev_transforms
	);
	void RunAnimationUpdateSystem(
		wiECS::ComponentManager<AnimationComponent>& animations,
		wiECS::ComponentManager<TransformComponent>& transforms,
		float dt
	);
	void RunPhysicsUpdateSystem(
		wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<MeshComponent>& meshes,
		wiECS::ComponentManager<ObjectComponent>& objects,
		wiECS::ComponentManager<RigidBodyPhysicsComponent>& rigidbodies,
		wiECS::ComponentManager<SoftBodyPhysicsComponent>& softbodies,
		float dt
	);
	void RunTransformUpdateSystem(wiECS::ComponentManager<TransformComponent>& transforms);
	void RunHierarchyUpdateSystem(
		const wiECS::ComponentManager<HierarchyComponent>& hierarchy,
		wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<LayerComponent>& layers
	);
	void RunArmatureUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<ArmatureComponent>& armatures
	);
	void RunMaterialUpdateSystem(wiECS::ComponentManager<MaterialComponent>& materials, float dt);
	void RunObjectUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		const wiECS::ComponentManager<MeshComponent>& meshes,
		const wiECS::ComponentManager<MaterialComponent>& materials,
		wiECS::ComponentManager<ObjectComponent>& objects,
		wiECS::ComponentManager<AABB>& aabb_objects,
		AABB& sceneBounds,
		XMFLOAT4& waterPlane
	);
	void RunCameraUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<CameraComponent>& cameras
	);
	void RunDecalUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		const wiECS::ComponentManager<MaterialComponent>& materials,
		wiECS::ComponentManager<AABB>& aabb_decals,
		wiECS::ComponentManager<DecalComponent>& decals
	);
	void RunProbeUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<AABB>& aabb_probes,
		wiECS::ComponentManager<EnvironmentProbeComponent>& probes
	);
	void RunForceUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<ForceFieldComponent>& forces
	);
	void RunLightUpdateSystem(
		const CameraComponent& cascadeCamera,
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<AABB>& aabb_lights,
		wiECS::ComponentManager<LightComponent>& lights,
		WeatherComponent* weather = nullptr
	);
	void RunParticleUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		const wiECS::ComponentManager<MeshComponent>& meshes,
		wiECS::ComponentManager<wiEmittedParticle>& emitters,
		wiECS::ComponentManager<wiHairParticle>& hairs,
		float dt
	);

}
