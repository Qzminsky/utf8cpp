// Copyright © 2020 Alex Qzminsky.
// License: MIT. All rights reserved.

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "utf8string.hpp"

/// Bad assertions exception class
struct assertion_failure : std::logic_error
{
    explicit assertion_failure (const char* msg)
        : std::logic_error{ msg }
    {}
};

/**
 * \brief Compares the set of values by its equality and throws when at least one is different than others
 * 
 * \param value First value to compare
 * \param others Tail of the set of values to compare
 * 
 * \throw assertion_failure
*/
template <typename T,
          typename... Pack
>
auto assert_eq (T const& value, Pack const&... others) -> void
{
    if (!((value == others) && ...)) throw assertion_failure{ "Equality assertion failed" };
}

/**
 * \brief Invokes the given function and throws if it doesn't throw an exception
 * (specified by a template) itself
 * 
 * \param expr Function to invoke
 * 
 * \throw assertion_failure
*/
template <typename ErrT,
          typename Functor
>
auto assert_throws (Functor&& expr) -> void
{
    try { expr(); }
    catch (ErrT const&) { return; }
    catch (...)
    {
        throw assertion_failure{ "Assertion failed" };
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Entry point
*/
auto main() -> int
{
    // Sample string constructed from a C-string
    utf::string MyStr{ "Mr Dursley was the director of a firm called Grunnings" };

    // ⚡ Powerful chaining
    assert_eq(
        MyStr.first(MyStr.chars().backward().find_if(utf::is_space).as_forward_index()),
        //   ↑           ↑       ↑          ↑                      ↑
        //   │           └➀ take the string_view to access the characters
        //   │                   └➁ get its reversed copy          │
        //   │                              └➂ find first space character from the end
        //   │                                                     └➃ transform the iterator to its character's index
        //   └➄ ...and take the clipped string_view from the left side of original string
        "Mr Dursley was the director of a firm called"
    );

#if __cplusplus >= 2020'00
    // Multilingual construction — char8_t-literals is a C++20 feature
    assert_eq(utf::string{ u8"السلام " }.push(u8"عليكم"), u8"السلام عليكم");
    //        ^^^^^^^^^^^^^^^^^^^^^^^^^      ^^^^^^^^^^  ^^^^^^^^^^^^^^^^
    //                 strings                    └ and views ┘

    // WOW
    assert_eq(utf::string{ u8"Z͑ͫ̓ͪ̂ͫ̽͏̴̙̤̞͉͚̯̞̠͍A̴̵̜̰͔ͫ͗͢L̠ͨͧͩ͘G̴̻͈͍͔̹̑͗̎̅͛́Ǫ̵̹̻̝̳͂̌̌͘!͖̬̰̙̗̿̋ͥͥ̂ͣ̐́́͜͞" }.length(), 75);  // — in codepoints, not graphemes
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // 🧺 Capacity changes sample
    // In the beginning, constructed string have the capacity same as the size
    assert_eq(MyStr.size(), MyStr.capacity(), 54);
    
    // Manually increase the capacity of the memory buffer
    MyStr.reserve(100);
    //      ⇓
    assert_eq(MyStr.size(), 54);
    assert_eq(MyStr.capacity(), 100);   // (*)

    // Thus, we can insert characters until the place runs out
    assert_eq(MyStr.push(", which made drills"), "Mr Dursley was the director of a firm called Grunnings, which made drills");
    //      ⇓
    assert_eq(MyStr.capacity(), 100);   // same as (*)

    // Now, reallocate the memory and reduce the capacity back to the size
    MyStr.shrink_to_fit();
    //      ⇓
    assert_eq(MyStr.size(), MyStr.capacity(), 73);  // not 54 since we pushed some characters above

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // ⛔ Some errors sample
    assert_throws<utf::unicode_error>([=] () mutable { MyStr.push(0xFFFFFFFF); });
    //            ^^^^^^^^^^^^^^^^^^                   ^^^^^^^^^^^^^^^^^^^^^^^

    assert_throws<utf::length_error>([&] { MyStr.chars(10, -5); });             // (*)
    //            ^^^^^^^^^^^^^^^^^        ^^^^^^^^^^^^^^^^^^^^

    assert_throws<utf::invalid_argument>([&] { MyStr.chars(-10, 5); });         // (**)
    //            ^^^^^^^^^^^^^^^^^^^^^        ^^^^^^^^^^^^^^^^^^^^

    assert_throws<utf::underflow_error>([] { utf::string{}.pop(); });
    //            ^^^^^^^^^^^^^^^^^^^^       ^^^^^^^^^^^^^^^^^^^^

    assert_throws<utf::out_of_range>([&] { *MyStr.chars().find(0xA2); });       // (***)
    //            ^^^^^^^^^^^^^^^^^        ^^^^^^^^^^^^^^^^^^^^^^^^^^
    //                                     ↑             ↑
    //                                     │             └➀ the result of unsuccessful char-search is end()
    //                                     └➁ end iterator dereferencing is invalid

    assert_throws<utf::bad_operation>([&] { ++MyStr.chars().begin().free(); });
    //            ^^^^^^^^^^^^^^^^^^        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //                                      ↑                      ↑
    //                                      │                      └➀ an unbound iterator...
    //                                      └➁ ...cannot be modified or compared

    // (*), (**), (***) — Results ignoring. 🛇 Do not repeat it at home.

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // 🔍 Variadic modifiers and searchers sample
    MyStr = "Are you sure that's a real spell? Well, it's not very good, is it? "
            "I've tried a few simple spells just for practice";
    
    assert_eq(
        MyStr.replace_all("spell", "joke"),
        //      ⇓
        "Are you sure that's a real joke? Well, it's not very good, is it? "
        "I've tried a few simple jokes just for practice"
    );
    
    // Remove substrings by  patterns   and   range,      replace "simple" by "times"
    assert_eq(  //   ╭——————————˄——————————╮    ↓             ↓
        MyStr.remove(" joke", " jokes", "a ").erase(26, 33).replace(41, 6, "times"),
        //      ⇓
        "Are you sure that's real? I've tried few times just for practice"
    );

    MyStr = "One Hagrid $s";
    assert_eq(
        MyStr.replace(MyStr.chars().find('$'), 0xA5),   //              UTF-8 of '¥' (codepoint 0xA5)
        //      ⇓                                                                   ╭———˄———╮
        utf::string::from_bytes({ 79, 110, 101, 32, 72, 97, 103, 114, 105, 100, 32, 194, 165, 115 })
        // == "One Hagrid ¥s"
    );

    // Caesar transform
    MyStr = "Curiouser and curiouser!";
    assert_eq(
        MyStr.transform([] (utf::string::char_type ch)
        {
            return std::isalpha(ch) ? (ch + 2) : ch;
        }),
        //      ⇓
        "Ewtkqwugt cpf ewtkqwugt!"
    );

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // ↔ Numbers conversion sample
    assert_eq(utf::to_string(0xdeadf00d, 16).to_ascii_upper(), "DEADF00D");
    assert_eq(utf::to_string(std::numeric_limits<uint64_t>::max(), 2), utf::string{ '1', 64 });
    assert_eq(utf::to_string(std::numeric_limits<int64_t>::min()), "-9223372036854775808");
    assert_eq(utf::to_string(2.718281828, 'f', 5), "2.71828");

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // ♯ Hashing sample
    assert_eq(std::hash<utf::string>{}("Hashable magic"), 11248827619910581013ULL);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    std::cout << "Everything is correct" << std::endl;
    std::cin.get();
}
