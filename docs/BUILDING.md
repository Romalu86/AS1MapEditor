# Building MapEdit with Visual Studio 2022

1. Install Visual Studio 2022 with the **Desktop development with C++** workload
   and a Windows 10/11 SDK.
2. Open `vs2022/MapEdit.sln`.
3. Select `Release` and `Win32`.
4. Build the `MapEdit` project.
5. The executable is written to `build/Release/MapEdit.exe`.

No dependency downloader is used. The required Win32 static Ogg/Vorbis
libraries are included in `third_party/xiph/lib/Win32/Release`.

The Release configuration uses `/MT`, optimization, `NDEBUG`, warnings at
level 3, and warnings-as-errors. Runtime assertions are not part of the public
release.

## Runtime configuration files

`MapEdit.exe` requires both `mapedit.ini` and `MapEdit.cfg` in its current working
directory. Both files are included at the repository root. Copy them next to the
built executable (or otherwise ensure they are present in the working directory)
before starting the editor.
