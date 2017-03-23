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
#include "utility.hpp"

namespace kvasir
{

namespace bit
{

    namespace detail
    {

        template <typename TbitAction>
        struct exec;

        template <typename TLocation, unsigned ClearMask, unsigned SetMask>
        struct generic_read_mask_or_write
        {
            unsigned operator()(unsigned in = 0)
            {
                auto i = get_address<TLocation>::read();
                i &= ~ClearMask;
                i |= SetMask | in;
                get_address<TLocation>::write(i);
                return i;
            }
        };

        template <typename TLocation, unsigned ClearMask, unsigned XorMask>
        struct generic_read_mask_xor_write
        {
            unsigned operator()(unsigned in = 0)
            {
                auto i = get_address<TLocation>::read();
                i &= ~ClearMask;
                i ^= (XorMask | in);
                get_address<TLocation>::write(i);
                return i;
            }
        };

        // write literal with read modify write
        template <typename Taddress, unsigned Mask, typename Access, typename FieldType,
                  unsigned Data>
        struct exec<action<field_location<Taddress, Mask, Access, FieldType>,
                                             write_literal_action<Data>>>
            : generic_read_mask_or_write<field_location<Taddress, Mask, Access, FieldType>, Mask, Data>
        {
            static_assert((Data & (~Mask)) == 0, "bad mask");
        };

        template <typename Taddress, unsigned Mask, typename Access, typename FieldType>
        struct exec<
            action<field_location<Taddress, Mask, Access, FieldType>, write_action>>
            : generic_read_mask_or_write<field_location<Taddress, Mask, Access, FieldType>, Mask, 0>
        {
        };

        template <typename Taddress, unsigned Mask, typename Access, typename FieldType>
        struct exec<
            action<field_location<Taddress, Mask, Access, FieldType>, read_action>>
        {
            unsigned operator()(unsigned in = 0)
            {
                return get_address<Taddress>::read();
            }
        };
        template <typename Taddress, unsigned Mask, typename Access, typename FieldType,
                  unsigned Data>
        struct exec<action<field_location<Taddress, Mask, Access, FieldType>,
                                             xor_literal_action<Data>>>
            : generic_read_mask_or_write<field_location<Taddress, Mask, Access, FieldType>, Mask, Data>
        {
            static_assert((Data & (~Mask)) == 0, "bad mask");
            unsigned operator()(unsigned in = 0)
            {
                auto i = get_address<Taddress>::read();
                i ^= Data;
                get_address<Taddress>::write(i);
                return 0;
            }
        };
    }

    template <typename T, typename U>
    struct execute_seam : detail::exec<T>
    {
    };
}
}
