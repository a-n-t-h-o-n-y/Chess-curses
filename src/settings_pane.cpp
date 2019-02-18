#include "settings_pane.hpp"

#include <cppurses/painter/color.hpp>
#include <cppurses/widget/border.hpp>

using namespace cppurses;

Settings_pane::Settings_pane() {
    this->set_name("Settings_pane");
    this->brush.set_background(Color::White);
    this->brush.set_foreground(Color::Blue);
    for (auto& descendant : this->children.get_descendants()) {
        descendant->brush.set_background(Color::White);
        descendant->brush.set_foreground(Color::Blue);
    }
    return_btn.brush.set_background(Color::Blue);
    return_btn.brush.set_foreground(Color::White);

    this->border.enable();
    this->border.segments.north_east = L'─';
    this->border.segments.north_west = L'╭';
    this->border.segments.south_west = L'╰';
    this->border.segments.south.disable();
    this->border.segments.east.disable();
    this->border.segments.south_east.disable();

    show_moves_box.set_name("show_move_box in Settings_pane");
    hide_log_box.set_name("hide_log_box in Settings_pane");
    black_ai.set_name("black_ai in Settings_pane");
    white_ai.set_name("white_ai in Settings_pane");
    ruleset.set_name("ruleset in Settings_pane");
    reset_btn.set_name("reset_btn in Settings_pane");
    return_btn.set_name("return_btn in Settings_pane");
}
