////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <CNtity/Helper.hpp>
#include <random>
#include <iostream>

////////////////////////////////////////////////
const int MAX_WIDTH = 10;
const int MAX_HEIGHT = 24;
const int TILE_SIZE = 32;

////////////////////////////////////////////////
using Shape = std::vector<std::vector<bool>>;

////////////////////////////////////////////////
const std::array<Shape, 7> SHAPES = 
{
    // STRAIGHT
    Shape {
        { false, false, false, false },
        { true, true, true, true },
        { false, false, false, false },
        { false, false, false, false }
    },
    // SQUARE
    Shape {
        { true, true },
        { true, true }
    },
    // TEE
    Shape {
        { false, true, false },
        { true, true, true },
        { false, false, false }
    },
    // JAY
    Shape {
        { true, false, false },
        { true, true, true },
        { false, false, false }
    },
    // EL
    Shape {
        { false, false, true },
        { true, true, true },
        { false, false, false }
    },
    // SKEWS
    Shape {
        { false, true, true },
        { true, true, false },
        { false, false, false }
    },
    // SKEWZ
    Shape {
        { true, true, false },
        { false, true, true },
        { false, false, false }
    }
};

////////////////////////////////////////////////
struct Tile 
{
    Shape shape;
    sf::Color color;

    int bottom() const
    {
        for(int y = this->shape.size() - 1; y > 0; y--)
            for(int x = 0; x < this->shape[y].size(); x++)
                if(this->shape[y][x])
                    return y;

        return 0;
    }

    int left() const
    {
        for(int x = 0; x < this->shape[0].size(); x++)
            for(int y = 0; y < this->shape.size(); y++)
                if(this->shape[y][x])
                    return x;

        return 0;
    }

    int right() const
    {
        for(int x = this->shape[0].size() - 1; x > 0; x--)
            for(int y = 0; y < this->shape.size(); y++)
                if(this->shape[y][x])
                    return x;

        return 0;
    }
};

////////////////////////////////////////////////
struct Position
{
    int x;
    int y;
};

////////////////////////////////////////////////
struct Movable
{

};

////////////////////////////////////////////////
struct Player
{
    int score = 0;
    std::shared_ptr<Tile> next;  
};

////////////////////////////////////////////////
using Manager = CNtity::Helper<Tile, Position, Movable, Player>;

////////////////////////////////////////////////
class System
{
public:
    enum class State
    {
        PLAY,
        PAUSE,
        END,
        CLOSE,
        NONE
    };

    System(Manager& helper) : m_helper(helper) {}
    virtual ~System() {}

    virtual void update() = 0;

    virtual State state() 
    {
        return State::NONE;
    }

protected: 
    Manager& m_helper;
};

////////////////////////////////////////////////
class Collision : public System
{
public:
    using System::System;

    static bool left(Manager& helper, const Position* position, const Tile* tile, bool next = true)
    {
        bool collision = false;

        for(int y = 0; y < tile->shape.size(); ++y)
        {
            for(int x = 0; x < tile->shape[0].size(); ++x)
            {
                if(!tile->shape[y][x])
                    continue;

                helper.for_each<Tile, Position>([&](auto entity, auto other) 
                {
                    if(helper.has<Movable>(entity))
                        return;

                    auto other_position = helper.get<Position>(entity);

                    int i = position->x + x - other_position->x - (next ? 1 : 0);
                    int j = position->y + y - other_position->y;
                    
                    if(i >= 0 && i < other->shape[0].size() && j >= 0 && j < other->shape.size() && other->shape[j][i])
                    {
                        collision = true;
                        return;
                    }
                });
            }
        }

        if(collision)
            return false;

        return position->x + tile->left() > (next ? 0 : -1); 
    }
    
    static bool right(Manager& helper, const Position* position, const Tile* tile, bool next = true)
    {
        bool collision = false;

        for(int y = tile->shape.size() - 1; y >= 0; --y)
        {
            for(int x = 0; x < tile->shape[0].size(); ++x)
            {
                if(!tile->shape[y][x])
                    continue;

                helper.for_each<Tile, Position>([&](auto entity, auto other) 
                {
                    if(helper.has<Movable>(entity))
                        return;

                    auto other_position = helper.get<Position>(entity);

                    int i = position->x + x - other_position->x + (next ? 1 : 0);
                    int j = position->y + y - other_position->y;
                    
                    if(i >= 0 && i < other->shape[0].size() && j >= 0 && j < other->shape.size() && other->shape[j][i])
                    {
                        collision = true;
                        return;
                    }
                });
            }
        }

        if(collision)
            return false;

        return position->x + tile->right() + (next ? 1 : 0) < MAX_WIDTH;
    }

