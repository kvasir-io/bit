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
#include <type_traits>
#include <stdint.h> 


namespace kvasir{
namespace bit{

	struct sequence_point_t{};
	constexpr sequence_point_t sequence_point{};

	struct pushable_mode{};
	struct normal_mode{};
	struct special_read_mode{};

	template<unsigned A, unsigned WriteIgnoredIfZeroMask=0, unsigned WriteIgnoredIfOneMask = 0, typename TRegType = unsigned, typename TMode = normal_mode>
	struct address{};

	//write a compile time known value
	template<unsigned I>
	struct write_literal_action{};

	//write a run time known value
	struct write_action{};

	//read
	struct read_action{};

	//xor a compile time known mask
	template<unsigned I>
	struct xor_literal_action{};

	//xor a run time known value
	struct xor_action{};


	template<typename TLocation, typename TAction>
	struct action {
		template<typename... Ts>
		constexpr action(Ts...args) {}
	};
	template<typename TLocation>
	struct action<TLocation, write_action> {
		template<typename... Ts>
		constexpr action(const unsigned in) :value_{ in } {}
		unsigned value_;
	};
	template<typename TLocation>
	struct action<TLocation, xor_action> {
		template<typename... Ts>
		constexpr action(const unsigned in) :value_{ in } {}
		unsigned value_;
	};

	enum class modified_write_value_type {
		normal,
		oneToClear,
		oneToSet,
		oneToToggle,
		zeroToClear,
		zeroToSet,
		zeroToToggle,
		clear,
		set,
		modify
	};

	enum class read_action_type {
		normal,
		clear,
		set,
		modify,
		modifyExternal
	};

	enum class access_type {
		readOnly,
		writeOnly,
		readWrite,
		writeOnce,
		readWriteOnce
	};

	template<access_type, read_action_type = read_action_type::normal, modified_write_value_type = modified_write_value_type::normal>
	struct access {};

	using read_write_access = access<access_type::readWrite>;
	using read_only_access = access<access_type::readOnly>;
	using write_only_access = access<access_type::writeOnly>;
	using one_to_clear_access = access<access_type::readWrite, read_action_type::normal, modified_write_value_type::oneToClear>;

	template<typename Taddress, unsigned Mask, typename Access = read_write_access, typename TFieldType = unsigned>
	struct field_location{};

	namespace detail {
		template<typename T>
		struct get_field_data_type;
		template<typename Taddress, unsigned Mask, typename Access, typename TFieldType>
		struct get_field_data_type<field_location<Taddress, Mask, Access, TFieldType>> {
			using type = TFieldType;
		};
	}

	template<typename T, typename U>
	struct field_location_pair{};

	namespace detail{
		constexpr int position_of_first_set_bit(unsigned in, int pos=0){
			return (in & 0x01)?pos:position_of_first_set_bit(in >> 1, pos + 1);
		}
	}
	
	template<typename Tfield_location, typename detail::get_field_data_type<Tfield_location>::type Value>
	struct field_value{
		operator typename detail::get_field_data_type<Tfield_location>::type() const {
			return Value;
		}
	};
	template<typename Taddresses, typename Tfield_location>
	struct field_tuple;		//see below for implementation in specialization

	namespace detail {
		template<typename Object, typename Tfield_location>
		struct get_field_location_index;
		template<typename TA, typename TLocations, typename Tfield_location>
		struct get_field_location_index<field_tuple<TA, TLocations>, Tfield_location> : mpl::call<mpl::unpack<mpl::find_if<mpl::is_same<Tfield_location>,mpl::front>>, TLocations> {};

		template<typename Object, typename Tfield_location>
		using get_field_location_index_t = typename get_field_location_index<Object, Tfield_location>::Type;
	}

	template<uint32_t... Is, typename... TAs, unsigned... Masks, typename... TAccesss, typename... TRs>
	struct field_tuple<mpl::list<mpl::uint_<Is>...>,mpl::list<field_location<TAs,Masks,TAccesss,TRs>...>>{
		unsigned value_[sizeof...(Is)];
		template<std::size_t Index>
		mpl::eager::at<mpl::list<TRs...>, Index> get() const{
			using namespace mpl;
			using address = mpl::uint_<mpl::at<mpl::list<TAs...>, Index>::value>;
			constexpr unsigned index = sizeof...(Is) - mpl::c::call<mpl::c::find_if<mpl::bind1<std::is_same, address>,mpl::c::size>, mpl::list<mpl::uint_<Is>...>>::value;
			using ResultType = mpl::at<mpl::list<TRs...>, index>;
			constexpr unsigned mask = mpl::at<mpl::list<mpl::uint_<Masks>...>, index>::value;
			unsigned r = (value_[index] & mask) >> detail::position_of_first_set_bit(mask);
			return ResultType(r);
		}
		template<typename T>
		auto operator[](T)->decltype(get<detail::get_field_location_index<field_tuple, T>::value>()) {
			return get<detail::get_field_location_index<field_tuple, T>::value>();
		}
		template<typename... T>
		static constexpr unsigned getFirst(unsigned i, T...) {
			return i;
		}
		struct DoNotUse{
			template<typename T>
			DoNotUse(T){}
		};
		//implicitly convertible to the field type only if there is just one field
		using ConvertableTo = typename std::conditional < (sizeof...(TRs) == 1), mpl::eager::at<mpl::list<TRs...>,0>, DoNotUse>::type;
		operator ConvertableTo() {
			constexpr unsigned mask = getFirst(Masks...);
			return ConvertableTo((value_[0] & mask) >> detail::position_of_first_set_bit(mask));
		};
	};
	template<>
	struct field_tuple<mpl::list<>,mpl::list<>>{};

	template<std::size_t I, typename Tfield_tuple>
	auto get(Tfield_tuple o)->decltype(o.template get<I>()) {
		return o.template get<I>();
	}
	template<typename T, typename Tfield_tuple>
	auto get(T, Tfield_tuple o)->decltype(o.template get<detail::get_field_location_index<Tfield_tuple,T>::value>()) {
		return o.template get<detail::get_field_location_index<Tfield_tuple, T>::value>();
	}

	template<typename Tfield_tuple, typename TLocation, typename TLocation::DataType Value>
	bool operator==(const Tfield_tuple& f, const field_value<TLocation, Value>) {
		return get(TLocation{},f) == Value;
	}
}
}
