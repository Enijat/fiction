//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP

// TODO check if I really need all of these:
#include "fiction/layouts/bounding_box.hpp"
#include "fiction/traits.hpp"
//#include "fiction/types.hpp"
#include "fiction/utils/name_utils.hpp"
#include "fiction/utils/placement_utils.hpp"
#include "fiction/layouts/clocking_scheme.hpp"

//FORME I know I need these:
#include "fiction/utils/math_utils.hpp"

//#include <mockturtle/traits.hpp>
//#include <mockturtle/views/topo_view.hpp>

#include <limits>
#include <cstdint>
#include <array>
#include <cmath>

namespace fiction
{
// TODO after checking all -Wconversion errors and beeing finished, I should move the diagnostics-compiler info to the top and bottom of the file, so I don't have to repeat it everywhere I need it
// TODO inline and constexpr and noexcept should be added everywhere they are needed
// TODO alle "layout" erwähnungen durch "gate-level layout" ersetzen ?!
// TODO coordinates und positions einheitlich verwenden
namespace detail
{

/**
 * Utility function to translate the original hex coodrinates into the new supertile hex coordinates.
 * Offset will simple be added to the coordinates.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param tile Original position.
 * @param offset_x Offset that is added to the x coordinate.
 * @param offset_y Offset that is added to the y coordinate.
 * @param z z coordinate the translated `tile` should have.
 * @return Coodrinates which are translated into the supertile hex layout.
 */
template <typename HexLyt>
[[nodiscard]] constexpr std::array<int64_t,3> super_arraytype(const tile<HexLyt> original_tile, int64_t offset_x, int64_t offset_y, uint64_t z) noexcept
{
    // position a tile at 0,0 would have in the supertile hex layout
    int64_t new_x = 1 + offset_x;
    int64_t new_y = 1 + offset_y;

    // calculate column base position
    new_x += ((static_cast<int64_t>(original_tile.x) >> 1) * 5 ) + (static_cast<int64_t>(original_tile.x) % 2 == 0 ? 0 : 2);
    new_y += static_cast<int64_t>(original_tile.x);

    // calculate position by traversing column
        // rough traversion (the pattern for addition repeates every 4 steps and can be summed up)
        new_x += (static_cast<int64_t>(original_tile.y) >> 2) * -3;
        new_y += (static_cast<int64_t>(original_tile.y) >> 2) * 10;
        
        // fine traversion (traverse the 0 - 3 steps that are left)
        if (static_cast<uint64_t>(original_tile.x) % 2 == 0)
        {
            switch (static_cast<uint64_t>(original_tile.y) % 4)
            {
                default:
                case 0:
                    break;
                case 1:
                    new_x -= 2;
                    new_y += 2;
                    break;
                case 2:
                    new_x -= 2;
                    new_y += 5;
                    break;
                case 3:
                    new_x -= 4;
                    new_y += 7;
                    break;
            }
        } else {
            switch (static_cast<uint64_t>(original_tile.y) % 4)
            {
                default:
                case 0:
                    break;
                case 1:
                    new_x -= 2;
                    new_y += 2;
                    break;
                case 2:
                    new_x -= 1;
                    new_y += 5;
                    break;
                case 3:
                    new_x -= 3;
                    new_y += 7;
                    break;
            }
        }

    return {{new_x, new_y, z}};
}

//TODO description and also change the description above
//TODO maybe I can make this inline?
template <typename HexLyt>
[[nodiscard]] constexpr tile<HexLyt> super(const tile<HexLyt> original_tile, int64_t offset_x, int64_t offset_y, uint64_t z) noexcept
{
    std::array<int64_t,3> result = super_arraytype<HexLyt>(original_tile, offset_x, offset_y, z);

    const tile<HexLyt> temp{result[0], result[1], result[2]}; //TODO compact back again

    std::cout << "[i] Super translated tile " << static_cast<uint64_t>(original_tile.x) << "," << static_cast<uint64_t>(original_tile.y) << "," << static_cast<uint64_t>(original_tile.z) << " to " << static_cast<uint64_t>(temp.x) << "," << static_cast<uint64_t>(temp.y) << "," << static_cast<uint64_t>(temp.z) << std::endl;
    
    return temp;
}

//TODO description
template <typename HexLyt>
[[nodiscard]] inline constexpr std::array<int64_t,3> super_arraytype(const tile<HexLyt> original_tile) noexcept
{
    return super_arraytype<HexLyt>(original_tile, 0, 0, static_cast<uint64_t>(original_tile.z));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

/**
 * These reflect the possible directions in a pointy-top hexagonal layout.
 * They are additionally used to represent the positions in a supertile, in which case each
 * value represents the position in the direction its name reflects, relative to the central position.
 */
enum hex_direction {
    NE = 0, // values fixed because they are used in the hash functions
    E = 1,
    SE = 2,
    SW = 3,
    W = 4,
    NW = 5,
    X = 6 // represents invalid direction, used for spacing / filling
};

using namespace detail; //TODO For what did I need this again? -> put description here

/**
 * Hash function that maps the input values (if they follow the constrains outlined in the in the bachelor thesis "Super-Tile Routing for Omnidirectional Information Flow in Silicon Dangling Bond Logic" by F. Kiefhaber, 2025)
 * to the range 0 to 59 without gaps and or overlaps. The order of the input values `B` and `C` is not relevant. 
 * 
 * @param A Can reflect the single output in a 2-in 1-out logic gate or the single input in a fanout.
 * @param B Can reflect either of the two inputs in a 2-in 1-out logic gate or either of the two oupputs of a fanout.
 * @param C Can reflect either of the two inputs in a 2-in 1-out logic gate or either of the two oupputs of a fanout.
 * @return Hash result to be used in the respective lookup table.
 */
[[nodiscard]] uint8_t perfectHashFunction21(hex_direction A, hex_direction B, hex_direction C) noexcept //TODO put the constexpr back here
{
    uint8_t b = positive_mod((B - A), 6);
    uint8_t c = positive_mod((C - A), 6);

    if ((b + c) == 9)
    {
        return 10*A + (12 - 5);
    }
    else
    {
        return 10*A + ((2*(b + c) - abs(b - c)) - 5);
    }
}

/**
 * Hash function that maps the input values (if they follow the constrains outlined in the in the bachelor thesis "Super-Tile Routing for Omnidirectional Information Flow in Silicon Dangling Bond Logic" by F. Kiefhaber, 2025)
 * to the range 0 to 29 without gaps and or overlaps.
 * 
 * @param A Reflects the input of a wire or inverter.
 * @param B Reflects the output of a wire or inverter.
 * @return Hash result to be used in the respective lookup table.
 */
[[nodiscard]] constexpr uint8_t perfectHashFunction11(hex_direction A, hex_direction B) noexcept
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

/**
 * Utility function to find the required size of a supertile layout and the offset required for copying existing gates and wires, based on the original layout that is to be transformed.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param lyt Original layout that will should be transformed.
 * @param size_x Pointer to where to write the required size in the x-dimension.
 * @param size_y Pointer to where to write the required size in the y-dimension.
 * @param offset_x Pointer to where to write the required offset in the x-dimension.
 * @param offset_y Pointer to where to write the required offset in the y-dimension.
 */
template <typename HexLyt>
void find_super_layout_size(const HexLyt& lyt, uint64_t* size_x, uint64_t* size_y, int64_t* offset_x, int64_t* offset_y) noexcept
{
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");
    assert(lyt.x() <= std::numeric_limits<int64_t>::max()); // reason is that coordinates will be cast from uint64_t to int64_t
    assert(lyt.y() <= std::numeric_limits<int64_t>::max());

    // Search trough hexagonal layout to find the outermost, non-empty tiles (after translation into supertile-hexagonal layout)
    int64_t leftmost_core_tile_x = std::numeric_limits<int64_t>::max();
    int64_t rightmost_core_tile_x = std::numeric_limits<int64_t>::min();
    int64_t top_core_tile_y = std::numeric_limits<int64_t>::max();
    int64_t bottom_core_tile_y = std::numeric_limits<int64_t>::min();
    
    lyt.foreach_node(
        [&lyt, &leftmost_core_tile_x, &rightmost_core_tile_x, &top_core_tile_y, &bottom_core_tile_y](const auto& node)
        {
            const auto tile = lyt.get_tile(node);

            const std::array<int64_t,3> pos = super_arraytype<HexLyt>(tile);

            if (pos[0] < leftmost_core_tile_x)
                {leftmost_core_tile_x = pos[0];}
            if (pos[0] > rightmost_core_tile_x)
                {rightmost_core_tile_x = pos[0];}
            if (pos[1] < top_core_tile_y)
                {top_core_tile_y = pos[1];}
            if (pos[1] > bottom_core_tile_y)
                {bottom_core_tile_y = pos[1];}
        });

    if (leftmost_core_tile_x > rightmost_core_tile_x) //TODO I could just remove this
    {
        std::cout << "[w] didn't find any nodes in layout" << std::endl;
    }

    std::cout << "[i] Extreme position values: " << leftmost_core_tile_x << ", " << rightmost_core_tile_x << ", " << top_core_tile_y << ", " << bottom_core_tile_y << ", " << std::endl; //TODO remove

    /*
    * calculate the offset that
    * A) keeps every supertile (so core and wires) in positive coordinates
    * B) doesn't wastes to much space (meaning the coordinates are as small as possible)
    * C) keeps the clocking such that the top left clock zone from the original is the same as the top left super clock zone
    */
    // clang-format off

    static constexpr std::array<std::array<int8_t,2>,56> even_coord_slice{{
            {{0,0}},{{1,0}},
            {{-1,10}},{{0,10}},{{1,10}},{{2,10}},{{3,10}},{{4,10}},{{5,10}},{{6,10}},{{7,10}},
            {{-2,6}},{{-1,6}},{{0,6}},{{1,6}},{{2,6}},{{3,6}},{{4,6}},{{5,6}},{{6,6}},{{7,6}},
            {{-2,2}},{{-1,2}},{{0,2}},{{1,2}},{{2,2}},{{3,2}},{{4,2}},{{5,2}},{{6,2}},
            {{4,12}},{{4,13}},
            {{-4,8}},{{-3,8}},{{-2,8}},{{-1,8}},{{0,8}},{{1,8}},{{2,8}},{{3,8}},{{4,8}},{{5,8}},{{6,8}},{{7,8}},
            {{-2,4}},{{-1,4}},{{0,4}},{{1,4}},{{2,4}},{{3,4}},{{4,4}},{{5,4}},{{6,4}},{{7,4}},{{8,4}},{{9,4}}
            }};

    static constexpr std::array<std::array<int8_t,2>,56> odd_coord_slice{{
            {{0,1}},{{1,1}},{{2,1}},{{3,1}},{{4,1}},
            {{2,11}},{{3,11}},{{4,11}},{{5,11}},{{6,11}},
            {{-3,7}},{{-2,7}},{{-1,7}},{{0,7}},{{1,7}},{{2,7}},{{3,7}},{{4,7}},{{5,7}},{{6,7}},{{7,7}},
            {{-2,3}},{{-1,3}},{{0,3}},{{1,3}},{{2,3}},{{3,3}},{{4,3}},{{5,3}},{{6,3}},{{7,3}},{{8,3}},{{9,3}},
            {{-3,9}},{{-2,9}},{{-1,9}},{{0,9}},{{1,9}},{{2,9}},{{3,9}},{{4,9}},{{5,9}},{{6,9}},{{7,9}},{{8,9}},
            {{-1,5}},{{0,5}},{{1,5}},{{2,5}},{{3,5}},{{4,5}},{{5,5}},{{6,5}},{{7,5}},{{8,5}},{{9,5}}
            }};

    // clang-format on

    // get the top left corner position, of the rectangle encapsulating all core tiles, relative to the 4x4 super clock zone group it is in
    std::array<int8_t,2> relative_position = fiction::super_4x4_group_lookup<std::array<int8_t,2>>(leftmost_core_tile_x, top_core_tile_y, even_coord_slice, odd_coord_slice);
    
    int8_t relative_x = relative_position[0];
    int8_t relative_y = relative_position[1];

    std::cout << "[i] Relative values bevore: " << static_cast<int>(relative_x) << ", " << static_cast<int>(relative_y) << std::endl; //TODO remove

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

    std::cout << "[i] Relative values after: " << static_cast<int>(relative_x) << ", " << static_cast<int>(relative_y) << std::endl; //TODO remove

    // calculate offset for translated tiles
    int64_t x_offset = static_cast<int64_t>(relative_x) - leftmost_core_tile_x;
    int64_t y_offset = static_cast<int64_t>(relative_y) - top_core_tile_y;

    *size_x = static_cast<uint64_t>(rightmost_core_tile_x + 1 + x_offset);
    *size_y = static_cast<uint64_t>(bottom_core_tile_y + 1 + y_offset);
    *offset_x = x_offset;
    *offset_y = y_offset;
}

/**
 * Utility function that returns a tile next to the given one, based on the passed direction.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param refference `tile` that gives the position to be refferenced in the hexagonal layout. 
 * @param direction Direction in which the requested `tile` is positioned, relative to `refference`. 
 * @param z The z-position that the requested tile shoudl have.
 * @return The requested `tile`.
 */
template <typename HexLyt>
[[nodiscard]] constexpr tile<HexLyt> get_near_position(const tile<HexLyt> refference, hex_direction direction, uint8_t z) noexcept
{
    assert(refference.x > 0);
    assert(refference.y > 0);

    uint64_t x = refference.x;
    uint64_t y = refference.y;

    switch (direction)
    {
    case NE:
        y--;
        if (refference.y % 2 == 0)
            {x++;}
        break;
    case E:
        x++;
        break;
    case SE:
        y++;
        if (refference.y % 2 == 0)
            {x++;}
        break;
    case SW:
        y++;
        if (refference.y % 2 != 0)
            {x--;}
        break;
    case W:
        x--;
        break;
    case NW:
        y--;
        if (refference.y % 2 != 0)
            {x--;}
        break;
    default:
        x = refference.x;
        y = refference.y;
        break;
    }

    return tile<HexLyt>{x, y, static_cast<uint64_t>(z)};
}

/**
 * Utility function that returns the `hex_direction` in which an adjacent tile is relative to the refference tile. Z position is ignored.
 * Undefined behaviour when the same tile is passed twice.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param refference `tile` that gives the position to be refferenced in the hexagonal layout. 
 * @param position Position of the other `tile`, defines the `hex_direction` from the refference tile. 
 * @return The requested `hex_direction`.
 */
template <typename HexLyt>
[[nodiscard]] constexpr hex_direction get_near_direction(const tile<HexLyt> refference, const tile<HexLyt> position) noexcept
{
    if (position.y < refference.y)
    {
        if (position.x == refference.x)
        {
            if (refference.y % 2 == 0)
                {return NW;}
            else
                {return NE;}
        }
        else
        {
            if (refference.y % 2 == 0)
                {return NE;}
            else
                {return NW;}
        }
    }
    else if (position.y == refference.y)
    {
        if (position.x > refference.x)
            {return E;}
        else
            {return W;}
    }
    else
    {
        if (position.x == refference.x)
        {
            if (refference.y % 2 == 0)
                {return SW;}
            else
                {return SE;}
        }
        else
        {
            if (refference.y % 2 == 0)
                {return SE;}
            else
                {return SW;}
        }
    }
    return X;
}

/**
 * Utility function that tries to place and connect a wire with the given positions. Will place another wire without incoming signal if the `input_position` has no node.
 * If in it's own `position` a wire already exists, it will only connect the input.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param lyt Layout that houses the tiles.
 * @param position Position that the wire should have.
 * @param input_position Position of the incomming signal to the wire.
 * @return `true` if an unfinished wire was already present at the `position`, else `false`.
 */
template <typename HexLyt>
[[nodiscard]] bool place_wire(HexLyt& lyt, const tile<HexLyt> position, const tile<HexLyt> input_position) noexcept
{   
    const auto current_node = lyt.get_node(position);
    const auto input_node = lyt.get_node(input_position);

    if (current_node == 0)
    {
        if (input_node == 0) // the tile from which the signal should be coming is empty, so a unfinished wire will be placed there that will later be connected
        {
            std::cout << "[i] Creating unconneted wire at " << static_cast<int>(position.x) << "," << static_cast<int>(position.y) << "," << static_cast<int>(position.z) << " (x,y,z)" << std::endl; //TODO remove
            lyt.create_buf(lyt.create_unconnected_buf(input_position), position);
        } else {
            std::cout << "[i] Creating normal wire at " << static_cast<int>(position.x) << "," << static_cast<int>(position.y) << "," << static_cast<int>(position.z) << " (x,y,z)" << std::endl; //TODO remove
            lyt.create_buf(lyt.make_signal(input_node), position);
        }
    }
    else // the wire already exists, it only needs it's incoming signal
    {   
        std::cout << "[i] Wire at " << static_cast<int>(position.x) << "," << static_cast<int>(position.y) << "," << static_cast<int>(position.z) << " (x,y,z) already existed, only connecting it" << std::endl; //TODO remove
        lyt.connect(lyt.make_signal(input_node), current_node);
        return true;
    }

    return false;
}

/**
 * Utility function that places one string of input wires in a supertile, whichs position is defined by the core `tile`.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @tparam table_size Size of the lookup table that is passed.
 * @param lyt Hexagonal gate-level supertile layout that will house the wires.
 * @param core `tile` that represents the central position in a supertile.
 * @param lookup_table Lookup table that defines the path the wires will take.
 * @param table_position Position in the lookup table where the start of the current wire string is positioned.
 * @param last_z_position Pointer to where this function will write the z position of the last wire that was placed.
 * @return Returns the updated position in the lookup table that reflects the position of the next entity in the table.
 */
template <typename HexLyt, std::size_t table_size>
[[nodiscard]] uint8_t place_input_wires(HexLyt& lyt, const tile<HexLyt> core, const std::array<std::array<hex_direction,2>,table_size>& lookup_table, uint8_t table_position, uint8_t* last_z_position) noexcept
{
    uint8_t internal_last_z_position = 0;
    const auto first_input_wire_position = get_near_position<HexLyt>(core, lookup_table[table_position][0], 0);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    place_wire<HexLyt>(lyt, first_input_wire_position, get_near_position<HexLyt>(first_input_wire_position, lookup_table[table_position][1], internal_last_z_position));
    #pragma GCC diagnostic pop

    table_position++;
    while (lookup_table[table_position][0] != X)
    {
        const auto input_wire_position = get_near_position<HexLyt>(core, lookup_table[table_position][0], 1);

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        place_wire<HexLyt>(lyt, input_wire_position, get_near_position<HexLyt>(input_wire_position, lookup_table[table_position][1], internal_last_z_position));
        #pragma GCC diagnostic pop
        
        internal_last_z_position = 1;
        table_position++;
    }
    *last_z_position = internal_last_z_position;
    return ++table_position; // to skip the spacer
}

/**
 * Utility function that places one string of output wires in a supertile, whichs position is defined by the core `tile`.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @tparam table_size Size of the lookup table that is passed.
 * @param lyt Hexagonal gate-level supertile layout that will house the wires.
 * @param core `tile` that represents the central position in a supertile.
 * @param lookup_table Lookup table that defines the path the wires will take.
 * @param table_position Position in the lookup table where the start of the current wire string is positioned.
 * @param found_wire Pointer to where this method will write `true` if an unfinished wire was already present at the position of the last wire placed by this function.
 * @return Returns the updated position in the lookup table that reflects the position of the next entity in the table.
 */
template <typename HexLyt, std::size_t table_size>
[[nodiscard]] uint8_t place_output_wires(HexLyt& lyt, const tile<HexLyt> core, const std::array<std::array<hex_direction,2>,table_size>& lookup_table, uint8_t table_position, bool* found_wire) noexcept //This one also needs a pointer to a bool where it can write if it found a existing wire
{
    uint8_t last_z_position = 0;
    while (table_position + 1u < table_size and lookup_table[table_position + 1u][0] != X)
    {
        
        const auto output_wire_position = get_near_position<HexLyt>(core, lookup_table[table_position][0], 1);

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        place_wire<HexLyt>(lyt, output_wire_position, get_near_position<HexLyt>(output_wire_position, lookup_table[table_position][1], last_z_position));
        #pragma GCC diagnostic pop

        last_z_position = 1;
        table_position++;
    }
    const auto last_output_wire_position = get_near_position<HexLyt>(core, lookup_table[table_position][0], 0);
    if (place_wire<HexLyt>(lyt, last_output_wire_position, get_near_position<HexLyt>(last_output_wire_position, lookup_table[table_position][1], last_z_position)))
    {
        *found_wire = true;
    }
    return static_cast<uint8_t>(table_position + 2); // to skip the last placed wire and a potential spacer
}

//TODO note about where they are from and how to create them
constexpr const std::array<std::array<std::array<hex_direction,2>,9>,60> lookup_table_2in1out = {{
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{NW, NE}}, {{NE, W}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{NE, SE}}}},
{{{{SW, W}}, {{SE, W}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{SE, W}}, {{X, X}}, {{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}}},
{{{{SW, W}}, {{SE, W}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}}},
{{{{SW, W}}, {{SE, W}}, {{X, X}}, {{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}}},
{{{{NE, E}}, {{X, X}}, {{SW, W}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{NW, NE}}, {{NE, W}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{SE, W}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{SE, NE}}}},
{{{{SW, W}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{NW, NE}}, {{NE, W}}, {{X, X}}, {{SE, NW}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SW, W}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SW, W}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{NW, NE}}, {{NE, W}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{NW, E}}, {{X, X}}, {{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{NW, NE}}, {{NE, W}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{W, NW}}, {{NW, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{W, NW}}, {{NW, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{W, NW}}, {{NW, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{SW, NW}}}},
{{{{NE, E}}, {{NW, E}}, {{X, X}}, {{E, SE}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{NW, E}}, {{X, X}}, {{SE, SW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}}},
{{{{SW, W}}, {{SE, W}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{NW, NE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}}},
{{{{NE, E}}, {{NW, E}}, {{X, X}}, {{E, SE}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{NW, E}}, {{X, X}}, {{E, SE}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{NW, SW}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{X, X}}, {{NW, SE}}, {{X, X}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SE, SW}}, {{SW, E}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{SE, W}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{SW, W}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{W, NW}}, {{SW, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}}}};

constexpr const std::array<std::array<std::array<hex_direction,2>,9>,60> lookup_table_1in2out = {{
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{SE, W}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{NW, SW}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{NW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{SW, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{SW, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{NW, SW}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{NW, SW}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{SW, NW}}, {{SE, W}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{SW, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{SW, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{NW, SW}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{NW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{SW, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{NW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{NW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{NE, W}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{NW, E}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{SE, W}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{NE, W}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}, {{NE, W}}, {{X, X}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}, {{X, X}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{SE, NE}}, {{SW, E}}, {{X, X}}, {{NW, SE}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{NE, W}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{SE, W}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{SE, W}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}, {{NW, SE}}, {{NE, W}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{SE, W}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{SE, W}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{SW, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}, {{X, X}}, {{X, X}}}}}};

constexpr const std::array<std::array<std::array<hex_direction,2>,5>,30> lookup_table_1in1out_INVERTER = {{
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NW, SE}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}},
{{{{NE, E}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}}},
{{{{NE, E}}, {{E, NW}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}, {{W, NE}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NW, SE}}, {{X, X}}}},
{{{{SE, SW}}, {{E, SW}}, {{NE, SE}}, {{X, X}}, {{SW, NE}}}},
{{{{SE, SW}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{E, SE}}, {{NE, SE}}, {{X, X}}, {{SE, NW}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NW, SE}}, {{W, NE}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NW, SE}}, {{X, X}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SW, NE}}, {{W, SE}}, {{X, X}}}},
{{{{E, SE}}, {{SE, NE}}, {{X, X}}, {{NE, SW}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{X, X}}, {{X, X}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{X, X}}}},
{{{{NW, NE}}, {{W, NE}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}}},
{{{{SW, W}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}}},
{{{{W, NW}}, {{SW, NW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{E, SW}}, {{X, X}}}},
{{{{SW, W}}, {{W, SE}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SE, NW}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SE, NW}}, {{X, X}}, {{X, X}}}},
{{{{SE, SW}}, {{X, X}}, {{NE, SW}}, {{E, NW}}, {{X, X}}}},
{{{{W, NW}}, {{NW, SW}}, {{X, X}}, {{SW, NE}}, {{X, X}}}},
{{{{NW, NE}}, {{X, X}}, {{SW, NE}}, {{X, X}}, {{X, X}}}}}};

/**
 * Utility function that populates a supertile by reading in the `original_tile`s node and it's inputs and outputs,
 * and then looking up the required supertile layout in the respective lookup table.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param origianl_lyt Layout that houses the `original_tile`.
 * @param super_lyt Layout that houses the supertile that is to be populated.
 * @param original_tile `tile` that should be replaced by the supertile.
 * @param offset_x Offset in x direction required for the translation of the `original_tile` to the super_lyt.
 * @param offset_y Offset in y direction required for the translation of the `original_tile` to the super_lyt.
 * @param output_a If the supertile populated by this function has a ouput to another supertile which isn't populated yet,
 * this function will place the `hex_direction` of this other supertile at the location of this pointer.
 * @param output_b This pointer has the same functionallity as `output_a` and will only be used if there is a second `hex_output` direction into another unpopulated supertile. 
 * @return `true` if a unknown gate/fanout was placed at `original_tile`, else `false`. This includes primary inputs.
 */
template <typename HexLyt>
[[nodiscard]] bool populate_supertile(const HexLyt& original_lyt, HexLyt& super_lyt, const tile<HexLyt> original_tile, int64_t offset_x, int64_t offset_y, hex_direction* output_a, hex_direction* output_b) noexcept
{
    const auto original_node = original_lyt.get_node(original_tile);
    const auto incoming_signals = original_lyt.incoming_data_flow(original_tile);
    const auto outgoing_signals = original_lyt.outgoing_data_flow(original_tile);

    if (incoming_signals.size() == 1)
    {
        if (original_lyt.is_po(original_node))
        {
            hex_direction in = get_near_direction<HexLyt>(original_tile, incoming_signals[0]);
            
            const auto core_tile = super<HexLyt>(original_tile, offset_x, offset_y, 0);

            // place input wire
            const auto input_wire_position = get_near_position<HexLyt>(core_tile, in, 0);

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wunused-result"
            place_wire<HexLyt>(super_lyt, input_wire_position, get_near_position<HexLyt>(input_wire_position, static_cast<hex_direction>((in + 1) % 6), 0));
            #pragma GCC diagnostic pop

            // place output
            super_lyt.create_po(super_lyt.make_signal(super_lyt.get_node(input_wire_position)), original_lyt.get_name(original_node), core_tile);

        }
        else if (original_lyt.is_wire(original_node) and !original_lyt.is_fanout(original_node))
        {
            hex_direction in = get_near_direction<HexLyt>(original_tile, incoming_signals[0]);
            hex_direction out = get_near_direction<HexLyt>(original_tile, outgoing_signals[0]);

            const auto core_tile = super<HexLyt>(original_tile, offset_x, offset_y, 0);

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wunused-result"

            // place input wire
            const auto input_wire_position = get_near_position<HexLyt>(core_tile, in, 0);
            place_wire<HexLyt>(super_lyt, input_wire_position, get_near_position<HexLyt>(input_wire_position, static_cast<hex_direction>((in + 1) % 6), 0));

            // place core wire
            place_wire<HexLyt>(super_lyt, core_tile, get_near_position<HexLyt>(core_tile, in, 0));

            #pragma GCC diagnostic pop

            // place output wire
            const auto output_wire_position = get_near_position<HexLyt>(core_tile, out, 0);
            if (!place_wire<HexLyt>(super_lyt, output_wire_position, core_tile))
            {
                *output_a = out; 
            }
        }
        else if (original_lyt.is_inv(original_node))
        {
            hex_direction in = get_near_direction<HexLyt>(original_tile, static_cast<tile<HexLyt>>(incoming_signals[0]));
            hex_direction out = get_near_direction<HexLyt>(original_tile, static_cast<tile<HexLyt>>(outgoing_signals[0]));

            const auto core_tile = super<HexLyt>(original_tile, offset_x, offset_y, 0);

            std::array<std::array<hex_direction,2>,5> lookup_table = lookup_table_1in1out_INVERTER[perfectHashFunction11(in, out)];

            uint8_t table_position = 0;
            uint8_t last_z_position = 0;

            // place input wires
            table_position = place_input_wires<HexLyt,5>(super_lyt, core_tile, lookup_table, table_position, &last_z_position);
            const auto last_wire = get_near_position<HexLyt>(core_tile, lookup_table[table_position - 2][0], last_z_position);

            // place inverter core
            super_lyt.create_not(super_lyt.make_signal(super_lyt.get_node(last_wire)), core_tile);

            // place output wires
            bool found_wire = false;
            place_output_wires<HexLyt,5>(super_lyt, core_tile, lookup_table, table_position, &found_wire);
            if (!found_wire)
            {
                *output_a = out;
            }
        }
        else if (original_lyt.is_fanout(original_node) and outgoing_signals.size() == 2) // original_node is 2out fanout
        {

            hex_direction in = get_near_direction<HexLyt>(original_tile, incoming_signals[0]);
            hex_direction out1 = get_near_direction<HexLyt>(original_tile, outgoing_signals[0]);
            hex_direction out2 = get_near_direction<HexLyt>(original_tile, outgoing_signals[1]);

            const auto core_tile = super<HexLyt>(original_tile, offset_x, offset_y, 0);

            std::array<std::array<hex_direction,2>,9> lookup_table = lookup_table_1in2out[perfectHashFunction21(in, out1, out2)];

            uint8_t table_position = 0;
            uint8_t last_z_position = 0;

            // place input wires
            table_position = place_input_wires<HexLyt,9>(super_lyt, core_tile, lookup_table, table_position, &last_z_position);
            const auto last_wire = get_near_position<HexLyt>(core_tile, lookup_table[table_position - 2][0], last_z_position);

            // place fanout core
            super_lyt.create_buf(super_lyt.make_signal(super_lyt.get_node(last_wire)), core_tile);

            // place output wires 1
            bool found_wire = false;
            table_position = place_output_wires<HexLyt,9>(super_lyt, core_tile, lookup_table, table_position, &found_wire);
            if (!found_wire)
            {
                std::cout << "[i] writing " << static_cast<int>(lookup_table[table_position - 2][0]) << " too output_a in populate_supertile" << std::endl; //TODO remove
                *output_a = lookup_table[table_position - 2][0];
            }
            // place output wires 2
            found_wire = false;
            table_position = place_output_wires<HexLyt,9>(super_lyt, core_tile, lookup_table, table_position, &found_wire);
            if (!found_wire)
            {
                std::cout << "[i] writing " << static_cast<int>(lookup_table[table_position - 2][0]) << " too output_b in populate_supertile" << std::endl; //TODO remove
                *output_b = lookup_table[table_position - 2][0];
            }
        }
        else
        {
            return true; // unknown core gate
        }
    }
    else if (incoming_signals.size() == 2 and outgoing_signals.size() == 1) //FORME original_node is on of the following logic gates: OR, NOR, AND, NAND, XOR, XNOR
    {
        hex_direction in1 = get_near_direction<HexLyt>(original_tile, incoming_signals[0]);
        hex_direction in2 = get_near_direction<HexLyt>(original_tile, incoming_signals[1]);
        hex_direction out = get_near_direction<HexLyt>(original_tile, outgoing_signals[0]);

        const auto core_tile = super<HexLyt>(original_tile, offset_x, offset_y, 0);

        std::array<std::array<hex_direction,2>,9> lookup_table = lookup_table_2in1out[perfectHashFunction21(out, in1, in2)];
        
        std::cout << "[i] detected 2in1out gate and choose lookuptable entry " << static_cast<int>(perfectHashFunction21(out, in1, in2)) << std::endl; //TODO delete
        std::cout << "    -> in1/in2/out = " << static_cast<int>(in1) << "/" << static_cast<int>(in2) << "/" << static_cast<int>(out) << std::endl; //TODO delete

        uint8_t table_position = 0;
        uint8_t last_z_position = 0;

        // place input wires 1
        table_position = place_input_wires<HexLyt,9>(super_lyt, core_tile, lookup_table, table_position, &last_z_position);
        const auto last_wire_1 = get_near_position<HexLyt>(core_tile, lookup_table[table_position - 2][0], last_z_position);

        // place input wires 2
        table_position = place_input_wires<HexLyt,9>(super_lyt, core_tile, lookup_table, table_position, &last_z_position);
        const auto last_wire_2 = get_near_position<HexLyt>(core_tile, lookup_table[table_position - 2][0], last_z_position);

        // place core
        const auto last_signal_1 = super_lyt.make_signal(super_lyt.get_node(last_wire_1));
        const auto last_signal_2 = super_lyt.make_signal(super_lyt.get_node(last_wire_2));
        if (original_lyt.is_and(original_node))
        {
            super_lyt.create_and(last_signal_1, last_signal_2, core_tile);
        }
        else if (original_lyt.is_nand(original_node))
        {
            super_lyt.create_nand(last_signal_1, last_signal_2, core_tile);
        }
        else if (original_lyt.is_or(original_node))
        {
            super_lyt.create_or(last_signal_1, last_signal_2, core_tile);
        }
        else if (original_lyt.is_nor(original_node))
        {
            super_lyt.create_nor(last_signal_1, last_signal_2, core_tile);
        }
        else if (original_lyt.is_xor(original_node))
        {
            super_lyt.create_xor(last_signal_1, last_signal_2, core_tile);
        }
        else if (original_lyt.is_xnor(original_node))
        {
            super_lyt.create_xnor(last_signal_1, last_signal_2, core_tile);
        }
        else if (original_lyt.is_function(original_node))
        {
            const auto original_node_fun = original_lyt.node_function(original_node);

            super_lyt.create_node({last_signal_1, last_signal_2}, original_node_fun, core_tile);
        }

        // place output wires
        bool found_wire = false;
        place_output_wires<HexLyt,9>(super_lyt, core_tile, lookup_table, table_position, &found_wire);
        if (!found_wire)
        {
            *output_a = out;
        }
    }
    else
    {
        return true;
    }
    return false;
}

/**
 * Utility function that adds a tile to a vector of tiles, but only if there is no tile with the same x and y coordinates present.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param vector Vector to which the tile will be added.
 * @param new_tile Tile to be added.
 */
template <typename HexLyt>
[[nodiscard]] void add_unique(std::vector<tile<HexLyt>>& vector, const tile<HexLyt> new_tile) noexcept
{
    const auto pos = std::find_if(vector.begin(), vector.end(), 
        [new_tile](const tile<HexLyt> existing_tile) noexcept
        {
            return (existing_tile.x == new_tile.x) and (existing_tile.y == new_tile.y);
        });

    if (pos == vector.end())
    {
        std::cout << "[i] added tile at position: " << static_cast<int>(new_tile.x) << "," << static_cast<int>(new_tile.y) << " (x,y) to vector" << std::endl; //TODO remove
        vector.push_back(new_tile);
    }
}

//TODO description (mainly used to copy z)
template <typename HexLyt>
[[nodiscard]] tile<HexLyt> get_outgoing_from_direction(const std::vector<tile<HexLyt>> outgoing_tiles, const tile<HexLyt> refference, hex_direction direction) noexcept
{
    const auto sample = get_near_position<HexLyt>(refference, direction, 0);
    for (tile<HexLyt> outgoing : outgoing_tiles)
    {
        if (sample.x == outgoing.x and sample.y == outgoing.y)
        {
            return outgoing;
        }
    }
    std::cout << "[w] couldn't find tile in vector from the given direction" << std::endl; //TODO maybe there is a better / prettier solution
    return sample;
}
}

/**
 * Transform a AMY-clocked hexagonal even-row gate-level layout into a AMYSUPER-clocked hexagonal even-row gate-level layout
 * that only contains gates that can be realised with gates/wires from the SiDB extendagon gate library.
 * This algorithm was proposed in the bachelor thesis "Super-Tile Routing for Omnidirectional Information Flow in Silicon Dangling Bond Logic" by F. Kiefhaber, 2025.
 * 
 * @tparam HexLyt Even-row hexagonal gate-level layout return type.
 * @param original_lyt the gate-level layout that is to be transformed.
 * @return Either to new supertile gate-level layout or the original layout if something went wrong.
 */
//TODO throw the right errors in this method instead of just returning the original layout
template <typename HexLyt>
[[nodiscard]] HexLyt supertilezation(const HexLyt& original_lyt) noexcept
{
    static_assert(is_gate_level_layout_v<HexLyt>, "HexLyt is not a gate-level layout");
    static_assert(is_hexagonal_layout_v<HexLyt>, "HexLyt is not a hexagonal layout");
    static_assert(has_even_row_hex_arrangement_v<HexLyt>, "HexLyt does not have an even row hexagon arrangement");
    assert(original_lyt.z() == 0); //TODO remove because it doesn't work anyways and I have to add wire crossings + maybe add check for 1 instead of 2 ?
    //TODO instead check for max fanout that is used!

    uint64_t size_x;
    uint64_t size_y;
    int64_t offset_x;
    int64_t offset_y;
    detail::find_super_layout_size<HexLyt>(original_lyt, &size_x, &size_y, &offset_x, &offset_y);
    HexLyt super_lyt{{size_x, size_y, 1}, original_lyt.is_clocking_scheme(clock_name::AMY) ? fiction::amy_supertile_clocking<HexLyt>() : fiction::row_supertile_clocking<HexLyt>(), original_lyt.get_layout_name()};
    std::vector<tile<HexLyt>> path_beginnings;

    //std::cout << "[i] size_x: " << size_x << ", size_y: " << size_y << ", offset_x: " << offset_x << ", offset_y: "<< offset_y << std::endl; //TODO remove

    // replace all inputs and save their output tile
    original_lyt.foreach_pi(
            [&original_lyt, &super_lyt, offset_x, offset_y, &path_beginnings](const auto& original_node)
            {
                //TODO: inputs können wohl auch inverter sein ?! wie und wo muss ich das handlen? -> aparently nicht weil exact das nicht produziert, aber ich sollte es evtl trotzdem einfach abfangen
                const tile<HexLyt> original_tile = original_lyt.get_tile(original_node);
                const tile<HexLyt> super_tile = detail::super<HexLyt>(original_tile, offset_x, offset_y, 0);
                const auto super_signal = super_lyt.create_pi(original_lyt.get_name(original_lyt.get_node(original_tile)), super_tile);
                const auto original_output_position = original_lyt.outgoing_data_flow(original_tile)[0];
                const auto super_output_wire_position = detail::get_near_position<HexLyt>(super_tile, detail::get_near_direction<HexLyt>(original_tile, original_output_position), 0);
                super_lyt.create_buf(super_signal, super_output_wire_position);
                detail::add_unique<HexLyt>(path_beginnings, original_output_position);
            });

    while (!path_beginnings.empty())
    {
        tile<HexLyt> current_original_tile = path_beginnings.back();
        path_beginnings.pop_back();

        std::cout << "[i] Starting path at tile " << static_cast<int>(current_original_tile.x) << "," << static_cast<int>(current_original_tile.y) << " (x,y)" << std::endl; //TODO remove

        while (true)
        {
            detail::hex_direction output_a = detail::hex_direction::X;
            detail::hex_direction output_b = detail::hex_direction::X;
            if (detail::populate_supertile(original_lyt, super_lyt, current_original_tile, offset_x, offset_y, &output_a, &output_b))
            {
                std::cout << "[e] found unknown gate while populating supertile, abort translation" << std::endl; //TODO remove
                return original_lyt; // Error, so the process was aborted
            }
            if (output_a == detail::hex_direction::X) // path is finished
            {
                std::cout << "[i] Ended path at tile " << static_cast<int>(current_original_tile.x) << "," << static_cast<int>(current_original_tile.y) << " (x,y)" << std::endl; //TODO remove
                break;
            }
            else if (output_b == detail::hex_direction::X) // path continues on one path
            {
                std::cout << "[i] continuing path like normal" << std::endl; //TODO remove
                current_original_tile = detail::get_outgoing_from_direction<HexLyt>(original_lyt.outgoing_data_flow(current_original_tile), current_original_tile, output_a);
            }
            else // path splits up
            {
                std::cout << "[i detected path splitting" << std::endl; //TODO remove
                detail::add_unique<HexLyt>(path_beginnings, detail::get_outgoing_from_direction<HexLyt>(original_lyt.outgoing_data_flow(current_original_tile), current_original_tile, output_b));
                current_original_tile = detail::get_outgoing_from_direction<HexLyt>(original_lyt.outgoing_data_flow(current_original_tile), current_original_tile, output_a);
            }
        }
    }

    //TODO OPTIONAL: am ende die z dimension nochmal checken und evtl wieder auf 0 setzen wenn möglich (mit resize()?)    
    return super_lyt;
}
}

#endif  // FICTION_SUPERTILE_HPP