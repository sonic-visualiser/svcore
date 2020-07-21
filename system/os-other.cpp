
extern "C" {
    
bool
OSReportsDarkThemeActive()
{
    return false;
}

bool
OSQueryAccentColour(int &r, int &g, int &b)
{
    (void)r;
    (void)g;
    (void)b;
    return false;
}

}
