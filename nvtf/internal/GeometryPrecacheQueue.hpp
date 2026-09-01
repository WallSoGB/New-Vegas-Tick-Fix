#pragma once

#include "Game/Gamebryo/NiObject.hpp"
#include "Game/Gamebryo/NiDX9Renderer.hpp"
#include <vector>
#include <atomic>

#include <shared/BSMemory/BSScrapMemory.hpp>
template<class T>
using BSScrapVector = std::vector<T, BSScrapAllocator<T>>;

class NiD3DShaderDeclaration;
class NiNode;

class GeometryPrecacheQueue : public NiDX9Renderer {
public:
	static void InitHooks();

	void PurgeGeometryData(NiObject* apGeomData);
	bool PrecacheGeometry_MT(NiObject* apGeometry, uint32_t auiBonesPerPartition, uint32_t auiBonesPerVertex, NiD3DShaderDeclaration* apShaderDeclaration);
	void PerformPrecache_MT();

private:
	struct QueuedObject {
		NiPointer<NiObject>		spGeometry;
		NiObject*				pGeometryData;
		uint32_t				uiBonesPerPartition = 0;
		uint32_t				uiBonesPerVertex	= 0;
		NiD3DShaderDeclaration* pShaderDeclaration	= nullptr;
	};

	static HANDLE hTaskEvent;
	static HANDLE hPauseEvent;

	static std::atomic_bool							bIsProcessing;
	static SRWLOCK									kQueueLock;
	static std::vector<QueuedObject>				kQueue;
	static HANDLE 									hThread;

	void StartProcessing();
	void StopProcessing();

	static DWORD __stdcall ThreadProc(LPVOID lpThreadParameter);
	static bool InitializeThread();
};