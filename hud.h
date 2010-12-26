#ifndef HUD_H_INCLUDED
#define HUD_H_INCLUDED

#include <string>
#include <vector>

using namespace std;

class CHUD
{
    public:
        void Init();
        void Render();
        void Printf(const char* pszFormat, ...);
    private:
        vector<string> console;
};

#endif // HUD_H_INCLUDED
