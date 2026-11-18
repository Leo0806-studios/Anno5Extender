#pragma once
#include "AddressTemplate.h"
namespace A5Ext::AddressLibrary::Anno2070::AI::Support {
	class CThridPartyBehaviourManager;
	class __declspec(novtable) CThirdPartyBehaviour {
	
		CThridPartyBehaviourManager* manager;
	public:
		virtual void* ScalarDeletingDestructor(uint32_t deleteFlag) = 0;
		virtual void Init() = 0;
		virtual void unknown_0x10() = 0;
		virtual void unknown_0x18() = 0;
		virtual void Step() = 0;
		virtual bool Serialize(void* serializer, Undefined8 unused_context, uint32_t flags) = 0;
		virtual void unknown_0x30() = 0;
		virtual void unknown_0x38() = 0;
		__declspec(dllimport) CThirdPartyBehaviour();
	}; 
}



