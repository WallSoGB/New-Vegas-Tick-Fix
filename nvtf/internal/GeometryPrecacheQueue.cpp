#include "GeometryPrecacheQueue.hpp"
#include "Game/Bethesda/TESMain.hpp"
#include "Game/Gamebryo/NiDX9Renderer.hpp"

#define DEBUG_PRINTS 0

#if DEBUG_PRINTS
#define DEBUG_MSG(...) _MESSAGE(__VA_ARGS__)
#else
#define DEBUG_MSG(...) __noop(__VA_ARGS__)
#endif

std::atomic_bool									GeometryPrecacheQueue::bIsProcessing = false;
SRWLOCK												GeometryPrecacheQueue::kQueueLock = SRWLOCK_INIT;
std::vector<GeometryPrecacheQueue::QueuedObject>    GeometryPrecacheQueue::kQueue;
HANDLE 												GeometryPrecacheQueue::hThread;
HANDLE												GeometryPrecacheQueue::hTaskEvent;
HANDLE												GeometryPrecacheQueue::hPauseEvent;

NiObject* __fastcall GetModelData(const NiObject* apGeometry) {
	return ThisCall<NiObject*>(0x5495F0, apGeometry);
}

const char* __fastcall GetGeometryName(const NiObject* apGeometry) {
	return *ThisCall<const char**>(0x413F40, apGeometry);
}

HookUtils::VirtFuncDetour kPrecacheGeometryDetour;
bool __fastcall PrecacheGeometry_ST(NiDX9Renderer* apThis, NiObject* apGeometry, uint32_t auiBonesPerPartition, uint32_t auiBonesPerVertex, NiD3DShaderDeclaration* apShaderDeclaration) {
	return ThisCall<bool>(kPrecacheGeometryDetour, apThis, apGeometry, auiBonesPerPartition, auiBonesPerVertex, apShaderDeclaration);
}

HookUtils::VirtFuncDetour kPurgeGeometryDataDetour;
void GeometryPrecacheQueue::PurgeGeometryData(NiObject* apGeomData) {
	const DWORD dwThreadID = GetCurrentThreadId();
	if (bIsProcessing) [[unlikely]] {
		DEBUG_MSG("[ %08X ] Purging geometry data %p while processing queue", dwThreadID, apGeomData);
	}

	AcquireSRWLockExclusive(&kQueueLock);
	const uint32_t uiQueueSize = kQueue.size();
	for (uint32_t i = 0; i < uiQueueSize; ++i) {
		QueuedObject& rObject = kQueue[i];
		if (rObject.pGeometryData == apGeomData) {
			DEBUG_MSG("[ %08X ] Purging geometry data %p for \"%s\" from queue", dwThreadID, apGeomData, GetGeometryName(rObject.spGeometry));
			rObject.spGeometry = nullptr;
			rObject.pGeometryData = nullptr;
			rObject = std::move(kQueue.back());
			kQueue.pop_back();
			break;
		}
	}
	ReleaseSRWLockExclusive(&kQueueLock);
	ThisCall(kPurgeGeometryDataDetour, this, apGeomData);
}

void GeometryPrecacheQueue::InitHooks() {
	if (!InitializeThread())
		return;

	kPrecacheGeometryDetour.ReplaceVirtualFunc(0x10EE5A8, &PrecacheGeometry_MT);
	HookUtils::WriteRelJump(0xE74120, &PerformPrecache_MT);

	kPurgeGeometryDataDetour.ReplaceVirtualFunc(0x10EE5AC, &PurgeGeometryData);

	HookUtils::ReplaceCall(0x86FF94, &StopProcessing);
	HookUtils::ReplaceCall(0x870594, &StartProcessing);
}

bool GeometryPrecacheQueue::PrecacheGeometry_MT(NiObject* apGeometry, uint32_t auiBonesPerPartition, uint32_t auiBonesPerVertex, NiD3DShaderDeclaration* apShaderDeclaration) {
	if (!apGeometry || !apGeometry->IsTriBasedGeometry()) [[unlikely]]
		return false;

	const DWORD dwThreadID = GetCurrentThreadId();
	if (dwThreadID == TESMain::GetSingleton()->uiMainThreadID) [[unlikely]] {
		DEBUG_MSG("[ %08X ] Pre-caching geometry \"%s\" with data %p on main thread", dwThreadID, GetGeometryName(apGeometry), GetModelData(apGeometry));
		return PrecacheGeometry_ST(this, apGeometry, auiBonesPerPartition, auiBonesPerVertex, apShaderDeclaration);
	}

	AcquireSRWLockExclusive(&kQueueLock);
	DEBUG_MSG("[ %08X ] Queuing geometry \"%s\" with data %p for precaching", dwThreadID, GetGeometryName(apGeometry), GetModelData(apGeometry));
	kQueue.emplace_back(apGeometry, GetModelData(apGeometry), auiBonesPerPartition, auiBonesPerVertex, apShaderDeclaration);
	ReleaseSRWLockExclusive(&kQueueLock);

	SetEvent(hTaskEvent);
	return true;
}

