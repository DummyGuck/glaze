// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include <charconv>
#include <iterator>
#include <ostream>
#include <variant>

#include "glaze/core/format.hpp"
#include "glaze/core/write.hpp"
#include "glaze/core/write_chars.hpp"
#include "glaze/json/ptr.hpp"
#include "glaze/reflection/reflect.hpp"
#include "glaze/util/dump.hpp"
#include "glaze/util/for_each.hpp"
#include "glaze/util/itoa.hpp"

namespace glz
{
   namespace detail
   {
      template <class T = void>
      struct to_json
      {};

      template <auto Opts, class T, class Ctx, class B, class IX>
      concept write_json_invocable = requires(T&& value, Ctx&& ctx, B&& b, IX&& ix) {
         to_json<std::remove_cvref_t<T>>::template op<Opts>(std::forward<T>(value), std::forward<Ctx>(ctx),
                                                            std::forward<B>(b), std::forward<IX>(ix));
      };

      template <>
      struct write<json>
      {
         template <auto Opts, class T, is_context Ctx, class B, class IX>
         GLZ_ALWAYS_INLINE static void op(T&& value, Ctx&& ctx, B&& b, IX&& ix) noexcept
         {
            if constexpr (write_json_invocable<Opts, T, Ctx, B, IX>) {
               to_json<std::remove_cvref_t<T>>::template op<Opts>(std::forward<T>(value), std::forward<Ctx>(ctx),
                                                                  std::forward<B>(b), std::forward<IX>(ix));
            }
            else {
               static_assert(false_v<T>, "Glaze metadata is probably needed for your type");
            }
         }
      };

      template <glaze_value_t T>
      struct to_json<T>
      {
         template <auto Opts, class Value, is_context Ctx, class B, class IX>
         GLZ_ALWAYS_INLINE static void op(Value&& value, Ctx&& ctx, B&& b, IX&& ix) noexcept
         {
            using V = std::remove_cvref_t<decltype(get_member(std::declval<Value>(), meta_wrapper_v<T>))>;
            to_json<V>::template op<Opts>(get_member(std::forward<Value>(value), meta_wrapper_v<T>),
                                          std::forward<Ctx>(ctx), std::forward<B>(b), std::forward<IX>(ix));
         }
      };

