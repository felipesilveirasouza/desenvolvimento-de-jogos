#include "TileMap.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <fstream>
#include <random>

#include "Dungeon2D.hpp"

namespace {

/// Writes EMPTY_TILE into every cell of rect (inclusive corners), clipped to
/// the map so a bad rectangle can never write out of bounds.
void carve(std::vector<int>& cells, int width, int height, const Rect& rect) {
    const int left   = std::max(rect.left, 0);
    const int top    = std::max(rect.top, 0);
    const int right  = std::min(rect.right, width - 1);
    const int bottom = std::min(rect.bottom, height - 1);

    for (int row = top; row <= bottom; ++row) {
        for (int col = left; col <= right; ++col) {
            cells[row * width + col] = EMPTY_TILE;
        }
    }
}

/// Carves the room of every leaf below node.
void carveLeaves(std::vector<int>& cells, int width, int height, const BSPNode& node) {
    if (node.isLeaf()) {
        carve(cells, width, height, node.room);
        return;
    }
    carveLeaves(cells, width, height, *node.left);
    carveLeaves(cells, width, height, *node.right);
}

/// Parses one CSV line ("1,0,1") strictly: no spaces, no empty fields, no
/// trailing comma. Returns false on any deviation.
[[nodiscard]] bool parseRow(const std::string& line, std::vector<int>& out) {
    const char* p   = line.data();
    const char* end = line.data() + line.size();

    while (true) {
        int value = 0;
        const auto [next, error] = std::from_chars(p, end, value);
        if (error != std::errc() || next == p) {
            return false;   // not a number (also catches empty fields)
        }
        out.push_back(value);
        p = next;

        if (p == end) {
            return true;    // last number of the line
        }
        if (*p != ',' || p + 1 == end) {
            return false;   // wrong separator, or trailing comma
        }
        ++p;                // skip the comma
    }
}

} // namespace

// --- Construction from geometry --------------------------------------------

void TileMap::loadFromDungeon(const Dungeon2D& dungeon) {
    m_width  = dungeon.width;
    m_height = dungeon.height;
    m_cells.assign(static_cast<std::size_t>(m_width) * m_height, WALL_TILE);

    if (dungeon.root) {
        carveLeaves(m_cells, m_width, m_height, *dungeon.root);
    }
    for (const Corridor& corridor : dungeon.corridors) {
        carve(m_cells, m_width, m_height, corridor);
    }
}

// --- CSV persistence -------------------------------------------------------

bool TileMap::saveCSV(const std::string& path) const {
    std::ofstream file(path);
    if (!file) {
        return false;
    }

    for (int row = 0; row < m_height; ++row) {
        for (int col = 0; col < m_width; ++col) {
            if (col > 0) {
                file << ',';   // separator only BETWEEN values: no trailing comma
            }
            file << at(col, row);
        }
        file << '\n';
    }
    return static_cast<bool>(file);
}

bool TileMap::loadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    // Parse into locals; members are only touched once everything is valid,
    // so a failed load leaves the map unchanged.
    std::vector<int> cells;
    int width  = 0;
    int height = 0;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();   // tolerate Windows line endings
        }

        std::vector<int> row;
        if (!parseRow(line, row)) {
            return false;
        }
        if (height == 0) {
            width = static_cast<int>(row.size());
        } else if (static_cast<int>(row.size()) != width) {
            return false;      // every row must have the same length
        }
        cells.insert(cells.end(), row.begin(), row.end());
        ++height;
    }

    if (height == 0 || file.bad()) {
        return false;
    }

    m_width  = width;
    m_height = height;
    m_cells  = std::move(cells);
    return true;
}

// --- Queries ---------------------------------------------------------------

int TileMap::width() const noexcept {
    return m_width;
}

int TileMap::height() const noexcept {
    return m_height;
}

int TileMap::at(int col, int row) const {
    assert(col >= 0 && col < m_width && row >= 0 && row < m_height);
    return m_cells[static_cast<std::size_t>(row) * m_width + col];
}

bool TileMap::isWall(int col, int row) const {
    return at(col, row) == WALL_TILE;
}

const int* TileMap::data() const noexcept {
    return m_cells.data();
}

// --- Continuous <-> discrete translation -----------------------------------

void TileMap::toIndices(const Vector2D& world, int& col, int& row) const noexcept {
    // floor, not a plain int cast: (int)(-0.5f) is 0, but the cell is -1.
    col = static_cast<int>(std::floor(world.x / TILE_SIZE));
    row = static_cast<int>(std::floor(world.y / TILE_SIZE));
}

Vector2D TileMap::toWorld(int col, int row) const noexcept {
    return Vector2D((static_cast<float>(col) + 0.5f) * TILE_SIZE,
                    (static_cast<float>(row) + 0.5f) * TILE_SIZE);
}

void TileMap::randomEmptyCell(int& col, int& row) const {
    std::vector<int> empty;
    for (int i = 0; i < static_cast<int>(m_cells.size()); ++i) {
        if (m_cells[i] == EMPTY_TILE) {
            empty.push_back(i);
        }
    }
    assert(!empty.empty() && "map has no empty cell");

    // The contract takes no seed, so this generator is seeded once from the
    // system's entropy source and shared by every call.
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<std::size_t> pick(0, empty.size() - 1);

    const int index = empty[pick(rng)];
    col = index % m_width;
    row = index / m_width;
}
