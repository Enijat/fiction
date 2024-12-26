//
// Created by Felix Kiefhaber on 26.12.2024
//

#ifndef FICTION_SUPERTILE_HPP
#define FICTION_SUPERTILE_HPP

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

    /**
     * Utility function to find a center tile orientation and according wires in the outer tiles
     * that represent the functionality and connection points of the original center tile, but the new tiles
     * all have existing SiDB implementations.
     * TODO inputs and outputs angeben
     */

    //TODO: (optional) write method for wire optimisation and call it in the right places
}

//FORME: Put supertile command here
template <typename HexLyt>
[[nodiscard]] HexLyt supertilezation() noexcept
{
    //TODO make sure to catch every exception (mabye use "catch (...)" if I don't know what kind of errors there are)
}

}

#endif  // FICTION_SUPERTILE_HPP