//
// Created by marcel on 26.07.18.
//

#ifndef FICTION_FCN_GATE_LIBRARY_HPP
#define FICTION_FCN_GATE_LIBRARY_HPP

#include "fiction/layouts/coordinates.hpp"
#include "fiction/technology/cell_ports.hpp"
#include "fiction/technology/cell_technologies.hpp"
#include "fiction/utils/array_utils.hpp"
#include "fiction/utils/hash.hpp"

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>

#include <array>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <vector>

namespace fiction
{

/**
 * Exception to be thrown when a layout hosts a gate type that is not implemented in the applied gate library.
 *
 * @tparam CoordinateType Type of the layout coordinates.
 */
template <typename CoordinateType>
class unsupported_gate_type_exception : public std::exception
{
  public:
    explicit unsupported_gate_type_exception(const CoordinateType& c) noexcept : std::exception(), coord{c} {}

    [[nodiscard]] CoordinateType where() const noexcept
    {
        return coord;
    }

  private:
    const CoordinateType coord;
};
/**
 * Exception to be thrown when a layout hosts a gate with an orientation that is unsupported by the applied gate
 * library.
 *
 * @tparam CoordinateType Type of the layout coordinates.
 * @tparam PortType Type of the library ports.
 */
template <typename CoordinateType, typename PortType>
class unsupported_gate_orientation_exception : public std::exception
{
  public:
    unsupported_gate_orientation_exception(const CoordinateType& c, const port_list<PortType>& p) noexcept :
            std::exception(),
            coord{c},
            ports{p}
    {}

    [[nodiscard]] CoordinateType where() const noexcept
    {
        return coord;
    }

    [[nodiscard]] port_list<PortType> which_ports() const noexcept
    {
        return ports;
    }

  private:
    const CoordinateType      coord;
    const port_list<PortType> ports;
};

/**
 * Base class for various FCN libraries used to map gate-level layouts to cell-level ones. Any new gate library can
 * extend `fcn_gate_library` if it benefits from its features but does not have to. The only requirement is that it must
 * be a static class that provides a
 *
   \verbatim embed:rst
   .. code-block:: c++

      template <typename GateLyt>
      static fcn_gate set_up_gate(const GateLyt& lyt, const tile<GateLyt>& t)

   \endverbatim
 *
 * public member function. Additionally, a
 *
   \verbatim embed:rst
   .. code-block:: c++

      template <typename CellLyt>
      static void post_layout_optimization(CellLyt& lyt)
   \endverbatim
 *
 * can optionally be provided if some cleanup or optimization is necessary on the cell-level layout after each gate has
 * been mapped.
 *
 * Additionally, a
 *
   \verbatim embed:rst
   .. code-block:: c++

      static gate_functions get_functional_implementations()
   \endverbatim
 *
 * can optionally be provided to allow reverse access to the gate structures by functional implementation. This
 * interface is for example used to determine which gate types to blacklist on tiles in P&R algorithms by considering
 * gate implementation.
 *
 * Finally, a
 *
   \verbatim embed:rst
   .. code-block:: c++

      static gate_ports<PortType> get_gate_ports()
   \endverbatim
 *
 * can optionally be provided to allow reverse access to the gate ports given a gate implementation. This interface is
 * for example used in `sidb_surface_analysis` to determine which ports to blacklist.
 *
 * @tparam Technology FCN technology type of the implementing gate library.
 * @tparam GateSizeX Tile size in x-dimension.
 * @tparam GateSizeY Tile size in y-dimension.
 */
template <typename Technology, uint16_t GateSizeX, uint16_t GateSizeY>
class fcn_gate_library
{
  public:
    using technology = Technology;

