#pragma once

class NiRTTI {
public:
	NiRTTI() : m_pcName(nullptr), m_pkBaseRTTI(nullptr) {}
	NiRTTI(const char* apName, const NiRTTI* const apBase) : m_pcName(apName), m_pkBaseRTTI(apBase) {}
	NiRTTI(const char* apName, const NiRTTI& arBase) : m_pcName(apName), m_pkBaseRTTI(&arBase) {}
	NiRTTI(const NiRTTI&) = delete;
	NiRTTI(NiRTTI&&) = delete;

	const char*		m_pcName;
	const NiRTTI*	m_pkBaseRTTI;

	const char* GetName() const { return m_pcName; }
	const NiRTTI* GetBase() const { return m_pkBaseRTTI; }

	bool IsKindOf(const NiRTTI* const apRTTI) const {
		for (const NiRTTI* i = this; i; i = i->GetBase()) {
			if (i == apRTTI)
				return true;
		}
		return false;
	}

	template <typename T_RTTI>
	bool IsKindOf() const {
		return IsKindOf(&T_RTTI::ms_RTTI);
	}

	bool IsExactKindOf(const NiRTTI* const apRTTI) const {
		return this == apRTTI;
	}

	template <typename T_RTTI>
	bool IsExactKindOf() const {
		return IsExactKindOf(&T_RTTI::ms_RTTI);
	}
};

ASSERT_SIZE(NiRTTI, 0x8);

#define NIRTTI_ADDRESS(address) \
	static constexpr AddressPtr<NiRTTI, address> const ms_RTTI;