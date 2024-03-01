#include <map>
#include <concepts>

#include <QMap>

#include <glaze/api/name.hpp>

template<typename T>
concept qmap_type = std::same_as<T, QMap<typename T::key_type, typename T::mapped_type>>;

template <class T> 
    requires qmap_type<T>
struct glz::meta<T>
{
   static constexpr auto custom_write = true;
   static constexpr auto custom_read = true;
};

namespace glz::detail {
    template <class T> 
        requires qmap_type<T>
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
                    if (skip_member<Opts>(it.value())) {
                        return true;
                    }
                    write_pair_content<Opts>(it.key(), it.value(), ctx, args...);
                    return false;
                };

                auto it = value.begin();
                [[maybe_unused]] bool starting = write_first_entry(it);
                for (++it; it != value.end(); ++it) {
                    if (skip_member<Opts>(it.value())) {
                        continue;
                    }

                    if constexpr (Opts.skip_null_members) {
                        if (!starting) {
                            write_entry_separator<Opts>(ctx, args...);
                        }
                    }
                    else {
                        write_entry_separator<Opts>(ctx, args...);
                    }

                    write_pair_content<Opts>(it.key(), it.value(), ctx, args...);
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

    template <class T> 
        requires qmap_type<T>
    struct from_json<T> {
        template <glz::opts Opts, class... Args>
        GLZ_ALWAYS_INLINE static void op(auto& value, is_context auto&& ctx, Args&&... args) noexcept
        {
            std::map<typename T::key_type, typename T::mapped_type> map;
            read<Opts.format>::template op<Opts>(map, ctx, args...);
            value = QMap<typename T::key_type, typename T::mapped_type>{std::move(map)};
        }
    };

    template <class T>
        requires qmap_type<T>
    struct to_binary<T> final
    {
        template <auto Opts, class... Args>
        GLZ_FLATTEN static auto op(auto&& value, is_context auto&& ctx, Args&&... args) noexcept
        {
            using Key = typename T::key_type;

            constexpr uint8_t type = str_t<Key> ? 0 : (std::is_signed_v<Key> ? 0b000'01'000 : 0b000'10'000);
            constexpr uint8_t byte_cnt = str_t<Key> ? 0 : byte_count<Key>;
            constexpr uint8_t tag = tag::object | type | (byte_cnt << 5);
            dump_type(tag, args...);

            dump_compressed_int<Opts>(value.size(), args...);
            for (auto&& it = value.keyValueBegin(); it != value.keyValueEnd(); ++it ) {
                write<binary>::no_header<Opts>(it->first, ctx, args...);
                write<binary>::op<Opts>(it->second, ctx, args...);
            }
        }
    };

    template <class T> 
        requires qmap_type<T>
    struct from_binary<T> {
        template <glz::opts Opts, class... Args>
        GLZ_ALWAYS_INLINE static void op(auto& value, is_context auto&& ctx, Args&&... args) noexcept
        {
            std::map<typename T::key_type, typename T::mapped_type> map;
            read<Opts.format>::template op<Opts>(map, ctx, args...);
            value = QMap<typename T::key_type, typename T::mapped_type>{std::move(map)};
        }
    };
}