      template <is_bitset T>
      struct to_json<T>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&& value, auto&&, auto&& b, auto&& ix) noexcept
         {
            dump<'"'>(b, ix);
            for (size_t i = value.size(); i > 0; --i) {
               value[i - 1] ? dump<'1'>(b, ix) : dump<'0'>(b, ix);
            }
            dump<'"'>(b, ix);
         }
      };

      template <glaze_flags_t T>
      struct to_json<T>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&&, auto&& b, auto&& ix) noexcept
         {
            static constexpr auto N = std::tuple_size_v<meta_t<T>>;

            dump<'['>(b, ix);

            for_each<N>([&](auto I) {
               static constexpr auto item = glz::get<I>(meta_v<T>);

               if (get_member(value, glz::get<1>(item))) {
                  dump<'"'>(b, ix);
                  dump(glz::get<0>(item), b, ix);
                  dump<'"'>(b, ix);
                  dump<','>(b, ix);
               }
            });

            if (b[ix - 1] == ',') {
               b[ix - 1] = ']';
            }
            else {
               dump<']'>(b, ix);
            }
         }
      };

      template <>
      struct to_json<hidden>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&&, is_context auto&&, auto&&... args) noexcept
         {
            dump(R"("hidden type should not have been written")", args...);
         }
      };

      template <>
      struct to_json<skip>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&&, is_context auto&&, auto&&... args) noexcept
         {
            dump(R"("skip type should not have been written")", args...);
         }
      };

      template <is_member_function_pointer T>
      struct to_json<T>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&&, is_context auto&&, auto&&...) noexcept
         {}
      };

      template <is_reference_wrapper T>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(auto&& value, Args&&... args) noexcept
         {
            using V = std::decay_t<decltype(value.get())>;
            to_json<V>::template op<Opts>(value.get(), std::forward<Args>(args)...);
         }
      };

      template <complex_t T>
      struct to_json<T>
      {
         template <auto Opts, class B>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, B&& b, auto&& ix) noexcept
         {
            dump<'['>(b, ix);
            write<json>::op<Opts>(value.real(), ctx, b, ix);
            dump<','>(b, ix);
            write<json>::op<Opts>(value.imag(), ctx, b, ix);
            dump<']'>(b, ix);
         }
      };

      template <boolean_like T>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(const bool value, is_context auto&&, Args&&... args) noexcept
         {
            if (value) {
               dump<"true">(std::forward<Args>(args)...);
            }
            else {
               dump<"false">(std::forward<Args>(args)...);
            }
         }
      };

      template <num_t T>
      struct to_json<T>
      {
         template <auto Opts, class B>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, B&& b, auto&& ix) noexcept
         {
            if constexpr (Opts.quoted_num) {
               dump<'"'>(b, ix);
            }
            write_chars::op<Opts>(value, ctx, b, ix);
            if constexpr (Opts.quoted_num) {
               dump<'"'>(b, ix);
            }
         }
      };

      constexpr uint16_t combine(const char chars[2]) noexcept
      {
         return uint16_t(chars[0]) | (uint16_t(chars[1]) << 8);
      }

      // clang-format off
      constexpr std::array<uint16_t, 256> char_escape_table = { //
         0, 0, 0, 0, 0, 0, 0, 0, combine(R"(\b)"), combine(R"(\t)"), //
         combine(R"(\n)"), 0, combine(R"(\f)"), combine(R"(\r)"), 0, 0, 0, 0, 0, 0, //
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
         0, 0, 0, 0, combine(R"(\")"), 0, 0, 0, 0, 0, //
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
         0, 0, combine(R"(\\)") //
      };
      // clang-format on

      template <size_t Bytes>
         requires(Bytes == 8)
      GLZ_ALWAYS_INLINE void serialize_string(const auto* in, auto* out, auto& ix) noexcept
      {
         uint64_t swar;
         while (true) {
            std::memcpy(&swar, in, Bytes);
            std::memcpy(out + ix, in, Bytes);
            auto next = has_quote(swar) | has_escape(swar) | is_less_16(swar);

            if (next) {
               next = std::countr_zero(next) >> 3;
               const auto escape_char = char_escape_table[uint32_t(in[next])];
               if (escape_char == 0) {
                  ix += next;
                  return;
               }
               ix += next;
               in += next;
               std::memcpy(out + ix, &escape_char, 2);
               ix += 2;
               ++in;
            }
            else {
               ix += 8;
               in += 8;
            }
         }
      }

      template <class T>
         requires str_t<T> || char_t<T>
      struct to_json<T>
      {
         template <auto Opts, class B>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&&, B&& b, auto&& ix) noexcept
         {
            if constexpr (Opts.number) {
               // TODO: Should we check if the string number is valid?
               dump(value, b, ix);
            }
            else if constexpr (char_t<T>) {
               if constexpr (Opts.raw) {
                  dump(value, b, ix);
               }
               else {
                  dump<'"'>(b, ix);

                  switch (value) {
                  case '"':
                     dump<"\\\"">(b, ix);
                     break;
                  case '\\':
                     dump<"\\\\">(b, ix);
                     break;
                  case '\b':
                     dump<"\\b">(b, ix);
                     break;
                  case '\f':
                     dump<"\\f">(b, ix);
                     break;
                  case '\n':
                     dump<"\\n">(b, ix);
                     break;
                  case '\r':
                     dump<"\\r">(b, ix);
                     break;
                  case '\t':
                     dump<"\\t">(b, ix);
                     break;
                  case '\0':
                     // escape character treated as empty string
                     break;
                  default:
                     // Hiding warning for build, this is an error with wider char types
                     dump(static_cast<char>(value), b,
                          ix); // TODO: This warning is an error We need to be able to dump wider char types
                  }

                  dump<'"'>(b, ix);
               }
            }
            else {
               if constexpr (Opts.raw_string) {
                  const sv str = [&]() -> sv {
                     if constexpr (!char_array_t<T> && std::is_pointer_v<std::decay_t<T>>) {
                        return value ? value : "";
                     }
                     else {
                        return value;
                     }
                  }();

                  // We need at space for quotes and the string length: 2 + n.
                  if constexpr (resizeable<B>) {
                     const auto n = str.size();
                     const auto k = ix + 2 + n;
                     if (k >= b.size()) [[unlikely]] {
                        b.resize((std::max)(b.size() * 2, k));
                     }
                  }
                  // now we don't have to check writing

                  dump_unchecked<'"'>(b, ix);
                  dump_unchecked(str, b, ix);
                  dump_unchecked<'"'>(b, ix);
               }
               else {
                  const sv str = [&]() -> sv {
                     if constexpr (!char_array_t<T> && std::is_pointer_v<std::decay_t<T>>) {
                        return value ? value : "";
                     }
                     else {
                        return value;
                     }
                  }();
                  const auto n = str.size();

                  // In the case n == 0 we need two characters for quotes.
                  // For each individual character we need room for two characters to handle escapes.
                  // So, we need 2 + 2 * n characters to handle all cases.
                  // We add another 8 characters to support SWAR
                  if constexpr (resizeable<B>) {
                     const auto k = ix + 10 + 2 * n;
                     if (k >= b.size()) [[unlikely]] {
                        b.resize((std::max)(b.size() * 2, k));
                     }
                  }
                  // now we don't have to check writing

                  if constexpr (Opts.raw) {
                     dump(str, b, ix);
                  }
                  else {
                     dump_unchecked<'"'>(b, ix);

                     // IMPORTANT: This code breaks address sanitizers (GCC and MSVC), because they don't like
                     // string memory being read beyond the length of the string, even if there is properly
                     // allocated memory.
                     // Also, this is only valid if Small String Optimization also has enough aligned memory when
                     // parsing reserving extra space does nothing for SSO and we must rely on padded alignment, which
                     // has always worked but is risky
                     /*if constexpr (string_t<T> && !std::is_const_v<std::remove_reference_t<decltype(value)>>) {
                        // we know the output buffer has enough space, but we must ensure the string buffer has space
                        // for swar as well
                        // say a string is of length 16, then the null character sits at index 16, which means we need
                        // another 8 bytes to read so we add one to the length to include the null character and round
                        // this up to the nearest multiple of 8
                        value.reserve(round_up_to_multiple<8>(n + 1));
                        serialize_string<8>(value.data(), data_ptr(b), ix);
                     }
                     else*/
                     {
                        const auto* c = str.data();
                        const auto* const e = c + n;

                        if (str.size() > 7) {
                           for (const auto end_m7 = e - 7; c < end_m7;) {
                              std::memcpy(data_ptr(b) + ix, c, 8);
                              uint64_t chunk;
                              std::memcpy(&chunk, c, 8);
                              const uint64_t test_chars = has_quote(chunk) | has_escape(chunk) | is_less_16(chunk);
                              if (test_chars) {
                                 const auto length = (std::countr_zero(test_chars) >> 3);
                                 c += length;
                                 ix += length;

                                 if (const auto escaped = char_escape_table[uint8_t(*c)]; escaped) [[likely]] {
                                    std::memcpy(data_ptr(b) + ix, &escaped, 2);
                                 }
                                 ix += 2;
                                 ++c;
                              }
                              else {
                                 ix += 8;
                                 c += 8;
                              }
                           }
                        }

                        // Tail end of buffer. Uncommon for long strings.
                        for (; c < e; ++c) {
                           if (const auto escaped = char_escape_table[uint8_t(*c)]; escaped) {
                              std::memcpy(data_ptr(b) + ix, &escaped, 2);
                              ix += 2;
                           }
                           else {
                              std::memcpy(data_ptr(b) + ix, c, 1);
                              ++ix;
                           }
                        }
                     }

                     dump_unchecked<'"'>(b, ix);
                  }
               }
            }
         }
      };

      template <glaze_enum_t T>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            using key_t = std::underlying_type_t<T>;
            static constexpr auto frozen_map = detail::make_enum_to_string_map<T>();
            const auto& member_it = frozen_map.find(static_cast<key_t>(value));
            if (member_it != frozen_map.end()) {
               const sv str = {member_it->second.data(), member_it->second.size()};
               // TODO: Assumes people dont use strings with chars that need to be escaped for their enum names
               // TODO: Could create a pre quoted map for better performance
               dump<'"'>(args...);
               dump(str, args...);
               dump<'"'>(args...);
            }
            else [[unlikely]] {
               // What do we want to happen if the value doesnt have a mapped string
               write<json>::op<Opts>(static_cast<std::underlying_type_t<T>>(value), ctx, std::forward<Args>(args)...);
            }
         }
      };

      template <class T>
         requires(std::is_enum_v<std::decay_t<T>> && !glaze_enum_t<T>)
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            write<json>::op<Opts>(static_cast<std::underlying_type_t<std::decay_t<T>>>(value), ctx,
                                  std::forward<Args>(args)...);
         }
      };

      template <func_t T>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&&, Args&&... args) noexcept
         {
            dump<'"'>(args...);
            dump(name_v<std::decay_t<decltype(value)>>, args...);
            dump<'"'>(args...);
         }
      };

      template <class T>
      struct to_json<basic_raw_json<T>>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&&, auto&& b, auto&& ix) noexcept
         {
            dump(value.str, b, ix);
         }
      };

      template <glz::opts Opts>
      GLZ_ALWAYS_INLINE void write_entry_separator(is_context auto&& ctx, auto&&... args) noexcept
      {
         dump<','>(args...);
         if constexpr (Opts.prettify) {
            dump<'\n'>(args...);
            dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
         }
      }

      template <opts Opts>
      GLZ_ALWAYS_INLINE void write_array_to_json(auto&& value, is_context auto&& ctx, auto&&... args)
      {
         dump<'['>(args...);

         if (!empty_range(value)) {
            if constexpr (Opts.prettify) {
               ctx.indentation_level += Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }

            auto it = std::begin(value);
            write<json>::op<Opts>(*it, ctx, args...);
            ++it;
            for (const auto fin = std::end(value); it != fin; ++it) {
               write_entry_separator<Opts>(ctx, args...);
               write<json>::op<Opts>(*it, ctx, args...);
            }
            if constexpr (Opts.prettify) {
               ctx.indentation_level -= Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
         }

         dump<']'>(args...);
      }

      template <writable_array_t T>
      struct to_json<T>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, auto&&... args) noexcept
         {
            write_array_to_json<Opts>(value, ctx, args...);
         }
      };

      template <opts Opts, class Key, class Value, is_context Ctx>
      GLZ_ALWAYS_INLINE void write_pair_content(const Key& key, Value&& value, Ctx& ctx, auto&&... args) noexcept
      {
         if constexpr (str_t<Key> || char_t<Key> || glaze_enum_t<Key> || Opts.quoted_num) {
            write<json>::op<Opts>(key, ctx, args...);
         }
         else {
            write<json>::op<opt_false<Opts, &opts::raw_string>>(quoted_t<const Key>{key}, ctx, args...);
         }
         if constexpr (Opts.prettify) {
            dump<": ">(args...);
         }
         else {
            dump<':'>(args...);
         }

         write<json>::op<opening_and_closing_handled_off<Opts>()>(std::forward<Value>(value), ctx, args...);
      }

      template <opts Opts, class Value>
      [[nodiscard]] GLZ_ALWAYS_INLINE constexpr bool skip_member(const Value& value) noexcept
      {
         if constexpr (null_t<Value> && Opts.skip_null_members) {
            if constexpr (always_null_t<Value>)
               return true;
            else {
               return !bool(value);
            }
         }
         else {
            return false;
         }
      }

      template <pair_t T>
      struct to_json<T>
      {
         template <glz::opts Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(const T& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            const auto& [key, val] = value;
            if (skip_member<Opts>(val)) {
               return dump<"{}">(args...);
            }

            dump<'{'>(args...);
            if constexpr (Opts.prettify) {
               ctx.indentation_level += Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }

            write_pair_content<Opts>(key, val, ctx, args...);

            if constexpr (Opts.prettify) {
               ctx.indentation_level -= Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            dump<'}'>(args...);
         }
      };

      template <writable_map_t T>
      struct to_json<T>
      {
         template <glz::opts Opts, class... Args>
            requires(!Opts.concatenate)
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            write_array_to_json<Opts>(value, ctx, args...);
         }

         template <glz::opts Opts, class... Args>
            requires(Opts.concatenate)
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            if constexpr (!Opts.opening_handled) {
               dump<'{'>(args...);
            }

            if (!empty_range(value)) {
               if constexpr (!Opts.opening_handled) {
                  if constexpr (Opts.prettify) {
                     ctx.indentation_level += Opts.indentation_width;
                     dump<'\n'>(args...);
                     dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
                  }
               }

               auto write_first_entry = [&ctx, &args...](auto&& it) {
                  if constexpr (requires {
                                   it->first;
                                   it->second;
                                }) {
                     // Allow non-const access, unlike ranges
                     if (skip_member<Opts>(it->second)) {
                        return true;
                     }
                     write_pair_content<Opts>(it->first, it->second, ctx, args...);
                     return false;
                  }
                  else {
                     const auto& [key, entry_val] = *it;
                     if (skip_member<Opts>(entry_val)) {
                        return true;
                     }
                     write_pair_content<Opts>(key, entry_val, ctx, args...);
                     return false;
                  }
               };

               auto it = std::begin(value);
               [[maybe_unused]] bool starting = write_first_entry(it);
               for (++it; it != std::end(value); ++it) {
                  // I couldn't find an easy way around this code duplication
                  // Ranges need to be decomposed with const auto& [...],
                  // but we don't want to const qualify our maps for the sake of reflection writing
                  // we need to be able to populate the tuple of pointers when writing with reflection
                  if constexpr (requires {
                                   it->first;
                                   it->second;
                                }) {
                     if (skip_member<Opts>(it->second)) {
                        continue;
                     }

                     // When Opts.skip_null_members, *any* entry may be skipped, meaning separator dumping must be
                     // conditional for every entry. Avoid this branch when not skipping null members.
                     // Alternatively, write separator after each entry except last but then branch is permanent
                     if constexpr (Opts.skip_null_members) {
                        if (!starting) {
                           write_entry_separator<Opts>(ctx, args...);
                        }
                     }
                     else {
                        write_entry_separator<Opts>(ctx, args...);
                     }

                     write_pair_content<Opts>(it->first, it->second, ctx, args...);
                  }
                  else {
                     const auto& [key, entry_val] = *it;
                     if (skip_member<Opts>(entry_val)) {
                        continue;
                     }

                     // When Opts.skip_null_members, *any* entry may be skipped, meaning separator dumping must be
                     // conditional for every entry. Avoid this branch when not skipping null members.
                     // Alternatively, write separator after each entry except last but then branch is permanent
                     if constexpr (Opts.skip_null_members) {
                        if (!starting) {
                           write_entry_separator<Opts>(ctx, args...);
                        }
                     }
                     else {
                        write_entry_separator<Opts>(ctx, args...);
                     }

                     write_pair_content<Opts>(key, entry_val, ctx, args...);
                  }

                  starting = false;
               }

               if constexpr (!Opts.closing_handled) {
                  if constexpr (Opts.prettify) {
                     ctx.indentation_level -= Opts.indentation_width;
                     dump<'\n'>(args...);
                     dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
                  }
               }
            }

            if constexpr (!Opts.closing_handled) {
               dump<'}'>(args...);
            }
         }
      };

      template <nullable_t T>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            if (value)
               write<json>::op<Opts>(*value, ctx, std::forward<Args>(args)...);
            else {
               dump<"null">(std::forward<Args>(args)...);
            }
         }
      };

      template <always_null_t T>
      struct to_json<T>
      {
         template <auto Opts>
         GLZ_ALWAYS_INLINE static void op(auto&&, is_context auto&&, auto&&... args) noexcept
         {
            dump<"null">(args...);
         }
      };

      template <is_variant T>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_FLATTEN static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            std::visit(
               [&](auto&& val) {
                  using V = std::decay_t<decltype(val)>;

                  if constexpr (Opts.write_type_info && !tag_v<T>.empty() && glaze_object_t<V>) {
                     constexpr auto num_members = std::tuple_size_v<meta_t<V>>;

                     // must first write out type
                     if constexpr (Opts.prettify) {
                        dump<"{\n">(args...);
                        ctx.indentation_level += Opts.indentation_width;
                        dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
                        dump<'"'>(args...);
                        dump(tag_v<T>, args...);
                        dump<"\": \"">(args...);
                        dump(ids_v<T>[value.index()], args...);
                        if constexpr (num_members == 0) {
                           dump<"\"\n">(args...);
                        }
                        else {
                           dump<"\",\n">(args...);
                        }
                        dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
                     }
                     else {
                        dump<"{\"">(args...);
                        dump(tag_v<T>, args...);
                        dump<"\":\"">(args...);
                        dump(ids_v<T>[value.index()], args...);
                        if constexpr (num_members == 0) {
                           dump<R"(")">(args...);
                        }
                        else {
                           dump<R"(",)">(args...);
                        }
                     }
                     write<json>::op<opening_handled<Opts>()>(val, ctx, args...);
                  }
                  else {
                     write<json>::op<Opts>(val, ctx, args...);
                  }
               },
               value);
         }
      };

      template <class T>
      struct to_json<array_variant_wrapper<T>>
      {
         template <auto Opts, class... Args>
         GLZ_FLATTEN static void op(auto&& wrapper, is_context auto&& ctx, Args&&... args) noexcept
         {
            auto& value = wrapper.value;
            dump<'['>(args...);
            if constexpr (Opts.prettify) {
               ctx.indentation_level += Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            dump<'"'>(args...);
            dump(ids_v<T>[value.index()], args...);
            dump<"\",">(args...);
            if constexpr (Opts.prettify) {
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            std::visit([&](auto&& v) { write<json>::op<Opts>(v, ctx, args...); }, value);
            if constexpr (Opts.prettify) {
               ctx.indentation_level -= Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            dump<']'>(args...);
         }
      };

      template <class T>
         requires is_specialization_v<T, arr>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_FLATTEN static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            using V = std::decay_t<decltype(value.value)>;
            static constexpr auto N = std::tuple_size_v<V>;

            dump<'['>(args...);
            if constexpr (N > 0 && Opts.prettify) {
               ctx.indentation_level += Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            for_each<N>([&](auto I) {
               if constexpr (glaze_array_t<V>) {
                  write<json>::op<Opts>(get_member(value.value, glz::get<I>(meta_v<T>)), ctx, args...);
               }
               else {
                  write<json>::op<Opts>(glz::get<I>(value.value), ctx, args...);
               }
               constexpr bool needs_comma = I < N - 1;
               if constexpr (needs_comma) {
                  write_entry_separator<Opts>(ctx, args...);
               }
            });
            if constexpr (N > 0 && Opts.prettify) {
               ctx.indentation_level -= Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            dump<']'>(args...);
         }
      };

      template <class T>
         requires glaze_array_t<std::decay_t<T>> || tuple_t<std::decay_t<T>>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_FLATTEN static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            static constexpr auto N = []() constexpr {
               if constexpr (glaze_array_t<std::decay_t<T>>) {
                  return std::tuple_size_v<meta_t<std::decay_t<T>>>;
               }
               else {
                  return std::tuple_size_v<std::decay_t<T>>;
               }
            }();

            dump<'['>(args...);
            if constexpr (N > 0 && Opts.prettify) {
               ctx.indentation_level += Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            using V = std::decay_t<T>;
            for_each<N>([&](auto I) {
               if constexpr (glaze_array_t<V>) {
                  write<json>::op<Opts>(get_member(value, glz::get<I>(meta_v<T>)), ctx, args...);
               }
               else {
                  write<json>::op<Opts>(glz::get<I>(value), ctx, args...);
               }
               constexpr bool needs_comma = I < N - 1;
               if constexpr (needs_comma) {
                  write_entry_separator<Opts>(ctx, args...);
               }
            });
            if constexpr (N > 0 && Opts.prettify) {
               ctx.indentation_level -= Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            dump<']'>(args...);
         }
      };

      template <class T>
      struct to_json<includer<T>>
      {
         template <auto Opts, class... Args>
         GLZ_ALWAYS_INLINE static void op(auto&&, is_context auto&&, Args&&...) noexcept
         {}
      };

      template <class T>
         requires is_std_tuple<std::decay_t<T>>
      struct to_json<T>
      {
         template <auto Opts, class... Args>
         GLZ_FLATTEN static void op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
         {
            static constexpr auto N = []() constexpr {
               if constexpr (glaze_array_t<std::decay_t<T>>) {
                  return std::tuple_size_v<meta_t<std::decay_t<T>>>;
               }
               else {
                  return std::tuple_size_v<std::decay_t<T>>;
               }
            }();

            dump<'['>(args...);
            if constexpr (N > 0 && Opts.prettify) {
               ctx.indentation_level += Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            using V = std::decay_t<T>;
            for_each<N>([&](auto I) {
               if constexpr (glaze_array_t<V>) {
                  write<json>::op<Opts>(value.*std::get<I>(meta_v<V>), ctx, args...);
               }
               else {
                  write<json>::op<Opts>(std::get<I>(value), ctx, args...);
               }
               constexpr bool needs_comma = I < N - 1;
               if constexpr (needs_comma) {
                  write_entry_separator<Opts>(ctx, args...);
               }
            });
            if constexpr (N > 0 && Opts.prettify) {
               ctx.indentation_level -= Opts.indentation_width;
               dump<'\n'>(args...);
               dumpn<Opts.indentation_char>(ctx.indentation_level, args...);
            }
            dump<']'>(args...);
         }
      };

      template <const std::string_view& S>
      GLZ_ALWAYS_INLINE constexpr auto array_from_sv() noexcept
      {
         constexpr auto N = S.size();
         std::array<char, N> arr;
         std::copy_n(S.data(), N, arr.data());
         return arr;
      }

      GLZ_ALWAYS_INLINE constexpr bool needs_escaping(const auto& S) noexcept
      {
         for (const auto& c : S) {
            if (c == '"') {
               return true;
            }
         }
         return false;
      }

      template <class T>
         requires is_specialization_v<T, glz::obj> || is_specialization_v<T, glz::obj_copy>
      struct to_json<T>
      {
         template <auto Options>
         GLZ_FLATTEN static void op(auto&& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
         {
            if constexpr (!Options.opening_handled) {
               dump<'{'>(b, ix);
               if constexpr (Options.prettify) {
                  ctx.indentation_level += Options.indentation_width;
                  dump<'\n'>(b, ix);
                  dumpn<Options.indentation_char>(ctx.indentation_level, b, ix);
               }
            }

            using V = std::decay_t<decltype(value.value)>;
            static constexpr auto N = std::tuple_size_v<V> / 2;

            bool first = true;
            for_each<N>([&](auto I) {
               static constexpr auto Opts = opening_and_closing_handled_off<ws_handled_off<Options>()>();
               decltype(auto) item = glz::get<2 * I + 1>(value.value);
               using val_t = std::decay_t<decltype(item)>;

               if (skip_member<Opts>(item)) {
                  return;
               }

               // skip
               if constexpr (is_includer<val_t> || std::is_same_v<val_t, hidden> || std::same_as<val_t, skip>) {
                  return;
               }
               else {
                  if (first) {
                     first = false;
                  }
                  else {
                     // Null members may be skipped so we cant just write it out for all but the last member unless
                     // trailing commas are allowed
                     write_entry_separator<Opts>(ctx, b, ix);
                  }

                  using Key = typename std::decay_t<std::tuple_element_t<2 * I, V>>;

                  if constexpr (str_t<Key> || char_t<Key>) {
                     const sv key = glz::get<2 * I>(value.value);
                     write<json>::op<Opts>(key, ctx, b, ix);
                     dump<':'>(b, ix);
                     if constexpr (Opts.prettify) {
                        dump<' '>(b, ix);
                     }
                  }
                  else {
                     dump<'"'>(b, ix);
                     write<json>::op<Opts>(item, ctx, b, ix);
                     dump(Opts.prettify ? "\": " : "\":", b, ix);
                  }

                  write<json>::op<Opts>(item, ctx, b, ix);
               }
            });

            if constexpr (!Options.closing_handled) {
               if constexpr (Options.prettify) {
                  ctx.indentation_level -= Options.indentation_width;
                  dump<'\n'>(b, ix);
                  dumpn<Options.indentation_char>(ctx.indentation_level, b, ix);
               }
               dump<'}'>(b, ix);
            }
         }
      };

      template <class T>
         requires is_specialization_v<T, glz::merge>
      struct to_json<T>
      {
         template <auto Options>
         GLZ_FLATTEN static void op(auto&& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
         {
            if constexpr (!Options.opening_handled) {
               dump<'{'>(b, ix);
               if constexpr (Options.prettify) {
                  ctx.indentation_level += Options.indentation_width;
                  dump<'\n'>(b, ix);
                  dumpn<Options.indentation_char>(ctx.indentation_level, b, ix);
               }
            }

            using V = std::decay_t<decltype(value.value)>;
            static constexpr auto N = std::tuple_size_v<V>;

            for_each<N>([&](auto I) {
               write<json>::op<opening_and_closing_handled<Options>()>(glz::get<I>(value.value), ctx, b, ix);
               if constexpr (I < N - 1) {
                  dump<','>(b, ix);
               }
            });

            if constexpr (Options.prettify) {
               ctx.indentation_level -= Options.indentation_width;
               dump<'\n'>(b, ix);
               dumpn<Options.indentation_char>(ctx.indentation_level, b, ix);
            }
            dump<'}'>(b, ix);
         }
      };

      template <class T>
         requires glaze_object_t<T> || reflectable<T>
      struct to_json<T>
      {
         template <auto Options, class V>
         GLZ_FLATTEN static void op(V&& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
         {
            using ValueType = std::decay_t<V>;
            if constexpr (detail::has_unknown_writer<ValueType> && Options.write_unknown) {
               constexpr auto& writer = meta_unknown_write_v<ValueType>;

               using WriterType = meta_unknown_write_t<ValueType>;
               if constexpr (std::is_member_object_pointer_v<WriterType>) {
                  write<json>::op<write_unknown_off<Options>()>(glz::merge{value, value.*writer}, ctx, b, ix);
               }
               else if constexpr (std::is_member_function_pointer_v<WriterType>) {
                  write<json>::op<write_unknown_off<Options>()>(glz::merge{value, (value.*writer)()}, ctx, b, ix);
               }
               else {
                  static_assert(false_v<T>, "unknown_write type not handled");
               }
            }
            else {
               op_base<write_unknown_on<Options>()>(std::forward<V>(value), ctx, b, ix);
            }
         }

         // handles glaze_object_t without extra unknown fields
         template <auto Options>
         GLZ_FLATTEN static void op_base(auto&& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
         {
            if constexpr (!Options.opening_handled) {
               dump<'{'>(b, ix);
               if constexpr (Options.prettify) {
                  ctx.indentation_level += Options.indentation_width;
                  dump<'\n'>(b, ix);
                  dumpn<Options.indentation_char>(ctx.indentation_level, b, ix);
               }
            }

            using Info = object_type_info<Options, T>;

            using V = std::decay_t<T>;
            static constexpr auto N = Info::N;

            [[maybe_unused]] decltype(auto) t = [&] {
               if constexpr (reflectable<T>) {
                  if constexpr (std::is_const_v<std::remove_reference_t<decltype(value)>>) {
#if ((defined _MSC_VER) && (!defined __clang__))
                     static thread_local auto tuple_of_ptrs = make_const_tuple_from_struct<T>();
#else
                     static thread_local constinit auto tuple_of_ptrs = make_const_tuple_from_struct<T>();
#endif
                     populate_tuple_ptr(value, tuple_of_ptrs);
                     return tuple_of_ptrs;
                  }
                  else {
#if ((defined _MSC_VER) && (!defined __clang__))
                     static thread_local auto tuple_of_ptrs = make_tuple_from_struct<T>();
#else
                     static thread_local constinit auto tuple_of_ptrs = make_tuple_from_struct<T>();
#endif
                     populate_tuple_ptr(value, tuple_of_ptrs);
                     return tuple_of_ptrs;
                  }
               }
               else {
                  return nullptr;
               }
            }();

            [[maybe_unused]] bool first = true;
            static constexpr auto first_is_written = Info::first_will_be_written;
            for_each<N>([&](auto I) {
               static constexpr auto Opts = opening_and_closing_handled_off<ws_handled_off<Options>()>();

               using Element = glaze_tuple_element<I, N, T>;
               static constexpr size_t member_index = Element::member_index;
               static constexpr bool use_reflection = Element::use_reflection;
               using val_t = typename Element::type;

               decltype(auto) member = [&] {
                  if constexpr (reflectable<T>) {
                     return std::get<I>(t);
                  }
                  else {
                     return get<member_index>(get<I>(meta_v<V>));
                  }
               }();

               if constexpr (null_t<val_t> && Opts.skip_null_members) {
                  if constexpr (always_null_t<T>)
                     return;
                  else {
                     auto is_null = [&]() {
                        if constexpr (raw_nullable<val_t>) {
                           return !bool(member(value).val);
                        }
                        else {
                           return !bool(get_member(value, member));
                        }
                     }();
                     if (is_null) return;
                  }
               }

               // skip file_include
               if constexpr (is_includer<val_t>) {
                  return;
               }
               else if constexpr (std::is_same_v<val_t, hidden> || std::same_as<val_t, skip>) {
                  return;
               }
               else {
                  if constexpr (first_is_written && I > 0) {
                     write_entry_separator<Opts>(ctx, b, ix);
                  }
                  else {
                     if (first) {
                        first = false;
                     }
                     else {
                        // Null members may be skipped so we cant just write it out for all but the last member unless
                        // trailing commas are allowed
                        write_entry_separator<Opts>(ctx, b, ix);
                     }
                  }

                  static constexpr sv key = key_name<I, T, use_reflection>;
                  if constexpr (needs_escaping(key)) {
                     write<json>::op<Opts>(key, ctx, b, ix);
                     dump<':'>(b, ix);
                     if constexpr (Opts.prettify) {
                        dump<' '>(b, ix);
                     }
                  }
                  else {
                     static constexpr auto quoted_key = join_v < chars<"\"">, key,
                                           Opts.prettify ? chars<"\": "> : chars < "\":" >>
                        ;
                     dump<quoted_key>(b, ix);
                  }

                  write<json>::op<Opts>(get_member(value, member), ctx, b, ix);

                  static constexpr size_t comment_index = member_index + 1;
                  static constexpr auto S = std::tuple_size_v<typename Element::Item>;
                  if constexpr (Opts.comments && S > comment_index) {
                     static constexpr auto i = glz::get<I>(meta_v<V>);
                     if constexpr (std::is_convertible_v<decltype(get<comment_index>(i)), sv>) {
                        static constexpr sv comment = get<comment_index>(i);
                        if constexpr (comment.size() > 0) {
                           if constexpr (Opts.prettify) {
                              dump<' '>(b, ix);
                           }
                           dump<"/*">(b, ix);
                           dump(comment, b, ix);
                           dump<"*/">(b, ix);
                        }
                     }
                  }
               }
            });
            if constexpr (!Options.closing_handled) {
               if constexpr (Options.prettify) {
                  ctx.indentation_level -= Options.indentation_width;
                  dump<'\n'>(b, ix);
                  dumpn<Options.indentation_char>(ctx.indentation_level, b, ix);
               }
               dump<'}'>(b, ix);
            }
         }
      };
   } // namespace detail

   template <class T, class Buffer>
   [[nodiscard]] inline auto write_json(T&& value, Buffer&& buffer) noexcept
   {
      return write<opts{}>(std::forward<T>(value), std::forward<Buffer>(buffer));
   }

   template <class T>
   [[nodiscard]] inline auto write_json(T&& value) noexcept
   {
      std::string buffer{};
      write<opts{}>(std::forward<T>(value), buffer);
      return buffer;
   }

   template <class T, class Buffer>
   inline void write_jsonc(T&& value, Buffer&& buffer) noexcept
   {
      write<opts{.comments = true}>(std::forward<T>(value), std::forward<Buffer>(buffer));
   }

   template <class T>
   [[nodiscard]] inline auto write_jsonc(T&& value) noexcept
   {
      std::string buffer{};
      write<opts{.comments = true}>(std::forward<T>(value), buffer);
      return buffer;
   }

   template <opts Opts = opts{}, class T>
   [[nodiscard]] inline write_error write_file_json(T&& value, const std::string& file_name, auto&& buffer) noexcept
   {
      write<set_json<Opts>()>(std::forward<T>(value), buffer);
      return {buffer_to_file(buffer, file_name)};
   }
}
