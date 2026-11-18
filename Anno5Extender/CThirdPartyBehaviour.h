#pragma once
#include "AddressTemplate.h"
namespace A5Ext::AddressLibrary::Anno2070::AI::Support {
	class CThridPartyBehaviourManager;
	class CThirdPartyBehaviour {
	
		CThridPartyBehaviourManager* manager;
	public:
		CThirdPartyBehaviour();
		~CThirdPartyBehaviour();
	};
}