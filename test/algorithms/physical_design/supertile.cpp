//
// Created by Felix Kiefhaber on 05.02.2025
//

#include <catch2/catch_test_macros.hpp>

#include "utils/blueprints/layout_blueprints.hpp"
#include "utils/equivalence_checking_utils.hpp"
#include "fiction/types.hpp"

#include <fiction/algorithms/verification/equivalence_checking.hpp>
#include <fiction/algorithms/physical_design/supertile.hpp>

using namespace fiction;

void check_mapping_equiv_layout(const hex_even_row_gate_clk_lyt& lyt)
{
    const auto super_layout = supertilezation<hex_even_row_gate_clk_lyt_disrespect_clocking, hex_even_row_gate_clk_lyt>(lyt);
    check_eq<hex_even_row_gate_clk_lyt, hex_even_row_gate_clk_lyt_disrespect_clocking, true>(lyt, super_layout);
    CHECK(lyt.get_layout_name() == super_layout.get_layout_name()); 
}

TEST_CASE("Layout equivalence", "[supertilezation]")
{
    check_mapping_equiv_layout(blueprints::row_clocked_and_xor_gate_layout<hex_even_row_gate_clk_lyt>());
    check_mapping_equiv_layout(blueprints::amy_clocked_path_balanced_and_xor_gate_layout<hex_even_row_gate_clk_lyt>());
}