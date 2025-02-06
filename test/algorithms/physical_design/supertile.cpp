//
// Created by Felix Kiefhaber on 05.02.2025
//

#include <catch2/catch_test_macros.hpp>

#include "utils/blueprints/layout_blueprints.hpp"
#include "utils/equivalence_checking_utils.hpp"

#include <fiction/algorithms/physical_design/supertile.hpp>

using namespace fiction;

template <typename Lyt>
void check_mapping_equiv_layout(const Lyt& lyt)
{
    const auto super_layout = supertilezation<Lyt>(lyt);

    check_eq<Lyt, Lyt, true>(lyt, super_layout);
    //check_eq(lyt, super_layout);
    CHECK(lyt.get_layout_name() == super_layout.get_layout_name());
}

TEST_CASE("Hexagonal to supertile hexagonal")
{
    SECTION("Simple and-xor gate level layout")
    {
        check_mapping_equiv_layout<hex_even_row_gate_clk_lyt>(blueprints::row_clocked_and_xor_gate_layout<hex_even_row_gate_clk_lyt>());
    }
}