#pragma once

#include <vector>
#include <map>
#include <set>
#include <atomic>

#include <thread>
#include <mutex>

#include <cstdint>

/// Forward declaration of SDL_Renderer
class SDL_Renderer;

/// Forward declaration of SDL_Window
class SDL_Window;

/// Forward declaration of SDL_Texture
class SDL_Texture;

/// Forward declaration of TTF_Font
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;


/**
 * This class handles setting up SDL windows; it is a form of a window manager
 * in addition to handling user input.
 *
 * This includes creating/destroying windows, handing out window contexts and such.
 *
 * This also includes static memberes for setting a "singleton" that can be retrieved by
 * other classes, without explicitly passing around references to this.
 *
 * Finally, this also can register callback classes per window, of which the redraw
 * command can be triggered.
 */

class Renderable
{
    public:
    /// Base constructor that registers this renderable with a given renderer
    explicit Renderable(SDL_Renderer* renderer_);

    /// Base constructor that constructs a new window given a width and height value
    Renderable(uint16_t width, uint16_t height);

    /// Virtual base deconstructor that deregisters this renderable
    virtual ~Renderable();

    /// Virtual interface function called by the drawing thread that redraws on the set window
    virtual void redraw() = 0;

    /// Schedules a redraw of the linked renderer
    void scheduleRedraw();

    /// Deregisters this renderable (called automatically in the deconstuctor if not down manually)
    void deregister();

    protected:
    /// Drawing renderer variable
    SDL_Renderer* renderer;

    /// Function to draw text of a given size at a given location
    /**
     * Function to draw text of a given size at a given location.
     *
     * The shouldCache flag can be set to false if you do not wish to cache the render result.
     * This should be set to false in instances where you are outputting text that is reasonably non-constant.
     * By default, this otherwise does a cache lookup, rendering the texture if necessary.
     */
    void drawText(const std::string& text, uint8_t fontsize, uint16_t x, uint16_t y, bool shouldCache = true);

    /// Stores loaded fonts. Fonts are destroyed when we destroy the renderable.
    std::map<uint8_t, TTF_Font*> fontCache;

    /// Stores generated textures for text rendered
    std::map<uint8_t, std::map<std::string, SDL_Texture*>> textCache;
};

class UI
{
    public:
    /// Inits SDL
    UI();

    /// Shuts down SDL properly
    ~UI();

    /// Remove the copy constructor
    UI(const UI& ui) = delete;

    /// And remove the move constructor
    UI(UI&& ui) = delete;

    /// Remove the copy assignment operator
    UI& operator=(const UI& other) = delete;

    /// and remove the move assignment operator
    UI&& operator=(UI&& other) = delete;

    /// Gets the singleton pointer to this class. Throws an exception if no singleton has been set
    static UI* getGlobalUI();

    /// Sets the singleton pointer to this class
    static void setGlobalUI(UI* singleton_);
    
    /**
     * Requests that a new renderer be constructed, at least as big as the specified size.
     *
     * Upon fullfillment, the finishRendererCallback function of the caller is called
     */
    void requestRenderer(uint16_t width, uint16_t height, SDL_Renderer** renderer, Renderable* target);


    /// For a given renderer, registeres a Renderable for that renderer that can be redrawn
    void registerRenderable(SDL_Renderer* renderer, Renderable* renderable);

    /// Deregisters a given renderer, destroying any of its windows
    void deregisterRenderable(SDL_Renderer* renderer, Renderable* renderable);

    /// Triggers a redraw on the given renderer
    void triggerRedraw(SDL_Renderer* renderer);

    /// Queues a text input change event.
    void changeTextInput(bool receiveText);

    /// Protection mutex for the toRedraw set
    std::mutex redrawMux;


private:
    /**
     * This is the GUI loop thread function. This is capped at 60FPS,
     * and takes care of redrawing any open windows. We run this is its
     * own thread because of thread-local variables generated by SDL.
     *
     * Takes a reference to a boolean startup variable which is set to true
     * when startup is complete. This is protected by the mutex also passed
     * in as a reference. The caller busy-waits until startup is complete,
     * as we expect it not to take very long
     */
    void runSDLloop(bool& startupDone, std::mutex& startupMux);

    /// Internal verison of register renderable that does not hold the lock
    void internalRegisterRenderable(SDL_Renderer* renderer, Renderable* renderable);

    /**
     * Returns an SDL_Renderer that is at least as big as the specified size
     */
    SDL_Renderer* getFreeRenderer(uint16_t minWidth, uint16_t minHeight);

    /// Thread object for the redraw loop.
    std::thread sdlThread;

    static UI* singleton;

    /// Stores allocated windows
    std::vector<SDL_Window*> windows;

    /// Stores allocated renderers
    std::vector<SDL_Renderer*> renderers;

    /// Stores a mapping between Renderer's and windows
    std::map<SDL_Renderer*, SDL_Window*> ownedWindows;

    /// Stores the list of windows to destroy
    std::vector<Renderable*> toDestroy;

    /// Stores the list of registered renderables for a given renderer
    std::map<SDL_Renderer*, std::set<Renderable*>> renderStack;

    /// Stores the list of renderers that should be redrawn on the next pass
    std::set<SDL_Renderer*> toRedraw;

    /// Protection mutex for shutdown
    std::mutex shutdownMux;

    /// Protection mutex for redraw requests
    std::mutex redrawRequestMux;

    /// Shutdown boolean
    bool shouldShutdown;

    /// Stores if we should update the text input
    std::atomic<bool> shouldUpdateText;

    std::atomic<bool> textStatus;

    /// General state mutex
    std::mutex stateMux;

    struct RendererRequest
    {
        uint16_t width;
        uint16_t height;
        SDL_Renderer** renderer;
        Renderable* target;
    };

    /// Stores the list of current new renderer requests
    std::vector<RendererRequest> rendererRequests;
};