    static bool bottom(Manager& helper, const Position* position, const Tile* tile, bool next = true)
    {
        bool collision = false;

        for(int x = 0; x < tile->shape[0].size(); ++x)
        {
            for(int y = tile->shape.size() - 1; y >= 0; --y)
            {
                if(!tile->shape[y][x])
                    continue;

                helper.for_each<Tile, Position>([&](auto entity, auto other) 
                {
                    if(helper.has<Movable>(entity))
                        return;

                    auto other_position = helper.get<Position>(entity);
                    
                    int i = position->x + x - other_position->x;
                    int j = position->y + y - other_position->y + (next ? 1 : 0);

                    if(i >= 0 && i < other->shape[0].size() && j >= 0 && j < other->shape.size() && other->shape[j][i])
                    {
                        collision = true;
                        return;
                    }
                });
            }
        }

        if(collision)
            return false;

        return position->y + tile->bottom() + (next ? 1 : 0) < MAX_HEIGHT;
    }

    virtual void update()
    {
        this->m_helper.for_each<Position, Tile, Movable>([this](auto entity, auto position)
        {
            const Tile* tile = this->m_helper.get<Tile>(entity);

            if(!bottom(this->m_helper, position, tile))
            {
                this->m_helper.remove<Movable>(entity);
            }
        });
    }
};

////////////////////////////////////////////////
class Movement : public System
{
public:
    using System::System;

    virtual void update()
    {
        if(m_clock.getElapsedTime().asMilliseconds() > m_interval)
        {
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                this->m_helper.for_each<Position, Tile, Movable>([this](auto entity, auto position)
                {
                    if(Collision::left(this->m_helper, position, this->m_helper.get<Tile>(entity)))
                    {
                        (position->x)--;
                        m_clock.restart();
                    }       
                });
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                this->m_helper.for_each<Position, Tile, Movable>([this](auto entity, auto position)
                {
                    if(Collision::right(this->m_helper, position, this->m_helper.get<Tile>(entity)))
                    {
                        (position->x)++;
                        m_clock.restart();
                    }
                });
            }
        }
    }

private:
    sf::Clock m_clock;
    int m_interval = 200;
};

////////////////////////////////////////////////
class Rotation : public System
{
public:
    using System::System;

    virtual void update()
    {
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && m_clock.getElapsedTime().asMilliseconds() > m_interval)
        {
            this->m_helper.for_each<Tile, Movable, Position>([this](auto entity, auto tile)
            {
                Shape shape = tile->shape;
                
                for(int i = 0; i < tile->shape.size(); i++)
                {
                    for(int j = 0; j < tile->shape[i].size(); j++)
                    {
                        shape[i][j] = tile->shape[tile->shape.size() - 1 - j][i];
                    }
                }

                const Tile tmp = { shape, tile->color };
                const Position* position = this->m_helper.get<Position>(entity);
                if(Collision::bottom(this->m_helper, position, &tmp, false) && Collision::right(this->m_helper, position, &tmp, false) && Collision::left(this->m_helper, position, &tmp, false))
                    tile->shape = shape;
            });

            m_clock.restart();
        }
    }

private:
    sf::Clock m_clock;
    int m_interval = 500;
};

////////////////////////////////////////////////
class Gravity : public System
{
public:
    using System::System;

    virtual void update()
    {
        if(m_clock.getElapsedTime().asMilliseconds() > m_interval)
        {
            this->m_helper.for_each<Position, Movable>([this](auto entity, auto position)
            {
                (position->y)++;
            });

            m_clock.restart();
        }

        m_interval = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ? 100 : 1000;
    }

private:
    sf::Clock m_clock;
    int m_interval = 1000;
};

////////////////////////////////////////////////
class Cleaner : public System
{
public:
    using System::System;

