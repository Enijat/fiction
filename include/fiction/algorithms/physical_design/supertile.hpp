//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP

#include "fiction/traits.hpp"
#include "fiction/layouts/gate_level_layout.hpp"

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
        static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");//TODO did the "fiction::" fix it?

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

        // generate new hexagonal layout
            if (leftmost_core_tile_x > rightmost_core_tile_x) // There was not a single node in the layout
            {
                return NULL; //TODO Throw propper error here?
            }
            uint64_t size_x = rightmost_core_tile_x - leftmost_core_tile_x + 3;
            uint64_t size_y = bottom_core_tile_y - top_core_tile_y + 3;

            HexLyt super_lyt{{size_x, size_y, 0/*TODO check if 1 or 0 (or prior value)*/}, lyt.get_layout_name()};

        // move tiles to new layout
            // calculate offset for translated tiles
            uint64_t x_offset = 1 - leftmost_core_tile_x;
            uint64_t y_offset = 1 - top_core_tile_y;

            lyt.foreach_node( //TODO maybe foreach_gate instead?
                []()
                {
                    //continue
                });

        // TODO FORME ACHTUNG: wenn die annahme stimmt das die koordinaten zu passen müssen das bei 0,0 die obere linke ecke des clocking scemes sitzen muss hab ich deutlich weniger freiheit was 
        // das verschieben des layouts angeht, aber dafür müsste ich mir die base tile nicht mehr merken.            
        // TODO FORME: ich muss mir unbedingt die base/origianl 0,0 übersetzte koordinate merken, denn die brauch ich für wire generation und clocking zuordnen
        /**
         * FORME: Methods that could be usefull:
         * - "ground_coordinates" for iteration over a range of hex tiles (has default mode for whole matrix)
         * - struct coord_t 
         * - connect (for connecting signals)
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
    using namespace fiction;

    static_assert(is_gate_level_layout_v<HexLyt>, "HexLyt is not a gate-level layout");
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");
    static_assert(has_even_row_hex_arrangement_v<HexLyt>, "HexLyt does not have an even row hexagon arrangement");

    return detail::supertile_core_and_wire_generation<HexLyt>(detail::grow_to_supertiles<HexLyt>(lyt));
    //TODO make sure to catch every exception (mabye use "catch (...)" if I don't know what kind of errors there are)
}

}

#endif  // FICTION_SUPERTILE_HPP