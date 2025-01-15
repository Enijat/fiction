//
// Created by Felix Kiefhaber on 27.04.2024
//

#ifndef FICTION_CMD_SUPERTILE_HPP
#define FICTION_CMD_SUPERTILE_HPP

#include <fiction/algorithms/physical_design/supertile.hpp>
#include <fiction/traits.hpp>

#include <alice/alice.hpp>

namespace alice
{

/**
 * Transforms a hexagonal layout by TODO gescheide Erklärung finden
 */
class supertile_command : public command
{
  public:
    /**
     * Standard constructor. Adds descriptive information, options, and flags.
     * 
     * @param e alice::environment that specifies stores etc.
     */
    explicit supertile_command(const environment::ptr& e) :
            command(e, "Does super stuff") //TODO hier gescheiden text und dabei auf benennung achten (also layout vs tiles vs gates, etc.)
    {}

  protected:
    /**
     * Fucntion to transform a gexagonal layout by ... TODO hier Erklärung von oben kopieren
     */
    void execute() override
    {
        auto& gls = store<fiction::gate_layout_t>();

        // error case: empty gate-level layout store
        if (gls.empty())
        {
            env->out() << "[w] no gate layout in store" << std::endl;
            return;
        }

        const auto& lyt = gls.current();

        const auto check_clocking_scheme_pattern = [](auto&& lyt_ptr)
        { return lyt_ptr->is_clocking_scheme(fiction::clock_name::AMY); };

        // error case: layout is not AMY-clocked
        if (const auto is_amy_clocked = std::visit(check_clocking_scheme_pattern, lyt); !is_amy_clocked)
        {
            env->out() << "[e] layout has to be AMY-clocked" << std::endl;
            return;
        }

        const auto apply_supertilezation = [](auto&& lyt_ptr) -> std::optional<fiction::hex_even_row_gate_clk_lyt>
        {
            using Lyt = typename std::decay_t<decltype(lyt_ptr)>::element_type;

            if constexpr (fiction::is_hexagonal_layout_v<Lyt>)
            {
                return fiction::supertilezation<fiction::hex_even_row_gate_clk_lyt>(*lyt_ptr);
            }
            else
            {
                std::cout << "[e] layout has to be Hexagonal" << std::endl;
            }

            return std::nullopt;
        };

        try
        {
            if (const auto hex_lyt = std::visit(apply_supertilezation, lyt); hex_lyt.has_value())
            {
                gls.extend() = std::make_shared<fiction::hex_even_row_gate_clk_lyt>(*hex_lyt);
            }
        }
        catch (...)
        {
            env->out() << "[e] an error occurred while transforming" << std::endl;
        }
    }
};

ALICE_ADD_COMMAND(supertile, "Physical Design")

}  // namespace alice

#endif  // FICTION_CMD_SUPERTILE_HPP