    /**
     * A `cell_list` is an array of size `GateSizeX` \f$\times\f$ `GateSizeY` of type `T`.
     */
    template <typename T>
    using cell_list = std::array<std::array<T, GateSizeX>, GateSizeY>;
    /**
     * Each gate is thus a `cell_list` of cell types defined in `Technology`.
     */
    using fcn_gate = cell_list<typename Technology::cell_type>;
    /**
     * Maps truth tables to respective FCN gate implementations.
     */
    using gate_functions =
        std::unordered_map<kitty::dynamic_truth_table, std::vector<fcn_gate>, kitty::hash<kitty::dynamic_truth_table>>;
    /**
     * Maps FCN gate implementations to respective port lists indicating their possible orientations.
     */
    template <typename PortType>
    using gate_ports = std::unordered_map<fcn_gate, std::vector<port_list<PortType>>>;
    /**
     * Gate libraries should not be instantiated but used as static objects.
     */
    explicit fcn_gate_library() = delete;
    /**
     * Converts a `cell_list` of type `T` to an `fcn_gate` at compile time. This function allows to conveniently
     * specify `fcn_gate` instances in a semi-readable way in code. For examples usages see `qca_one_library.hpp`.
     *
     * @tparam T Element type of given `cell_list`.
     * @param c Cell list to convert.
     * @return An `fcn_gate` created from the representation provided in `c`.
     */
    template <typename T>
    static constexpr fcn_gate cell_list_to_gate(const cell_list<T>& c) noexcept
    {
        return convert_array_of_arrays<typename Technology::cell_type, T, GateSizeY, GateSizeX>(c);
    }
    /**
     * Rotates the given `fcn_gate` by 90° clockwise at compile time.
     *
     * @param g `fcn_gate` to rotate.
     * @return Rotated `fcn_gate`.
     */
    static constexpr fcn_gate rotate_90(const fcn_gate& g) noexcept
    {
        return reverse_columns(transpose(g));
    }
    /**
     * Rotates the given `fcn_gate` by 180° at compile time.
     *
     * @param g `fcn_gate` to rotate.
     * @return Rotated `fcn_gate`.
     */
    static constexpr fcn_gate rotate_180(const fcn_gate& g) noexcept
    {
        return reverse_columns(reverse_rows(g));
    }
    /**
     * Rotates the given `fcn_gate` by 270° clockwise at compile time.
     *
     * @param g `fcn_gate` to rotate.
     * @return Rotated `fcn_gate`.
     */
    static constexpr fcn_gate rotate_270(const fcn_gate& g) noexcept
    {
        return transpose(reverse_columns(g));
    }
    /**
     * Mirrors the given 'fcn_gate' around vertical axis at compile time.
     * After mirroring it shifts each value one position to the left.
     * This is because it's build to mirror SiDB gates, which are stored in a 60x50 matrix, but they have a central column that shouldn't move.
     * 
     * @param g `fcn_gate` to mirror.
     * @return Mirrored `fcn_gate`.
     */
    static constexpr fcn_gate mirror_horizontal(const fcn_gate& g) noexcept
    {
        return shift_left_once(reverse_columns_at_compiletime(g));
    }
    /**
     * Merges multiple `fcn_gate`s into one at compile time. This is intended to be used for wires. Unexpected behavior
     * can be caused, if more than one `fcn_gate` has a cell at the same position.
     *
     * @param gates Vector of gates to be merged.
     * @return Merged `fcn_gate`.
     */
    static constexpr fcn_gate merge(const std::vector<fcn_gate>& gates) noexcept
    {
        auto merged = EMPTY_GATE;

        for (auto x = 0ul; x < GateSizeX; ++x)
        {
            for (auto y = 0ul; y < GateSizeY; ++y)
            {
                for (const auto& g : gates)
                {
                    if (!Technology::is_empty_cell(g[x][y]))
                    {
                        merged[x][y] = g[x][y];
                    }
                }
            }
        }

        return merged;
    }
    /**
     * Since the `std::vector` used in `merge()` is a `non-literal` but I need it to be,
     * this does the exact same as `merge()`, just actually at compile time. (by using `std::array`)
     * Also x and y are swappend in this method because the SiDB gates are constructed in such a way that each row is an array, instead of each array, which is how this class intends it to be.
     * TODO -> One could implement a method that changes the way a gate is stored (from row arrays in a single column array to column array stored in a single row array) so that all the methods here are usable again. (method should be true `constexpr`!)
     * 
     * Merges two `fcn_gate`s into one at compile time. This is intended to be used for wires. Unexpected behavior
     * can be caused, if more than one `fcn_gate` has a cell at the same position.
     *
     * @param gates Array of gates to be merged.
     * @return Merged `fcn_gate`.
     */
    static constexpr fcn_gate merge_at_compiletime(const std::array<fcn_gate, 2>& gates) noexcept
    {
        auto merged = EMPTY_GATE;

        for (auto x = 0ul; x < GateSizeX; x++)
        {
            for (auto y = 0ul; y < GateSizeY; y++)
            {
                for (const auto& g : gates)
                {
                    if (!Technology::is_empty_cell(g[y][x]))
                    {
                        merged[y][x] = g[y][x];
                    }
                }
            }
        }

        return merged;
    }
    /**
     * Applies given mark to given `fcn_gate` `g` at given port `p` at compile time.
     *
     * @param g Gate to apply mark to.
     * @param p Port specifying where to apply the mark.
     * @param mark Mark to be applied
     * @return Marked `fcn_gate`.
     */
    template <typename Port>
    static constexpr fcn_gate mark_cell(const fcn_gate& g, const Port& p,
                                        const typename Technology::cell_mark& mark) noexcept
    {
        auto marked_gate = g;

        marked_gate[p.y][p.x] = static_cast<typename Technology::cell_type>(mark);

        return marked_gate;
    }
    /**
     * Returns horizontal size of gate blocks.
     *
     * @return `GateSizeX`.
     */
    [[nodiscard]] static constexpr uint16_t gate_x_size() noexcept
    {
        return GateSizeX;
    }
    /**
     * Returns vertical size of gate blocks.
     *
     * @return `GateSizeY`.
     */
    [[nodiscard]] static constexpr uint16_t gate_y_size() noexcept
    {
        return GateSizeY;
    }

