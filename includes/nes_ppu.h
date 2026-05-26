#ifndef NES_PPU_H
#define NES_PPU_H
#include <SDL3/SDL.h>
#include <functional>

namespace NES
{
    class PPU
    {
    public:
        PPU(const char *title, int width, int height);
        ~PPU() noexcept;

        SDL_Window *getWindow() const { return _window; }
        SDL_Renderer *getRenderer() const { return _renderer; }

        struct Color
        {
            uint8_t r, g, b, a;
        };

        bool isRunning() const { return _running; }
        void handleEvents(std::function<void(SDL_Event&)> callback = nullptr);
        void clear();
        void present();

        void setClearColor(Color color);

    private:
        SDL_Window *_window{nullptr};
        SDL_Renderer *_renderer{nullptr};
        bool _running{true};

        Color _clearColor {0, 0, 0, 255};

        void createWindow(const char *title, int width, int height);
        void destroyWindow() noexcept;
    };
} // namespace NES

#endif // NES_PPU_H