    void update()
    {
        for(int line = 0; line < MAX_HEIGHT; ++line)
        {
            // Count tiles on line
            std::vector<std::vector<bool>::iterator> tiles;   

            this->m_helper.for_each<Tile, Position>([&](auto entity, auto tile)
            {
                auto position = this->m_helper.get<Position>(entity);

                if(this->m_helper.has<Movable>(entity) || line - position->y >= tile->shape.size())
                    return;

                for(int x = 0; x < tile->shape[0].size(); ++x)
                {
                    if(tile->shape[line - position->y][x])
                    {
                        tiles.push_back(tile->shape[line - position->y].begin() + x);
                    }
                }
            });

            if(tiles.size() < MAX_WIDTH)
                continue;

            // If line is full
            for(auto tile : tiles)
            {
                *tile = false;
            }

            // Add score to player
            this->m_helper.for_each<Player>([&](auto entity, auto player)
            {
                player->score += tiles.size();
                std::cout << player->score << std::endl;
            });

            this->m_helper.for_each<Tile, Position>([&](auto entity, auto tile)
            {
                if(this->m_helper.has<Movable>(entity))
                    return;

                // Erase empty tiles
                bool empty = true;
                for(int y = 0; y < tile->shape.size() && empty; ++y)
                    for(int x = 0; x < tile->shape[y].size() && empty; ++x)
                        if(tile->shape[y][x])
                            empty = false;

                if(empty)
                {
                    this->m_helper.remove<Tile>(entity);
                    return;
                }

                // Move tiles down
                auto position = this->m_helper.get<Position>(entity);

                if(position->y <= line)
                {
                    if(position->y + tile->bottom() <= line)
                    {
                        (position->y)++;
                    }
                    else
                    {
                        for(int y = 0; y < line - position->y && y < tile->shape.size(); ++y)
                            for(int x = 0; x < tile->shape[y].size(); ++x)
                            {
                                if(tile->shape[y][x])
                                {
                                    tile->shape[y][x] = false;
                                    tile->shape[y + 1][x] = true;
                                }
                            }
                    }
                }   
            });
        }
    }
};

////////////////////////////////////////////////
class Factory : public System
{
public:
    Factory(Manager& helper) : System(helper), m_gen(m_device()), m_distrib(0, SHAPES.size() - 1), m_end(false)
    {

    }

    static sf::Color color(int index)
    {
        switch(index)
        {
            case 0: return sf::Color::Red;
            case 1: return sf::Color::Green;
            case 2: return sf::Color::Blue;
            case 3: return sf::Color::Yellow;
            case 4: return sf::Color::Magenta;
            case 5: return sf::Color::Cyan;
            case 6: return sf::Color::White;
            default: return sf::Color::White;
        }
    }

    virtual System::State state()
    {
        return m_end ? System::State::END : System::State::NONE;
    }

    virtual void update()
    {
        if(this->m_helper.acquire<Movable>().empty())
        {
            this->m_helper.for_each<Player>([this](auto entity, auto player)
            {
                auto next = player->next;

                int index = m_distrib(m_gen);
                player->next = std::make_shared<Tile>(Tile{SHAPES[index], color(index)});

                index = m_distrib(m_gen);
                auto tile = this->m_helper.create<Tile, Position, Movable>(next ? *next : Tile{SHAPES[index], color(index)}, {MAX_WIDTH / 2, 0}, {});
            
                if(!Collision::bottom(this->m_helper, this->m_helper.get<Position>(tile), this->m_helper.get<Tile>(tile)))
                    this->m_end = true;
            });
        }
    }

private:
    std::random_device m_device;  
    std::mt19937 m_gen; 
    std::uniform_int_distribution<> m_distrib;
    bool m_end;
};

////////////////////////////////////////////////
class Render : public System
{
public:
    Render(Manager& helper) : System(helper), m_window(sf::VideoMode(600, 800), "Tetris"), m_grid({MAX_WIDTH * TILE_SIZE, MAX_HEIGHT * TILE_SIZE}), m_paused(false)
    {
        m_grid.setFillColor(sf::Color::Transparent);
        m_grid.setOutlineColor(sf::Color::Blue);
        m_grid.setOutlineThickness(1);

        m_grid_view.setCenter(m_grid.getLocalBounds().width / 2, m_grid.getLocalBounds().height / 2);
        m_grid_view.setSize(m_window.getSize().x, m_window.getSize().y);

        m_ui_view.setCenter(m_window.getSize().x / 2, m_window.getSize().y / 2);
        m_ui_view.setSize(m_window.getSize().x, m_window.getSize().y);
    }

