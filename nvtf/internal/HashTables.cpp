#include "HashTables.hpp"

namespace HashTables {
	namespace Setting {
		bool bResizeHashtables = true;
	}

	void ReadINI(const char* iniPath) {
		Setting::bResizeHashtables = GetPrivateProfileInt("Hashtables", "bResizeHashtables", 1, iniPath);
	}

	void InitHooks() {
		if (!Setting::bResizeHashtables) [[unlikely]]
			return;

		HookUtils::SafeWrite32(0x473F69, 5009);		// NiTMap<unsigned int,TESFile *> - Owned Temp ID Map (ONAM)

		HookUtils::SafeWrite32(0x6B5C76, 10037);	// NiTPointerMap<unsigned int,NavMeshInfo *> - NavMeshInfoMap
		HookUtils::SafeWrite32(0x6B7A30, 2819);		// NiTMap<unsigned int,BSSimpleArray<NavMeshInfo *,1024> *> - NavMeshInfoMap's world NavMeshInfos

		HookUtils::SafeWrite32(0x845558, 7057);		// NiTPointerMap<unsigned int,BGSFormChanges *> - BGSSaveLoadChangesMap
		HookUtils::SafeWrite32(0x846FFB, 12049);	// BGSSaveLoadFormIDMap - BGSSaveLoadGame's FormID map (constructor)
		HookUtils::SafeWrite32(0x848072, 12049);	// BGSSaveLoadFormIDMap - BGSSaveLoadGame's FormID map (save loader)
		HookUtils::SafeWrite8(0x84703E, 59);		// BGSSaveLoadFormIDMap - BGSSaveLoadGame's worldspace FormID map
		HookUtils::SafeWrite8(0x8470FA, 59);		// NiTMap<unsigned int,unsigned int> - BGSSaveLoadGame's changed FormID map
		HookUtils::SafeWrite8(0x84AB60, 127);		// NiTMap<__int64,int> - BGSSaveLoadGame's expired cell map

		HookUtils::SafeWrite8(0x544FA7, 41);		// NiTMap<TESObjectREFR *,NiNode *> - TESObjectCELL's animated references
		HookUtils::SafeWrite8(0x544FC9, 29);		// NiTMap<TESObjectREFR *,NiNode *> - TESObjectCELL's external emittance objects

		HookUtils::SafeWrite8(0x582CA2, 127);		// NiTPointerMap<unsigned int,BSSimpleList<TESObjectREFR *> *> - TESWorldSpace's fixed persistent references
		HookUtils::SafeWrite8(0x582CEF, 53);		// NiTMap<TESFile *,TESWorldSpace::OFFSET_DATA *> - TESWorldSpace's file offset map
		HookUtils::SafeWrite32(0x583FF6, 1709);		// NiTPointerMap<int,TESObjectCELL *> - TESWorldSpace's cell map (form loader)
		HookUtils::SafeWrite8(0x582D64, 31);		// NiTPointerMap<int,TESObjectCELL *> - TESWorldSpace's cell map (constructor)
		HookUtils::SafeWrite8(0x587AC9, 43);		// NiTPointerMap<unsigned int,BSSimpleList<TESObjectREFR *> *> - TESWorldSpace's overlapped multibounds

		HookUtils::SafeWrite8(0x6C02F8, 127);		// NiTMap<unsigned int,NiPointer<ReferenceObstacleArray>> - NavMeshObstacleManager's obstacle map
		HookUtils::SafeWrite8(0x6C035F, 97);		// NiTMap<bhkRigidBody *,NiPointer<ObstacleData>> - NavMeshObstacleManager's obstacle data map
		HookUtils::SafeWrite8(0x6C0397, 97);		// NiTMap<unsigned int,NiPointer<ReferenceObstacleArray>> - NavMeshObstacleManager's open doors map
		HookUtils::SafeWrite8(0x6C03AB, 89);		// NiTMap<unsigned int,NiPointer<ReferenceObstacleArray>> - NavMeshObstacleManager's closed doors map

		HookUtils::SafeWrite8(0x6E13AF, 53);		// NiTMap<unsigned int,unsigned int> - PathingLOSMap

		HookUtils::SafeWrite8(0xAD9169, 113);		// NiTMapBase<NiTPointerAllocator<unsigned int>,int,BSGameSound *> - BSAudioManager's playing sounds
		HookUtils::SafeWrite8(0xAD9189, 113);		// NiTMapBase<NiTPointerAllocator<unsigned int>,int,BSSoundInfo *> - BSAudioManager's playing sound infos
		HookUtils::SafeWrite8(0xAD91CC, 41);		// NiTMapBase<NiTPointerAllocator<unsigned int>,int,NiPointer<NiAVObject>> - BSAudioManager's moving sounds
	}
}