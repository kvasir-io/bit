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
#include "types.hpp"

namespace kvasir{
namespace bit{
	constexpr unsigned mask_from_range(int high, int low){
		return (0xFFFFFFFFULL >> (31-(high-low)))<<low;
	}
	template<typename... Is>
	constexpr unsigned mask_from_range(int high, int low, Is...args){
		return mask_from_range(high,low) | mask_from_range(args...);
	}
	namespace detail{
		using namespace mpl;

		constexpr int mask_starts_at(unsigned mask, int bitNum = 0) {
			return mask & 1 ? bitNum : mask_starts_at(mask >> 1, bitNum + 1);
		}

		constexpr bool only_one_bit_set(unsigned i){
			return 	(i==(1u<<0)) ||
					(i==(1u<<1)) ||
					(i==(1u<<2)) ||
					(i==(1u<<3)) ||
					(i==(1u<<4)) ||
					(i==(1u<<5)) ||
					(i==(1u<<6)) ||
					(i==(1u<<7)) ||
					(i==(1u<<8)) ||
					(i==(1u<<9)) ||
					(i==(1u<<10)) ||
					(i==(1u<<11)) ||
					(i==(1u<<12)) ||
					(i==(1u<<13)) ||
					(i==(1u<<14)) ||
					(i==(1u<<15)) ||
					(i==(1u<<16)) ||
					(i==(1u<<17)) ||
					(i==(1u<<18)) ||
					(i==(1u<<19)) ||
					(i==(1u<<20)) ||
					(i==(1u<<21)) ||
					(i==(1u<<22)) ||
					(i==(1u<<23)) ||
					(i==(1u<<24)) ||
					(i==(1u<<25)) ||
					(i==(1u<<26)) ||
					(i==(1u<<27)) ||
					(i==(1u<<28)) ||
					(i==(1u<<29)) ||
					(i==(1u<<30)) ||
					(i==(1u<<31));
		}

		constexpr unsigned or_all_of(){
			return 0;
		}
		constexpr unsigned or_all_of(unsigned l){
			return l;
		}

		template<typename... Ts>
		constexpr unsigned or_all_of(unsigned l, unsigned r, Ts... args){
			return l | r | or_all_of(args...);
		}


		template<typename T>
		struct value_to_unsigned;
		template<typename T, T I>
		struct value_to_unsigned<mpl::integral_constant<T,I>> : mpl::uint_<unsigned(I)>{};


		template<typename T>
		struct get_field_type;
		template<typename Taddress, unsigned Mask, typename TAccess, typename TFieldType, typename TAction>
		struct get_field_type<action<field_location<Taddress,Mask,TAccess,TFieldType>,TAction>> {
			using Type = TFieldType;
		};
		template<typename Taddress, unsigned Mask, typename TAccess, typename TFieldType>
		struct get_field_type<field_location<Taddress,Mask,TAccess,TFieldType>>{
			using Type = TFieldType;
		};
		template<typename T>
		using get_field_type_t = typename get_field_type<T>::Type;


		template<typename T>
		struct is_write_literal : std::false_type{};

		template<typename T>
		struct is_write_runtime : std::false_type{};

		template<typename T>
		struct is_field_location : std::false_type{};
		template<typename Taddress, unsigned Mask, typename Access, typename TFieldType>
		struct is_field_location<field_location<Taddress, Mask, Access, TFieldType>> : std::true_type {};

		template<typename T>
		struct is_writable : std::false_type{};
		template<typename Taddress, unsigned Mask, read_action_type RAction, modified_write_value_type WAction, typename TFieldType>
		struct is_writable<field_location<Taddress, Mask, access<access_type::readWrite, RAction, WAction>, TFieldType>> : std::true_type {};
		template<typename Taddress, unsigned Mask, read_action_type RAction, modified_write_value_type WAction, typename TFieldType>
		struct is_writable<field_location<Taddress, Mask, access<access_type::writeOnly, RAction, WAction>, TFieldType>> : std::true_type {};

		template<typename T>
		struct is_set_to_clear : std::false_type{};
		template<typename Taddress, unsigned Mask, access_type AT, read_action_type RAction, typename TFieldType>
		struct is_set_to_clear<field_location<Taddress, Mask, access<AT, RAction, modified_write_value_type::oneToClear>, TFieldType>> : std::true_type {};

		template<typename T, typename U>
		struct write_location_and_compile_time_value_types_are_same : mpl::bool_<true> {};
		template<typename AT, unsigned M, typename A, typename FT, FT V>
		struct write_location_and_compile_time_value_types_are_same<field_location<AT, M, A, FT>,mpl::integral_constant<FT,V>> : mpl::bool_<true>{};

		//getters for specific parameters of an Action
		template<typename T>
			struct get_address;
		template<unsigned A, unsigned WIIZ, unsigned SOTC, typename TRegType, typename TMode>
			struct get_address<address<A, WIIZ, SOTC, TRegType, TMode>> {
				static constexpr unsigned value = A;
				static unsigned read() {
					volatile TRegType& reg = *reinterpret_cast<volatile TRegType*>(value);
					return reg;
				}
				static void write(unsigned i) {
					volatile TRegType& reg = *reinterpret_cast<volatile TRegType*>(value);
					reg = i;
				}
				using type = mpl::uint_<A>;
			};
		template<typename Taddress, unsigned Mask, typename TAccess, typename TFiledType>
		struct get_address<field_location<Taddress, Mask, TAccess, TFiledType>> : get_address<Taddress> {};
		template<typename TReadLoc, typename TWriteLoc>
		struct get_address<field_location_pair<TReadLoc,TWriteLoc>> {
			static constexpr unsigned value = TReadLoc::value;
			static unsigned read(){
				volatile unsigned& reg = *reinterpret_cast<volatile unsigned*>(value);
				return reg;
			}
			static void write(unsigned i){
				volatile unsigned& reg = *reinterpret_cast<volatile unsigned*>(value);
				reg = i;
			}
			using type = mpl::uint_<value>;
		};
		template<typename Tfield_location, typename TAction>
		struct get_address<action<Tfield_location,TAction>> : get_address<Tfield_location> {};

		template<typename T>
		struct get_field_location;
		template<typename TLocation, typename TAction>
		struct get_field_location<action<TLocation,TAction>>{
			using type = TLocation;
		};

		//predecate retuning result of left < right for bitOptions
		template<typename TLeft, typename TRight>
		struct bit_action_less;
		template<typename T1, typename U1, typename T2, typename U2>
		struct bit_action_less< action<T1,U1>, action<T2,U2> > {
			using type = mpl::bool_<(get_address<T1>::value < get_address<T2>::value)>;
		};

		//predicate returns true if action is a read
		template<typename T>
		struct is_read : mpl::bool_<false> {};
		template<typename A>
		struct is_read< action<A,read_action> > : mpl::bool_<true>{};
		
		//predicate returns true if action is a read
		template<typename T>
		struct is_runtime_write : mpl::bool_<false> {};
		template<typename A>
		struct is_runtime_write< action<A, write_action> > : mpl::bool_<true> {};


		template<typename T>
		struct get_mask;
		//from field_locations
		template<typename address, unsigned Mask, typename TAccess, typename ResultType>
		struct get_mask<field_location<address,Mask,TAccess,ResultType>> : integral_constant<unsigned,Mask>{};
		//from Action
		template<typename Tfield_location, typename TAction>
		struct get_mask<action<Tfield_location,TAction>> : get_mask<Tfield_location>{};

	}
}
}
