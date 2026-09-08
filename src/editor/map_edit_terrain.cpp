#include "mapedit/runtime.hpp"

namespace {
constexpr unsigned int FourCC(char a, char b, char c, char d)
{
    return static_cast<unsigned int>(static_cast<unsigned char>(a)) |
           (static_cast<unsigned int>(static_cast<unsigned char>(b)) << 8) |
           (static_cast<unsigned int>(static_cast<unsigned char>(c)) << 16) |
           (static_cast<unsigned int>(static_cast<unsigned char>(d)) << 24);
}
}

// Imports a terrain VID set through mapedit.ini, then instantiates the extra VID
// on a 256-unit grid when the source contains multiple directions.
void MAP_EDIT::LoadTerrain(STRING filename)
{
    RESOURCE map;
    STRING ini;
    STRING tail;

    // The original destination at 0x004CE388 is a zero-initialized char buffer
    // referenced only here, so its observable value is an empty replacement.
    filename.Replace(FCurrentDirectory(), STRING(""));
    filename.RemoveBeginChars("\\");

    if (map.OpenForRead(&filename, FourCC('V', 'I', 'D', ' ')))
        return;

    map.Shift(4);
    int directionCount = 0;
    map.Read(&directionCount, 2);
    map.Close();

    if (!ini.LoadFile(STRING("mapedit.ini"))) {
        ::Error->Window("Can't load file mapedit.ini");
        return;
    }

    tail = ini.After("NoDir=");
    tail.RemoveBeginChars(" 0123456789");
    ini = ini.Before("NoDir=") + "NoDir=" + Int2Str(directionCount) + tail;
    ini = ini.Before("VidName=") + "VidName=" + filename;
    ini.SaveFile(STRING("mapedit.ini"));

    DeleteExtraVid();
    Save(STRING("tmp.map"));

    STRING temporaryMapName("tmp.map");
    RESOURCE temporaryMap(RESOURCE::OpenRead, &temporaryMapName, FourCC('M', 'A', 'P', ' '));
    if (temporaryMap.IsOpen())
        { STRING iniName("mapedit.ini"); temporaryMap.LoadIni(&iniName); }
    temporaryMap.Close();

    tail = m_mapName;
    Load(STRING("terrain vid"));
    editFileName = tail;
    DrawMapName();

    int terrainIndex = 0;
    while (terrainIndex < m_noVid) {
        if (VidSlot(terrainIndex) && VidSlot(terrainIndex)->IsExtraType())
            break;
        ++terrainIndex;
    }

    if (terrainIndex >= m_noVid) {
        Error(13, const_cast<char*>("terrain vid"), 0);
        return;
    }

    VID* terrainVid = Vid(terrainIndex);
    if (directionCount > 1) {
        const int columns = (static_cast<int>(SizeX()) - 1) / 256 + 1;
        for (int direction = 0; direction < directionCount; ++direction) {
            const float x = static_cast<float>((direction % columns) << 8) + 128.0f;
            const float y = static_cast<float>((direction / columns) << 8) + 128.0f;
            const ANGLE angle(static_cast<uint8_t>((direction << 8) / directionCount));
            CreateSprite(terrainVid, x, y, 0.0f, angle, 0);
        }
    } else {
        CreateSprite(terrainVid, SizeX() / 2.0f, SizeY() / 2.0f, 0.0f, ANGLE(static_cast<uint8_t>(0)), 0);
    }
}
