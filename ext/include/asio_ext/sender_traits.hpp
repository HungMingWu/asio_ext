
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>
#include <boost/mp11/algorithm.hpp>
#include <asio_ext/type_traits.hpp>
#include <tuple>

namespace asio_ext
{
    namespace detail
    {
        template <class... T>
        struct type_list
        {};

        template<class T>
        struct flattened_value_type
        {
            using type = T;
        };

        template<template<class...> class Tuple, template<class...> class Variant, class Type>
        struct flattened_value_type<Variant<Tuple<Type>>>
        {
            using type = Type;
        };

        template<template<class...> class Tuple, template<class...> class Variant, class...Types>
        struct flattened_value_type<Variant<Tuple<Types...>>>
        {
            using type = Tuple<Types...>;
        };

        template <template <class...> class Variant, class Sender, class... ErrorTypes>
        struct append_error_types_base
        {
            using sender_errors = typename Sender::template error_types<Variant>;
            using concatenated = boost::mp11::mp_append<sender_errors, Variant<ErrorTypes...>>;
            using type = boost::mp11::mp_unique<concatenated>;
        };

        struct void_value_tag_type
        {
        };

        template <template <class...> class Tuple, template <class...> class Variant, class T1, class T2>
        struct append_value_types_impl;

        template <template <class...> class Tuple, template <class...> class Variant, class... ValueTuples1,
            class... ValueTuples2>
            struct append_value_types_impl<Tuple, Variant, Variant<ValueTuples1...>, Variant<ValueTuples2...>>
        {
            using values1_void_values_tagged =
                boost::mp11::mp_replace<Variant<ValueTuples1...>, Tuple<>, void_value_tag_type>;
            using values2_void_values_tagged =
                boost::mp11::mp_replace<Variant<ValueTuples2...>, Tuple<>, void_value_tag_type>;
            using concatenated =
                boost::mp11::mp_append<values1_void_values_tagged, values2_void_values_tagged>;
            using unique_concat = boost::mp11::mp_unique<concatenated>;
            using type = boost::mp11::mp_replace<unique_concat, void_value_tag_type, Tuple<>>;
        };
    } // namespace detail

    template <template <class...> class Variant, class Sender, class... ErrorTypes>
    using append_error_types =
        typename detail::append_error_types_base<Variant, Sender, ErrorTypes...>::type;

    template <template <class...> class Variant, class Sender, class Sender2>
    using merge_error_types = append_error_types<Variant, Sender, 
        typename asio::execution::sender_traits<Sender2>::template error_types<Variant>>;

    template <template <class...> class Tuple, template <class...> class Variant, class Sender,
        class... ValueTuples>
        using append_value_types =
        typename detail::append_value_types_impl<Tuple, Variant,
        typename Sender::template value_types<Tuple, Variant>,
        Variant<ValueTuples...>>::type;


    template <template<class...> class Tuple, template<class...> class Variant, class Types1, class Types2>
    using concat_value_types = typename detail::append_value_types_impl<Tuple, Variant, Types1, Types2>::type;

    template <template <class...> class Tuple, template <class...> class Variant, class Sender1,
        class Sender2>
        using merge_sender_value_types = typename detail::append_value_types_impl<
        Tuple, Variant, 
        typename asio::execution::sender_traits<Sender1>::template value_types<Tuple, Variant>,
        typename asio::execution::sender_traits<Sender2>::template value_types<Tuple, Variant>>::type;

    namespace detail
    {
        template<template<class...> class Variant, class Function>
        struct function_result_types_helper
        {
            template<class T>
            struct is_applyable;

            template<template<class...> class Tuple, class...ValueT>
            struct is_applyable<Tuple<ValueT...>>
            {
                static constexpr bool value = std::is_invocable_v<Function, ValueT&...>;
            };
            template<class Tuple>
            using apply_result = decltype(std::apply(std::declval<Function>(), std::declval<Tuple&>()));

            template<class Tuple>
            using not_applyable = std::negation<is_applyable<Tuple>>;

            template<class Sender>
            using sender_value_types = typename asio::execution::sender_traits<Sender>::template value_types<std::tuple, Variant>;

            template<class Sender>
            using viable_sender_values = boost::mp11::mp_remove_if<sender_value_types<Sender>, not_applyable>;

            template<class Sender>
            using sender_results = boost::mp11::mp_transform<apply_result, viable_sender_values<Sender>>;
        };
    }

    template<template<class...> class Variant, class Function, class Sender>
    using function_result_types = typename detail::function_result_types_helper<Variant, Function>::template sender_results<Sender>;
} // namespace asio_ext