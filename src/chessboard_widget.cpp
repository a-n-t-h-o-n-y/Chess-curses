#include "chessboard_widget.hpp"

#include <algorithm>
#include <iterator>
#include <mutex>

#include <signals_light/signal.hpp>

#include <cppurses/painter/color.hpp>
#include <cppurses/painter/glyph_string.hpp>
#include <cppurses/painter/painter.hpp>
#include <cppurses/system/mouse.hpp>
#include <cppurses/system/system.hpp>
#include <cppurses/widget/detail/link_lifetimes.hpp>

#include "chess_move_request_event.hpp"
#include "figure.hpp"
#include "move.hpp"
#include "shared_user_input.hpp"
#include "side.hpp"

using namespace cppurses;
using namespace chess;

namespace {

Glyph piece_to_glyph(Piece piece)
{
    Glyph symbol;
    // Character
    if (piece.figure == Figure::Bishop) {
        symbol = L'♝';
    }
    else if (piece.figure == Figure::King) {
        symbol = L'♚';
    }
    else if (piece.figure == Figure::Knight) {
        symbol = L'♞';
    }
    else if (piece.figure == Figure::Pawn) {
        symbol = L'♟';
    }
    else if (piece.figure == Figure::Queen) {
        symbol = L'♛';
    }
    else if (piece.figure == Figure::Rook) {
        symbol = L'♜';
    }
    // Side
    if (piece.side == Side::Black) {
        symbol.brush.set_foreground(cppurses::Color::Black);
    }
    else if (piece.side == Side::White) {
        symbol.brush.set_foreground(cppurses::Color::White);
    }
    return symbol;
}

Position board_to_screen_position(Position board_position)
{
    int y = 8 - board_position.row;
    int x = 1 + (board_position.column - 1) * 3;
    return Position{x, y};
}

Position screen_to_board_position(Position screen_position)
{
    int row    = 8 - screen_position.column;
    int column = (screen_position.row / 3) + 1;
    return Position{row, column};
}
}  // namespace

Chessboard_widget::Chessboard_widget()
{
    this->height_policy.fixed(8);
    this->width_policy.fixed(24);

    engine_.move_made.connect([this](Move m) { this->move_made(m); });
    engine_.move_made.connect([this](Move) { this->update(); });
    engine_.capture.connect([this](Piece p) { this->capture(p); });
    engine_.invalid_move.connect(
        [this](const Move& m) { this->invalid_move(m); });
    engine_.checkmate.connect([this](Side s) { this->checkmate(s); });
    engine_.check.connect([this](Side s) { this->check(s); });
    engine_.state().board_reset.connect([this] { this->board_reset(); });
}

void Chessboard_widget::toggle_show_moves()
{
    show_moves_ = !show_moves_;
    this->update();
}

void Chessboard_widget::reset_game()
{
    engine_.state().reset();
    this->update();
}

void Chessboard_widget::make_move(const Move& move) { engine_.make_move(move); }

Side Chessboard_widget::current_side() const
{
    return engine_.state().current_side;
}

void Chessboard_widget::exit_game_loop()
{
    Shared_user_input::exit_requested = true;
    game_loop_.exit(0);
    game_loop_.wait();
}

void Chessboard_widget::pause() { this->exit_game_loop(); }

void Chessboard_widget::start()
{
    Shared_user_input::exit_requested = false;
    game_loop_.wait();
    game_loop_.run_async();
}

void Chessboard_widget::take_turn()
{
    Move m;
    try {
        if (engine_.state().current_side == Side::Black) {
            m = engine_.player_black()->get_move();
        }
        else {
            m = engine_.player_white()->get_move();
        }
    }
    catch (Chess_loop_exit_request e) {
        this->exit_game_loop();
        return;
    }
    System::post_event(chess_move_request_event(*this, m));
}

void Chessboard_widget::move_request_event(Move m) { engine_.make_move(m); }

Chess_engine& Chessboard_widget::engine() { return engine_; }

const Chess_engine& Chessboard_widget::engine() const { return engine_; }

