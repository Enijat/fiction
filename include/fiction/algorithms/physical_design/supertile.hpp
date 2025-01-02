//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP

#include "fiction/traits.hpp"

#include <limits>
#include <cstdint>

namespace ficiton
{

//FRAGE: brauch/soll ich das statistics struct auch? (ist in hexagonalisation an dieser stelle drin)

namespace detail
{
    /**
     * Utility function to inflate a hexagonal layout such that each tile is now encased by six new empty tiles
     * TODO inputs und outputs angeben
     */
    template <typename HexLyt>
    [[nodiscrad]] HexLyt grow_to_supertiles(const HexLyt& lyt) noexcept
    {
        //TODO have all the required static_asserts here
        static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");

        //TODO have all the required runtime asserts here
        assert(lyt.z() == 0); //FRAGE: how does z height work with hexagonal layouts

        // Brute force search trough hexagonal layout column by colum to find the outermost, non-empty tiles (after translation into supertile-hexagonal layout)
        //TODO maybe write a function for position translation? Would allow to directly iterate over the EXISTING gates with "ground_coordinates" function
            // TODO x and y the right names here?
            int64_t leftmost_core_tile_x = std::numeric_limits<int64_t>::max();
            int64_t rightmost_core_tile_x = std::numeric_limits<int64_t>::min();
            int64_t top_core_tile_y = std::numeric_limits<int64_t>::max();
            int64_t bottom_core_tile_y = std::numeric_limits<int64_t>::min();

            // translated base cords of current column
            uint64_t supertile_column_base_x = 1;
            uint64_t supertile_column_base_y = 1;

            // translated base cords of current tile
            int64_t supertile_core_x = 1;
            int64_t supertile_core_y = 1;

            uint8_t tile_shift_loop_position = 0;
            bool column_shift_loop_position = false; // the loop is only 2 entries long, so a boolean operator suffices 

            // iterate though original layout
            for (uint64_t x = 0; x <= lyt.x(); ++x)
            {
                for (uint64_t y = 0; y <= lyt.y(); ++y)
                {
                    const tile<HexLyt> original_tile{x, y, 0};

                    if (!lyt.is_empty_tile(original_tile))
                    {
                        // set the min/max values
                        supertile_core_x < leftmost_core_tile_x ? leftmost_core_tile_x = supertile_core_x : ;
                        supertile_core_x > rightmost_core_tile_x ? rightmost_core_tile_x = supertile_core_x : ;
                        supertile_core_y < top_core_tile_y ? top_core_tile_y = supertile_core_y : ;
                        supertile_core_y > bottom_core_tile_y ? bottom_core_tile_y = supertile_core_y : ;
                    }

                    // shift coordinates to the next tiles position
                    switch (tile_shift_loop_position)
                    {
                    default:
                    case 0:
                        supertile_core_x -= 2;
                        supertile_core_y += 2;
                        tile_shift_loop_position++;
                        break;
                    case 1:
                        supertile_core_y += 3;
                        tile_shift_loop_position++;
                        break;
                    case 2:
                        supertile_core_x -= 2;
                        supertile_core_y += 2;
                        tile_shift_loop_position++;
                        break;
                    case 3:
                        supertile_core_x += 1;
                        supertile_core_y += 3;
                        tile_shift_loop_position = 0;
                        break;
                    }
                }

                // set current column base and right loop position for tile shift
                supertile_column_base_y++;
                if (column_shift_loop_position) {
                    tile_shift_loop_position = 0;
                    supertile_column_base_x += 3;
                    column_shift_loop_position = false;
                } else {
                    tile_shift_loop_position = 2;
                    supertile_column_base_x += 2;
                    column_shift_loop_position = true;
                }

                // set current column tile new
                supertile_core_x = supertile_column_base_x;
                supertile_core_y = supertile_column_base_y;
            }

        // generate new hexagonal layout

            // calculate size
            if (leftmost_core_tile_x > rightmost_core_tile_x) // There was not a single node in the layout
            {
                return NULL; //TODO Throw propper error here?
            }
            uint64_t size_x = rightmost_core_tile_x - leftmost_core_tile_x + 3;
            uint64_t size_y = bottom_core_tile_y - top_core_tile_y + 3;

            // create new layout
            HexLyt super_hex_lyt{{size_x, size_y, 0/*TODO check if 1 or 0 (or prior value)*/}, lyt.get_layout_name()};

        // move tiles to new layout
        // TODO: this is a lot of copied structure, I should either get this into it's own function or figure out how I can add the nodes in the first step already!!!
        //CONTINUE
        
        /**
         * FORME: Methods that could be usefull:
         * - "ground_coordinates" for iteration over a range of hex tiles (has default mode for whole matrix)
         * - struct coord_t 
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

//FORME: Put supertile command here
template <typename HexLyt>
[[nodiscard]] HexLyt supertilezation(const HexLyt& lyt) noexcept
{
    static_assert(is_gate_level_layout_v<HexLyt>, "HexLyt is not a gate-level layout");
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");
    static_assert(has_even_row_hex_arrangement_v<HexLyt>, "HexLyt does not have an even row hexagon arrangement");

    return detail::supertile_core_and_wire_generation<HexLyt>(detail::grow_to_supertiles<HexLyt>(lyt));
    //TODO make sure to catch every exception (mabye use "catch (...)" if I don't know what kind of errors there are)
}

}

#endif  // FICTION_SUPERTILE_HPP