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
    position super(uint64_t x, uint64_t y, int64_t x_offset, int64_t y_offset)
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
     * @warning Negative target coordinates will be handled according to the implementation of ucoord_t,
     * which currently casts them to a unsigned integer without conversion!
     * 
     * @param x x position
     * @param y y postion
     * @return coodrinates which are translated into the supertile hex layout
     */
    position super(uint64_t x, uint64_t y) noexcept
    {
        return super(x, y, 0, 0);
    }

    /**
     * Utility function to inflate a hexagonal layout such that each tile is now encased by six new empty tiles.
     * TODO inputs und outputs beschreibung schreiben
     */
    template <typename HexLyt>
    [[nodiscrad]] HexLyt grow_to_supertiles(const HexLyt& lyt) noexcept
    {
        //TODO have all the required static_asserts here
        static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");

        //TODO have all the required runtime asserts here
        assert(lyt.z() == 0); //FRAGE: how does z height work with hexagonal layouts / SiDB gates

        // Search trough hexagonal layout to find the outermost, non-empty tiles (after translation into supertile-hexagonal layout)
        int64_t leftmost_core_tile_x = std::numeric_limits<int64_t>::max();
        int64_t rightmost_core_tile_x = std::numeric_limits<int64_t>::min();
        int64_t top_core_tile_y = std::numeric_limits<int64_t>::max();
        int64_t bottom_core_tile_y = std::numeric_limits<int64_t>::min();

        //CONTINUE foreach_node()
        if (!lyt.is_empty_tile(original_tile))
        {
            supertile_core_x < leftmost_core_tile_x ? leftmost_core_tile_x = supertile_core_x : ;
            supertile_core_x > rightmost_core_tile_x ? rightmost_core_tile_x = supertile_core_x : ;
            supertile_core_y < top_core_tile_y ? top_core_tile_y = supertile_core_y : ;
            supertile_core_y > bottom_core_tile_y ? bottom_core_tile_y = supertile_core_y : ;
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
            HexLyt super_lyt{{size_x, size_y, 0/*TODO check if 1 or 0 (or prior value)*/}, lyt.get_layout_name()};

        // move tiles to new layout
            // calculate offset for translated tiles
            uint64_t x_offset = 1 - leftmost_core_tile_x;
            uint64_t y_offset = 1 - top_core_tile_y;

            // copy inputs
            lyt.foreach_pi(
                [&lyt, &super_lyt, &x_offset, &y_offset](const auto& tile)
                {
                    //CONTINUE
                });


            // TODO: here is a lot of copied structure, I should either get this into it's own function or figure out how I can add the nodes in the first step already!!!

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
                    
                    // copy tiles
                    if (!lyt.is_empty_tile(original_tile))
                    {
                        //CONTINUE
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

        // copy outputs
        lyt.foreach_po();//CONTINUE
        
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