//
// Created by Felix Kiefhaber on 24.12.2024
//

#ifndef FICTION_SIDB_HEX_WIRES_LIBRARY_HPP
#define FICTION_SIDB_HEX_WIRES_LIBRARY_HPP

//TODO: Make shure I really need all these includes
#include "fiction/technology/cell_ports.hpp"
#include "fiction/technology/cell_technologies.hpp"
#include "fiction/technology/fcn_gate_library.hpp"
#include "fiction/traits.hpp"
#include "fiction/utils/array_utils.hpp"
#include "fiction/utils/hash.hpp"
#include "fiction/utils/truth_table_utils.hpp"
#include "fiction/technology/sidb_bestagon_library.hpp"

#include <phmap.h>

#include <stdexcept>
#include <utility>

namespace fiction
{
    /**
     * TODO give description here
     * Add: there are only wires here, not primary inputs / outputs (FOR NOW; LOOK INTO THAT, but should not be needed)
     * Also: Look at "sidb_bestagon_library" for the reasoning behind some values here
     */

class sidb_extendagon_library : public public fcn_gate_library<sidb_technology, 60, 46>  // width and height of a hexagon
{
  public:
        explicit sidb_extendagon_library() = delete;

    /**
     * Overrides the corresponding function in fcn_bestagon_library. Given a tile `t`, this function takes all necessary
     * information from the stored grid into account to choose the correct fcn_gate representation for that tile. May it
     * be a gate or wires. Rotation and special marks like input and output, const cells etc. are computed additionally.
     *
     * @tparam GateLyt Pointy-top hexagonal gate-level layout type.
     * @param lyt Layout that hosts tile `t`.
     * @param t Tile to be realized as a Extendagon gate.
     * @return Extendagon gate representation of `t` including mirroring.
     */
    template <typename GateLyt>
    [[nodiscard]] static fcn_gate set_up_gate(const GateLyt& lyt, const tile<GateLyt>& t)
    {
        static_assert(is_gate_level_layout_v<GateLyt>, "GateLyt must be a gate-level layout");
        static_assert(is_hexagonal_layout_v<GateLyt>, "GateLyt must be a hexagonal layout");
        static_assert(has_pointy_top_hex_orientation_v<GateLyt>, "GateLyt must be a pointy-top hexagonal layout");

        const auto n = lyt.get_node(t);
        const auto p = determine_port_routing(lyt, t);

        try
        {
            if constexpr (fiction::has_is_fanout_v<GateLyt>)
            {
                if (lyt.is_fanout(n))
                {
                    if (lyt.fanout_size(n) == 2)
                    {
                        return FANOUT_MAP.at(p);
                    }
                }
            }
            if constexpr (fiction::has_is_buf_v<GateLyt>)
            {
                if (lyt.is_buf(n))
                {
                    if (lyt.is_ground_layer(t))
                    {
                        // double wire
                        if (const auto at = lyt.above(t); (t != at) && lyt.is_wire_tile(at))
                        {
                            const auto pa = determine_port_routing(lyt, at);

                            return DOUBLE_WIRE_MAP.at({p, pa});
                        }
                        // regular wire

                        return SINGLE_WIRE_MAP.at(p);
                    }

                    return EMPTY_GATE;
                }
            }
            if constexpr (fiction::has_is_inv_v<GateLyt>)
            {
                if (lyt.is_inv(n))
                {
                    return NOT_MAP.at(p);
                }
            }
            if constexpr (mockturtle::has_is_and_v<GateLyt>)
            {
                if (lyt.is_and(n))
                {
                    return AND_MAP.at(p);
                }
            }
            if constexpr (mockturtle::has_is_or_v<GateLyt>)
            {
                if (lyt.is_or(n))
                {
                    return OR_MAP.at(p);
                }
            }
            if constexpr (fiction::has_is_nand_v<GateLyt>)
            {
                if (lyt.is_nand(n))
                {
                    return NAND_MAP.at(p);
                }
            }
            if constexpr (fiction::has_is_nor_v<GateLyt>)
            {
                if (lyt.is_nor(n))
                {
                    return NOR_MAP.at(p);
                }
            }
            if constexpr (mockturtle::has_is_xor_v<GateLyt>)
            {
                if (lyt.is_xor(n))
                {
                    return XOR_MAP.at(p);
                }
            }
            if constexpr (fiction::has_is_xnor_v<GateLyt>)
            {
                if (lyt.is_xnor(n))
                {
                    return XNOR_MAP.at(p);
                }
            }
        }
        catch (const std::out_of_range&)
        {
            throw unsupported_gate_orientation_exception(t, p);
        }

        throw unsupported_gate_type_exception(t);
    }
    
