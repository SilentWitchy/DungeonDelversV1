#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "gfx/Color.h"
#include "world/WorldGenSettings.h"

class Font;
class Renderer;

class Ui
{
public:
    explicit Ui(Font& font);

    void MainMenuTick(bool upPressed, bool downPressed, bool selectPressed);
    void MainMenuRender(Renderer& r);
    int  MainMenuSelection() const { return m_mainMenuSelection; }
    bool MainMenuActivated() const { return m_mainMenuActivated; }
    void ClearMainMenuActivated();

    void SettingsTick(bool upPressed, bool downPressed, bool selectPressed, bool backPressed);
    void SettingsRender(Renderer& r);
    bool SettingsBackRequested() const { return m_settingsBackRequested; }
    void ClearSettingsBackRequest();

    void WorldGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed,
        bool selectPressed, bool backPressed);
    void WorldGenRender(Renderer& r);

    void MapGenTick(bool upPressed, bool downPressed, bool leftPressed, bool rightPressed, bool confirmPressed, bool backPressed, int wheelDelta);
    void MapGenRender(Renderer& r);
    bool MapSelectionComplete() const { return m_spawnConfirmed; }
    bool MapGenBackRequested() const { return m_mapGenBackRequested; }
    void ClearMapSelectionFlags() { m_spawnConfirmed = false; m_mapGenBackRequested = false; }

    void BeginMapLoading(const WorldGenSettings& settings);
    void MapLoadingRender(Renderer& r);

    bool WorldGenStartRequested() const { return m_worldGenStartRequested; }
    bool WorldGenBackRequested() const { return m_worldGenBackRequested; }
    void ClearWorldGenRequests();

    WorldGenSettings GetWorldGenSettings() const;

    void SetStatusMessage(const std::string& text);

private:
    Font& m_font;

    // Main menu state
    int  m_mainMenuSelection = 0; // 0 = New World, 1 = Settings, 2 = Quit
    bool m_mainMenuActivated = false;

    // Settings menu state
    int  m_settingsSelection = 0;
    bool m_settingsBackRequested = false;
    std::string m_settingsDetail;

    int m_wgRow = 0;                 // which of the 7 options (0..6)
    int m_wgChoice[7] = { 2,2,2,2,2,2,2 }; // default to "Middle" option
    bool m_worldGenStartRequested = false;
    bool m_worldGenBackRequested = false;

    std::string m_statusMessage;

    int m_lastMapPreviewWorldSize = -1;
    uint32_t m_mapPreviewSeed = 0u;

    enum class MapViewStage
    {
        Region,
        Local
    };

    int RegionResolution() const;
    void BuildRegionBiomeSummaries();
    void GenerateLocalTerrain(int regionX, int regionY);
    int SampleBiome(uint8_t v) const;
    Color BiomeColor(int biome) const;

    MapViewStage m_mapViewStage = MapViewStage::Region;
    bool m_regionBiomesReady = false;
    int m_regionResolution = 0;
    std::vector<int> m_regionBiomeSummary; // size = res * res
    std::vector<int> m_localBlockBiomes;   // size = (res*16)^2 for region-level aggregation
    int m_selectedRegionX = 0;
    int m_selectedRegionY = 0;

    static constexpr int LOCAL_BLOCKS_PER_REGION = 16;
    static constexpr int TERRAIN_TILES_PER_BLOCK = 48;
    std::vector<int> m_localTerrainBiomes; // size = (LOCAL_BLOCKS_PER_REGION*TERRAIN_TILES_PER_BLOCK)^2 for selected region
    int m_localSelectionBlockX = 0;
    int m_localSelectionBlockY = 0;
    bool m_spawnConfirmed = false;
    bool m_mapGenBackRequested = false;

    WorldGenSettings m_loadingSettings{};
    std::string m_loadingMessage;
};
