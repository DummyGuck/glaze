#include <map>
#include <concepts>

#include <QString>
#include <QStringView>

#include <glaze/api/name.hpp>
#include <glaze/core/common.hpp>

template<typename T>
concept qstring_type = std::same_as<T, QString>;

template <class T> 
    requires qstring_type<T>
struct glz::meta<T>
{
   static constexpr auto custom_write = true;
   static constexpr auto custom_read = true;
};

namespace glz::detail {
   

template <class T>
    requires qstring_type<T>
struct to_json<T>
{
    template <auto Opts, class B>
    GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&&, B&& b, auto&& ix) noexcept {
        if constexpr (Opts.number) {
            // TODO: Should we check if the string number is valid?
            dump(value, b, ix);
        }
        else {
            if constexpr (Opts.raw_string) {
                const QStringView str = [&]() -> QStringView { return value; }();

                // We need at space for quotes and the string length: 2 + n.
                if constexpr (resizeable<B>) {
                    const auto n = str.size();
                    const auto k = ix + 2 + n;
                    if (k >= b.size()) [[unlikely]] {
                    b.resize((std::max)(b.size() * 4, k));
                    }
                }
                // now we don't have to check writing

                dump_unchecked<'"'>(b, ix);
                dump_unchecked(str, b, ix);
                dump_unchecked<'"'>(b, ix);
            }
            else {
                const QStringView str = [&]() -> QStringView { return value; }();
                const auto n = str.size() * 2;

                if constexpr (resizeable<B>) {
                    const auto k = ix + 10 + 2 * n;
                    if (k >= b.size()) [[unlikely]] {
                    b.resize((std::max)(b.size() * 2, k));
                    }
                }

                if constexpr (Opts.raw) {
                    dump(str, b, ix);
                }
                else {
                    dump_unchecked<'"'>(b, ix);
                    {
                    const auto* c = str.data();
                    const auto* const e = c + n;

                    if (str.size() > 7) {
                        for (const auto end_m7 = e - 7; c < end_m7;) {
                            std::memcpy(data_ptr(b) + ix, c, 16);
                            uint64_t chunk;
                            std::memcpy(&chunk, c, 16);
                            const uint64_t test_chars = has_quote(chunk) | has_escape(chunk) | is_less_16(chunk);
                            if (test_chars) {
                                const auto length = (std::countr_zero(test_chars) >> 3);
                                c += length;
                                ix += length;

                                if (const auto escaped = char_escape_table[(*c).unicode()]; escaped) [[likely]] {
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
                        if (const auto escaped = char_escape_table[(*c).unicode()]; escaped) {
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

template <class T> 
    requires qstring_type<T>
struct from_json<T> {
    template <glz::opts Opts, class... Args>
    GLZ_ALWAYS_INLINE static void op(auto& value, is_context auto&& ctx, Args&&... args) noexcept
    {
        std::string str;
        read<Opts.format>::template op<Opts>(str, ctx, args...);
        value = QString::fromStdString(std::move(str));
    }
};

}