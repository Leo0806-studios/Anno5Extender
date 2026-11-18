#pragma once


#include <vector>
#include <cstdint>
#include "CThirdPartyBehaviour.h"
#include "AddressTemplate.h"
namespace A5Ext::AddressLibrary::Anno2070::AI::Behaviour {
	struct CBehaviourVtable {

		void* (*scalarDeletingDestructor)(CBehaviourAutoBuild*, unsigned int);

	};
	class CBehaviourAutoBuild:Inheritance<CBehaviourVtable, Support::CThirdPartyBehaviour> {



		std::vector<BlockingPlaceholder<0x240>> islandRecords; 
		int64_t nextUpdateTick;
		BlockingPlaceholder<0x240> autoBuild;
	public:
		CBehaviourAutoBuild();
		~CBehaviourAutoBuild();
	};
}