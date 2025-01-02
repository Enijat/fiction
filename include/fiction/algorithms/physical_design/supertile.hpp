//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP

#include "fiction/traits.hpp"

#include <limits>

namespace ficiton
{

//FRAGE: brauch/soll ich das statistics struct auch? (ist in hexagonalisation an dieser stelle drin)

//FORME: Put in depth implementation here
namespace detail
{
    /**
     * Utility function to inflate a hexagonal layout such that each tile is now encased by six new empty tiles
     * TODO inputs und outputs angeben
     */
    template <typename HexLyt>
    [[nodiscrad]] HexLyt grow_to_supertiles(const HexLyt& lyt) noexcept
    {
        //TODO have all the required static_assters here that I need
        static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");
        
        // Brute force search for outermost four tiles
        // TODO x and y the right names here?
        int64_t upper_most_core_tile_y = std::numeric_limits<int64_t>::max();
        int64_t lower_most_core_tile_y = std::numeric_limits<int64_t>::min();
        int64_t left_most_core_tile_x = std::numeric_limits<int64_t>::max();
        int64_t right_most_core_tile_x = std::numeric_limits<int64_t>::min();

        /**
         * FORME: Methods that could be usefull:
         * - "ground_coordinates" for iteration over a range of hex tiles (has default mode for whole matrix)
         * - "x" and "y" to get size of matrix
         * - struct coord_t 
         * - // instantiate hexagonal layout
         *      HexLyt hex_layout{{hex_width, hex_height, hex_depth}, row_clocking<HexLyt>()};
         */

        // iterate trough hexagonal layout column by colum to find the outermost, non-empty tiles
        // (after translation into supertile-hexagonal layout)

        // translated base cords of current column
        uint64_t supertile_column_base_x = 1;
        uint64_t supertoöe_column_base_y = 1;

        // iterate though original layout
        for (uint64_t x = 0; x <= lyt.x(); x++)
        {
            
        }

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