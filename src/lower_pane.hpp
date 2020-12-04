#ifndef LOWER_PANE_HPP
#define LOWER_PANE_HPP
#include <signals_light/signal.hpp>

#include <cppurses/painter/color.hpp>
#include <cppurses/painter/trait.hpp>
#include <cppurses/widget/layouts/horizontal.hpp>
#include <cppurses/widget/tuple.hpp>
#include <cppurses/widget/widgets/button.hpp>
#include <cppurses/widget/widgets/status_bar.hpp>
#include <cppurses/widget/widgets/tile.hpp>

#include "chessboard_widget.hpp"
#include "move_input.hpp"

using Status = cppurses::
    Tuple<cppurses::layout::Horizontal<>, cppurses::Tile, cppurses::Status_bar>;

class Lower_pane : public cppurses::layout::Horizontal<> {
   public:
    Status& status = this->make_child<Status>();

    Move_input& move_input = this->make_child<Move_input>("Type Move");

    cppurses::Button& settings_btn =
        this->make_child<cppurses::Button>("Settings");

   public:
    Lower_pane();

    void toggle_status(const Chessboard_widget& board);
};

namespace slot {

auto toggle_status(Lower_pane& lp, const Chessboard_widget& board)
    -> sl::Slot<void(Move)>;

}  // namespace slot
#endif  // LOWER_PANE_HPP
