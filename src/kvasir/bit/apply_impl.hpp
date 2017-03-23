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
			using args_to_apply_are_plausible = c::ucall<c::flatten<c::all<c::cfe<arg_to_apply_is_plausible>>>,Ts...>;
		}
	}
}
