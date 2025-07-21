#pragma once

#include <cstdlib>
#include <type_traits>

namespace tarp {

// Non-standard layout objects are not supported because in their
// case we cannot necessarily get a pointer to the container from
// a pointer to a member simply by adjusting an offset.
// (Instead, conceivably multiple indirections may be required, etc).
template<typename Parent, typename Member, Member Parent::*Ptr>
struct is_safely_offsettable {
    static constexpr bool value =
      std::is_object_v<Member> &&
      std::is_member_object_pointer_v<decltype(Ptr)> &&
      std::is_standard_layout_v<Member> && std::is_standard_layout_v<Parent>;
};

template<typename Parent, typename Member, Member Parent::*Ptr>
[[maybe_unused]] static inline constexpr bool is_safely_offsettable_v =
  is_safely_offsettable<Parent, Member, Ptr>::value;

template<typename Parent, typename Member, Member Parent::*Ptr>
concept SafelyOffsettable = is_safely_offsettable<Parent, Member, Ptr>::value;

//

template<typename Parent, typename Member, Member Parent::*member_ptr>
constexpr std::ptrdiff_t member_offset() {
    static_assert(is_safely_offsettable<Parent, Member, member_ptr>::value,
                  "Unsafe use of member_to_parent: illegal layout");

    // Compute offset using pointer-to-member.
    // For the ->* syntax, see
    // https://en.cppreference.com/w/cpp/language/operator_member_access.html#Built-in_pointer-to-member_access_operators.
    // What we are doing here is:
    // 1. cast 0 (nullptr) to a Parent*. In other words, we imagine a Parent
    // exists that starts at address 0.
    // 2. We take the address of the member pointer, residing at some offset
    // inside this parent object. Because the Parent object starts at address 0,
    // the address of the member in this _particular_ case gives us
    // the **offset** in Parent of that member in the _general_ case.
    // Note we do not dereference any pointers. E.g. ->*member_ptr here is
    // in an unevaluated context (because it is the operand of the unary &
    // address-of operator). Since the reinterpret_cast result is never
    // dereferenced, the behavior is not undefined.
    return reinterpret_cast<std::ptrdiff_t>(
      &(reinterpret_cast<Parent *>(0)->*member_ptr));
}

// Get a pointer to the container (parent) of Member.
//
// Given a pointer to a member field of a given object,
// return a pointer to the containing object itself.
// If the input pointer is nullptr, nullptr is returned.
//
// NOTE:
// This accomplishes the same purpose as the classic container_of
// implementation in C (e.g. see libtarp/include/tarp/container.h
// and the notes there), often seen in kernel code.
// However, the C-based container_of implementation relies on macros
// and the use of offsetof. However, littering a c++ codebase with
// such macros is highly undesirable. As it turns out, however, the
// same thing is in fact achievable in c++ --- in a type safe manner
// and without resorting to any macros. Instead, the somewhat
// arcane pointer-to-member provides us the mechanism for calculating
// the required offset used to get back the pointer of the containing
// object (as mentioned, this is only safe for standard-layout types).
//
// NOTE: member_ptr uses pointer to member syntax,
// see https://en.cppreference.com/w/cpp/language/pointer.html.
template<typename Parent, typename Member, Member Parent::*member_ptr>
Parent *container_of(Member *member) {
    // Subtract to get parent pointer
    const auto member_addr = reinterpret_cast<char *>(member);
    return reinterpret_cast<Parent *>(
      member_addr - member_offset<Parent, Member, member_ptr>());
}

// Get a pointer to the container (parent) of Member.
// See non-const overload for details.
template<typename Parent, typename Member, Member Parent::*member_ptr>
const Parent *container_of(const Member *member) {
    const auto member_addr = reinterpret_cast<const char *>(member);
    return reinterpret_cast<const Parent *>(
      member_addr - member_offset<Parent, Member, member_ptr>());
}

};  // namespace tarp
