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
namespace kvasir
{

namespace bit
{

    namespace detail
    {
        namespace br = mpl;

        template <std::size_t I, typename O, typename A, typename L>
        struct MakeSeperatorsImpl
        {
            using type = O;
        };
        template <std::size_t I, typename... O, typename... A, typename... T>
        struct MakeSeperatorsImpl<I, br::list<O...>, br::list<A...>, br::list<br::uint_<I>, T...>>
            : MakeSeperatorsImpl<(I + 1), br::list<O..., br::list<A...>>, br::list<>,
                                 br::list<T...>>
        {
        };

        template <std::size_t I, typename... O, typename... A, typename U, typename... T>
        struct MakeSeperatorsImpl<I, br::list<O...>, mpl::list<A...>, mpl::list<U, T...>>
            : MakeSeperatorsImpl<I + 1, mpl::list<O...>, mpl::list<A..., unsigned>,
                                 mpl::list<U, T...>>
        {
        };
	
	template<typename T1, typename T2>
	using less = mpl::bool_<(T1::value<T2::value)>;


        template <typename T>
        using MakeSeperators =
            MakeSeperatorsImpl<0, mpl::list<>, mpl::list<>, mpl::sort<T, less>>;

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
        struct GetAction<Action<F, A>>
        {
            using type = Action<F, A>;
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
        struct IndexedActionLess<IndexedAction<Action<T1, U1>, TInputs1...>,
                                 IndexedAction<Action<T2, U2>, TInputs2...>>
            : mpl::bool_<(GetAddress<T1>::value < GetAddress<T2>::value)>
        {
        };
        template <typename T1, typename U1, typename T2, typename U2>
        struct IndexedActionLess<Action<T1, U1>, Action<T2, U2>>
            : mpl::bool_<(GetAddress<T1>::value < GetAddress<T2>::value)>
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
        template <typename TAddress, unsigned Mask1, unsigned Mask2, typename TAccess1,
                  typename TAccess2, typename TFieldType1, typename TFieldType2,
                  template <unsigned> class TActionTemplate, unsigned Value1, unsigned Value2,
                  typename... TInputs1, typename... TInputs2, typename... Ts,
                  typename... Us> // next input and last merged are mergable
        struct MergebitActions<
            mpl::list<
                IndexedAction<Action<FieldLocation<TAddress, Mask1, TAccess1, TFieldType1>,
                                     TActionTemplate<Value1>>,
                              TInputs1...>,
                Ts...>,
            mpl::list<
                IndexedAction<Action<FieldLocation<TAddress, Mask2, TAccess2, TFieldType2>,
                                     TActionTemplate<Value2>>,
                              TInputs2...>,
                Us...>>
            : MergebitActions<
                  mpl::list<Ts...>,
                  mpl::list<
                      IndexedAction<Action<FieldLocation<TAddress,
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
        template <typename TAddress, unsigned Mask1, unsigned Mask2, typename TAccess1,
                  typename TAccess2, typename TFieldType1, typename TFieldType2,
                  template <unsigned> class TActionTemplate, unsigned Value1, unsigned Value2,
                  typename... Ts, typename... Us> // next input and last merged are mergable
        struct MergebitActions<
            mpl::list<Action<FieldLocation<TAddress, Mask1, TAccess1, TFieldType1>,
                                 TActionTemplate<Value1>>,
                          Ts...>,
            mpl::list<Action<FieldLocation<TAddress, Mask2, TAccess2, TFieldType2>,
                                 TActionTemplate<Value2>>,
                          Us...>>
            : MergebitActions<
                  mpl::list<Ts...>,
                  mpl::list<Action<FieldLocation<TAddress,
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
        struct GetAddress<IndexedAction<TAction, TInputs...>> : GetAddress<TAction>
        {
        };

        template <bool TopLevel, typename TAction, typename Index>
        struct MakeIndexedActionImpl;
        // in default case there is no actual expected input
        template <bool TopLevel, typename TAddress, unsigned Mask, typename TAccess, typename TR,
                  typename TAction, int Index>
        struct MakeIndexedActionImpl<
            TopLevel, Action<FieldLocation<TAddress, Mask, TAccess, TR>, TAction>, int_<Index>>
        {
            using type = IndexedAction<Action<FieldLocation<TAddress, Mask, TAccess, TR>, TAction>>;
        };