    /**
     * Returns a map of all implementations supported by the library.
     *
     * This is an optional interface function that is required by some algorithms.
     *
     * @return Map of all implementations as hexagonal gates.
     */
    static gate_functions get_functional_implementations() noexcept
    {
        static const gate_functions implementations{
            {{create_id_tt(),
              // single wires
              {NORTH_EAST_EAST_WIRE, NORTH_EAST_SOUTH_EAST_WIRE, NORTH_EAST_SOUTH_WEST_WIRE, NORTH_EAST_WEST_WIRE, NORTH_EAST_NORTH_WEST_WIRE,
              EAST_SOUTH_EAST_WIRE, EAST_SOUTH_WEST_WIRE, EAST_WEST_WIRE, EAST_NORTH_WEST_WIRE, SOUTH_EAST_SOUTH_WEST_WIRE, SOUTH_EAST_WEST_WIRE,
              SOUTH_EAST_NORTH_WEST_WIRE, SOUTH_WEST_WEST_WIRE, SOUTH_WEST_NORTH_WEST_WIRE, WEST_NORTH_WEST_WIRE,
              // double wires (one straight, one bend)
              EAST_WEST_AND_SOUTH_EAST_SOUTH_WEST_WIRE, NORTH_EAST_SOUTH_WEST_AND_WEST_NORTH_WEST_WIRE, EAST_WEST_AND_NORTH_EAST_NORTH_WEST_WIRE, NORTH_EAST_SOUTH_WEST_AND_EAST_SOUTH_EAST_WIRE,
              // double wires (both bend)
              EAST_SOUTH_EAST_AND_SOUTH_WEST_WEST_WIRE, SOUTH_WEST_WEST_AND_NORTH_EAST_NORTH_WEST_WIRE, WEST_NORTH_WEST_AND_NORTH_EAST_EAST_WIRE, NORTH_EAST_EAST_AND_SOUTH_EAST_SOUTH_WEST_WIRE,
              // fanouts
              NORTH_EAST_TO_SOUTH_EAST_SOUTH_WEST_FANOUT, NORTH_WEST_TO_SOUTH_EAST_SOUTH_WEST_FANOUT, SOUTH_EAST_TO_NORTH_EAST_NORTH_WEST_FANOUT, SOUTH_WEST_TO_NORTH_EAST_NORTH_WEST_FANOUT}},
             {create_not_tt(),
              {NORTH_EAST_SOUTH_EAST_NOT, NORTH_EAST_SOUTH_WEST_NOT, SOUTH_EAST_NORTH_WEST_NOT, SOUTH_WEST_NORTH_WEST_NOT}},
             {create_and_tt(),
              {NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_AND, NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_AND, SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_AND, SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_AND}},
             {create_or_tt(),
              {NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_OR, NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_OR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_OR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_OR}},
             {create_nand_tt(),
              {NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_NAND, NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_NAND, SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_NAND, SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_NAND}},
             {create_nor_tt(),
              {NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_NOR, NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_NOR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_NOR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_NOR}},
             {create_xor_tt(),
              {NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_XOR, NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_XOR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_XOR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_XOR}},
             {create_xnor_tt(),
              {NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_XNOR, NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_XNOR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_XNOR, SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_XNOR}}}};

        return implementations;
    }
    /**
     * Returns a map of all different wire implementations and their respective port information.
     *
     * This is an optional interface function that is required by some algorithms.
     *
     * @return Map of all different wire implementations and their respective port information.
     */
    static gate_ports<port_direction> get_gate_ports() override noexcept
    {
        //TODO die primary inputs / outputs hinzufügen, bei den wires nehm ich alle drei straight wire (muss beide seiten als ein primary input und output markieren),
        // und bei invertern nehm ich die beiden straight inverter (auch wieder beide seiten mit in und out markieren)
        static const gate_ports<port_direction> ports{{
                                                        /** FORME: primary inputs / outputs
                                                        {DIAGONAL_WIRE,
                                                            {
                                                                {
                                                                    {
                                                                        {port_direction(port_direction::cardinal::NORTH_WEST)},
                                                                        {}
                                                                    },
                                                                    {
                                                                        {
                                                                            {port_direction(port_direction::cardinal::NORTH_WEST)},
                                                                            {port_direction(port_direction::cardinal::SOUTH_EAST)}
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        },
                                                        {MIRRORED_DIAGONAL_WIRE,
                                                            {
                                                                {
                                                                    {
                                                                        {port_direction(port_direction::cardinal::NORTH_EAST)},
                                                                        {port_direction(port_direction::cardinal::SOUTH_WEST)}
                                                                    }
                                                                }
                                                            }
                                                        },*/

                                                        /** FORME: samples
                                                        {1in1out,
                                                        {{{{IN},
                                                           {OUT}}}}},

                                                        {2in2out,
                                                        {{{{IN1,
                                                            IN2},
                                                           {OUT1,
                                                            OUT2}}}}},

                                                        {2in1out,
                                                        {{{{IN1,
                                                            IN2},
                                                           {OUT}}}}},

                                                        {1in2out,
                                                        {{{{IN},
                                                           {OUT1,
                                                            OUT2}}}}},*/
                                                        
        }};

         //TODO finish this method and remove the following line
        std::cout << "[e] you called an unfinished method" << std::endl;

        return ports;
    }


