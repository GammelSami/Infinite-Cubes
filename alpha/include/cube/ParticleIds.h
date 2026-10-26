// Particle packet types and client particle modes.
#pragma once
namespace cube {
enum ParticleType {
    ParticleLitCube = 0,    // world-lit voxel debris
    ParticleFire = 1,       // orange/yellow colour chosen by the receiver
    ParticleGlow = 2,       // packet colour, independent of world light
    ParticleStationary = 3, // ages without integrating motion
    ParticleLeaf = 4        // canopy colour, no gravity, passes through foliage
};
}