        // special case where there actually is expected input
        template <bool TopLevel, typename TAddress, unsigned Mask, typename TAccess, typename TR,
                  int Index>
        struct MakeIndexedActionImpl<
            TopLevel, Action<FieldLocation<TAddress, Mask, TAccess, TR>, WriteAction>, int_<Index>>
        {
            static_assert(
                TopLevel,
                "runtime values can only be executed in an apply, they cannot be stored in a list");
            using type =
                IndexedAction<Action<FieldLocation<TAddress, Mask, TAccess, TR>, WriteAction>,
                              mpl::uint_<Index>>;
        };

        // special case where there actually is expected input
        template <bool TopLevel, typename TAddress, unsigned Mask, typename TAccess, typename TR,
                  int Index>
        struct MakeIndexedActionImpl<
            TopLevel, Action<FieldLocation<TAddress, Mask, TAccess, TR>, XorAction>, mpl::int_<Index>>
        {
            static_assert(
                TopLevel,
                "runtime values can only be executed in an apply, they cannot be stored in a list");
            using type =
                IndexedAction<Action<FieldLocation<TAddress, Mask, TAccess, TR>, WriteAction>,
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
        struct MakeIndexedActionImpl<TopLevel, SequencePoint, Index>
        {
            using type = SequencePoint;
        };

        template <typename TAction, typename Index>
        using MakeIndexedAction = typename MakeIndexedActionImpl<true, TAction, Index>::type;

        template <unsigned I>
        struct IsAddressPred
        {
            template <typename T>
            struct apply : mpl::bool_<false>
            {
            };
            template <typename TAddress, unsigned Mask, typename TAccess, typename TFieldType,
                      typename Cmd>
            struct apply<Action<FieldLocation<TAddress, Mask, TAccess, TFieldType>, Cmd>>
                : mpl::integral_constant<bool, (I == GetAddress<TAddress>::value)>
            {
            };
        };

        // takes an args list or tree, flattens it and removes all actions which are not reads
        template <typename... TArgList>
        using GetReadsT =
            mpl::transform<		
	    	mpl::remove_if<
			mpl::flatten<
				mpl::list<TArgList...>
			>,
			IsNotReadPred
			>, 
			GetFieldLocation
	    >;

        template <typename... T>
        constexpr bool noneRuntime()
        {
            return mpl::size<mpl::remove_if<mpl::list<T...>, IsNotRuntimeWritePred>>::value ==0;
        }

        template <typename... T>
        struct AllCompileTime
        {
            static constexpr bool value =
                (mpl::size<detail::GetReadsT<mpl::list<T...>>>::value == 0) &&
                noneRuntime<T...>();
        };

        template <typename... T>
        struct NoReadsRuntimeWrites
        {
            static constexpr bool value =
                (mpl::size<detail::GetReadsT<mpl::list<T...>>>::value == 0) &&
                !noneRuntime<T...>();
        };

        template <typename T>
        struct GetReadMask : mpl::int_<0>
        {
        };

        template <typename T>
        struct GetAddresses;
        template <typename TAddresses, typename TLocations>
        struct GetAddresses<FieldTuple<TAddresses, TLocations>> : TAddresses
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
        template <typename... TActions, typename... TInputIndexes, typename... TRetAddresses,
                  typename TRetLocations>
        struct Apply<mpl::list<TActions...>, mpl::list<TInputIndexes...>,
                     FieldTuple<mpl::list<TRetAddresses...>, TRetLocations>>
        {
            using ReturnType = FieldTuple<mpl::list<TRetAddresses...>, TRetLocations>;
            template <unsigned A>

                typename std::enable_if<mpl::any<mpl::list<TRetAddresses...>, mpl::bind1<std::is_same, mpl::uint_<A>>::template f>::value>
                filterReturns(ReturnType & ret, unsigned in)
            {
					ret.value_[mpl::c::call<mpl::c::find_if<mpl::bind1<std::is_same, mpl::uint_<A>>, mpl::c::offset<mpl::uint_<sizeof...(TRetAddresses)>>>, mpl::list<TRetAddresses...>>::value] |= in;
            }
            template <unsigned A>
            void filterReturns(...)
            {
            }

            template <typename... T>
            ReturnType operator()(T... args)
            {
                ReturnType ret{{}}; // default constructed return
                const unsigned a[]{0U, (filterReturns<detail::GetAddress<TActions>::value>(
                                            ret, ExecuteSeam<TActions, ::kvasir::Tag::User>{}(
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
                const unsigned a[]{0U, (ExecuteSeam<TActions, ::kvasir::Tag::User>{}(
                                            Finder<TInputIndexes>{}(args...)),
                                        0U)...};
                ignore(a);
            }
        };

        // no read no runtime write apply
        template <typename... TActions>
        void noReadNoRuntimeWriteApply(mpl::list<TActions...> *)
        {
            const unsigned a[]{0U, ExecuteSeam<TActions, ::kvasir::Tag::User>{}(0U)...};
            ignore(a);
        }

        template <typename... Ts>
        using GetReturnType =
            FieldTuple<mpl::remove_adjacent<mpl::sort<
                           mpl::transform<
			   	GetReadsT<Ts...>,
				GetAddress
			   >, std::is_same>, std::is_same>,
                       GetReadsT<Ts...>>;
        template <typename T>
        struct ArgToApplyIsPlausible : mpl::bool_<false>
        {
        };
        template <typename L, typename A>
        struct ArgToApplyIsPlausible<Action<L, A>> : mpl::bool_<true>
        {
        };
        template <>
        struct ArgToApplyIsPlausible<SequencePoint> : mpl::bool_<true>
        {
        };
	template <typename... Ts>
        using ArgsToApplyArePlausible = mpl::all<mpl::flatten<mpl::list<Ts...>>, ArgToApplyIsPlausible>;
	//        template <typename T, typename... Ts>
//        struct ArgsToApplyArePlausible
//        {
//            using l = mpl::flatten<mpl::list<T, Ts...>>;
//            using type = mpl::bool_<
//                std::is_same<mpl::RepeatC<mpl::size<l>::value, mpl::TrueType>,
//                             mpl::transform<l, mpl::quote<ArgToApplyIsPlausible>>>::value>;
//            static constexpr int value = type::value;
//        };
    }

    // if apply contains reads return a FieldTuple
    template <typename... Args>
    inline
        typename std::enable_if<(mpl::c::call<mpl::c::count_if<mpl::lambda<detail::IsNotReadPred>>, mpl::flatten<mpl::list<Args...>>>::value >
                                 0),
                                detail::GetReturnType<Args...>>::type apply(Args... args)
    {
        static_assert(detail::ArgsToApplyArePlausible<Args...>::value,
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
        detail::Apply<Functors, Inputs, detail::GetReturnType<Args...>> a{};
        return a(detail::argToUnsigned(args)...);
    }

    // if apply does not contain reads return is void
    template <typename... Args>
    typename std::enable_if<detail::NoReadsRuntimeWrites<Args...>::value>::type
        apply(Args... args)
    {
        static_assert(detail::ArgsToApplyArePlausible<Args...>::value,
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
    
        typename std::enable_if<detail::AllCompileTime<Args...>::value>::type apply(Args... args)
    {
        static_assert(detail::ArgsToApplyArePlausible<Args...>::value,
                      "one of the supplied arguments is not supported");
        // using IndexedActions = mpl::transform<mpl::list<Args...>,
        // mpl::BuildIndicesT<sizeof...(Args)>, mpl::quote<detail::MakeIndexedAction>>;
        using FlattenedActions = mpl::flatten<mpl::list<Args...>>;
		using Steps = mpl::split_if<FlattenedActions, mpl::bind1<std::is_same, SequencePoint>::template f>;
        using Merged = detail::MergeActionStepsT<Steps>;
        using Actions = mpl::flatten<Merged>;
        // using Functors = mpl::transform<Actions, mpl::quote<detail::GetAction>>;
        detail::noReadNoRuntimeWriteApply((Actions *)nullptr);
    }

    // no parameters is allowed because it could be used in machine generated code
    // it does nothing
    inline void apply() {}
    inline void apply(mpl::list<>) {}

    template <typename TField, typename TField::DataType Value>
    inline bool fieldEquals(FieldValue<TField, Value>)
    {
        return apply(Action<TField, ReadAction>{}) == Value;
    }
}
}