  private:
    template <typename Lyt>
    [[nodiscard]] static port_list<port_direction> determine_port_routing(const Lyt& lyt, const tile<Lyt>& t) override noexcept
    {
        port_list<port_direction> ports{};

        /**
         * FORME: Notes about functions
         * is_pi/is_po : "is primary output/input" (if the whole circuit i guess)
         * is_buf = is_wire : ist ein buffer/wire, aka gate das genau das selbe weitergibt 
         */
        
        // determine incoming connector ports
        if (lyt.has_north_eastern_incoming_signal(t))
        {
            p.inp.emplace(port_direction::cardinal::NORTH_EAST);
        }
        if (lyt.has_eastern_incoming_signal(t))
        {
            p.inp.emplace(port_direction::cardinal::EAST);
        }
        if (lyt.has_south_eastern_incoming_signal(t))
        {
            p.inp.emplace(port_direction::cardinal::SOUTH_EAST);
        }
        if (lyt.has_south_western_incoming_signal(t))
        {
            p.inp.emplace(port_direction::cardinal::SOUTH_WEST);
        }
        if (lyt.has_western_incoming_signal(t))
        {
            p.inp.emplace(port_direction::cardinal::WEST);
        }
        if (lyt.has_north_western_incoming_signal(t))
        {
            p.inp.emplace(port_direction::cardinal::NORTH_WEST);
        }

        // determine outgoing connector ports
        if (lyt.has_north_eastern_outgoing_signal(t))
        {
            p.out.emplace(port_direction::cardinal::NORTH_EAST);
        }
        if (lyt.has_eastern_outgoing_signal(t))
        {
            p.out.emplace(port_direction::cardinal::EAST);
        }
        if (lyt.has_south_eastern_outgoing_signal(t))
        {
            p.out.emplace(port_direction::cardinal::SOUTH_EAST);
        }
        if (lyt.has_south_western_outgoing_signal(t))
        {
            p.out.emplace(port_direction::cardinal::SOUTH_WEST);
        }
        if (lyt.has_western_outgoing_signal(t))
        {
            p.out.emplace(port_direction::cardinal::WEST);
        }
        if (lyt.has_north_western_outgoing_signal(t))
        {
            p.out.emplace(port_direction::cardinal::NORTH_WEST);
        }

        return ports; 
    }

    //TODO fill out all the other gates here

    // clang-format off

