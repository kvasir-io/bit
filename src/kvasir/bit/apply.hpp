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
#include "apply_impl.hpp"
#include "tags.hpp"
#include "exec.hpp"
#include "kvasir/mpl/mpl.hpp"
#include "types.hpp"
#include "utility.hpp"
#include <utility>
namespace kvasir
{

namespace bit
{

    namespace detail
    {

        template <std::size_t I, typename O, typename A, typename L>
        struct MakeSeperatorsImpl
        {
            using type = O;
        };
        template <std::size_t I, typename... O, typename... A, typename... T>
        struct MakeSeperatorsImpl<I, mpl::list<O...>, mpl::list<A...>, mpl::list<mpl::uint_<I>, T...>>
            : MakeSeperatorsImpl<(I + 1), mpl::list<O..., mpl::list<A...>>, mpl::list<>,
                                 mpl::list<T...>>
        {
        };

        template <std::size_t I, typename... O, typename... A, typename U, typename... T>
        struct MakeSeperatorsImpl<I, mpl::list<O...>, mpl::list<A...>, mpl::list<U, T...>>
            : MakeSeperatorsImpl<I + 1, mpl::list<O...>, mpl::list<A..., unsigned>,
                                 mpl::list<U, T...>>
        {
        };

        template <typename T>
        using MakeSeperators = MakeSeperatorsImpl<0, mpl::list<>, mpl::list<>, mpl::sort<T, mpl::less_than>>;

        // an index action consists of an action (possibly merged) and
        // the inputs including masks which it needs
        template <typename TAction, typename... TInputs>
        struct IndexedAction
        {
            using type = IndexedAction<TAction, TInputs...>;
        };

        template <typename T>
        struct GetAction;
        template <typename A, typename... T>
        struct GetAction<IndexedAction<A, T...>>
        {
            using type = A;
        };

        template <typename F, typename A>
        struct GetAction<action<F, A>>
        {
            using type = action<F, A>;
        };

        template <typename T>
        struct GetInputsImpl;
        template <typename A, typename... T>
        struct GetInputsImpl<IndexedAction<A, T...>>
        {
            using type = MakeSeperators<mpl::list<T...>>;
        };
        template <typename T>
        using GetInputs = typename GetInputsImpl<T>::type;

        // predicate returning result of left < right for bitOptions
        template <typename TLeft, typename TRight>
        struct IndexedActionLess;
        template <typename T1, typename U1, typename T2, typename U2, typename... TInputs1,
                  typename... TInputs2>
        struct IndexedActionLess<IndexedAction<action<T1, U1>, TInputs1...>,
                                 IndexedAction<action<T2, U2>, TInputs2...>>
            : mpl::bool_<(get_address<T1>::value < get_address<T2>::value)>
        {
        };
        template <typename T1, typename U1, typename T2, typename U2>
        struct IndexedActionLess<action<T1, U1>, action<T2, U2>>
            : mpl::bool_<(get_address<T1>::value < get_address<T2>::value)>
        {
        };

        template <typename Tbits, typename TRet = mpl::list<>> // default
        struct MergebitActions;

        template <typename TNext, typename... Ts> // none processed yet
        struct MergebitActions<mpl::list<TNext, Ts...>, mpl::list<>>
            : MergebitActions<mpl::list<Ts...>, mpl::list<TNext>>
        {
        };

        // indexed
        template <typename Taddress, unsigned Mask1, unsigned Mask2, typename TAccess1,
                  typename TAccess2, typename TFieldType1, typename TFieldType2,
                  template <unsigned> class TActionTemplate, unsigned Value1, unsigned Value2,
                  typename... TInputs1, typename... TInputs2, typename... Ts,
                  typename... Us> // next input and last merged are mergable
        struct MergebitActions<
            mpl::list<
                IndexedAction<action<field_location<Taddress, Mask1, TAccess1, TFieldType1>,
                                     TActionTemplate<Value1>>,
                              TInputs1...>,
                Ts...>,
            mpl::list<
                IndexedAction<action<field_location<Taddress, Mask2, TAccess2, TFieldType2>,
                                     TActionTemplate<Value2>>,
                              TInputs2...>,
                Us...>>
            : MergebitActions<
                  mpl::list<Ts...>,
                  mpl::list<
                      IndexedAction<action<field_location<Taddress,
                                                         (Mask1 | Mask2), // merge
                                                         TAccess1>, // dont care, plausibility check
                                                                    // has already been done
                                           TActionTemplate<(Value1 | Value2)> // merge
                                           // TODO implement bit type here
                                           >,
                                    TInputs1..., TInputs2...>, // concatenate
                      Us...>                                   // pass through rest
                  >
        {
        };

