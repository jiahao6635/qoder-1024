#include "../worker_powerbank/Game.h"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace worker;

int main() {
  Game game;
  assert(game.mode() == Mode::Demo && game.state() == State::Idle);
  game.tick(1000);
  assert(game.energy() == 100 && !game.shake(1000));
  game.pressA(1000);
  game.tick(11000);
  assert(game.energy() == 50 && game.remainingMs() == 10000);
  game.pressA(11000);
  game.tick(60000);
  assert(game.state() == State::Paused && game.energy() == 50);
  assert(!game.shake(60000));
  game.pressA(60000);
  assert(game.shake(60000));
  assert(game.energy() == 70 && game.remainingMs() == 14000);
  game.tick(73999);
  assert(game.energy() == 1 && game.state() == State::Focus);
  game.tick(74000);
  assert(game.energy() == 0 && game.state() == State::Empty);
  assert(game.takeSound() == Sound::Empty);
  game.tick(90000);
  assert(game.takeSound() == Sound::None && !game.shake(90000));
  game.pressB(90000);
  assert(game.remainingMs() == 10000);
  game.tick(95000);
  assert(game.energy() == 50 && game.remainingMs() == 5000);
  game.tick(99999);
  assert(!game.isFull() && game.remainingMs() == 1);
  game.tick(100000);
  assert(game.isFull() && game.takeSound() == Sound::Full);
  game.pressB(101000);
  game.tick(200000);
  assert(game.takeSound() == Sound::None && game.isFull());

  game.pressA(200000);
  game.tick(210000);
  game.pressB(210000);
  assert(game.energy() == 50);
  game.pressA(210000);
  assert(game.energy() == 100 && game.state() == State::Focus);
  assert(game.shake(210000) && game.energy() == 100);
  game.tick(230000);
  game.pressA(230000);
  assert(game.energy() == 100 && game.takeSound() == Sound::None);

  game.toggleMode(230000);
  assert(game.mode() == Mode::Daily && game.state() == State::Idle && game.isFull());
  game.pressB(230000);
  assert(game.takeSound() == Sound::None);
  game.pressA(230000);
  // Varying small frame durations must have the same result as one large tick.
  for (uint32_t time = 230001; time < 1730000; time += 17) game.tick(time);
  game.tick(1730000);
  assert(game.state() == State::Empty && game.takeSound() == Sound::Empty);
  game.pressB(1730000);
  assert(game.remainingMs() == 300000);
  game.tick(2029999);
  assert(!game.isFull());
  game.tick(2030000);
  assert(game.isFull() && game.takeSound() == Sound::Full);
  game.toggleMode(2030000);
  assert(game.mode() == Mode::Demo && game.state() == State::Idle);

  Game wrapped;
  const uint32_t start = UINT32_MAX - 5000;
  wrapped.pressA(start);
  wrapped.tick(start + 10000u);
  assert(wrapped.energy() == 50);
  wrapped.pressB(start + 10000u);
  wrapped.tick(start + 15000u);
  assert(wrapped.isFull() && wrapped.takeSound() == Sound::Full);
  std::cout << "game_test: passed (timing, transitions, sound edges, boosts, rollover)\n";
}
