//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP



// TODO check if I really need all of these:
#include "fiction/layouts/bounding_box.hpp"
#include "fiction/traits.hpp"
#include "fiction/types.hpp"
#include "fiction/utils/name_utils.hpp"
#include "fiction/utils/placement_utils.hpp"
#include "fiction/layouts/clocking_scheme.hpp"

#include <mockturtle/traits.hpp>
#include <mockturtle/views/topo_view.hpp>

#include <limits>
#include <cstdint>
#include <array>
#include <cmath>

namespace fiction
{
// TODO after checking all -Wconversion errors and beeing finished, I should move the diagnostics-compiler info to the top and bottom of the file, so I don't have to repeat it everywhere I need it
// TODO inline and constexpr and noexcept should be added everywhere they are needed
namespace detail
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion" 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
/**
 * A modulo operation that will return the least positive residue instead of the remainder of the division.
 * 
 * `a mod b = c` <=> `c = mod(a,b)`
 * 
 * @param a number to be "divided"
 * @param b "dividend"
 * @return least positive residue 
 */
inline static constexpr uint8_t mod(uint8_t a, uint8_t b) noexcept
{
    int8_t remainder = a % b;
    return remainder >= 0 ? remainder : remainder + b;
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

/**
 * Utility function to translate the original hex coodrinates into the new supertile hex coordinates.
 * Offset will simple be added to the coordinates.
 * 
 * @param tile original position
 * @param x_offset offset that is added to the x coordinate
 * @param y_offset offset that is added to the y coordinate
 * @return coodrinates which are translated into the supertile hex layout
 */
template <typename HexLyt>
tile<HexLyt> super(tile<HexLyt> original_tile, int64_t x_offset, int64_t y_offset) noexcept
{
    // position a tile at 0,0 would have in the supertile hex layout
    int64_t new_x = 1 + x_offset;
    int64_t new_y = 1 + y_offset;

    // calculate column base position
    new_x += ((static_cast<int64_t>(original_tile.x) >> 1) * 5 ) + (static_cast<int64_t>(original_tile.x) % 2 == 0 ? 0 : 2);
    new_y += static_cast<int64_t>(original_tile.x);

    // calculate position by traversing column
        // rough traversion (the pattern for addition repeates every 4 steps and can be summed up)
        new_x += (static_cast<int64_t>(original_tile.y) >> 2) * -3;
        new_y += (static_cast<int64_t>(original_tile.y) >> 2) * 10;
        
        // fine traversion (traverse the 0 - 3 steps that are left)
        if (static_cast<int64_t>(original_tile.x) % 2 == 0)
        {
            switch (static_cast<int64_t>(original_tile.y) % 4)
            {
                default:
                case 0:
                    break;
                case 1:
                    new_x += -2;
                    new_y += 2;
                    break;
                case 2:
                    new_x += -2;
                    new_y += 5;
                    break;
                case 3:
                    new_x += -4;
                    new_y += 7;
                    break;
            }
        } else {
            switch (static_cast<int64_t>(original_tile.y) % 4)
            {
                default:
                case 0:
                    break;
                case 1:
                    new_x += -2;
                    new_y += 2;
                    break;
                case 2:
                    new_x += -1;
                    new_y += 5;
                    break;
                case 3:
                    new_x += -3;
                    new_y += 7;
                    break;
            }
        }
    
    return tile<HexLyt>{new_x, new_y, original_tile.z};
}

/**
 * Utility function to translate the original hex coodrinates into the new supertile hex coordinates.
 * If one already knows the target coordinates would be negative or if they should be moved closer to the border of the layout,
 * the super() method with offsets should be used.
 * 
 * @param tile original position
 * @return coodrinates which are translated into the supertile hex layout
 */
template <typename HexLyt>
inline tile<HexLyt> super(tile<HexLyt> tile)
{
    return super(tile, 0, 0);
}

//TODO remove and also remove the methods I created for it in the other file
/**
 * Utility function to copy the translation of a tile that was adjacent to a translated tile.
 * If the target tile was not adjacent no translation will be applied an the tile itself will be returned.
 * 
 * @param target_old tile that should copy the shift from the source
 * @param source_old old position of the source
 * @param source_super new position of the source that has been translated to the super clocking sceme
 * @param lyt hexagonal layout that houses the tiles
 * @return new position of the target that kept the same relative position to the source tile 
 */
template <typename HexLyt>
tile<HexLyt> copy_super_translation(tile<HexLyt> target_old, tile<HexLyt> source_old, tile<HexLyt> source_super, HexLyt lyt) noexcept
{
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");

    if (lyt.is_north_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.north(source_super));
    }
    if (lyt.is_north_east_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.north_east(source_super));
    }
    if (lyt.is_east_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.east(source_super));
    }
    if (lyt.is_south_east_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.south_east(source_super));
    }
    if (lyt.is_south_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.south(source_super));
    }
    if (lyt.is_south_west_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.south_west(source_super));
    }
    if (lyt.is_west_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.west(source_super));
    }
    if (lyt.is_north_west_of(source_old, target_old))
    {
        return static_cast<tile<HexLyt>>(lyt.north_west(source_super));
    }

    return target_old;
}