        // non indexed
        template <typename Taddress, unsigned Mask1, unsigned Mask2, typename TAccess1,
                  typename TAccess2, typename TFieldType1, typename TFieldType2,
                  template <unsigned> class TActionTemplate, unsigned Value1, unsigned Value2,
                  typename... Ts, typename... Us> // next input and last merged are mergable
        struct MergebitActions<
            mpl::list<action<field_location<Taddress, Mask1, TAccess1, TFieldType1>,
                                 TActionTemplate<Value1>>,
                          Ts...>,
            mpl::list<action<field_location<Taddress, Mask2, TAccess2, TFieldType2>,
                                 TActionTemplate<Value2>>,
                          Us...>>
            : MergebitActions<
                  mpl::list<Ts...>,
                  mpl::list<action<field_location<Taddress,
                                                     (Mask1 | Mask2), // merge
                                                     TAccess1>, // dont care, plausibility check has
                                                                // already been done
                                       TActionTemplate<(Value1 | Value2)> // merge
                                       // TODO implement bit type here
                                       >,
                                Us...> // pass through rest
                  >
        {
        };

        template <typename TNext, typename TLast, typename... Ts,
                  typename... Us> // next and last not mergable
        struct MergebitActions<mpl::list<TNext, Ts...>, mpl::list<TLast, Us...>>
            : MergebitActions<mpl::list<Ts...>, mpl::list<TNext, TLast, Us...>>
        {
        };

        template <typename... Ts> // done
        struct MergebitActions<mpl::list<>, mpl::list<Ts...>>
        {
            using type = mpl::list<Ts...>;
        };

        template <typename T>
        using MergebitActionsT = typename MergebitActions<T>::type;

        template <typename TList>
        struct MergeActionSteps;
        template <typename... Ts>
        struct MergeActionSteps<mpl::list<Ts...>>
        {
            using type = mpl::list<MergebitActionsT<
                mpl::sort<mpl::flatten<Ts>, detail::IndexedActionLess>
                >...>;
        };

        template <typename T>
        using MergeActionStepsT = typename MergeActionSteps<T>::type;

        template <typename TAction, typename... TInputs>
        struct get_address<IndexedAction<TAction, TInputs...>> : get_address<TAction>
        {
        };

        template <bool TopLevel, typename TAction, typename Index>
        struct MakeIndexedActionImpl;
        // in default case there is no actual expected input
        template <bool TopLevel, typename Taddress, unsigned Mask, typename TAccess, typename TR,
                  typename TAction, int Index>
        struct MakeIndexedActionImpl<
            TopLevel, action<field_location<Taddress, Mask, TAccess, TR>, TAction>, int_<Index>>
        {
            using type = IndexedAction<action<field_location<Taddress, Mask, TAccess, TR>, TAction>>;
        };

        // special case where there actually is expected input
        template <bool TopLevel, typename Taddress, unsigned Mask, typename TAccess, typename TR,
                  int Index>
        struct MakeIndexedActionImpl<
            TopLevel, action<field_location<Taddress, Mask, TAccess, TR>, write_action>, int_<Index>>
        {
            static_assert(
                TopLevel,
                "runtime values can only be executed in an apply, they cannot be stored in a list");
            using type =
                IndexedAction<action<field_location<Taddress, Mask, TAccess, TR>, write_action>,
                              mpl::uint_<Index>>;
        };

        // special case where there actually is expected input
        template <bool TopLevel, typename Taddress, unsigned Mask, typename TAccess, typename TR,
                  int Index>
        struct MakeIndexedActionImpl<
            TopLevel, action<field_location<Taddress, Mask, TAccess, TR>, xor_action>, mpl::int_<Index>>
        {
            static_assert(
                TopLevel,
                "runtime values can only be executed in an apply, they cannot be stored in a list");
            using type =
                IndexedAction<action<field_location<Taddress, Mask, TAccess, TR>, write_action>,
                              mpl::uint_<Index>>;
        };

