/**************************************************************************
 * This file contains the kvasir bit Abstraction DSL (Domain Specific Language)
 * which provide an extra layer between Hardware SFRs
 * (Special Function bits) and code accessing them.
 * Copyright 2015 Odin Holmes
 * Aditional contribution from Stephan Bökelmann

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
****************************************************************************/
#pragma once
#include "kvasir/mpl/mpl.hpp"
#include "types.hpp"
#include "factories.hpp"
#include "apply.hpp"


namespace kvasir {

	namespace bit{
		//this function produces an mpl::List just like mpl::list, however putting
		//one here allows adl to find it without the user having to write out the
		//whole namespace. Making the list take at least one parameter should
		//prevent ambiguity in a case where mpl::list is also an overload candidate
		template<typename T, typename... Ts>
		constexpr mpl::list<T,Ts...> list(T,Ts...){ return mpl::list<T,Ts...>{}; }

		//factory for compile time values
		template<unsigned I>
		constexpr mpl::uint_<I> value(){
			return {};
		}
		template<typename T, T I>
		constexpr mpl::uint_<I> value(){
			return {};
		}

		//bit helpers
		template<typename address, int Position, typename TFieldType = bool>
		using rw_bit_loc_t = field_location<address,(1<<Position),read_write_access,TFieldType>;
		template<typename address, int Position, typename TFieldType = bool>
		using ro_bit_loc_t = field_location<address,(1<<Position),read_only_access,TFieldType>;
		template<typename address, int Position, typename TFieldType = bool>
		using wo_bit_loc_t = field_location<address,(1<<Position),write_only_access,TFieldType>;

		//bit field helpers
		template<typename address, int HighestBit, int LowestBit, typename TFieldType = unsigned>
		using rw_field_loc_t = field_location<address,mask_from_range(HighestBit,LowestBit),read_write_access,TFieldType>;
		template<typename address, int HighestBit, int LowestBit, typename TFieldType = unsigned>
		using ro_field_loc_t = field_location<address, mask_from_range(HighestBit,LowestBit),read_only_access,TFieldType>;
		template<typename address, int HighestBit, int LowestBit, typename TFieldType = unsigned>
		using wo_field_loc_t = field_location<address, mask_from_range(HighestBit,LowestBit),write_only_access,TFieldType>;

	}
}