    // single wires
    //FORME: 01 wire TODO fill out
    static constexpr const fcn_gate NORTH_EAST_EAST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 02 wire
    static constexpr const fcn_gate NORTH_EAST_SOUTH_EAST_WIRE = MIRRORED_STRAIGHT_WIRE;
    //FORME: 03 wire
    static constexpr const fcn_gate NORTH_EAST_SOUTH_WEST_WIRE = MIRRORED_DIAGONAL_WIRE;
    //FORME: 04 wire TODO fill out
    static constexpr const fcn_gate NORTH_EAST_WEST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 05 wire TODO fill out
    static constexpr const fcn_gate NORTH_EAST_NORTH_WEST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 12 wire TODO fill out
    static constexpr const fcn_gate EAST_SOUTH_EAST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 13 wire TODO fill out
    static constexpr const fcn_gate EAST_SOUTH_WEST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 14 wire TODO fill out
    static constexpr const fcn_gate EAST_WEST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 15 wire
    static constexpr const fcn_gate EAST_NORTH_WEST_WIRE{mirror_horizontal(NORTH_EAST_WEST_WIRE)};
    //FORME: 23 wire TODO fill out
    static constexpr const fcn_gate SOUTH_EAST_SOUTH_WEST_WIRE{cell_list_to_gate<char>({{{/*Need to do by hand*/}}})};
    //FORME: 24 wire 
    static constexpr const fcn_gate SOUTH_EAST_WEST_WIRE{mirror_horizontal(EAST_SOUTH_WEST_WIRE)};
    //FORME: 25 wire
    static constexpr const fcn_gate SOUTH_EAST_NORTH_WEST_WIRE{mirror_horizontal(NORTH_EAST_SOUTH_WEST_WIRE)};
    //FORME: 34 wire
    static constexpr const fcn_gate SOUTH_WEST_WEST_WIRE{mirror_horizontal(EAST_SOUTH_EAST_WIRE)};
    //FORME: 35 wire
    static constexpr const fcn_gate SOUTH_WEST_NORTH_WEST_WIRE{mirror_horizontal(NORTH_EAST_SOUTH_EAST_WIRE)};
    //FORME: 45 wire
    static constexpr const fcn_gate WEST_NORTH_WEST_WIRE{mirror_horizontal(NORTH_EAST_EAST_WIRE)};

    // double wires (one straight, one bend)
    //FORME: 14_23 wire
    static constexpr const fcn_gate EAST_WEST_AND_SOUTH_EAST_SOUTH_WEST_WIRE = merge({EAST_WEST_WIRE, SOUTH_EAST_SOUTH_WEST_WIRE});
    //FORME: 03_45 wire
    static constexpr const fcn_gate NORTH_EAST_SOUTH_WEST_AND_WEST_NORTH_WEST_WIRE = merge({NORTH_EAST_SOUTH_WEST_WIRE, WEST_NORTH_WEST_WIRE});
    //FORME: 14_05 wire
    static constexpr const fcn_gate EAST_WEST_AND_NORTH_EAST_NORTH_WEST_WIRE = merge({EAST_WEST_WIRE, NORTH_EAST_NORTH_WEST_WIRE});
    //FORME: 03_12 wire
    static constexpr const fcn_gate NORTH_EAST_SOUTH_WEST_AND_EAST_SOUTH_EAST_WIRE = merge({NORTH_EAST_SOUTH_WEST_WIRE, EAST_SOUTH_EAST_WIRE});

    // double wires (both bend)
    //FORME: 12_34 wire
    static constexpr const fcn_gate EAST_SOUTH_EAST_AND_SOUTH_WEST_WEST_WIRE = merge({EAST_SOUTH_EAST_WIRE, SOUTH_WEST_WEST_WIRE});
    //FORME: 34_05 wire
    static constexpr const fcn_gate SOUTH_WEST_WEST_AND_NORTH_EAST_NORTH_WEST_WIRE = merge({SOUTH_WEST_WEST_WIRE, NORTH_EAST_NORTH_WEST_WIRE});
    //FORME: 45_01 wire
    static constexpr const fcn_gate WEST_NORTH_WEST_AND_NORTH_EAST_EAST_WIRE = merge({WEST_NORTH_WEST_WIRE, NORTH_EAST_EAST_WIRE});
    //FORME: 01_23 wire
    static constexpr const fcn_gate NORTH_EAST_EAST_AND_SOUTH_EAST_SOUTH_WEST_WIRE = merge({NORTH_EAST_EAST_WIRE, SOUTH_EAST_SOUTH_WEST_WIRE});

    // clang-format on

    using port_gate_map = phmap::flat_hash_map<port_list<port_direction>, fcn_gate>;
    using double_port_gate_map =
        phmap::flat_hash_map<std::pair<port_list<port_direction>, port_list<port_direction>>, fcn_gate>;
    
