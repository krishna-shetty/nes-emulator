#include "nes_mapper.h"

using namespace NES;

Mapper::Mapper(uint8_t numPRGBanks, uint8_t numCHRBanks)
    : _numPRGBanks(numPRGBanks), _numCHRBanks(numCHRBanks)
{
}