SPEC_NAKED void __fastcall PerformPrecache_ST(NiDX9Renderer* apThis) {
	__asm {
		sub		esp, 0x24
		push	ebx
		push	ebp
		push	0xE74125
		retn
	}
}

void GeometryPrecacheQueue::PerformPrecache_MT() {
	const DWORD dwThreadID = GetCurrentThreadId();
	if (dwThreadID == TESMain::GetSingleton()->uiMainThreadID) {
		const bool bLocked = TryLockRenderer();
		if (bLocked) {
			DEBUG_MSG("[ %08X ] Main thread locked renderer for precaching", dwThreadID);
			PerformPrecache_ST(this);
			UnlockRenderer();
			return;
		}
		else {
			DEBUG_MSG("[ %08X ] Main thread failed to lock renderer for precaching", dwThreadID);
		}
	}
	SetEvent(hTaskEvent);
}

// Runs after the frame has been rendered - this is where we can do our precaching
void GeometryPrecacheQueue::StartProcessing() {
	UnlockRenderer();
	SetEvent(hPauseEvent);
}

// Runs before the frame is rendered - we cannot do precaching while the frame is being rendered
void GeometryPrecacheQueue::StopProcessing() {
	ResetEvent(hPauseEvent);
	LockRenderer();
}

DWORD __stdcall GeometryPrecacheQueue::ThreadProc(LPVOID lpThreadParameter) {
	const DWORD dwThreadID = GetCurrentThreadId();
	BSScrapVector<NiPointer<NiObject>> kActiveObjects;
	while (true) {
		WaitForSingleObject(hPauseEvent, INFINITE);
		NiDX9Renderer* pRenderer = NiDX9Renderer::GetSingleton();
		if (!pRenderer) [[unlikely]]
			continue;

		AcquireSRWLockShared(&kQueueLock);
		bool bIsEmpty = kQueue.empty();
		ReleaseSRWLockShared(&kQueueLock);

		if (bIsEmpty)
			WaitForSingleObject(hTaskEvent, INFINITE);

		AcquireSRWLockShared(&kQueueLock);
		bIsEmpty = kQueue.empty();
		if (!bIsEmpty) [[likely]] {
			const uint32_t uiQueueSize = kQueue.size();
			kActiveObjects.reserve(uiQueueSize);
			DEBUG_MSG("[ %08X ] Pre-caching %i objects in queue", dwThreadID, uiQueueSize);
		}
		ReleaseSRWLockShared(&kQueueLock);

		bIsProcessing = true;
		while (!bIsEmpty && WaitForSingleObject(hPauseEvent, INFINITE) == WAIT_OBJECT_0) {
			AcquireSRWLockExclusive(&kQueueLock);
			if (!kQueue.empty()) {
				const QueuedObject& rObject = kQueue.back();

				ASSUME_ASSERT(rObject.spGeometry);
				ASSUME_ASSERT(rObject.spGeometry->IsGeometry());
				ASSUME_ASSERT(rObject.pGeometryData);

				kActiveObjects.push_back(rObject.spGeometry);

				DEBUG_MSG("[ %08X ] Pre-caching geometry \"%s\" with data %p", dwThreadID, GetGeometryName(rObject.spGeometry), rObject.pGeometryData);

				PrecacheGeometry_ST(pRenderer, rObject.spGeometry, rObject.uiBonesPerPartition, rObject.uiBonesPerVertex, rObject.pShaderDeclaration);

				kQueue.pop_back();
			}
			bIsEmpty = kQueue.empty();
			ReleaseSRWLockExclusive(&kQueueLock);
		}
		bIsProcessing = false;

		if (InterlockedCompareExchange(&pRenderer->uiPrePackObjectCount, 0, 0) && WaitForSingleObject(hPauseEvent, INFINITE) == WAIT_OBJECT_0) {
			DEBUG_MSG("[ %08X ] Pre-caching %i objects in pre-pack list", pRenderer->uiPrePackObjectCount, dwThreadID);
			PerformPrecache_ST(pRenderer);
		}

		kActiveObjects.clear();
		kActiveObjects.shrink_to_fit();
	}

	return 0;
}

bool GeometryPrecacheQueue::InitializeThread() {
	hTaskEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	hPauseEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr); 

	hThread = CreateThread(nullptr, 0x10000, ThreadProc, nullptr, 0, nullptr);
	if (hThread) {
		wchar_t cThreadName[32];
		_snwprintf_s(cThreadName, sizeof(cThreadName), L"[NVTF] Geometry Precache Queue");
		SetThreadDescription(hThread, cThreadName);
		return true;
	}
	else {
		assert(false);

		if (hTaskEvent) {
			CloseHandle(hTaskEvent);
			hTaskEvent = nullptr;
		}

		if (hPauseEvent) {
			CloseHandle(hPauseEvent);
			hPauseEvent = nullptr;
		}
		return false;
	}
}