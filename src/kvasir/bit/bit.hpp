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
		template<typename Address, int Position, typename TFieldType = bool>
		using RWBitLocT = FieldLocation<Address,(1<<Position),ReadWriteAccess,TFieldType>;
		template<typename Address, int Position, typename TFieldType = bool>
		using ROBitLocT = FieldLocation<Address,(1<<Position),ReadOnlyAccess,TFieldType>;
		template<typename Address, int Position, typename TFieldType = bool>
		using WOBitLocT = FieldLocation<Address,(1<<Position),WriteOnlyAccess,TFieldType>;

		//bit field helpers
		template<typename Address, int HighestBit, int LowestBit, typename TFieldType = unsigned>
		using RWFieldLocT = FieldLocation<Address,maskFromRange(HighestBit,LowestBit),ReadWriteAccess,TFieldType>;
		template<typename Address, int HighestBit, int LowestBit, typename TFieldType = unsigned>
		using ROFieldLocT = FieldLocation<Address,maskFromRange(HighestBit,LowestBit),ReadOnlyAccess,TFieldType>;
		template<typename Address, int HighestBit, int LowestBit, typename TFieldType = unsigned>
		using WOFieldLocT = FieldLocation<Address,maskFromRange(HighestBit,LowestBit),WriteOnlyAccess,TFieldType>;

	}
}
