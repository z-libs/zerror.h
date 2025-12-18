
#include <iostream>
#include <string>
#include <vector>

#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES
#define ZERROR_ENABLE_TRACE
#include "zerror.h"

using namespace z_error;


struct Player 
{
    int id;
    std::string name;
    int level;
};


result<Player> load_player(int id) 
{
    if (id <= 0) 
    {
        return zerr_create(404, "Player ID must be positive");
    }
    if (id == 99) 
    {
        return zerr_create(500, "Corrupted Player Data");
    }
    return Player{id, "Hero_" + std::to_string(id), 1};
}

result<int> calculate_xp_bonus(const Player& p) 
{
    ensure(p.level > 0, 400, "Level cannot be zero");

    if (p.name.find("Hero") == std::string::npos) 
    {
        return zerr_create(403, "Not a hero");
    }
    return 100 * p.level;
}

result<void> save_progress(int player_id) 
{
    std::cout << "[Disk] Saving progress for ID " << player_id << "...\n";
    
    Player p = ztry( load_player(player_id) );

    int bonus = ztry( calculate_xp_bonus(p) );

    std::cout << "[Disk] Saved " << p.name << " with bonus " << bonus << ".\n";
    
    return result<void>::success();
}

int main() 
{
    std::cout << "(1) Normal Flow:\n";
    if (save_progress(10).ok()) 
    {
        std::cout << "-> Success!\n";
    }

    std::cout << "\n(2) Error Flow (ID 99 is corrupt):\n";
    auto res = save_progress(99);
    if (!res) 
    {
        std::cerr << "-> Flow Failed: " << res.err << "\n";
    }

    std::cout << "\n(3) Panic Flow (unwrap):\n";
    std::cout << "Attempting to unwrap an invalid player... (Expect Crash)\n";
    
    // It will print the stack trace and abort because ID -5 returns an error.
    Player p = unwrap( load_player(-5) );
    
    // Unreachable.
    std::cout << "Got player: " << p.name << "\n";

    return 0;
}
