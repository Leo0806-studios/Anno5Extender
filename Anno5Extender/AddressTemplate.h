#pragma once
#include <cstdint>
#include <bit>
#include "Module.h"
namespace  A5Ext::AddressLibrary {
	template <uint64_t RVA, typename T>
	class AddressTemplate {
		static constexpr uint64_t rva = RVA;
		using type = T;
		inline static uint64_t address=0;
	public:
		T* Get() {
			if (address == 0) {
				address = Module::Get().GetBaseAddress() + rva;
			}
			return std::bit_cast<T*>(address);
		}
		
	};

	using Undefined = void;
	using Undefined8 = uint64_t;
	using Undefined4 = uint32_t;
	using Undefined2 = uint16_t;
	using Undefined1 = uint8_t;

	template <size_t N>
	class BlockingPlaceholder {

		uint8_t data[N];
		public 
			BlockingPlaceholder() {
			static_assert(N > 0, "BlockingPlaceholder size must be greater than 0");
			static_assert(N != N, "Remove all BlockingPlaceholders from the code");
		}
	};
	template <size_t N>
	class Placeholder {
		uint8_t data[N];
	public:
		Placeholder() {
			static_assert(N > 0, "Placeholder size must be greater than 0");
		}
	};

	template<typename VTable,typename Base>
	struct Inheritance {
		VTable* vtable;
		Base base;
	};

	template <typename VTble, typename ... Bases>
	struct MultipleInheritance {
		VTble* vtable;
		std::tuple<Bases...> bases;
	};

	//i hope virtual inheritance is either not used or is hopefully possible with the usual mechanisms
}