        // special case where a list of actions is passed
        template <bool TopLevel, typename... Ts, typename Index>
        struct MakeIndexedActionImpl<TopLevel, mpl::list<Ts...>, Index>
        {
            using type = mpl::list<typename MakeIndexedActionImpl<false, Ts, Index>::type...>;
        };
        // special case where a sequence point is passed
        template <bool TopLevel, typename Index>
        struct MakeIndexedActionImpl<TopLevel, sequence_point_t, Index>
        {
            using type = sequence_point_t;
        };

        template <typename TAction, typename Index>
        using MakeIndexedAction = typename MakeIndexedActionImpl<true, TAction, Index>::type;

        template <unsigned I>
        struct IsaddressPred
        {
            template <typename T>
            struct apply : mpl::bool_<false>
            {
            };
            template <typename Taddress, unsigned Mask, typename TAccess, typename TFieldType,
                      typename Cmd>
            struct apply<action<field_location<Taddress, Mask, TAccess, TFieldType>, Cmd>>
                : mpl::integral_constant<bool, (I == get_address<Taddress>::value)>
            {
            };
        };

        template <typename T>
        struct GetReadMask : mpl::int_<0>
        {
        };

        template <typename T>
        struct get_addresses;
        template <typename Taddresses, typename TLocations>
        struct get_addresses<field_tuple<Taddresses, TLocations>> : Taddresses
        {
        };

        template <typename T, typename = decltype(T::value_)>
        unsigned argToUnsigned(T arg)
        {
            return arg.value_;
        }
        inline unsigned argToUnsigned(...) { return 0; }

        // finder takes a list of lists of unsigned, each list represents a
        // pack of arguments to be ignored. All non ignored arguments will
        // be ored together

        template <typename T>
        struct Finder;
        template <>
        struct Finder<mpl::list<>>
        {
            unsigned operator()(...) { return 0; }
        };
        template <typename... A>
        struct Finder<mpl::list<mpl::list<A...>>>
        {
            template <typename... T>
            unsigned operator()(A..., unsigned a, T...)
            {
                return a;
            }
        };
        template <typename... A, typename... B>
        struct Finder<mpl::list<mpl::list<A...>, mpl::list<B...>>>
        {
            template <typename... T>
            unsigned operator()(A..., unsigned a, B..., unsigned b, T...)
            {
                return a | b;
            }
        };
        template <typename... A, typename... B, typename... Rest>
        struct Finder<mpl::list<mpl::list<A...>, mpl::list<B...>, Rest...>>
        {
            template <typename... T>
            unsigned operator()(A..., unsigned a, B..., unsigned b, T... t)
            {
                auto r = Finder<mpl::list<Rest...>>{};
                return a | b | r(t...);
            }
        };



        template <typename TActionList, typename TInputIndexList, typename TRetType>
        struct Apply;
        template <typename... TActions, typename... TInputIndexes, typename... TRetaddresses,
                  typename TRetLocations>
        struct Apply<mpl::list<TActions...>, mpl::list<TInputIndexes...>,
                     field_tuple<mpl::list<TRetaddresses...>, TRetLocations>>
        {
            using ReturnType = field_tuple<mpl::list<TRetaddresses...>, TRetLocations>;
            template <unsigned A>

                typename std::enable_if<mpl::c::ucall<mpl::c::any<mpl::c::same_as<mpl::uint_<A>>>, TRetaddresses...>::value>
                filterReturns(ReturnType & ret, unsigned in)
            {
					ret.value_[mpl::c::call<mpl::c::find_if<mpl::bind1<std::is_same, mpl::uint_<A>>, mpl::c::offset<mpl::uint_<sizeof...(TRetaddresses)>>>, mpl::list<TRetaddresses...>>::value] |= in;
            }
            template <unsigned A>
            void filterReturns(...)
            {
            }

            template <typename... T>
            ReturnType operator()(T... args)
            {
                ReturnType ret{{}}; // default constructed return
                const unsigned a[]{0U, (filterReturns<detail::get_address<TActions>::value>(
                                            ret, execute_seam<TActions, ::kvasir::Tag::User>{}(
                                                     Finder<TInputIndexes>{}(args...))),
                                        0U)...};
                ignore(a);

                return ret;
            }
        };

