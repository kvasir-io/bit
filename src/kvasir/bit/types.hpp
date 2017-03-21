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

	struct SequencePoint{
		using Type = SequencePoint;
	};
	constexpr SequencePoint sequencePoint{};

	template<int I>
	struct IsolatedByte{
		static constexpr int value = I;
		using Type = IsolatedByte<I>;
	};
	namespace Isolated{
		constexpr IsolatedByte<0> byte0{};
		constexpr IsolatedByte<1> byte1{};
		constexpr IsolatedByte<2> byte2{};
		constexpr IsolatedByte<3> byte3{};
	}

	struct PushableMode{};
	struct NormalMode{};
	struct SpecialReadMode{};

	template<unsigned A, unsigned WriteIgnoredIfZeroMask=0, unsigned WriteIgnoredIfOneMask = 0, typename TRegType = unsigned, typename TMode = NormalMode>
	struct Address{};

	//write a compile time known value
	template<unsigned I>
	struct WriteLiteralAction{};

	//write a run time known value
	struct WriteAction{};

	//read
	struct ReadAction{};

	//xor a compile time known mask
	template<unsigned I>
	struct XorLiteralAction{};

	//xor a run time known value
	struct XorAction{};


	template<typename TLocation, typename TAction>
	struct Action {
		template<typename... Ts>
		constexpr Action(Ts...args) :TAction{ args... } {}
	};
	template<typename TLocation>
	struct Action<TLocation, WriteAction> {
		template<typename... Ts>
		constexpr Action(const unsigned in) :value_{ in } {}
		unsigned value_;
	};
	template<typename TLocation>
	struct Action<TLocation, XorAction> {
		template<typename... Ts>
		constexpr Action(const unsigned in) :value_{ in } {}
		unsigned value_;
	};

	enum class ModifiedWriteValueType {
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

	enum class ReadActionType {
		normal,
		clear,
		set,
		modify,
		modifyExternal
	};

	enum class AccessType {
		readOnly,
		writeOnly,
		readWrite,
		writeOnce,
		readWriteOnce
	};

	template<AccessType, ReadActionType = ReadActionType::normal, ModifiedWriteValueType = ModifiedWriteValueType::normal>
	struct Access {};

	using ReadWriteAccess = Access<AccessType::readWrite>;
	using ReadOnlyAccess = Access<AccessType::readOnly>;
	using WriteOnlyAccess = Access<AccessType::writeOnly>;
	using ROneToClearAccess = Access<AccessType::readWrite, ReadActionType::normal, ModifiedWriteValueType::oneToClear>;

	template<typename TAddress, unsigned Mask, typename Access = ReadWriteAccess, typename TFieldType = unsigned>
	struct FieldLocation{};

	namespace detail {
		template<typename T>
		struct get_field_data_type;
		template<typename TAddress, unsigned Mask, typename Access, typename TFieldType>
		struct get_field_data_type<FieldLocation<TAddress, Mask, Access, TFieldType>> {
			using type = TFieldType;
		};
	}

	template<typename T, typename U>
	struct FieldLocationPair{};

	namespace detail{
		constexpr int positionOfFirstSetBit(unsigned in, int pos=0){
			return (in & 0x01)?pos:positionOfFirstSetBit(in >> 1, pos + 1);
		}
	}
	
	template<typename TFieldLocation, typename detail::get_field_data_type<TFieldLocation>::type Value>
	struct FieldValue{
		operator typename detail::get_field_data_type<TFieldLocation>::type() const {
			return Value;
		}
	};
	template<typename TAddresses, typename TFieldLocation>
	struct FieldTuple;		//see below for implementation in specialization

	namespace detail {
		template<typename Object, typename TFieldLocation>
		struct GetFieldLocationIndex;
		template<typename TA, typename TLocations, typename TFieldLocation>
		struct GetFieldLocationIndex<FieldTuple<TA, TLocations>, TFieldLocation> : mpl::c::call<mpl::c::find_if<mpl::bind1<std::is_same, TFieldLocation>,mpl::c::front>, TLocations> {};

		template<typename Object, typename TFieldLocation>
		using GetFieldLocationIndexT = typename GetFieldLocationIndex<Object, TFieldLocation>::Type;
	}

	template<uint32_t... Is, typename... TAs, unsigned... Masks, typename... TAccesss, typename... TRs>
	struct FieldTuple<mpl::list<mpl::uint_<Is>...>,mpl::list<FieldLocation<TAs,Masks,TAccesss,TRs>...>>{
		unsigned value_[sizeof...(Is)];
		template<std::size_t Index>
		mpl::at<mpl::list<TRs...>, Index> get() const{
			using namespace mpl;
			using Address = mpl::uint_<mpl::at<mpl::list<TAs...>, Index>::value>;
			constexpr unsigned index = sizeof...(Is) - mpl::c::call<mpl::c::find_if<mpl::bind1<std::is_same, Address>,mpl::c::size>, mpl::list<mpl::uint_<Is>...>>::value;
			using ResultType = mpl::at<mpl::list<TRs...>, index>;
			constexpr unsigned mask = mpl::at<mpl::list<mpl::uint_<Masks>...>, index>::value;
			unsigned r = (value_[index] & mask) >> detail::positionOfFirstSetBit(mask);
			return ResultType(r);
		}
		template<typename T>
		auto operator[](T)->decltype(get<detail::GetFieldLocationIndex<FieldTuple, T>::value>()) {
			return get<detail::GetFieldLocationIndex<FieldTuple, T>::value>();
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
		using ConvertableTo = typename std::conditional < (sizeof...(TRs) == 1), mpl::at<mpl::list<TRs...>,0>, DoNotUse>::type;
		operator ConvertableTo() {
			constexpr unsigned mask = getFirst(Masks...);
			return ConvertableTo((value_[0] & mask) >> detail::positionOfFirstSetBit(mask));
		};
	};
	template<>
	struct FieldTuple<mpl::list<>,mpl::list<>>{};

	template<std::size_t I, typename TFieldTuple>
	auto get(TFieldTuple o)->decltype(o.template get<I>()) {
		return o.template get<I>();
	}
	template<typename T, typename TFieldTuple>
	auto get(T, TFieldTuple o)->decltype(o.template get<detail::GetFieldLocationIndex<TFieldTuple,T>::value>()) {
		return o.template get<detail::GetFieldLocationIndex<TFieldTuple, T>::value>();
	}

	template<typename TFieldTuple, typename TLocation, typename TLocation::DataType Value>
	bool operator==(const TFieldTuple& f, const FieldValue<TLocation, Value>) {
		return get(TLocation{},f) == Value;
	}
}
}
