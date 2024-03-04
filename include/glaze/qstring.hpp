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
    GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, B&& b, auto&& ix) noexcept {
        /*
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
                    b.resize((std::max)(static_cast<const long long>(b.size()* 2) , k));
                    }
                }
                // now we don't have to check writing

                dump_unchecked<'"'>(b, ix);
                dump_unchecked(str, b, ix);
                dump_unchecked<'"'>(b, ix);
            }
            else {
                const QStringView str = [&]() -> QStringView { return value; }();
                const auto n = str.size();

                if constexpr (resizeable<B>) {
                    const auto k = ix + 10 + 2 * n;
                    if (k >= b.size()) [[unlikely]] {
                    b.resize((std::max)(static_cast<const long long unsigned>(b.size() * 2), k));
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
                            std::memcpy(data_ptr(b) + ix, c, 8);
                            uint64_t chunk;
                            std::memcpy(&chunk, c, 8);
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
        */
        std::string str{std::move(value.toStdString())};
        write<Opts.format>::template op<Opts>(str, ctx, b, ix);
        value = std::move(QString::fromStdString(str));
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

template <class T>
        requires qstring_type<T>
struct to_binary<T> final
{
    template <auto Opts>
    GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        constexpr uint8_t tag = tag::string;

        dump_type(tag, b, ix);
        const auto n = value.size() * 2;
        dump_compressed_int<Opts>(n, b, ix);

        if (ix + n > b.size()) [[unlikely]] {
            b.resize((std::max)(b.size() * 2, ix + n));
        }

        std::memcpy(b.data() + ix, value.data(), n);
        ix += n;
    }
};

template <class T>
        requires qstring_type<T>
struct from_binary<T> final
{
    template <auto Opts>
    GLZ_ALWAYS_INLINE static void op(auto&& value, is_context auto&& ctx, auto&& it, auto&& end) noexcept
    {
        /*
        if constexpr (Opts.no_header) {
            const auto n = int_from_compressed(it, end);
            value.resize(n);
            std::memcpy(value.data(), &(*it), n);
            std::advance(it, n);
        }
        else {
            constexpr uint8_t header = tag::string;

            const auto tag = uint8_t(*it);
            if (tag != header) [[unlikely]] {
                ctx.error = error_code::syntax_error;
                return;
            }

            ++it;

            const auto n = int_from_compressed(it, end);
            value.resize(n);
            std::memcpy(value.data(), &(*it), n);
            std::advance(it, n);
        }
        */
        std::string str;
        read<Opts.format>::template op<Opts>(str, ctx, it, end);
        value = QString::fromStdString(std::move(str));
    }
};
}