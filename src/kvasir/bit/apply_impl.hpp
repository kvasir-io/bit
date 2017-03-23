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
#include "tags.hpp"
#include "exec.hpp"
#include "kvasir/mpl/mpl.hpp"
#include "types.hpp"
#include "utility.hpp"
#include <utility>
namespace kvasir {
	namespace bit {
		namespace detail {
			using namespace mpl;

			//continuation takes an args list or tree, flattens it and removes all actions which are not reads and transforms to field_locations
			template<typename C = c::listify>
			using reads = c::flatten<c::cfe<list>, c::filter<c::cfe<is_read>, C>>;

			using unique_addresses = reads<c::transform<c::cfe<get_address>, c::sort<less_than, c::remove_adjacent<c::cfl<std::is_same>>>>>;

			template <typename... Ts>
			using return_type_of_apply = field_tuple<c::ucall<unique_addresses, Ts...>,
				c::ucall<reads<c::transform<c::cfl<get_field_location>>>,Ts...>>;


			//tests for diferent kinds of inputs to apply, used for SFINAE selecting the right overload
			template<typename...Ts>
			using num_runtime_writes = c::ucall<c::filter<c::cfe<is_runtime_write>, c::size<> >, Ts... >; //no need to flatten because runtime writes only allowed toplevel

			template<typename...Ts>
			using num_reads = c::ucall<reads<c::size<>>, Ts... >;

			template<typename...Ts>
			using all_compile_time = bool_<(num_reads<Ts...>::value == 0 && num_runtime_writes<Ts...>::value == 0)>;

			template<typename...Ts>
			using no_reads_but_runtime_writes = bool_<(num_reads<Ts...>::value == 0 && num_runtime_writes<Ts...>::value > 0)>;

			//sanity check
			template <typename T>
			struct arg_to_apply_is_plausible : bool_<false>
			{
			};
			template <typename L, typename A>
			struct arg_to_apply_is_plausible<action<L, A>> : bool_<true>{};
			template <>
			struct arg_to_apply_is_plausible<sequence_point_t> : bool_<true>{};

			template <typename... Ts>
			using args_to_apply_are_plausible = c::ucall<c::flatten<c::cfe<list>,c::all<c::cfe<arg_to_apply_is_plausible>>>,Ts...>;


			// an index action consists of an action (possibly merged) and
			// the inputs including masks which it needs
			template <typename TAction, typename... TInputs>
			struct indexed_action{};

			template <bool TopLevel, typename TAction, typename Index>
			struct make_indexed_action_impl;
			// in default case there is no actual expected input
			template <bool TopLevel, typename Taddress, unsigned Mask, typename TAccess, typename TR,
				typename TAction, int Index>
				struct make_indexed_action_impl<
				TopLevel, action<field_location<Taddress, Mask, TAccess, TR>, TAction>, int_<Index>>
			{
				using type = indexed_action<action<field_location<Taddress, Mask, TAccess, TR>, TAction>>;
			};

			// special case where there actually is expected input
			template <bool TopLevel, typename Taddress, unsigned Mask, typename TAccess, typename TR,
				int Index>
				struct make_indexed_action_impl<
				TopLevel, action<field_location<Taddress, Mask, TAccess, TR>, write_action>, int_<Index>>
			{
				static_assert(
					TopLevel,
					"runtime values can only be executed in an apply, they cannot be stored in a list");
				using type =
					indexed_action<action<field_location<Taddress, Mask, TAccess, TR>, write_action>,
					mpl::uint_<Index>>;
			};

			// special case where there actually is expected input
			template <bool TopLevel, typename Taddress, unsigned Mask, typename TAccess, typename TR,
				int Index>
				struct make_indexed_action_impl<
				TopLevel, action<field_location<Taddress, Mask, TAccess, TR>, xor_action>, mpl::int_<Index>>
			{
				static_assert(
					TopLevel,
					"runtime values can only be executed in an apply, they cannot be stored in a list");
				using type =
					indexed_action<action<field_location<Taddress, Mask, TAccess, TR>, write_action>,
					mpl::uint_<Index>>;
			};

			// special case where a list of actions is passed
			template <bool TopLevel, typename... Ts, typename Index>
			struct make_indexed_action_impl<TopLevel, mpl::list<Ts...>, Index>
			{
				using type = mpl::list<typename make_indexed_action_impl<false, Ts, Index>::type...>;
			};
			// special case where a sequence point is passed
			template <bool TopLevel, typename Index>
			struct make_indexed_action_impl<TopLevel, sequence_point_t, Index>
			{
				using type = sequence_point_t;
			};

			template <typename TAction, typename Index>
			using make_indexed_action = typename make_indexed_action_impl<true, TAction, Index>::type;

			template <typename T>
			struct get_action;
			template <typename A, typename... T>
			struct get_action<indexed_action<A, T...>>
			{
				using type = A;
			};

			template <typename F, typename A>
			struct get_action<action<F, A>>
			{
				using type = action<F, A>;
			};

			// predicate returning result of left < right for bitOptions
			template <typename TLeft, typename TRight>
			struct indexed_action_less;
			template <typename T1, typename U1, typename T2, typename U2, typename... TInputs1,
				typename... TInputs2>
				struct indexed_action_less<indexed_action<action<T1, U1>, TInputs1...>,
				indexed_action<action<T2, U2>, TInputs2...>>
				: mpl::bool_<(get_address<T1>::value < get_address<T2>::value)>
			{
			};
			template <typename T1, typename U1, typename T2, typename U2>
			struct indexed_action_less<action<T1, U1>, action<T2, U2>>
				: mpl::bool_<(get_address<T1>::value < get_address<T2>::value)>
			{
			};
		}
	}
}