    /**
     * Lookup table for wires. Maps ports to corresponding wires.
     */
    static inline const port_gate_map SINGLE_WIRE_MAP = {
        // NORTH_EAST_EAST_WIRE
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::EAST)}},
         NORTH_EAST_EAST_WIRE},
        {{{port_direction(port_direction::cardinal::EAST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         NORTH_EAST_EAST_WIRE},
        // NORTH_EAST_SOUTH_EAST_WIRE
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_SOUTH_EAST_WIRE},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         NORTH_EAST_SOUTH_EAST_WIRE},
        // NORTH_EAST_SOUTH_WEST_WIRE  //TODO primary input / output hinzufügen (siehe bestagon for refference)
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_SOUTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         NORTH_EAST_SOUTH_WEST_WIRE},
        // NORTH_EAST_WEST_WIRE
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::WEST)}},
         NORTH_EAST_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         NORTH_EAST_WEST_WIRE},
        // NORTH_EAST_NORTH_WEST_WIRE
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         NORTH_EAST_NORTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         NORTH_EAST_NORTH_WEST_WIRE},
        // EAST_SOUTH_EAST_WIRE
        {{{port_direction(port_direction::cardinal::EAST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         EAST_SOUTH_EAST_WIRE},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::EAST)}},
         EAST_SOUTH_EAST_WIRE},
        // EAST_SOUTH_WEST_WIRE
        {{{port_direction(port_direction::cardinal::EAST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         EAST_SOUTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::EAST)}},
         EAST_SOUTH_WEST_WIRE},
        // EAST_WEST_WIRE //TODO primary input / output hinzufügen (siehe bestagon for refference)
        {{{port_direction(port_direction::cardinal::EAST)},
          {port_direction(port_direction::cardinal::WEST)}},
         EAST_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::WEST)},
          {port_direction(port_direction::cardinal::EAST)}},
         EAST_WEST_WIRE},
        // EAST_NORTH_WEST_WIRE
        {{{port_direction(port_direction::cardinal::EAST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         EAST_NORTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::EAST)}},
         EAST_NORTH_WEST_WIRE},
        // SOUTH_EAST_SOUTH_WEST_WIRE
        {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_WIRE},
        // SOUTH_EAST_WEST_WIRE
        {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::WEST)}},
         SOUTH_EAST_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         SOUTH_EAST_WEST_WIRE},
        // SOUTH_EAST_NORTH_WEST_WIRE  //TODO primary input / output hinzufügen (siehe bestagon for refference)
        {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_NORTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         SOUTH_EAST_NORTH_WEST_WIRE},
        // SOUTH_WEST_WEST_WIRE
        {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::WEST)}},
         SOUTH_WEST_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         SOUTH_WEST_WEST_WIRE},
        // SOUTH_WEST_NORTH_WEST_WIRE
        {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_WEST_NORTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         SOUTH_WEST_NORTH_WEST_WIRE},
        // WEST_NORTH_WEST_WIRE
        {{{port_direction(port_direction::cardinal::WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         WEST_NORTH_WEST_WIRE},
        {{{port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::WEST)}},
         WEST_NORTH_WEST_WIRE},
        // empty gate (for crossing layer)
        {{{}, {}}, EMPTY_GATE}};

    /**
     * Lookup table for double wires. Maps ports to corresponding wires.
     * //TODO add the other three directions (see bestagon inverter map for examples)
     */
    static inline const double_port_gate_map DOUBLE_WIRE_MAP = {
        {{{{port_direction(port_direction::cardinal::EAST)},
           {port_direction(port_direction::cardinal::WEST)}},
          {{port_direction(port_direction::cardinal::SOUTH_EAST)},
           {port_direction(port_direction::cardinal::SOUTH_WEST)}}},
         EAST_WEST_AND_SOUTH_EAST_SOUTH_WEST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::NORTH_EAST)},
           {port_direction(port_direction::cardinal::SOUTH_WEST)}},
          {{port_direction(port_direction::cardinal::WEST)},
           {port_direction(port_direction::cardinal::NORTH_WEST)}}},
         NORTH_EAST_SOUTH_WEST_AND_WEST_NORTH_WEST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::EAST)},
           {port_direction(port_direction::cardinal::WEST)}},
          {{port_direction(port_direction::cardinal::NORTH_EAST)},
           {port_direction(port_direction::cardinal::NORTH_WEST)}}},
         EAST_WEST_AND_NORTH_EAST_NORTH_WEST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::NORTH_EAST)},
           {port_direction(port_direction::cardinal::SOUTH_WEST)}},
          {{port_direction(port_direction::cardinal::EAST)},
           {port_direction(port_direction::cardinal::SOUTH_EAST)}}},
         NORTH_EAST_SOUTH_WEST_AND_EAST_SOUTH_EAST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::EAST)},
           {port_direction(port_direction::cardinal::SOUTH_EAST)}},
          {{port_direction(port_direction::cardinal::SOUTH_WEST)},
           {port_direction(port_direction::cardinal::WEST)}}},
         EAST_SOUTH_EAST_AND_SOUTH_WEST_WEST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::SOUTH_WEST)},
           {port_direction(port_direction::cardinal::WEST)}},
          {{port_direction(port_direction::cardinal::NORTH_EAST)},
           {port_direction(port_direction::cardinal::NORTH_WEST)}}},
         SOUTH_WEST_WEST_AND_NORTH_EAST_NORTH_WEST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::WEST)},
           {port_direction(port_direction::cardinal::NORTH_WEST)}},
          {{port_direction(port_direction::cardinal::NORTH_EAST)},
           {port_direction(port_direction::cardinal::EAST)}}},
         WEST_NORTH_WEST_AND_NORTH_EAST_EAST_WIRE
        },
        {{{{port_direction(port_direction::cardinal::NORTH_EAST)},
           {port_direction(port_direction::cardinal::EAST)}},
          {{port_direction(port_direction::cardinal::SOUTH_EAST)},
           {port_direction(port_direction::cardinal::SOUTH_WEST)}}},
         NORTH_EAST_EAST_AND_SOUTH_EAST_SOUTH_WEST_WIRE
        }};

    /**
     * Lookup table for fanouts.  Maps ports to corresponding wires.
     */
    static inline const port_gate_map FANOUT_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_TO_SOUTH_EAST_SOUTH_WEST_FANOUT},
        {{{port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_WEST_TO_SOUTH_EAST_SOUTH_WEST_FANOUT},
         {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_TO_NORTH_EAST_NORTH_WEST_FANOUT},
          {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_WEST_TO_NORTH_EAST_NORTH_WEST_FANOUT}};

    /**
     * Lookup table for NOT gates. Maps ports to corresponding NOT gates. //TODO primary input / output hinzufügen (siehe bestagon for refference)
     * //TODO add the other direction (because they can be used bidirectionaly)
     */
    static inline const port_gate_map NOT_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_SOUTH_EAST_NOT},
        {{{port_direction(port_direction::cardinal::NORTH_EAST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_SOUTH_WEST_NOT},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_NORTH_WEST_NOT},
        {{{port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_WEST_NORTH_WEST_NOT}};
    /**
     * Lookup table for AND gates. Maps ports to corresponding AND gates.
     */
    static inline const port_gate_map AND_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_AND},
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_AND},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_AND},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_AND}};
    /**
     * Lookup table for OR gates. Maps ports to corresponding OR gates.
     */
    static inline const port_gate_map OR_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_OR},
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_OR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_OR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_OR}};
    /**
     * Lookup table for NAND gates. Maps ports to corresponding NAND gates.
     */
    static inline const port_gate_map NAND_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_NAND},
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_NAND},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_NAND},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_NAND}};
    /**
     * Lookup table for NOR gates. Maps ports to corresponding NOR gates.
     */
    static inline const port_gate_map NOR_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_NOR},
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_NOR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_NOR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_NOR}};
    /**
     * Lookup table for XOR gates. Maps ports to corresponding XOR gates.
     */
    static inline const port_gate_map XOR_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_XOR},
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_XOR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_XOR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_XOR}};
    /**
     * Lookup table for XNOR gates. Maps ports to corresponding XNOR gates.
     */
    static inline const port_gate_map XNOR_MAP = {
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_EAST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_EAST_XNOR},
        {{{port_direction(port_direction::cardinal::NORTH_EAST), port_direction(port_direction::cardinal::NORTH_WEST)},
          {port_direction(port_direction::cardinal::SOUTH_WEST)}},
         NORTH_EAST_NORTH_WEST_TO_SOUTH_WEST_XNOR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_EAST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_EAST_XNOR},
        {{{port_direction(port_direction::cardinal::SOUTH_EAST), port_direction(port_direction::cardinal::SOUTH_WEST)},
          {port_direction(port_direction::cardinal::NORTH_WEST)}},
         SOUTH_EAST_SOUTH_WEST_TO_NORTH_WEST_XNOR}};
};

}  // namespace fiction

#endif  // FICTION_SIDB_HEX_WIRES_LIBRARY_HPP