/**
 * Utility function to inflate a hexagonal layout such that each tile is now encased by six new empty tiles.
 * Hexagonal layouts dimensions can't be bigger then "std::numeric_limits<int64_t>::max()".
 * TODO inputs und outputs beschreibung schreiben
 */
template <typename HexLyt>
[[nodiscard]] HexLyt grow_to_supertiles(const HexLyt& lyt) noexcept
{
    using namespace fiction;

    //TODO have all the required static_asserts here
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");

    //TODO have all the required runtime asserts here
    assert(lyt.x() <= std::numeric_limits<int64_t>::max()); // reason is that coordinates will be cast from uint64_t to int64_t
    assert(lyt.y() <= std::numeric_limits<int64_t>::max());
    assert(lyt.is_clocking_scheme(clock_name::AMY));

    // Search trough hexagonal layout to find the outermost, non-empty tiles (after translation into supertile-hexagonal layout)
    int64_t leftmost_core_tile_x = std::numeric_limits<int64_t>::max();
    int64_t rightmost_core_tile_x = std::numeric_limits<int64_t>::min();
    int64_t top_core_tile_y = std::numeric_limits<int64_t>::max();
    int64_t bottom_core_tile_y = std::numeric_limits<int64_t>::min();
    
    lyt.foreach_node( //TODO maybe foreach_gate instead?
        [&lyt, &leftmost_core_tile_x, &rightmost_core_tile_x, &top_core_tile_y, &bottom_core_tile_y](const auto& node)
        {
            const auto tile = lyt.get_tile(node);

                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wsign-conversion" // has been checked at the beginning of this method
            const auto pos = super<HexLyt>(tile);
                #pragma GCC diagnostic pop

            if (pos.x < leftmost_core_tile_x)
                {leftmost_core_tile_x = pos.x;}
            if (pos.x > rightmost_core_tile_x)
                {rightmost_core_tile_x = pos.x;}
            if (pos.y < top_core_tile_y)
                {top_core_tile_y = pos.y;}
            if (pos.y > bottom_core_tile_y)
                {bottom_core_tile_y = pos.y;}
        });

    if (leftmost_core_tile_x > rightmost_core_tile_x)
    {
        std::cout << "[w] didn't find any nodes in layout" << std::endl;
        return lyt;
    }

    /*
        * calculate the offset that
        * A) keeps every supertile (so core and wires) in positive coordinates
        * B) doesn't wastes to much space (meaning the coordinates are as small as possible)
        * C) keeps the clocking such that the top left clock zone from the original is the same as the top left super clock zone
        */
        // clang-format off

        static constexpr std::array<std::array<int8_t, 2u>, 56u> even_coord_slice{{
                {{0,0}},{{1,0}},
                {{-1,10}},{{0,10}},{{1,10}},{{2,10}},{{3,10}},{{4,10}},{{5,10}},{{6,10}},{{7,10}},
                {{-2,6}},{{-1,6}},{{0,6}},{{1,6}},{{2,6}},{{3,6}},{{4,6}},{{5,6}},{{6,6}},{{7,6}},
                {{-2,2}},{{-1,2}},{{0,2}},{{1,2}},{{2,2}},{{3,2}},{{4,2}},{{5,2}},{{6,2}},
                {{4,12}},{{4,13}},
                {{-4,8}},{{-3,8}},{{-2,8}},{{-1,8}},{{0,8}},{{1,8}},{{2,8}},{{3,8}},{{4,8}},{{5,8}},{{6,8}},{{7,8}},
                {{-2,4}},{{-1,4}},{{0,4}},{{1,4}},{{2,4}},{{3,4}},{{4,4}},{{5,4}},{{6,4}},{{7,4}},{{8,4}},{{9,4}}
                }};

        static constexpr std::array<std::array<int8_t, 2u>, 56u> odd_coord_slice{{
                {{0,1}},{{1,1}},{{2,1}},{{3,1}},{{4,1}},
                {{2,11}},{{3,11}},{{4,11}},{{5,11}},{{6,11}},
                {{-3,7}},{{-2,7}},{{-1,7}},{{0,7}},{{1,7}},{{2,7}},{{3,7}},{{4,7}},{{5,7}},{{6,7}},{{7,7}},
                {{-2,3}},{{-1,3}},{{0,3}},{{1,3}},{{2,3}},{{3,3}},{{4,3}},{{5,3}},{{6,3}},{{7,3}},{{8,3}},{{9,3}},
                {{-3,9}},{{-2,9}},{{-1,9}},{{0,9}},{{1,9}},{{2,9}},{{3,9}},{{4,9}},{{5,9}},{{6,9}},{{7,9}},{{8,9}},
                {{-1,5}},{{0,5}},{{1,5}},{{2,5}},{{3,5}},{{4,5}},{{5,5}},{{6,5}},{{7,5}},{{8,5}},{{9,5}}
                }};

        // clang-format on

        // get the top left corner position, of the rectangle encapsulating all core tiles, relative to the 4x4 super clock zone group it is in
        std::array<int8_t, 2u> relative_position = fiction::super_4x4_group_lookup<std::array<int8_t, 2u>>(leftmost_core_tile_x, top_core_tile_y, even_coord_slice, odd_coord_slice);
        
        int8_t relative_x = relative_position[0];
        int8_t relative_y = relative_position[1];

        // check if offset needs to be moved to ...
        if (relative_x < 1/*1 instead of 0 to include future surrounding wires*/) // ... the next right super clock zone group (to stay in the positive coordinates)
        {
            relative_x += 10;
            relative_y += 4;
        }
        if (relative_y > 10/*10 instead of 9 to include future surrounding wires*/) // ... the next upper super clock zone group (to safe some space)
        {
            relative_x += 3;
            relative_y -= 10;
        }

        // calculate offset for translated tiles
        int64_t x_offset = static_cast<int64_t>(relative_x) - leftmost_core_tile_x;
        int64_t y_offset = static_cast<int64_t>(relative_y) - top_core_tile_y;

    // generate new hexagonal layout
    uint64_t size_x = rightmost_core_tile_x + 1 + x_offset;
    uint64_t size_y = bottom_core_tile_y + 1 + y_offset;

    HexLyt super_lyt{{size_x, size_y, lyt.z()}, fiction::amy_supertile_clocking<HexLyt>(), lyt.get_layout_name()};

    // copy tiles to new layout
        //inputs
        lyt.foreach_pi(
            [&lyt, &super_lyt, x_offset, y_offset](const auto& node)
            {
                const tile<HexLyt> old_tile = lyt.get_tile(node);
                const tile<HexLyt> super_tile = super<HexLyt>(old_tile, x_offset, y_offset);
                super_lyt.create_pi(lyt.get_name(lyt.get_node(old_tile)), super_tile);
            });

        // normal gates/wires
        lyt.foreach_node(
            [&lyt, &super_lyt, x_offset, y_offset](const auto& node)
            {
                if (lyt.is_pi(node))
                {
                    return;
                }

                const tile<HexLyt> old_tile = lyt.get_tile(node);
                const tile<HexLyt> super_tile = super<HexLyt>(old_tile, x_offset, y_offset);

                if (const auto signals = lyt.incoming_data_flow(old_tile); signals.size() == 1) // FORME node is wire, fanout or inverter
                {
                    const auto old_signal = signals[0];
                    const tile<HexLyt> super_signal = super_lyt.make_signal(super_lyt.get_node(copy_super_translation<HexLyt>(static_cast<tile<HexLyt>>(old_signal), old_tile, super_tile, lyt)));

                    if (!lyt.is_po(node) and lyt.is_wire(node)) //TODO move is_po to check 10 lines above so it abortes earlier?
                    {
                        super_lyt.create_buf(super_signal, super_tile);
                    } 
                    else if (lyt.is_inv(node))
                    {
                        super_lyt.create_not(super_signal, super_tile);
                    }
                }
                else if (signals.size() = 2)
                {
                    const auto signal_a = signals[0];
                    const auto signal_b = signals[0];

                    const tile<HexLyt> super_signal_a = super_lyt.make_signal(super_lyt.get_node(copy_super_translation<HexLyt>(static_cast<tile<HexLyt>>(signal_a), old_tile, super_tile, lyt)));
                    const tile<HexLyt> super_signal_b = super_lyt.make_signal(super_lyt.get_node(copy_super_translation<HexLyt>(static_cast<tile<HexLyt>>(signal_b), old_tile, super_tile, lyt)));

                    if (lyt.is_and(node))
                    {
                        super_lyt.create_and(super_signal_a, super_signal_b, super_tile);
                    }
                    else if (lyt.is_nand(node))
                    {
                        super_lyt.create_nand(super_signal_a, super_signal_b, super_tile);
                    }
                    else if (lyt.is_or(node))
                    {
                        super_lyt.create_or(super_signal_a, super_signal_b, super_tile);
                    }
                    else if (lyt.is_nor(node))
                    {
                        super_lyt.create_nor(super_signal_a, super_signal_b, super_tile);
                    }
                    else if (lyt.is_xor(node))
                    {
                        super_lyt.create_xor(super_signal_a, super_signal_b, super_tile);
                    }
                    else if (lyt.is_xnor(node))
                    {
                        super_lyt.create_xnor(super_signal_a, super_signal_b, super_tile);
                    }
                    else if (lyt.is_function(node))
                    {
                        const auto node_fun = lyt.node_function(node);

                        super_lyt.create_node({super_signal_a, super_signal_b}, node_fun, super_tile);
                    }
                }
            });

        //ouputs
        lyt.foreach_po(
            [&lyt, &super_lyt, x_offset, y_offset](const auto& signal)
            {
                const tile<HexLyt> old_tile = lyt.get_tile(lyt.get_node(signal));
                const tile<HexLyt> super_tile = super<HexLyt>(old_tile, x_offset, y_offset);

                const auto old_signal = lyt.incoming_data_flow(old_tile)[0];
                const tile<HexLyt> super_signal = super_lyt.make_signal(super_lyt.get_node(copy_super_translation<HexLyt>(static_cast<tile<HexLyt>>(old_signal), old_tile, super_tile, lyt)));
                super_lyt.create_po(super_signal, lyt.get_name(lyt.get_node(old_tile)), super_tile);

            });

    //TODO maybe I can use "restore_names<CartLyt, HexLyt>(lyt, super_lyt);"
    return super_lyt;
    /**
     * FORME: Methods that could be usefull:
     * - "ground_coordinates" for iteration over a range of hex tiles (has default mode for whole matrix)
     * - struct coord_t 
     * - connect() (for connecting signals)
     */
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

//TODO description (mention that it's perfectly space efficient (no empty or double spaces)) and ignores input order of a and b
uint8_t perfectHashFunction21(uint8_t A, uint8_t B, uint8_t C)
{
    uint8_t b = mod((B - A), 6);
    uint8_t c = mod((C - A), 6);
    if ((b + c) == 9)
    {
        return 10*A + (12 - 5);
    }
    else
    {
        return 10*A + ((2*(b + c) - abs(A -B)) - 5);
    }
}

//TODO description (mention that it's perfectly space efficient (no empty or double spaces)) and respects input order
uint8_t perfectHashFunction11(uint8_t A, uint8_t B)
{
    uint8_t base = A > B ? 15 : 0;
    if ((A * B) == 2)
    {
        return 12 + base;
    }
    else if ((A + B) == 9)
    {
        return base;
    }
    else
    {
        return 2*(A + B) - abs(A - B) + base;
    }
}

#pragma GCC diagnostic pop

//static constexpr const 

/**
 * Utility function to find a center tile orientation and according wires in the outer tiles
 * that represent the functionality and connection points of the original center tile, but the new tiles
 * all have existing SiDB implementations.
 * TODO inputs and outputs angeben
 * TODO mein github hier verlinken um zu dokumentieren wo die layouts her kommen
 */
template <typename HexLyt>
[[nodiscard]] HexLyt supertile_core_and_wire_generation(const HexLyt& lyt) noexcept
{
    assert(lyt.is_clocking_scheme(clock_name::AMY_SUPER)); // To catch if grow_to_supertiles() didn't work
    assert(lyt.z() == 0); // Method doesn't handle wire crossings yet

    /**
     * FORME: Methods that could be usefull:
     * - "ground_coordinates" for iteration over a range of hex tiles (has default mode for whole matrix)
     * - struct coord_t 
     * - connect() (for connecting signals) (can also use move() without actually moving to update the children)
     */

    //TODO have all the required static_assters here that I need
    //FORME: Don't forget the hash method(s) I crafted
    //TODO need to handle fanouts!
    //TODO need to build new lookup table for inverters, (so 1 in 1 out but not all directions are available)
    //TODO need to build new lookup table for primary input inverters (so just the straight ones, I can do that by hand)
    //FORME: inputs können wohl auch inverter sein, 
    return lyt;//TODO placeholder
}

    //TODO: (optional) write method for wire optimisation and call it in the right places
}

template <typename HexLyt>
[[nodiscard]] HexLyt supertilezation(const HexLyt& lyt) noexcept
{
    using namespace fiction;

    static_assert(is_gate_level_layout_v<HexLyt>, "HexLyt is not a gate-level layout");
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");
    static_assert(has_even_row_hex_arrangement_v<HexLyt>, "HexLyt does not have an even row hexagon arrangement");

    return detail::supertile_core_and_wire_generation<HexLyt>(detail::grow_to_supertiles<HexLyt>(lyt));
    //TODO make sure to catch every exception (mabye use "catch (...)" if I don't know what kind of errors there are) or maybe I should move thos to the CLI where it is actually executed ?!
}

}

#endif  // FICTION_SUPERTILE_HPP