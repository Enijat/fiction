//
// Created by Felix Kiefhaber on 27.04.2024
//

#ifndef FICTION_CMD_SUPERTILE_HPP
#define FICTION_CMD_SUPERTILE_HPP

#include <fiction/algorithms/physical_design/supertile.hpp>

#include <alice/alice.hpp>

namespace alice //FRAGE: wieso alice hier?
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
            command(e, "Transforms a hexagonal ...") //TODO hier gescheiden text und dabei auf benennung achten (also layout vs tiles vs gates, etc.)
    {}

  protected:
    /**
     * Fucntion to transform a gexagonal layout by ... TODO hier Erklärung von oben kopieren
     */
    void execute() override
    {
        //TODO Implement actual logic here
        env->out() << "There is currently no logic here :/" << std::endl;
    }
};

ALICE_ADD_COMMAND(supertile, "Physical Design")

}  // namespace alice

#endif  // FICTION_CMD_SUPERTILE_HPP
