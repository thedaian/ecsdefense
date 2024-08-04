#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

#include <iostream>
#include <format>

struct position {
    sf::Vector2f pos;
};

struct moving {
    sf::Vector2f velocity;
    sf::Vector2i goal;
    float speed;
};

struct health {
    double health;
    double maxHealth;
};

struct raw_damage {
    double dmg;
    double range;
    double cooldown;
};

struct cooldown {
    double time;
};

struct death { bool dead; };

void moveObjects(entt::registry& registry, float dt) {
    auto view = registry.view<position, moving, sf::CircleShape>();

    // use a callback
    view.each([&](auto& p, auto& vel, auto& c) { p.pos += vel.velocity * vel.speed * dt; c.setPosition(floor(p.pos.x), floor(p.pos.y)); });

}

bool isInRange(sf::Vector2f pos, double range, sf::Vector2f other) {
    double distance = (other.x - pos.x) * (other.x - pos.x) + (other.y - pos.y) * (other.y - pos.y);
    return distance <= range * range;
}

void attack(entt::registry& registry, float dt) {
    auto defense = registry.view<position, raw_damage>(entt::exclude<cooldown>);
    auto enemies = registry.view<position, health, sf::CircleShape>();

    for (auto [tower, def_p, dmg] : defense.each()) {
        for (auto enemy : enemies) {
            auto& e_pos = enemies.get<position>(enemy);
            if (isInRange(def_p.pos, dmg.range, e_pos.pos)) {
                std::cout << std::format("tower {:3} shooting - ", static_cast<int>(tower));
                registry.emplace<cooldown>(tower, dmg.cooldown);
                auto& h = enemies.get<health>(enemy);
                h.health -= dmg.dmg;
                if (h.health >= 3) {
                    auto& c = enemies.get<sf::CircleShape>(enemy);
                    c.setPointCount(floor(h.health));
                    std::cout << std::format("{:4} health left: {:4}\n", static_cast<int>(enemy), h.health);
                }
                else {
                    std::cout << std::format("**BOOM** goes {:4}\n", static_cast<int>(enemy));
                    registry.emplace<death>(enemy, true);
                }
            }
        }
    }
    auto dead_enemies = registry.view<death>();
    registry.destroy(dead_enemies.begin(), dead_enemies.end());

    auto defense_cool = registry.view<cooldown>();
    for (auto [tower, cool] : defense_cool.each()) {
        if (cool.time > dt)
        {
            cool.time -= dt;
        }
        else {
            registry.remove<cooldown>(tower);
        }
    }
}

void spawn(entt::registry& registry, sf::Vector2f spawnPoint, sf::Vector2i goal, int level) {
    const auto entity = registry.create();
    const auto& p = registry.emplace<position>(entity, spawnPoint);
    auto& c = registry.emplace<sf::CircleShape>(entity, 20, level + 3);
    c.setFillColor(sf::Color(220, 0, 0, 255));
    c.setPosition(p.pos);
    c.setOrigin(c.getGlobalBounds().width / 2, c.getGlobalBounds().height / 2);
    spawnPoint = sf::Vector2f(goal - sf::Vector2i(spawnPoint));
    sf::Vector2f v{ 0, 0 };
    if (spawnPoint.x > 0) { v.x = 1; }
    else if (spawnPoint.x < 0) { v.x = -1; }
    if (spawnPoint.y > 0) { v.y = 1; }
    else if (spawnPoint.y < 0) { v.y = -1; }
    std::cout << std::format("spawn: {:4}, {:4} - goal: {:4}, {:4} - velocity: {:4}, {:4}\n", spawnPoint.x, spawnPoint.y, goal.x, goal.y, v.x, v.y);
    //sf::Vector2f v(2.f, 2.f);
    registry.emplace<moving>(entity, v, goal, level + 10.f);
    registry.emplace<health>(entity, static_cast<double>(level + 3), static_cast<double>(level + 3));
}

int main() {
    auto window = sf::RenderWindow{ { 800u, 600u }, "CMake SFML Project" };
    //auto window = sf::RenderWindow{ { 1920u, 1080u }, "CMake SFML Project" , sf::Style::Fullscreen};
    window.setFramerateLimit(60);

    sf::VertexArray triangle(sf::LineStrip, 2);

    // define the position of the triangle's points
    triangle[0].position = sf::Vector2f(10.f, 0.f);
    triangle[1].position = sf::Vector2f(20.f, 10.f);

    entt::registry registry;

    sf::Texture t;
    t.loadFromFile("map.png");
    sf::Sprite s;
    s.setTexture(t);

    const auto heart = registry.create();
    const auto& h_p = registry.emplace<position>(heart, sf::Vector2f(window.getSize().x / 2, 20));
    auto& c = registry.emplace<sf::CircleShape>(heart, 10, 5);
    c.setFillColor(sf::Color(0, 220, 0, 255));
    c.setPosition(h_p.pos);
    c.setOrigin(c.getGlobalBounds().width / 2, c.getGlobalBounds().height / 2);

    for (int i = 0; i < 10; ++i) {
        spawn(registry, sf::Vector2f{ h_p.pos.x, static_cast<float>(window.getSize().y + i * 100) }, sf::Vector2i(h_p.pos), i);
    }

    for (int i = 0; i < 10; ++i) {
        const auto t = registry.create();
        const auto& p = registry.emplace<position>(t, sf::Vector2f(window.getSize().x / 2 - 10, h_p.pos.y + i * 40));
        auto& c = registry.emplace<sf::CircleShape>(t, 10, 5);
        c.setFillColor(sf::Color(0, 0, 220, 255));
        c.setPosition(p.pos);
        c.setOrigin(c.getGlobalBounds().width / 2, c.getGlobalBounds().height / 2);
        registry.emplace<raw_damage>(t, 1.f, 20.f, 250.f);
    }

    sf::Clock timer;

    while (window.isOpen()) {
        float dt = timer.restart().asMilliseconds();
        for (auto event = sf::Event{}; window.pollEvent(event);) {
            switch (event.type)
            {
            case sf::Event::Closed:
            case sf::Event::KeyReleased:
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                break;
            default:
                break;
            }
        }

        moveObjects(registry, dt / 100);
        attack(registry, dt);

        window.clear();
        window.draw(s);
        auto view = registry.view<sf::CircleShape>();
        for (auto [entity, c] : view.each()) {
            window.draw(c);
        }
        window.draw(triangle);
        window.display();
    }
}