namespace slot {

using cppurses::slot::link_lifetimes;

auto toggle_show_moves(Chessboard_widget& cbw) -> sl::Slot<void()>
{
    return link_lifetimes([&cbw] { cbw.toggle_show_moves(); }, cbw);
}

auto reset_game(Chessboard_widget& cbw) -> sl::Slot<void()>
{
    return link_lifetimes([&cbw] { cbw.reset_game(); }, cbw);
}

auto make_move(Chessboard_widget& cbw) -> sl::Slot<void(Move)>
{
    return link_lifetimes([&cbw](Move m) { cbw.make_move(m); }, cbw);
}

}  // namespace slot

bool Chessboard_widget::paint_event()
{
    auto const cell1 = Glyph_string{"   ", bg(cppurses::Color::Light_gray)};
    auto const cell2 = Glyph_string{"   ", bg(cppurses::Color::Dark_blue)};

    // Checkerboard
    auto p = Painter{*this};
    for (auto i = 0; i < 4; ++i) {
        p.put(cell1 + cell2 + cell1 + cell2 + cell1 + cell2 + cell1 + cell2, 0,
              i * 2);
        p.put(cell2 + cell1 + cell2 + cell1 + cell2 + cell1 + cell2 + cell1, 0,
              i * 2 + 1);
    }

    // Valid Moves
    chess::State const& state = engine_.state();
    if (show_moves_ && selected_position_.has_value() &&
        state.board.has_piece_at(*selected_position_) &&
        state.board.at(*selected_position_).side == state.current_side) {
        auto const valid_moves =
            engine_.get_valid_positions(*selected_position_);
        auto const highlight =
            Glyph_string{"   ", bg(cppurses::Color::Light_green)};
        for (Position possible_position : valid_moves) {
            Position const where = board_to_screen_position(possible_position);
            p.put(highlight, where.row - 1, where.column);
        }
    }

    {
        auto const lock =
            std::lock_guard<std::recursive_mutex>{state.board.mtx};
        for (auto& pos_piece : state.board.pieces) {
            auto const piece_position = pos_piece.first;
            auto piece_visual         = piece_to_glyph(pos_piece.second);
            piece_visual.brush.set_background(get_tile_color(piece_position));
            // TODO change to be Point.
            auto const where = board_to_screen_position(piece_position);
            p.put(piece_visual, where.row, where.column);
        }
    }
    return Widget::paint_event();
}

bool Chessboard_widget::mouse_press_event(const Mouse& m)
{
    int loc_x = static_cast<int>(m.local.x);
    int loc_y = static_cast<int>(m.local.y);
    Position clicked_pos{screen_to_board_position(Position{loc_x, loc_y})};
    selected_position_ = clicked_pos;

    const chess::State& state{engine_.state()};
    if (state.board.has_piece_at(clicked_pos) and
        state.board.at(clicked_pos).side == state.current_side) {
        first_position_ = clicked_pos;
    }
    else if (first_position_.has_value()) {
        Shared_user_input::move.set(Move{*first_position_, clicked_pos});
        first_position_    = std::nullopt;
        selected_position_ = std::nullopt;
    }
    this->update();
    return Widget::mouse_press_event(m);
}

bool Chessboard_widget::enable_event()
{
    this->start();
    return Widget::enable_event();
}

bool Chessboard_widget::disable_event()
{
    this->pause();
    return Widget::disable_event();
}

cppurses::Color Chessboard_widget::get_tile_color(Position p)
{
    const State& state{engine_.state()};
    if (show_moves_ and selected_position_.has_value() and
        state.board.has_piece_at(*selected_position_) and
        state.board.at(*selected_position_).side == state.current_side) {
        auto valid_moves = engine_.get_valid_positions(*selected_position_);
        auto at = std::find(std::begin(valid_moves), std::end(valid_moves), p);
        if (at != std::end(valid_moves)) {
            return cppurses::Color::Light_green;
        }
    }
    if (p.row % 2 == 0) {
        if (p.column % 2 == 0) {
            return cppurses::Color::Dark_blue;
        }
        return cppurses::Color::Light_gray;
    }
    if (p.row % 2 != 0) {
        if (p.column % 2 == 0) {
            return cppurses::Color::Light_gray;
        }
        return cppurses::Color::Dark_blue;
    }
    return cppurses::Color::Orange;
}
