#include "pixie/runtime/ComponentTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>

#if PIXIE_DATA_CASE == 1
union Data { int value; double other; };
#elif PIXIE_DATA_CASE == 2
struct Data { int value; Data() : value(7) {} };
#elif PIXIE_DATA_CASE == 3
struct Data { int value; ~Data() {} };
#elif PIXIE_DATA_CASE == 4
struct Data { Data() = delete; int value; };
#elif PIXIE_DATA_CASE == 7
struct Data { entt::entity value; };
#elif PIXIE_DATA_CASE == 8
struct Data { std::uint64_t value; };
#elif PIXIE_DATA_CASE == 9
struct Data { std::int64_t value; };
#elif PIXIE_DATA_CASE == 10
struct Data { std::string value; };
#elif PIXIE_DATA_CASE == 11
struct Data { std::vector<pxPoint> value; };
#elif PIXIE_DATA_CASE == 12
struct Data : pixie::Behaviour { int value; };
#elif PIXIE_DATA_CASE == 0
enum class Mode : std::uint8_t { First, Second };
struct Data { int value; unsigned limit; bool flag; float ratio; double amount; Mode mode; };
#else
struct Data { int value; };
#endif

int main() {
    pixie::ComponentTypes types;
#if PIXIE_DATA_CASE == 5
    pixie::ComponentBuilder<Data> forged(types, 0);
    (void)forged;
#elif PIXIE_DATA_CASE == 6
    pixie::DataComponentBuilder<Data> forged(types, 0);
    (void)forged;
#else
    auto builder = types.AddData<Data>("Data").Field<&Data::value>("value");
#if PIXIE_DATA_CASE == 0
    builder.Field<&Data::limit>("limit").Field<&Data::flag>("flag").Field<&Data::ratio>("ratio")
        .Field<&Data::amount>("amount").Field<&Data::mode>("mode", {"First", "Second"});
#else
    (void)builder;
#endif
#endif
    return 0;
}
