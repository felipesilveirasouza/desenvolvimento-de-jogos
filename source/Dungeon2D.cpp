#include "Dungeon2D.hpp"

#include <algorithm>
#include <cassert>
#include <random>

// Rect corners are INCLUSIVE cell indices: a Rect covers every cell with
// left <= col <= right and top <= row <= bottom. Its size in tiles is
// (right - left + 1) x (bottom - top + 1).

namespace {

constexpr int MIN_ROOM_SIZE   = 5;   ///< Smallest room side, in tiles.
constexpr int CORRIDOR_WIDTH  = 3;   ///< Corridor thickness, in tiles.
constexpr int ROOM_MARGIN     = 1;   ///< Wall cells kept between a room and its region border.
/// Smallest region side that still fits a room plus its margins.
constexpr int MIN_REGION_SIZE = MIN_ROOM_SIZE + 2 * ROOM_MARGIN;

/// Maximum tree depth: at most 2^4 = 16 rooms, whatever the map size.
constexpr int MAX_DEPTH       = 4;
/// From this depth on, a node may randomly stop splitting (1 chance in 4),
/// so the rooms do not all have the same size.
constexpr int EARLY_STOP_DEPTH = 3;

[[nodiscard]] int rectWidth(const Rect& r) noexcept  { return r.right - r.left + 1; }
[[nodiscard]] int rectHeight(const Rect& r) noexcept { return r.bottom - r.top + 1; }

/// Uniform integer in [lo, hi]. Uses std::mt19937's output directly, whose
/// sequence is fixed by the C++ standard, so the same seed gives the same
/// dungeon with any compiler (std::uniform_int_distribution does not
/// guarantee that).
[[nodiscard]] int randomInt(std::mt19937& rng, int lo, int hi) {
    assert(lo <= hi);
    const auto span = static_cast<std::mt19937::result_type>(hi - lo + 1);
    return lo + static_cast<int>(rng() % span);
}

/// Position of a cut along a side that starts at `start` and has `length`
/// cells. The cut is the first cell of the second child. It stays between
/// 30% and 70% of the side (no thin strips) and leaves both children with
/// at least MIN_REGION_SIZE cells.
[[nodiscard]] int randomCut(std::mt19937& rng, int start, int length) {
    const int lo = std::max(start + MIN_REGION_SIZE, start + length * 3 / 10);
    const int hi = std::min(start + length - MIN_REGION_SIZE, start + length * 7 / 10);
    return randomInt(rng, lo, hi);
}

/// Recursively splits node.region until it is too small to split or the
/// maximum depth is reached.
void split(BSPNode& node, std::mt19937& rng, int depth) {
    const int w = rectWidth(node.region);
    const int h = rectHeight(node.region);
    const bool canSplitVertically   = w >= 2 * MIN_REGION_SIZE;   // cut along x
    const bool canSplitHorizontally = h >= 2 * MIN_REGION_SIZE;   // cut along y

    if (depth >= MAX_DEPTH || (!canSplitVertically && !canSplitHorizontally)) {
        return;   // stays a leaf
    }
    if (depth >= EARLY_STOP_DEPTH && randomInt(rng, 0, 3) == 0) {
        return;   // random early stop: this region keeps one big room
    }

    // Prefer cutting the longer side, so regions do not become thin strips.
    bool vertical;
    if (canSplitVertically && canSplitHorizontally) {
        if      (w > h * 5 / 4) vertical = true;
        else if (h > w * 5 / 4) vertical = false;
        else                    vertical = randomInt(rng, 0, 1) == 0;
    } else {
        vertical = canSplitVertically;
    }

    node.left  = std::make_unique<BSPNode>();
    node.right = std::make_unique<BSPNode>();
    node.left->region  = node.region;
    node.right->region = node.region;

    if (vertical) {
        const int cut = randomCut(rng, node.region.left, w);
        node.left->region.right = cut - 1;
        node.right->region.left = cut;
    } else {
        const int cut = randomCut(rng, node.region.top, h);
        node.left->region.bottom = cut - 1;
        node.right->region.top   = cut;
    }

    split(*node.left, rng, depth + 1);
    split(*node.right, rng, depth + 1);
}

/// Places a random room inside every leaf. Each side is at least half of
/// the space available in the region (and never below MIN_ROOM_SIZE), so
/// rooms fill a good part of their region.
void carveRooms(BSPNode& node, std::mt19937& rng) {
    if (!node.isLeaf()) {
        carveRooms(*node.left, rng);
        carveRooms(*node.right, rng);
        return;
    }

    const Rect& region = node.region;
    const int availW = rectWidth(region)  - 2 * ROOM_MARGIN;
    const int availH = rectHeight(region) - 2 * ROOM_MARGIN;
    const int roomW  = randomInt(rng, std::max(MIN_ROOM_SIZE, availW / 2), availW);
    const int roomH  = randomInt(rng, std::max(MIN_ROOM_SIZE, availH / 2), availH);

    node.room.left   = randomInt(rng, region.left + ROOM_MARGIN,
                                      region.right - ROOM_MARGIN - roomW + 1);
    node.room.top    = randomInt(rng, region.top + ROOM_MARGIN,
                                      region.bottom - ROOM_MARGIN - roomH + 1);
    node.room.right  = node.room.left + roomW - 1;
    node.room.bottom = node.room.top + roomH - 1;
}

/// Appends every leaf of the subtree rooted at node.
void collectLeaves(const BSPNode& node, std::vector<const BSPNode*>& out) {
    if (node.isLeaf()) {
        out.push_back(&node);
        return;
    }
    collectLeaves(*node.left, out);
    collectLeaves(*node.right, out);
}

[[nodiscard]] int centreX(const Rect& r) noexcept { return (r.left + r.right) / 2; }
[[nodiscard]] int centreY(const Rect& r) noexcept { return (r.top + r.bottom) / 2; }

/// Adds an L-shaped corridor (two rectangles) joining cells a and b.
void addCorridor(int ax, int ay, int bx, int by, std::mt19937& rng,
                 std::vector<Corridor>& corridors) {
    const int half = CORRIDOR_WIDTH / 2;   // 1: the corridor is centre +- 1

    // Horizontal-then-vertical or vertical-then-horizontal, at random.
    if (randomInt(rng, 0, 1) == 0) {
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    // Horizontal leg along row ay, from ax to bx.
    corridors.push_back(Corridor{ std::min(ax, bx) - half, ay - half,
                                  std::max(ax, bx) + half, ay + half });
    // Vertical leg along column bx, from ay to by. Both legs contain the
    // 3 x 3 block around the corner (bx, ay), so they are joined.
    corridors.push_back(Corridor{ bx - half, std::min(ay, by) - half,
                                  bx + half, std::max(ay, by) + half });
}

/// For every internal node, joins the CLOSEST pair of rooms between its left
/// and right subtrees. By induction each subtree is connected, so the whole
/// dungeon is connected; choosing the closest pair keeps corridors short.
void connect(const BSPNode& node, std::mt19937& rng, std::vector<Corridor>& corridors) {
    if (node.isLeaf()) {
        return;
    }
    connect(*node.left, rng, corridors);
    connect(*node.right, rng, corridors);

    std::vector<const BSPNode*> leftLeaves;
    std::vector<const BSPNode*> rightLeaves;
    collectLeaves(*node.left, leftLeaves);
    collectLeaves(*node.right, rightLeaves);

    const Rect* bestA = nullptr;
    const Rect* bestB = nullptr;
    long long bestDistance = 0;
    for (const BSPNode* l : leftLeaves) {
        for (const BSPNode* r : rightLeaves) {
            const long long dx = centreX(l->room) - centreX(r->room);
            const long long dy = centreY(l->room) - centreY(r->room);
            const long long distance = dx * dx + dy * dy;   // squared: no sqrt needed
            if (!bestA || distance < bestDistance) {
                bestA = &l->room;
                bestB = &r->room;
                bestDistance = distance;
            }
        }
    }

    // Room centres. Rooms are >= 5 tiles per side, so centre +- 1 is still
    // inside the room and the corridor enters it along its full width.
    addCorridor(centreX(*bestA), centreY(*bestA), centreX(*bestB), centreY(*bestB),
                rng, corridors);
}

} // namespace

Dungeon2D::Dungeon2D(int widthTiles, int heightTiles)
    : width(widthTiles), height(heightTiles) {
    assert(widthTiles > 0 && heightTiles > 0);
}

void Dungeon2D::generate(unsigned seed) {
    // A 5 x 5 room plus a wall margin on each side must fit.
    assert(width >= MIN_REGION_SIZE && height >= MIN_REGION_SIZE
           && "dungeon too small for the smallest room");

    std::mt19937 rng(seed);

    corridors.clear();
    root = std::make_unique<BSPNode>();   // replaces (and frees) any old tree
    root->region = Rect{ 0, 0, width - 1, height - 1 };

    split(*root, rng, 0);
    carveRooms(*root, rng);
    connect(*root, rng, corridors);
}