    virtual System::State state()
    {
        return m_window.isOpen() ? (m_paused ? System::State::PAUSE : System::State::PLAY) : System::State::CLOSE;
    }

    sf::VertexArray generate(Tile* tile)
    {
        const int width = tile->shape[0].size();
        const int height = tile->shape.size();

        sf::VertexArray vertices(sf::Quads, width * height * 4);

        for (unsigned int i = 0; i < width; ++i) 
        {
            for (unsigned int j = 0; j < height; ++j)
            {
                sf::Vertex* quad = &vertices[(i + j * width) * 4];

                quad[0].position = sf::Vector2f(i * TILE_SIZE, j * TILE_SIZE);
                quad[1].position = sf::Vector2f((i + 1) * TILE_SIZE, j * TILE_SIZE);
                quad[2].position = sf::Vector2f((i + 1) * TILE_SIZE, (j + 1) * TILE_SIZE);
                quad[3].position = sf::Vector2f(i * TILE_SIZE, (j + 1) * TILE_SIZE);

                for(unsigned int k = 0; k < 4; ++k)
                    quad[k].color = tile->shape[j][i] ? tile->color : sf::Color::Transparent;
            }
        }

        return vertices;
    }

    virtual void update()
    {
        sf::Event event;
        while(m_window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
                m_window.close();
            else if(event.type == sf::Event::Resized)
            {
                m_grid_view.setSize(m_window.getSize().x, m_window.getSize().y);

                m_ui_view.setCenter(m_window.getSize().x / 2, m_window.getSize().y / 2);
                m_ui_view.setSize(m_window.getSize().x, m_window.getSize().y);
            }
            if(event.type == sf::Event::KeyPressed)
            {
                if(event.key.code == sf::Keyboard::Escape)
                {
                    m_paused = !m_paused;
                    m_grid.setFillColor(m_paused ? sf::Color{0, 0, 0, 64} : sf::Color::Transparent);
                }
                    
            }
        }

        m_window.clear();

        m_window.setView(m_grid_view);

        this->m_helper.for_each<Tile, Position>([this](auto entity, auto tile)
        {
            const Position* position = this->m_helper.get<Position>(entity);

            sf::Transform translation;
            translation.translate(position->x * TILE_SIZE, position->y * TILE_SIZE);

            this->m_window.draw(generate(tile), sf::RenderStates(translation));
        });

        m_window.draw(m_grid);

        m_window.setView(m_ui_view);

        this->m_helper.for_each<Player>([this](auto entity, auto player)
        {
            sf::Transform translation;
            translation.translate(m_window.getSize().x / 1.25, 50);

            this->m_window.draw(generate(player->next.get()), sf::RenderStates(translation));
        });

        m_window.display();
    }

private:
    sf::RenderWindow m_window;
    sf::View m_grid_view;
    sf::View m_ui_view;
    sf::RectangleShape m_grid;
    sf::Clock m_clock;
    bool m_paused;
};

////////////////////////////////////////////////
int main() 
{
    Manager helper;
    helper.create<Player>({});

    std::vector<std::shared_ptr<System>> systems 
    { 
        std::make_shared<Collision>(helper),
        std::make_shared<Gravity>(helper),
        std::make_shared<Movement>(helper),
        std::make_shared<Rotation>(helper),
        std::make_shared<Cleaner>(helper),
        std::make_shared<Factory>(helper),
        std::make_shared<Render>(helper)
    };

    bool pause = false;
    while(true)
    {
        for(int i = 0; i < systems.size(); ++i)
        {
            auto state = systems[i]->state();
            
            if(state == System::State::CLOSE)
                return 0;
            else if(state == System::State::PAUSE || state == System::State::END)
                pause = true;
            else if(state == System::State::PLAY)
                pause = false;

            if(pause && state != System::State::PAUSE)
                continue;

            systems[i]->update();
        }
    }

    return 0;
}