  protected:
    /**
     * Transposes the given `fcn_gate` at compile time.
     *
     * @param g `fcn_gate` to transpose.
     * @return Transposed `fcn_gate`.
     */
    static constexpr fcn_gate transpose(const fcn_gate& g) noexcept
    {
        auto trans = EMPTY_GATE;

        for (auto x = 0ul; x < GateSizeX; ++x)
        {
            for (auto y = 0ul; y < GateSizeY; ++y)
            {
                trans[y][x] = g[x][y];
            }
        }

        return trans;
    }
    /**
     * Reverses the columns of the given `fcn_gate` at compile time.
     *
     * @param g `fcn_gate` whose columns are to be reversed.
     * @return `fcn_gate` with reversed columns.
     */
    static constexpr fcn_gate reverse_columns(const fcn_gate& g) noexcept
    {
        fcn_gate rev_cols = g;

        std::for_each(std::begin(rev_cols), std::end(rev_cols),
                      [](auto& i) { std::reverse(std::begin(i), std::end(i)); });

        return rev_cols;
    }
    /**
     * Since the `std::reverse` and `std::for_each` method used in `reverse_columns()` isn't a `constexpr` in the C++ version used here (17) and I need it to be (would be in 20),
     * this does the exact same as `reverse_columns()`, just actually at compile time.
     * Also x and y are swappend in this method because the SiDB gates are constructed in such a way that each row is an array, instead of each column, which is not how this class intends it to be.
     * TODO -> One could implement a method that changes the way a gate is stored (from row arrays in a single column array to column array stored in a single row array) so that all the methods here are usable again. (method should be true `constexpr`!)
     * 
     * Reverses the columns of the given `fcn_gate` at compile time.
     *
     * @param g `fcn_gate` whose columns are to be reversed.
     * @return `fcn_gate` with reversed columns.
     */
    static constexpr fcn_gate reverse_columns_at_compiletime(const fcn_gate& g) noexcept
    {
        fcn_gate rev_cols = g;

        for (auto y = 0ul; y < GateSizeY; y++)
        {
            for (auto x = 0ul; x < GateSizeX / 2; x++)
            {
                auto temp = rev_cols[y][x];
                rev_cols[y][x] = rev_cols[y][GateSizeX-1-x];
                rev_cols[y][GateSizeX-1-x] = temp;
            }
        }

        return rev_cols;
    }
    /**
     * Reverses the rows of the given `fcn_gate` at compile time.
     *
     * @param g `fcn_gate` whose rows are to be reversed.
     * @return `fcn_gate` with reversed rows.
     */
    static constexpr fcn_gate reverse_rows(const fcn_gate& g) noexcept
    {
        fcn_gate rev_rows = g;

        std::reverse(std::begin(rev_rows), std::end(rev_rows));

        return rev_rows;
    }
    /**
     * Shifts each value of the given `fcn_gate` at compile time one to the left, overflow is discarded and new places are filled with `cell_type::EMPTY`.
     * This method was created for SiDB gates, so the x and y coordinate are swapped, since the SiDB gates are constructed in such a way that each row is an array,
     * instead of each column, which is not how this class intends it to be.
     * 
     * @param `fcn_gate` to shift.
     * @return Shifted `fcn_gate`.
     */
    static constexpr fcn_gate shift_left_once(const fcn_gate& g) noexcept
    {
        auto shifted = EMPTY_GATE;

        for (auto y = 0ul; y < GateSizeY; y++)
        {
            for (auto x = 1ul; x < GateSizeX; x++)
            {
                shifted[y][x-1] = g[y][x-1];
            }
            shifted[y][GateSizeX - 1] = Technology::cell_type::EMPTY;
        }

        return shifted;
    }
    /**
     * Single empty gate in given technology and tile size. Used as a blue print to create new ones in merge and
     * transpose for example.
     */
    static constexpr const fcn_gate EMPTY_GATE =
        fiction::create_array<GateSizeY>(fiction::create_array<GateSizeX>(Technology::cell_type::EMPTY));
};

}  // namespace fiction

#endif  // FICTION_FCN_GATE_LIBRARY_HPP
