#include "NiCriticalSection.hpp"

NiCriticalSection::NiCriticalSection() {
	(*reinterpret_cast<decltype(InitializeCriticalSection)**>(0xFDF054))(&m_kCriticalSection);
}

NiCriticalSection::~NiCriticalSection() {
	(*reinterpret_cast<decltype(DeleteCriticalSection)**>(0xFDF058))(&m_kCriticalSection);
}

// GAME - 0x82F1B0
void NiCriticalSection::Lock() {
	ThisCall(0x82F1B0, this);
}

// GAME - 0x78D1D0
bool NiCriticalSection::TryLock() {
	return ThisCall<bool>(0x78D1D0, this);
}

// GAME - 0x82F1F0
void NiCriticalSection::Unlock() {
	ThisCall(0x82F1F0, this);
}