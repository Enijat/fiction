//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP

#include "fiction/traits.hpp"
//#include "fiction/layouts/gate_level_layout.hpp"

#include <limits>
#include <cstdint>
#include <array>
#include <cmath>

namespace fiction
{

/**
 * Utility function that allows to look up values which are repeated endlessly, and are based on a 4x4 group of supertiles,
 * that are arranged as shown in the bachelor thesis "Super-Tile Routing for Omnidirectional Information Flow in Silicon Dangling Bond Logic" by F. Kiefhaber, 2025
 * 
 * @param x x coordinate of the looked up position
 * @param y y coordinate of the looked up position
 * @param even_slice lookup array for rows with an even reduced_y coodrinate
 * @param odd_slice lookup array for rows with an odd reduced_y coordinate
 * @return looked up value from provided arrays
 */
template <typename ArrayType>
const ArrayType super_4x4_group_lookup(uint64_t x, uint64_t y, const std::array<ArrayType, 56u>& even_slice, const std::array<ArrayType, 56u>& odd_slice) noexcept
{
    // reduce to repeating block coordinates
    uint8_t reduced_x = x % 56;
    uint8_t reduced_y = y % 112;

    // translate into top row coordinates
    int8_t mod = (reduced_x - 10 * static_cast<uint8_t>(reduced_y / 4)) % 56;
    reduced_x = mod >= 0 ? mod : mod + 56;
    reduced_y = reduced_y % 4;

    // look up by traversing one super tile clocking block
    switch (reduced_y)
    {
        case 0:
            return even_slice[reduced_x];
        case 1:
            return odd_slice[reduced_x];
        case 2:
            return even_slice[(reduced_x + 23/*to compensate the shift in the array*/) % 56];
        case 3:
            return odd_slice[(reduced_x + 23/*to compensate the shift in the array*/) % 56];
    }
}

//FRAGE: brauch/soll ich das statistics struct auch? (ist in hexagonalisation an dieser stelle drin)
// TODO inline and constexpr and noexcept should be added everywhere they are needed
namespace detail
{
    struct position
    {
        int64_t x;
        int64_t y;
        // TODO z?

        position(int64_t x_new, int64_t y_new)
        {
            x = x_new;
            y = y_new;
        }
    };

    /**
     * Utility function to translate the original hex coodrinates into the new supertile hex coordinates.
     * Offset will simple be added to the coordinates.
     * 
     * @param x x position
     * @param y y postion
     * @param x_offset offset that is added to the x coordinate
     * @param y_offset offset that is added to the y coordinate
     * @return coodrinates which are translated into the supertile hex layout
     */
    position super(int64_t x, int64_t y, int64_t x_offset, int64_t y_offset) noexcept
    {
        // position a tile at 0,0 would have in the supertile hex layout
        int64_t new_x = 1 + x_offset;
        int64_t new_y = 1 + y_offset;

        // calculate column base position
        new_x += ((x >> 1) * 5 ) + (x % 2 == 0 ? 0 : 2);
        new_y += x;

        // calculate position by traversing column
            // rough traversion (the pattern for addition repeates every 4 steps and can be summed up)
            new_x += (y >> 2) * -3;
            new_y += (y >> 2) * 10;
            
            // fine traversion (traverse the 0 - 3 steps that are left)
            if (x % 2 == 0)
            {
                switch (y % 4)
                {
                    default: // also case 0
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
                switch (y % 4)
                {
                    default: // also case 0
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

        return position(new_x, new_y);
    }

    /**
     * Utility function to translate the original hex coodrinates into the new supertile hex coordinates.
     * If one already knows the target coordinates would be negative or if they should be moved closer to the border of the layout,
     * the super() method with offsets should be used.
     * 
     * @param x x position
     * @param y y postion
     * @return coodrinates which are translated into the supertile hex layout
     */
    inline position super(int64_t x, int64_t y) noexcept
    {
        return super(x, y, 0, 0);
    }

    /**
     * Utility function to inflate a hexagonal layout such that each tile is now encased by six new empty tiles.
     * Hexagonal layouts dimensions can't be bigger then "std::numeric_limits<int64_t>::max()".
     * TODO inputs und outputs beschreibung schreiben
     */
    template <typename HexLyt>
    [[nodiscrad]] HexLyt grow_to_supertiles(const HexLyt& lyt) noexcept
    {
        using namespace fiction;

        //TODO have all the required static_asserts here
        static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");

        //TODO have all the required runtime asserts here
        assert(lyt.x() <= std::numeric_limits<int64_t>::max()); // reason is that coordinates will be cast from uint64_t to int64_t
        assert(lyt.y() <= std::numeric_limits<int64_t>::max());
        assert(lyt.z() == 0); //FRAGE: how does z height work with hexagonal layouts / SiDB gates

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
                position pos = super(tile.x, tile.y);
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

        if (leftmost_core_tile_x > rightmost_core_tile_x) // There was not a single node in the layout
        {
            return NULL; //TODO Throw propper error here?, altho I am not allowed to throw an error here
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
            std::array<int8_t, 2u> relative_position = super_4x4_group_lookup<std::array<int8_t, 2u>>(leftmost_core_tile_x, top_core_tile_y, even_coord_slice, odd_coord_slice);
            
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

        HexLyt super_lyt{{size_x, size_y, 0/*TODO check if 1 or 0 (or prior value)*/}, lyt.get_layout_name()};


        // move tiles to new layout
            lyt.foreach_node( //TODO maybe foreach_gate instead?
                [&super_lyt, &lyt](const auto& node)
                {
                    
                });

        /**
         * FORME: Methods that could be usefull:
         * - "ground_coordinates" for iteration over a range of hex tiles (has default mode for whole matrix)
         * - struct coord_t 
         * - connect() (for connecting signals)
         */
    }

    /**
     * Utility function to find a center tile orientation and according wires in the outer tiles
     * that represent the functionality and connection points of the original center tile, but the new tiles
     * all have existing SiDB implementations.
     * TODO inputs and outputs angeben
     */
    template <typename HexLyt>
    [[nodiscard]] HexLyt supertile_core_and_wire_generation(const HexLyt& lyt) noexcept
    {
        //TODO have all the required static_assters here that I need
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
    //TODO make sure to catch every exception (mabye use "catch (...)" if I don't know what kind of errors there are)
}

}

#endif  // FICTION_SUPERTILE_HPP