#include "nes_ppu.h"
#include <stdexcept>

using namespace NES;

PPU::PPU(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error(SDL_GetError());

    createWindow(title, width, height);
}

PPU::~PPU() noexcept
{
    destroyWindow();
    SDL_Quit();
}

void PPU::createWindow(const char* title, int width, int height)
{
    if (!SDL_CreateWindowAndRenderer(title, width, height, 0, &_window, &_renderer))
    {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }
}

void PPU::destroyWindow() noexcept
{
    if (_renderer)
    {
        SDL_DestroyRenderer(_renderer);
        _renderer = nullptr;
    }
    if (_window)
    {
        SDL_DestroyWindow(_window);
        _window = nullptr;
    }
}

void PPU::handleEvents(std::function<void(SDL_Event&)> callback)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (callback)
            callback(event);
        if (event.type == SDL_EVENT_QUIT)
            _running = false;
    }
}

void PPU::clear()
{
    SDL_SetRenderDrawColor(_renderer, _clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    SDL_RenderClear(_renderer);
}

void PPU::present()
{
    SDL_RenderPresent(_renderer);
}

void PPU::setClearColor(Color color)
{
    _clearColor = color;
}

void PPU::clock()
{
    _cycle++;

    if (_cycle > 340)
    {
        _cycle = 0;
        _scanline++;
        if (_scanline >= 261)
        {
            _scanline = -1;
            _frameComplete = true;
        }
    }
}