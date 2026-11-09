#ifndef ENEMYDOWN_HPP
#define ENEMYDOWN_HPP

// ==================== ENEMY DOWN ====================
// Sinks the closest enemy below its original Y (2.0 / 1.8) every tick
// ====================================================

#include <atomic>
#include <thread>

class EnemyDown {
public:
    static void Work();
    static void Stop();
};

#endif // ENEMYDOWN_HPP