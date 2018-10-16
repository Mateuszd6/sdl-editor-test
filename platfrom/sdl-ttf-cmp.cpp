
namespace global
{
    static SDL_Surface* test_text_surf;
}

static void test_sdl_ttf_text(FT_Bitmap pixmap, int color, int yoffset, int minx)
{
    // Create the target surface
    auto textbuf = SDL_CreateRGBSurface(SDL_SWSURFACE, pixmap.width, pixmap.rows, 32,
                                        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (textbuf == nullptr)
    {
        global::test_text_surf = nullptr;
        return;
    }

    // Adding bound checking to avoid all kinds of memory corruption errors that
    // may occur.
    auto dst_check = static_cast<Uint32*>(textbuf->pixels) + textbuf->pitch/4 * textbuf->h;

    // Load and render each character
    auto xstart = 0;
    auto pixel = color;
    SDL_FillRect(textbuf, nullptr, pixel & 0x00FFFFFF);

    // Ensure the width of the pixmap is correct. On some cases, freetype may
    // report a larger pixmap than possible.
    auto width = pixmap.width;
#if 0
    if (font->outline <= 0 && width > glyph->maxx - glyph->minx)
        width = glyph->maxx - glyph->minx;
#endif

    for (auto row = 0; row < pixmap.rows; ++row)
    {
        // Make sure we don't go either over, or under the limit
        if (row + yoffset < 0)
            continue;

        if (row + yoffset >= textbuf->h)
            continue;

        auto dst = static_cast<Uint32*>(textbuf->pixels) +
            (row + yoffset) * textbuf->pitch/4 +
            xstart + minx;

        // Added code to adjust src pointer for pixmaps to account for pitch.
        auto src = static_cast<Uint8*>(pixmap.buffer) + pixmap.pitch * row;
        for (auto col = width; col>0 && dst < dst_check; --col)
        {
            auto alpha = *src++;
            *dst++ |= pixel | (alpha << 24);
        }
    }

    global::test_text_surf = textbuf;
}
