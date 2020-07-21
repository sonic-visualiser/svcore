
#ifndef AVOID_WINRT_DEPENDENCY
#ifdef _MSC_VER
#include <winrt/Windows.UI.ViewManagement.h>
#endif
#endif

extern "C" {

bool
OSReportsDarkThemeActive()
{
#ifndef AVOID_WINRT_DEPENDENCY
#ifdef _MSC_VER
    using namespace winrt::Windows::UI::ViewManagement;
    UISettings settings;
    auto background = settings.GetColorValue(UIColorType::Background);
    if (int(background.R) + int(background.G) + int(background.B) < 384) {
        return true;
    }
#endif
#endif
    return false;
}

bool
OSQueryAccentColour(int *r, int *g, int *b)
{
#ifndef AVOID_WINRT_DEPENDENCY
#ifdef _MSC_VER
    using namespace winrt::Windows::UI::ViewManagement;
    bool dark = OSReportsDarkThemeActive();
    UISettings settings;
    auto accent = settings.GetColorValue
        (dark ? UIColorType::AccentLight1 : UIColorType::Accent);
    *r = accent.R;
    *g = accent.G;
    *b = accent.B;
    return true;
#endif
#endif
    (void)r;
    (void)g;
    (void)b;
    return false;
}

}

