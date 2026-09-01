#pragma once

#include "NiObject.hpp"
#include "NiCriticalSection.hpp"

class NiShader;
class NiDynamicEffectState;
class NiRenderedCubeMap;
class NiAccumulator;
class NiDX9Renderer;
class NiPropertyState;
class NiRenderTargetGroup;
class NiFrustum;

class NiRenderer : public NiObject {
public:
	NiRenderer();
	virtual ~NiRenderer();

	NiPointer<NiAccumulator>		m_spAccum;
	NiPropertyState*				m_pkCurrProp;
	NiDynamicEffectState*			m_pkCurrEffects;
	NiShader*						m_spErrorShader;
	NiCriticalSection				m_kRendererLock;
	NiCriticalSection				m_kPrecacheCriticalSection;
	NiCriticalSection				m_kSourceDataCriticalSection;
	uint32_t						m_eSavedFrameState;
	uint32_t						m_eFrameState;
	uint32_t						m_uiFrameID;
	bool							m_bRenderTargetGroupActive;
	bool							m_bBatchRendering;

	void LockRenderer();
	bool TryLockRenderer();
	void UnlockRenderer();
};

ASSERT_SIZE(NiRenderer, 0x280)
ASSERT_OFFSET(NiRenderer, m_kRendererLock, 0x80)