        // no read apply
        template <typename TActionList, typename TInputIndexList>
        struct NoReadApply;
        template <typename... TActions, typename... TInputIndexes>
        struct NoReadApply<mpl::list<TActions...>, mpl::list<TInputIndexes...>>
        {
            template <typename... T>
            void operator()(T... args)
            {
                const unsigned a[]{0U, (execute_seam<TActions, ::kvasir::Tag::User>{}(
                                            Finder<TInputIndexes>{}(args...)),
                                        0U)...};
                ignore(a);
            }
        };

        // no read no runtime write apply
        template <typename... TActions>
        void noReadNoRuntimeWriteApply(mpl::list<TActions...> *)
        {
            const unsigned a[]{0U, execute_seam<TActions, ::kvasir::Tag::User>{}(0U)...};
            ignore(a);
        }

    }

    // if apply contains reads return a field_tuple
    template <typename... Args>
    inline typename std::enable_if<(detail::num_reads<Args...>::value > 0),
                                detail::return_type_of_apply<Args...>>::type 
		apply(Args... args)
    {
        static_assert(detail::args_to_apply_are_plausible<Args...>::value,
                      "one of the supplied arguments is not supported");
        using IndexedActions =
            mpl::c::call<mpl::c::transform<mpl::lambda<detail::MakeIndexedAction>>, mpl::list<Args...>, mpl::make_int_sequence<mpl::int_<sizeof...(Args)>>>;
        using FlattenedActions = mpl::flatten<IndexedActions>;
        using Steps = mpl::split_if<FlattenedActions, mpl::bind1<std::is_same, SequencePoint>::template f>;
        using Merged = detail::MergeActionStepsT<Steps>;
        using Actions = mpl::flatten<Merged>;
        using Functors = mpl::transform<Actions, detail::GetAction>;
        using Inputs =
            mpl::transform<Actions, detail::GetInputs>; // list of lists of lits
                                                                            // of unsigned
                                                                            // seperators
        detail::Apply<Functors, Inputs, detail::return_type_of_apply<Args...>> a{};
        return a(detail::argToUnsigned(args)...);
    }

    // if apply does not contain reads return is void
    template <typename... Args>
    typename std::enable_if<detail::no_reads_but_runtime_writes<Args...>::value>::type
        apply(Args... args)
    {
        static_assert(detail::args_to_apply_are_plausible<Args...>::value,
                      "one of the supplied arguments is not supported");
        using IndexedActions =
            mpl::c::call<mpl::c::zip_with<mpl::lambda<detail::MakeIndexedAction>>, mpl::list<Args...>, mpl::make_int_sequence<mpl::int_<sizeof...(Args)>>>;
        using FlattenedActions = mpl::flatten<IndexedActions>;
		using Steps = mpl::split_if<FlattenedActions, mpl::bind1<std::is_same, SequencePoint>::template f>;
        using Merged = detail::MergeActionStepsT<Steps>;
        using Actions = mpl::flatten<Merged>;
        using Functors = mpl::transform<Actions, detail::GetAction>;
        using Inputs = mpl::transform<Actions, detail::GetInputs>;
        detail::NoReadApply<Functors, Inputs> a{};
        a(detail::argToUnsigned(args)...);
    }

    // if apply does not contain reads or runtime writes we can speed things up
    template <typename... Args>
    typename std::enable_if<detail::all_compile_time<Args...>::value>::type apply(Args... args)
    {
        static_assert(detail::args_to_apply_are_plausible<Args...>::value,
                      "one of the supplied arguments is not supported");
        using FlattenedActions = mpl::flatten<mpl::list<Args...>>;
		using Steps = mpl::split_if<FlattenedActions, mpl::bind1<std::is_same, SequencePoint>::template f>;
        using Merged = detail::MergeActionStepsT<Steps>;
        using Actions = mpl::flatten<Merged>;
        using Functors = mpl::transform<Actions, detail::GetAction>;
        detail::noReadNoRuntimeWriteApply((Actions *)nullptr);
    }

    // no parameters is allowed because it could be used in machine generated code
    // it does nothing
    inline void apply() {}
    inline void apply(mpl::list<>) {}

    template <typename TField, typename TField::DataType Value>
    inline bool fieldEquals(field_value<TField, Value>)
    {
        return apply(Action<TField, read_action>{}) == Value;
    }
}
}
