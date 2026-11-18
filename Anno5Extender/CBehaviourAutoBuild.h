#pragma once


#include <vector>
#include <cstdint>
#include "CThirdPartyBehaviour.h"
#include "AddressTemplate.h"
#pragma warning(push)
#pragma warning(disable:4265)
namespace A5Ext::AddressLibrary::Anno2070::AI::Behaviour {

	class CBehaviourAutoBuild:public Support::CThirdPartyBehaviour {



		std::vector<Placeholder<0x240>> islandRecords; 
		int64_t nextUpdateTick;
		Placeholder<0x240> autoBuild;
	public:


		// Inherited via CThirdPartyBehaviour
		__declspec(dllimport) void* ScalarDeletingDestructor(uint32_t deleteFlag) override;

		__declspec(dllimport) void Init() override;

		__declspec(dllimport) void unknown_0x10() override;

		__declspec(dllimport) void unknown_0x18() override;

		__declspec(dllimport) void Step() override;

		__declspec(dllimport) bool Serialize(void* serializer, Undefined8 unused_context, uint32_t flags) override;

		__declspec(dllimport) void unknown_0x30() override;

		__declspec(dllimport) void unknown_0x38() override;

	};
}
#pragma warning(pop)