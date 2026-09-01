#pragma once

#include "NiRenderer.hpp"

class NiRefObject;
class NiD3DShaderDeclaration;

class NiDX9Renderer : public NiRenderer {
public:
	char pad[0x39C];
	uint32_t uiPrePackObjectCount;

	static NiDX9Renderer* GetSingleton();

	static bool IsD3D9Create();
};

ASSERT_OFFSET(NiDX9Renderer, uiPrePackObjectCount, 0x61C);