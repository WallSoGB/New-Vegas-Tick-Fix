#include "NiRenderer.hpp"

// GAME - 0x4A0370
void NiRenderer::LockRenderer() {
	ThisCall(0x4A0370, this);
}

bool NiRenderer::TryLockRenderer() {
	return m_kRendererLock.TryLock();
}

// GAME - 0x4A03C0
void NiRenderer::UnlockRenderer() {
	ThisCall(0x4A03C0, this);
}