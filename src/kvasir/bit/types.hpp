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
	struct Address{
		using Type = Address<A,WriteIgnoredIfZeroMask,WriteIgnoredIfOneMask,TRegType,TMode>;
		static constexpr unsigned value = A;
	};

	//write a compile time known value
	template<unsigned I>
	struct WriteLiteralAction{
		static constexpr unsigned value = I;
	};

	//write a run time known value
	struct WriteAction{
		unsigned value_;
	};

	//read
	struct ReadAction{

	};

	//xor a compile time known mask
	template<unsigned I>
	struct XorLiteralAction{
		static constexpr unsigned value = I;
	};

	//xor a run time known value
	struct XorAction{
		unsigned value_;
	};


	template<typename TLocation, typename TAction>
	struct Action : TAction {
		template<typename... Ts>
		constexpr Action(Ts...args):TAction{args...}{}
		using Type = Action<TLocation,TAction>;
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
	struct FieldLocation{
		using Type = FieldLocation<TAddress, Mask, Access, TFieldType>;
		using DataType = TFieldType;
	};

	template<typename T, typename U>
	struct FieldLocationPair{
		using Type = FieldLocationPair<T,U>;
	};

	namespace Detail{
		using namespace mpl;
		constexpr int positionOfFirstSetBit(unsigned in, int pos=0){
			return (in & 0x01)?pos:positionOfFirstSetBit(in >> 1, pos + 1);
		}
	}
	
	template<typename TFieldLocation, typename TFieldLocation::DataType Value>
	struct FieldValue{
		using Type = FieldValue<TFieldLocation, Value>;
		operator typename TFieldLocation::DataType() const {
			return Value;
		}
	};
	template<typename TAddresses, typename TFieldLocation>
	struct FieldTuple;		//see below for implementation in specialization
	namespace Detail {
		template<typename Object, typename TFieldLocation>
		struct GetFieldLocationIndex;
		template<typename TA, typename TLocations, typename TFieldLocation>
		struct GetFieldLocationIndex<FieldTuple<TA, TLocations>, TFieldLocation> : mpl::find_if<mpl::bind<std::is_same, TFieldLocation>::template f, TLocations> {};

		template<typename Object, typename TFieldLocation>
		using GetFieldLocationIndexT = typename GetFieldLocationIndex<Object, TFieldLocation>::Type;
	}

	template<uint32_t... Is, typename... TAs, unsigned... Masks, typename... TAccesss, typename... TRs>
	struct FieldTuple<mpl::list<mpl::uint_<Is>...>,mpl::list<FieldLocation<TAs,Masks,TAccesss,TRs>...>>{
		unsigned value_[sizeof...(Is)];
		template<std::size_t Index>
		mpl::at<Index, mpl::list<TRs...>> get() const{
			using namespace mpl;
			using Address = mpl::uint_<mpl::at<Index, mpl::list<TAs...>>::value>;
			constexpr unsigned index = sizeof...(Is) - mpl::size<mpl::find_if<mpl::bind<std::is_same, Address>::template f, mpl::list<mpl::uint_<Is>...>>>::value;
			using ResultType = mpl::at<index, mpl::list<TRs...>>;
			constexpr unsigned mask = mpl::at<index, mpl::list<mpl::uint_<Masks>...>>::value;
			unsigned r = (value_[index] & mask) >> Detail::positionOfFirstSetBit(mask);
			return ResultType(r);
		}
		template<typename T>
		auto operator[](T)->decltype(get<Detail::GetFieldLocationIndex<FieldTuple, T>::value>()) {
			return get<Detail::GetFieldLocationIndex<FieldTuple, T>::value>();
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
		using ConvertableTo = typename std::conditional < (sizeof...(TRs) == 1), mpl::at<0, mpl::list<TRs...>>, DoNotUse>::type;
		operator ConvertableTo() {
			constexpr unsigned mask = getFirst(Masks...);
			return ConvertableTo((value_[0] & mask) >> Detail::positionOfFirstSetBit(mask));
		};
	};
	template<>
	struct FieldTuple<mpl::list<>,mpl::list<>>{};

	template<std::size_t I, typename TFieldTuple>
	auto get(TFieldTuple o)->decltype(o.template get<I>()) {
		return o.template get<I>();
	}
	template<typename T, typename TFieldTuple>
	auto get(T, TFieldTuple o)->decltype(o.template get<Detail::GetFieldLocationIndex<TFieldTuple,T>::value>()) {
		return o.template get<Detail::GetFieldLocationIndex<TFieldTuple, T>::value>();
	}

	template<typename TFieldTuple, typename TLocation, typename TLocation::DataType Value>
	bool operator==(const TFieldTuple& f, const FieldValue<TLocation, Value>) {
		return get(TLocation{},f) == Value;
	}
}
}
