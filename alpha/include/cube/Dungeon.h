// Dungeon stores a width-by-depth-by-height grid of cells for dungeon generation. It owns the allocated cells.
#pragma once
#include <vector>
#include "plasma/Vector.h"

namespace cube {

// Layout kinds assigned by addRoom/carveCorridor and generateDungeon.
enum DungeonCellKind {
    DungeonCellEmpty = 0,
    DungeonCellRoom = 1,
    DungeonCellStairColumn = 2,
    DungeonCellCorridor = 3,
    DungeonCellEntrance = 4
};
// Individual flags are bit indices; use (1 << flag) to form a mask.
enum DungeonCellFlag {
    DungeonCellFlagStairsUp = 0,
    DungeonCellFlagCarpet = 1,
    DungeonCellFlagGuard = 2
};

class Dungeon {
public:
    // One cell (2 bytes, zeroed by the constructor: one word store per cell). Not yet examined.
    struct Cell {
        Cell()
            : kind(DungeonCellEmpty),
              flags(0)
        {}

        // Whether the cell is empty (0), a room (1) or a stair column (2): World::generateDungeon's test
        // for open space above and beside a cell.
        bool isOpen() const
        {
            return kind == DungeonCellEmpty || kind == DungeonCellStairColumn || kind == DungeonCellRoom;
        }

        // The dungeon generator uses 1 room, 2 stair column, 3 corridor or hall, 4 entrance.
        unsigned char kind;
        // DungeonCellFlagStairsUp, DungeonCellFlagCarpet and DungeonCellFlagGuard bits.
        unsigned char flags;
    };

    // Dungeon generation uses a three-by-three-by-three room layout on a grid spaced seven cells apart. Room constructors zero their fields.
    struct Room {
        Room()
            : used(false),
              guard(false),
              noPatrol(false),
              fixedSize(false),
              offset(0, 0, 0),
              size(0, 0, 0)
        {}

        bool used;
        bool guard;                    // marks the room's centre cell (bit 4: a guard)
        bool noPatrol;                 // no patrol in the room
        bool fixedSize;                // a fixed 3 x 3 x 2 room at the grid point
        plasma::Vector<3, int> offset; // from the grid point, in cells
        plasma::Vector<3, int> size;   // added to 3 x 3 x 1 cells
    };
    // A corridor between two rooms, by grid coordinates (0x18 bytes).
    struct Link {
        Link(const plasma::Vector<3, int> &from, const plasma::Vector<3, int> &to)
            : from(from),
              to(to)
        {}

        plasma::Vector<3, int> from;
        plasma::Vector<3, int> to;
    };
    // The races of a room's patrol (0x18 bytes): the guard's, then its followers'.
    struct Style {
        std::vector<int> guardRaces;
        std::vector<int> followerRaces;
    };

    Dungeon(int sizeX, int sizeY, int sizeZ);
    virtual ~Dungeon();

    // Cell extents along x and y after rotation, and height along z.
    int sizeX() const;
    int sizeY() const;
    void transform(int &x, int &y, int &z) const;
    void inverseTransform(int &x, int &y, int &z) const;
    Cell &cell(int x, int y, int z);
    int sizeZ() const;
    // Marks a room's cells and the cells of the corridors between two rooms.
    void addRoom(const plasma::Vector<3, int> &position, const plasma::Vector<3, int> &size, bool platform);
    void connect(const plasma::Vector<3, int> &fromPosition, const plasma::Vector<3, int> &fromSize,
                 const plasma::Vector<3, int> &toPosition, const plasma::Vector<3, int> &toSize);
    // Marks the cells of a straight corridor between two cells.
    void carveCorridor(const plasma::Vector<3, int> &from, const plasma::Vector<3, int> &to);
    // Whether a cell is empty, a room or the entrance: the generator's test for battlements and roofs.
    bool isFlat(int x, int y, int z)
    {
        int type = cell(x, y, z).kind;
        return type == DungeonCellEmpty || type == DungeonCellEntrance || type == DungeonCellRoom;
    }

    int m_rotation;  // quarter turns
    bool m_reflectY; // reflect y after rotation (same transform)
    int m_sizeX;
    int m_sizeY;
    int m_sizeZ;
    Cell *m_cells; // sizeX * sizeY * sizeZ cells
};

}
