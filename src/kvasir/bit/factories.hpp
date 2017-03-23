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
#include "atomic_factories.hpp"
#include "utility.hpp"
#include <type_traits>

namespace kvasir{
namespace bit{
	namespace detail{
		using namespace mpl;

		//factory for write literal
		template<typename TLocation, unsigned Value>
		struct write;
		template<typename TAddress, unsigned Mask, typename Access, typename TFieldType, unsigned Value>
		struct write<field_location<TAddress, Mask, Access, TFieldType>, Value>{
			using type = action<
				field_location<
				TAddress,
				Mask,
				Access,
				TFieldType>,
				write_literal_action<(Value << position_of_first_set_bit(Mask))>>;
		};


		template<typename TLocation>
		struct set;
		template<typename TAddress, unsigned Mask, typename Access, typename TFieldType>
		struct set<field_location<TAddress, Mask, Access, TFieldType>>{
			using type = action<
				field_location<
				TAddress,
				Mask,
				Access,
				TFieldType>,
				write_literal_action<Mask>>;
	
			static_assert(only_one_bit_set(Mask),"bit::set only works on single bit. Use bit::write to write values to wider bit fields");
		};


		template<typename TLocation>
		struct clear;
		template<typename TAddress, unsigned Mask, typename Access, typename TFieldType>
		struct clear<
			field_location<
				TAddress,
				Mask,
				Access,
				TFieldType>> {
			using type = action<
				field_location<
				TAddress,
				Mask,
				Access,
				TFieldType>,
				write_literal_action<0>>;
		
			static_assert(only_one_bit_set(Mask),"bit::clear only works on single bit. Use bit::write to write values to wider bit fields");
		};


		template<typename TLocation>
		struct reset;
		template<typename TAddress, unsigned Mask, typename Access, typename TFieldType>
		struct reset<field_location<	TAddress, Mask, Access, TFieldType>> {
			using type = action <
				field_location<
				TAddress,
				Mask,
				Access,
				TFieldType>,
				write_literal_action < (1 << position_of_first_set_bit(Mask)) >> ;
			static_assert(only_one_bit_set(Mask),"bit::reset only works on single bit that are marked as set to clear");
			static_assert(detail::is_set_to_clear<field_location<TAddress, Mask, Access, TFieldType >>::value, "Access violation: bit::reset only works on set to clear bit");
		};
	}

//Action factories which turn a field_location into an Action
	template<typename T>
	constexpr inline std::enable_if<detail::is_field_location<T>::value,action<T,read_action>>
	read(T){
		return {};
	}

	template<typename T, typename U, typename... Ts>
	constexpr decltype(mpl::make_list(read(T{}), read(U{}), read(Ts{})...)) read(T,U,Ts...){
		return {};
	}

	template<typename T>
	constexpr std::enable_if<detail::is_field_location<T>::value,typename detail::set<T>::type>
	set(T){
		return {};
	}

	template<typename T, typename U, typename... Ts>
	constexpr decltype(mpl::make_list(set(T{}), set(U{}), set(Ts{})...)) set(T,U,Ts...){
		return {};
	}

	template<typename T>
	constexpr std::enable_if<detail::is_field_location<T>::value,typename detail::clear<T>::type>
	clear(T){
		return {};
	}

	template<typename T, typename U, typename... Ts>
	constexpr decltype(mpl::make_list(clear(T{}), clear(U{}), clear(Ts{})...)) clear(T,U,Ts...){
		return {};
	}

	template<typename T>
	constexpr std::enable_if<detail::is_field_location<T>::value,typename detail::reset<T>::type>
	reset(T){
		static_assert(detail::is_set_to_clear<T>::value,"Access violation: bit::reset only works on set to clear bit");
		return {};
	}

	template<typename T, typename U, typename... Ts>
	constexpr decltype(mpl::make_list(reset(T{}), reset(U{}), reset(Ts{})...)) reset(T,U,Ts...){
		return {};
	}


	//Write of runtime value
	//T must be bit location or function will be removed from overload set
	template<typename T>
	constexpr inline std::enable_if<detail::is_field_location<T>::value,action<T,write_action>>
	write(T,typename detail::get_field_type<T>::type in){
		static_assert(detail::is_writable<T>::value,"Access violation: The field_location provided is not marked as writable");
		return action<T, write_action>{detail::get_mask<T>::value & (unsigned(in) << detail::mask_starts_at(detail::get_mask<T>::value))};
	}

	//compile time value
	//T must be bit location or function will be removed from overload set
	//U mst be compile time value or function will be removed from overload set
	template<typename T, typename U>
	constexpr inline std::enable_if<
		(detail::is_field_location<T>::value && mpl::is_integral<U>{}),
		typename detail::write<T, detail::value_to_unsigned<U>::value>>
	write(T, U) {
		static_assert(detail::write_location_and_compile_time_value_types_are_same<T, U>::value, "type mismatch: the field_location field type and the compile time Value type must be the same");
		return { };
	}
	
	//compile time field value
	//T must be a field value or function will be removed from overload set
	template<typename T, typename T::DataType V>
	constexpr inline decltype(write(T{}, mpl::integral_constant<typename T::DataType, V>{}))
		write(field_value<T, V>) {
			return { };
		}
	
	//variadic compile time field values
	template<typename T, typename U, typename... Ts>
		constexpr mpl::list<decltype(write(std::declval<T>())), decltype(write(std::declval<U>())), decltype(write(std::declval<Ts>()))...> write(T, U, Ts...) {
			return { };
		}
}
}
