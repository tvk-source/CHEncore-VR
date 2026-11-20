/*
 * CH: Encore: PC-derived layered note/receptor artwork and geometry.
 * Gameplay/timing uses HOWL's exported C++ pipeline. Existing PS5 Vulkan base,
 * volk/RADV, dynamic rendering and direct-to-display presentation are retained.
 */
// The build wraps this file in `namespace sample_cloneheronative { ... }`.
// stb_image's symbols live in the GLOBAL namespace (compiled into the base via
// VulkanglTFModel.cpp). Close the wrapper's namespace to declare them, then reopen.
}  // close sample_cloneheronative (opened by the build wrapper)
extern "C" {
unsigned char* stbi_load(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels);
void stbi_image_free(void* retval_from_stbi_load);
const char* stbi_failure_reason(void);
}
#include "gameplay/drum_session.hpp"
#include "gameplay/draw_sort.hpp"
#include "downloader/song_package.hpp"
#include "gameplay/pc_drums_visuals.hpp"
#include "gameplay/profile_menu.hpp"
#include "gameplay/player_calibration.hpp"
#include "gameplay/controller_profile.hpp"
#include "gameplay/controller_navigation.hpp"
#include "gameplay/player_seats.hpp"
#include "gameplay/ps5_usb_input.hpp"
#include "gameplay/ps5_usb_midi.hpp"
#include "gameplay/midi_setup.hpp"
#include "gameplay/midi_gameplay.hpp"
#include "gameplay/song_storage.hpp"
#include "gameplay/case_file.hpp"
#include "gameplay/chart_catalog.hpp"
#include "gameplay/persistent_storage.hpp"
#include <unordered_map>
#include "gameplay/menu_preview.hpp"
#include "gameplay/play_menus.hpp"
#include "gameplay/gameplay_audio.hpp"
#include "gameplay/guitar_particles.hpp"
#include "gameplay/menu_repeat.hpp"
#include "gameplay/menu_press_filter.hpp"
#include "gameplay/menu_stick.hpp"
#include "gameplay/song_start_gate.hpp"
#include "gameplay/display_timing.hpp"
#include "gameplay/instrument_keyboard.hpp"
#include "gameplay/ps5_native_ime.hpp"
#include "gameplay/input_diagnostics.hpp"
#include "gameplay/results_stats.hpp"
#include "gameplay/pc_scoring.hpp"
#include "gameplay/pc_sustain.hpp"
#include "gameplay/pc_sustain_rules.hpp"
#include "gameplay/pc_solo.hpp"
#include "gameplay/saved_score.hpp"
#include "gameplay/diagnostics.hpp"
#include "gameplay/application_update.hpp"
#include "gameplay/application_version.hpp"
#include "gameplay/asset_pack.hpp"
#include "gameplay/update_package.hpp"
#include <unordered_map>
#include "gameplay/pc_star_power.hpp"
#include "gameplay/pc_star_power_clock.hpp"
#include "gameplay/pc_song_end.hpp"
#include "gameplay/pc_tick_clock.hpp"
#include "gameplay/pc_chart_rules.hpp"
#include "gameplay/pc_beats.hpp"
#include "gameplay/pc_countdown.hpp"
#include "gameplay/pc_resume.hpp"
#include "gameplay/pc_lyrics.hpp"
#include "gameplay/highway_view.hpp"
#include "gameplay/pc_player_camera.hpp"
#include "ps5_samples.h"
#if defined(PS5_HOST_REFERENCE)
#include "gameplay/host_reference_audio.hpp"
#else
#include "gameplay/audio_player.hpp"
#include "platform.h"
#endif
namespace sample_cloneheronative {

#include "vulkanexamplebase.h"
#include <vector>
#include <array>
#include <cstddef>
#include <bit>
#include <cctype>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#if !defined(PS5_HOST_REFERENCE)
#include <dev/usb/usb.h>
#include <dev/usb/usb_ioctl.h>
#include <sys/ioctl.h>
#include <atomic>
#include <pthread.h>
#endif

#if !defined(PS5_HOST_REFERENCE)
namespace {
struct Ps5ImeParam {
    int32_t userId;
    int32_t type;
    uint64_t supportedLanguages;
    int32_t enterLabel;
    int32_t inputMethod;
    void* filter;
    uint32_t option;
    uint32_t maxTextLength;
    uint16_t* inputTextBuffer;
    float posx;
    float posy;
    uint32_t horizontalAlignment;
    uint32_t verticalAlignment;
    const uint16_t* placeholder;
    const uint16_t* title;
    int8_t reserved[16];
};
struct Ps5ImeResult { int32_t endStatus; int8_t reserved[12]; };
static_assert(sizeof(Ps5ImeParam) == 96);
static_assert(offsetof(Ps5ImeParam, inputTextBuffer) == 40);
static_assert(offsetof(Ps5ImeParam, title) == 72);
static_assert(sizeof(Ps5ImeResult) == 16);
extern "C" int wsi_videoout_set_flip_rate(int rate);
// Exported output-status ABI documented by john-tornblom/ps5-payload-hbldr
// test.c. Query before the existing Vulkan WSI opens its own output handle.
struct Ps5OutputStatus {
    uint32_t resolution{},reserved0{};
    uint64_t reserved[5]{};
};
static_assert(sizeof(Ps5OutputStatus)==48);
extern "C" int sceVideoOutOpen(int32_t,int32_t,int32_t,const void*);
extern "C" int sceVideoOutClose(int32_t);
extern "C" int sceVideoOutGetOutputStatus(int32_t,Ps5OutputStatus*);
extern "C" int sceSysmoduleLoadModule(uint16_t moduleId);
extern "C" int sceUserServiceGetForegroundUser(int32_t* userId);
extern "C" int sceCommonDialogInitialize(void);
extern "C" int sceImeDialogInit(const Ps5ImeParam* param, const void* extended);
extern "C" int sceImeDialogGetStatus(void);
extern "C" int sceImeDialogGetResult(Ps5ImeResult* result);
extern "C" int sceImeDialogTerm(void);
extern "C" int sceImeDialogAbort(void);
extern "C" void sceImeParamInit(rhythm::native_ime::Param*);
extern "C" int sceImeOpen(const rhythm::native_ime::Param*,const void*);
extern "C" int sceImeUpdate(rhythm::native_ime::Handler);
extern "C" int sceImeClose(void);
}
#endif

struct Vertex    { float pos[3]; float color[3]; };
struct TexVertex {
    float pos[3];
    float uv[2];
    float color[4];
    float uvNext[2]{};
    float frameBlend{};
};

#include "pc_hud_assets.hpp"
#include "pc_meter_art.hpp"
#include "pc_render_order.hpp"
#include "pc_drums_assets.hpp"
#include "pc_six_fret_assets.hpp"
#include "pc_guitar_fx_assets.hpp"
#include "pc_highway_assets.hpp"
#include "pc_string_attach.hpp"
#include "pc_player_scaling.hpp"
#include "pc_meter_attachments.hpp"
#include "pc_score_assets.hpp"
#include "pc_gameplay_labels.hpp"

struct Tex {
    VkImage image{ VK_NULL_HANDLE };
    VkDeviceMemory mem{ VK_NULL_HANDLE };
    VkImageView view{ VK_NULL_HANDLE };
    VkSampler sampler{ VK_NULL_HANDLE };
};

class VulkanExample : public VulkanExampleBase
{
public:
    // textured sprites (notes + receptors), rebuilt per frame
    std::array<vks::Buffer, maxConcurrentFrames> texBuffers;
    uint32_t texVertexCount{ 0 };
    std::array<vks::Buffer, maxConcurrentFrames> sustainBuffers;
    uint32_t sustainVertexCount{ 0 };
    // scrolling board quad, rebuilt per frame
    std::array<vks::Buffer, maxConcurrentFrames> boardBuffers;
    uint32_t boardVertexCount{ 0 };

    glm::vec3 gameplayEye{0.0f,5.0f,19.85f};
    struct UniformData { glm::mat4 viewProj; glm::vec4 sustainFade; } uniformData;
    std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;
    std::array<VkDescriptorSet, maxConcurrentFrames> atlasSets{},drumAtlasSets{},sixFretSets{},guitarFxSets{};
    std::array<VkDescriptorSet, maxConcurrentFrames> boardSets{};
    std::array<int,maxConcurrentFrames> boardStyleIds{};
    VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkPipeline highwayGlowPipeline{ VK_NULL_HANDLE };
    VkPipeline texPipeline{ VK_NULL_HANDLE };
    VkPipeline sustainPipeline{ VK_NULL_HANDLE };
    VkPipeline boardPipeline{ VK_NULL_HANDLE };

    Tex meterTex;
    std::array<VkDescriptorSet,maxConcurrentFrames> meterSets{};
    std::array<vks::Buffer,maxConcurrentFrames> meterBuffers;
    VkPipeline meterPipeline{},meterGlowPipeline{},spMeterPipeline{};
    struct MeterDraw { uint32_t first; bool additive,sp,fade,numeralGlow; int renderer; };
    std::vector<TexVertex> meterScratch;
    std::vector<MeterDraw> meterDraws;
    enum class GameplayDrawKind { Board,Atlas,Drums,SixFret,GuitarFx,Highway,Beat,Sustain,Meter };
    struct GameplayDraw {
        pc_render_order::Key key;
        GameplayDrawKind kind;
        uint32_t first,count;
        bool additive{};
        size_t meterIndex{},sequence{};
    };
    std::vector<GameplayDraw> spriteDraws,gameplayDraws;
    bool renderProfileQa{},renderBatching{true};
    VkQueryPool renderTimestampPool{VK_NULL_HANDLE};
    std::vector<bool> renderTimestampPending;
    double profileGpuMs{-1},profileUpdateMs{},profileGeometryMs{};
    unsigned profileDraws{},profileBinds{},profileVertices{};
    struct MeterParams { glm::vec4 state; glm::vec4 electricity; };

    Tex highwayTex;
    std::array<VkDescriptorSet,maxConcurrentFrames> highwaySets{};
    uint32_t highwayStaticCount{}, highwayVertexCount{};
    std::array<float,5> stringAttachScale{{1,1,1,1,1}};
    std::array<float,2> meterAttachScale{{1,1}};
    float spHighwayAlpha{}, spHighwayTime{};
    bool spHighwayActive{};
    Tex beatTex;
    std::array<VkDescriptorSet,maxConcurrentFrames> beatSets{};
    uint32_t beatVertexCount{};
    struct BeatLine { double time; unsigned kind; };
    std::vector<BeatLine> beatLines;
    std::vector<double> measureTimes;
    std::vector<rhythm::PcCountdown> countdownRanges;
    std::array<Tex,4> highwayStyleTextures{};
    Tex atlasTex, drumAtlasTex, sixFretTex, guitarFxTex, boardTex, menuLogoTex, menuButtonTex, menuAlbumTex;
    Tex starPowerBaseTex, starPowerFillTex;
    Tex failMeterBaseTex, failMeterUnderTex, hudArrowTex;
    Tex failMeterGlowTextures[3];
    Tex multiplierBackgroundTex, multiplierOverlayTex, multiplierXTex;
    Tex scoreGlyphTextures[11], multiplierGlyphTextures[10];
    std::array<Tex, std::size(pc_score::layers)> sourceScoreTextures;
    std::array<ImTextureID, std::size(pc_score::layers)> sourceScoreIds{};
    Tex scorePanelTex;
    ImTextureID scorePanelId{};
    Tex starBadgeGoldTex, starBadgeGreyTex, starOutlineGoldTex, starOutlineGreyTex, starGlowTex;
    Tex starProgressBackTex, starProgressFillTex;
    std::array<Tex, 14> menuBackgroundTextures{};
    std::array<Tex, 4> downloaderInstrumentTextures{};
    std::array<Tex, 5> chartTrackExtraTextures{};
    std::array<Tex, 7> songSelectControlTextures{};
    std::array<Tex,8> menuDetailTextures{};
    std::array<Tex,8> resultsUiTextures{};
    std::array<ImTextureID,8> resultsUiIds{};
    std::array<ImTextureID,8> menuDetailIds{};
    Tex fiveLaneDrumsInstrumentTexture, rhythmInstrumentTexture;
    std::array<ImTextureID, 11> scoreGlyphIds{};
    std::array<ImTextureID, 10> multiplierGlyphIds{};
    std::array<ImTextureID, 14> menuBackgroundIds{};
    std::array<ImTextureID, 4> downloaderInstrumentIds{};
    std::array<ImTextureID, 5> chartTrackExtraIds{};
    std::array<ImTextureID, 7> songSelectControlIds{};
    ImTextureID fiveLaneDrumsInstrumentId{}, rhythmInstrumentId{};
    Tex downloaderCoverTex;
    ImTextureID downloaderCoverId{};
    std::string downloaderCoverLoadedPath;
    Tex menuPlayerPanelTex;
    ImTextureID menuLogoId{}, menuButtonId{}, menuAlbumId{}, menuPlayerPanelId{};
    ImVec2 menuLogoUV0{0,0},menuLogoUV1{1,1};
    float menuLogoAspect{1.0f};
    ImTextureID starPowerBaseId{}, starPowerFillId{};
    ImTextureID failMeterBaseId{}, failMeterUnderId{}, hudArrowId{};
    ImTextureID failMeterGlowIds[3]{};
    ImTextureID multiplierBackgroundId{}, multiplierOverlayId{}, multiplierXId{};
    ImTextureID starBadgeGoldId{}, starBadgeGreyId{}, starOutlineGoldId{}, starOutlineGreyId{}, starGlowId{};
    ImTextureID starProgressBackId{}, starProgressFillId{};
    bool menuAlbumVisible{ false };
    int menuBackgroundIndex{ 0 };
    int menuBackgroundAnimation{ 2 };
    std::string menuAlbumTexturePath, menuPreviewChartPath, menuPreviewAlbumPath;
    std::string menuArtPendingChart,menuArtPendingPath;double menuArtPendingSince{};
    std::string menuPreviewAudioChartPath;
    std::string loadedAudioChartPath;
    int menuMusicSelection{ -1 };
    rhythm::PreviewHistory menuPreviewHistory;
    std::vector<std::string> menuPreviewFailedPaths;
    double previewClipStart{},previewClipLength{};
    bool previewPlayingForMenu{false},menuPreviewMuted{false};
    bool menuIntroStarted{false},menuIntroFinished{false};
    double menuWidgetRevealAt{-1.0};
    unsigned badNoteVariant{};
    double menuPreviewChangedAt{ 0.0 };
    int previewQueuedSongSelection{ -1 };
    double previewQueueChangedAt{ 0.0 };

    enum class Screen { MainMenu, SongSelect, SongSearch, PlayerSelect, Online, Settings, SettingsDetail, ControllerRemap, Downloader, Gameplay, Results, ControllerSetup, SongCopyConfirm, SongRemove, MidiSetup };
    enum class PlayMode { Quickplay, Versus, Practice };
    enum class PauseAction { Restart, NewSong, Practice, Quickplay, Quit };
    PauseAction pendingPauseAction{PauseAction::Quit};
    int confirmationReturnRow{};
    enum class PausePage { Main, Options, Volume, Calibration, SongSettings, PracticeSection, Sections, ConfirmQuit };
    Screen screen{ Screen::MainMenu };
    Screen playerSelectReturn{ Screen::MainMenu };
    PlayMode playMode{ PlayMode::Quickplay };
    PausePage pausePage{ PausePage::Main };
    rhythm::PcResume resumeRewind;
    // Keep the PC Main Options ordering; Quickplay is the initial highlight.
    static constexpr const char* buildVersion=rhythm::updates::applicationVersionLabel;
    int mainSelection{ 0 }, songSelection{ 0 }, settingsSelection{ 0 }, settingsCategorySelection{ 0 }, settingsCategory{ 0 };
    enum class SettingsEditor { None, Choices, Slider };
    SettingsEditor settingsEditor{SettingsEditor::None};
    int settingsChoice{0};
    float settingsSliderValue{0};
    bool versionNotesExpanded{false};
    float versionNotesReveal{},versionNotesScrollTime{},versionNotesSlideElapsed{};
    bool versionNotesExpanding{false};
    bool encoreFeaturesExpanded{false},encoreFeaturesExpanding{false};
    float encoreFeaturesReveal{},encoreFeaturesScrollTime{},encoreFeaturesSlideElapsed{};
    void toggleEncoreFeatures() noexcept {
        encoreFeaturesExpanded=!encoreFeaturesExpanded;
        if(encoreFeaturesExpanded){encoreFeaturesScrollTime=0;versionNotesExpanded=false;}
    }
    void toggleMenuPreviewMute() noexcept {
        menuPreviewMuted=!menuPreviewMuted;
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.frameBudget) {
            FILE* trace=std::fopen("/app0/preview-test.txt","a");
            if(trace) { std::fprintf(trace,"mute %d selection %d\n",menuPreviewMuted?1:0,mainSelection);std::fclose(trace); }
        }
#endif
    }
    void toggleVersionNotes() noexcept {
        versionNotesExpanded=!versionNotesExpanded;
        if(versionNotesExpanded){versionNotesScrollTime=0;encoreFeaturesExpanded=false;}
    }
    int resultsSelection{ 0 };
    float resultsScroll{};
    std::string resultsIconKey;
    rhythm::ResultsStats resultStats;
    rhythm::MenuRepeat menuRepeat;
    std::array<rhythm::MenuPressFilter,PAD_PLAYERS> menuPressFilters;
    std::array<rhythm::MenuPressFilter,PAD_PLAYERS> keyboardAssistFilters;
    std::array<uint64_t,PAD_PLAYERS> menuPressFrames{};
    std::array<uint32_t,PAD_PLAYERS> menuPressCache{};
    uint64_t menuInputFrame{};
    double menuInputTime{};
    uint32_t filterMenuPresses(int player,uint32_t edges) {
        // P1's global screen and its player card read the same input in a frame.
        // Share the accepted edges so filtering one consumer cannot starve another.
        if(menuPressFrames[player]!=menuInputFrame) {
            menuPressFrames[player]=menuInputFrame;
            menuPressCache[player]=menuPressFilters[player].update(edges,menuInputTime);
        }
        return menuPressCache[player];
    }
    std::array<rhythm::MenuStick,PAD_PLAYERS> menuSticks;
    std::array<rhythm::MenuRepeat,PAD_PLAYERS> playerMenuRepeat;
    struct PlayerRemap {
        bool active{},waitForRelease{true},raw{},calibrated{};
        rhythm::controls::Remap wizard;
        rhythm::controls::Mapping navigation;
        rhythm::controls::ControllerProfile draft;
        rhythm::controls::ReportCapture capture;
        rhythm::controls::CapturePressFilter captureFilter;
        rhythm::controls::Locator candidate;
        double calibrationAt{};
        int assistSource{-1};
        uint32_t previousButtons{};
        std::string message;
    };
    std::array<PlayerRemap,PAD_PLAYERS> playerRemaps;
    struct PlayerCalibration {
        enum class Phase {None,WaitingAudio,Timing,TimingReview,Velocity,VelocityReview};
        Phase phase{Phase::None};rhythm::TimingCalibration timing;rhythm::midi::SetupWizard velocity;
        int type{},profileId{-2},source{-1},row{};uint64_t generation{},audioEpoch{};
        uint32_t previousButtons{},previousTap{};uint64_t lastAudioAt{};bool waitRelease{};std::string message;
        bool active()const{return phase!=Phase::None;}
    };
    std::array<PlayerCalibration,PAD_PLAYERS> playerCalibrations;
    rhythm::controls::ControllerProfile activeControllerProfile;
    int unknownController{-1},unknownControllerChoice{};
    std::array<uint32_t,4> ignoredControllers{};
    int remapPlayer{}; // Last opened panel, for bounded diagnostic traces only.
    int pauseSelection{ 0 };
    float noteSpeedScale{ 1.5f };
    int gemSizePercent{ 100 };
    int practiceStartDelayMs{ 2000 };
    bool practiceStarPowerEnabled{ true };
    int noteAnimationFps{ 20 };
    int starAnimationFps{ 20 };
    double noteAnimationRemainder{};
    double starAnimationRemainder{};
    size_t noteAnimationFrame{};
    size_t starAnimationFrame{};
    float noteAnimationBlend{};
    float starAnimationBlend{};
    float practiceRate{ 1.0f };
    double practiceCountdownRemaining{ 0.0 };
    double practiceSectionA{ 0.0 };
    double practiceSectionB{ 0.0 };
    bool practiceSectionASet{ false }, practiceSectionBSet{ false };
    char practiceSectionMessage[96]{};
    struct PracticeMark {std::string name;double time;};
    std::vector<PracticeMark> practiceMarks;
    std::vector<rhythm::LyricPhrase> lyricPhrases;
    bool showLyrics{true};
    float hitWindowMs{ 80.0f };
    float strumDebounceMs{5.0f};
    float videoOffsetMs{ 0.0f };
    float audioOffsetMs{ 0.0f };
    double chartOffsetSeconds{ 0.0 };
    double trackStartOffsetSeconds{ 0.0 };
    double globalChartEndSeconds{},authoredSongEndSeconds{-1.0};
    // Match PC settings.ini defaults: master=50, song=100, menu/preview/sounds/mute=50.
    float masterVolume{ 0.5f }, musicVolume{ 1.0f }, menuVolume{ 0.5f }, previewVolume{ 0.5f };
    float effectsVolume{ 0.5f }, missVolume{ 0.5f }, missSoundVolume{1.0f};
    // PC per-stem volume defaults: guitar, bass, rhythm, vocals, drums, keys, crowd.
    std::array<float, 7> instrumentVolumes{{ 1, 1, 1, 1, 1, 1, 1 }};
    float menuScale{ 1.0f }, rumbleStrength{ 0.65f };
    // 0 follows presentation pacing; 1 and 2 cap submissions at 60 and 120.
    int frameRateLimit{ 0 };
    rhythm::DisplayTiming displayTiming;
    int appliedFlipRate{-1},appliedFrameRatePreference{-1};
    bool displayPacingApplied{};
    void inspectOutputResolution() {
#if !defined(PS5_HOST_REFERENCE)
        // One-launch diagnostic, gated by the bounded console test runner.
        if(ps5.frameBudget)if(FILE* flag=std::fopen("/app0/ime-1080-test.txt","r")) {
            std::fclose(flag);std::remove("/app0/ime-1080-test.txt");
            width=1920;height=1080;return;
        }
        Ps5OutputStatus status{};
        int result=-1;
        const int handle=sceVideoOutOpen(0xff,0,0,nullptr);
        if(handle>=0) {
            result=sceVideoOutGetOutputStatus(handle,&status);
            (void)sceVideoOutClose(handle);
        }
        // Public VideoOut implementations identify 1 as 1080p and 2 as
        // 2160p. Unknown selectors retain the existing safe render size;
        // in particular, do not guess the selector for 1440p.
        const uint32_t targetWidth=result==0?(status.resolution==1?1920:status.resolution==2?3840:0):0;
        const uint32_t targetHeight=targetWidth==1920?1080:2160;
        bool matched=false;
        uint32_t count=0;
        if(targetWidth && vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice,&count,nullptr)==VK_SUCCESS) {
            std::vector<VkDisplayPropertiesKHR> displays(count);
            if(vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice,&count,displays.data())==VK_SUCCESS)
                for(const auto& display:displays) {
                    uint32_t modes=0;
                    if(vkGetDisplayModePropertiesKHR(physicalDevice,display.display,&modes,nullptr)!=VK_SUCCESS)continue;
                    std::vector<VkDisplayModePropertiesKHR> properties(modes);
                    if(vkGetDisplayModePropertiesKHR(physicalDevice,display.display,&modes,properties.data())!=VK_SUCCESS)continue;
                    for(const auto& mode:properties)if(mode.parameters.visibleRegion.width==targetWidth &&
                        mode.parameters.visibleRegion.height==targetHeight)matched=true;
                }
        }
        if(matched){width=targetWidth;height=targetHeight;}
        if(FILE* file=std::fopen("/app0/output-status.txt","w")) {
            std::fprintf(file,"query_result %d resolution_code %u matched %d render %u %u\n",result,status.resolution,matched,width,height);
            std::fclose(file);
        }
#endif
    }
    void inspectDisplayTiming() {
#if !defined(PS5_HOST_REFERENCE)
        uint32_t count=0;
        if(vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice,&count,nullptr)!=VK_SUCCESS)return;
        std::vector<VkDisplayPropertiesKHR> displays(count);
        if(vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice,&count,displays.data())!=VK_SUCCESS)return;
        uint32_t fastest=0;
        for(const auto& display:displays) {
            uint32_t modes=0;
            if(vkGetDisplayModePropertiesKHR(physicalDevice,display.display,&modes,nullptr)!=VK_SUCCESS)continue;
            std::vector<VkDisplayModePropertiesKHR> properties(modes);
            if(vkGetDisplayModePropertiesKHR(physicalDevice,display.display,&modes,properties.data())!=VK_SUCCESS)continue;
            for(const auto& mode:properties)if(mode.parameters.visibleRegion.width==width && mode.parameters.visibleRegion.height==height)
                fastest=std::max(fastest,mode.parameters.refreshRate);
        }
        if(fastest)displayTiming.refreshMilliHz=fastest;
#endif
    }
    void applyDisplayPacing() {
        const int rate=displayTiming.flipRate(frameRateLimit);
        if(rate==appliedFlipRate && frameRateLimit==appliedFrameRatePreference)return;
        appliedFlipRate=rate;appliedFrameRatePreference=frameRateLimit;
#if !defined(PS5_HOST_REFERENCE)
        displayPacingApplied=wsi_videoout_set_flip_rate(rate)==0;
        if(FILE* f=std::fopen("/app0/display-status.txt","w")) {
            std::fprintf(f,"render %u %u refresh_millihz %u supports_120 %d preference %d effective %d flip_rate %d pacing %d\n",
                width,height,displayTiming.refreshMilliHz,displayTiming.supports120(),frameRateLimit,
                displayTiming.effectiveChoice(frameRateLimit),rate,displayPacingApplied);
            std::fclose(f);
        }
#endif
    }
    bool gamepadMode{ false };
    // PC BaseEngine starts the rock meter at 50%; PC No Fail defaults on.
    float gameplayHealth{ 0.5f };
    bool noFail{ true };
    bool songFailed{ false };
    bool showFps{ false }, showHitWindow{ false }, muteOnMiss{ true }, horizontalStrum{ false }, controllerVibration{ true };
    bool showSongDisplay{ true };
    // PC game.measure_countdown chooses measure numbers vs rounded seconds
    // during long rests; it does not turn the song's pre-roll on/off.
    bool measureCountdown{ true };
    // PC game.allow_duplicate_songs: when enabled, include separate installs
    // whose chart files contain identical chart data during a rescan.
    bool allowDuplicateSongs{ false };
    // PC video.highway_shake (default on): briefly moves the guitar camera
    // when a chart note expires without a hit.
    bool highwayShakeEnabled{ true };
    float highwayShakeTime{ 0.0f };
    // PC video.note_sp_colors (default on): draw star phrase gems with their
    // special atlas art. The phrase/judgement data remains unchanged when off.
    bool noteStarPowerColors{ true };
    bool missMuted{ false };
    bool rumbleActive{ false };
    double rumbleStopTime{ 0.0 };
    float currentSpeedZ() const { return SPEED_Z * noteSpeedScale; }

    int settingsDetailRowCount() const {
        switch (settingsCategory) {
        case 0: return 12; // General, song management and application updates
        case 1: return 14; // Audio controls plus independent missed-note sounds
        case 2: return 10; // PC video settings, lyrics and native frame pacing.
        case 3: return 11; // Gameplay and independently adjustable strum filter
        case 4: return 3; // PS5 controller extensions; mapping lives in player menus
        default: return 0;
        }
    }

    void loadSettings() {
        FILE* file = fopen(CH_USERDATA "/clonehero-settings.cfg", "rb");
        if (!file) return;

        char magic[32]{};
        unsigned version = 0;
        float storedMenuScale = 1.0f, storedMaster = 1.0f, storedMusic = 1.0f;
        float storedEffects = 1.0f, storedAudioOffset = 0.0f, storedNoteSpeed = 1.5f;
        float storedHitWindow = 80.0f, storedVideoOffset = 0.0f, storedRumble = 0.65f;
        int storedShowFps = 0, storedGamepad = 0, storedShowHitWindow = 0, storedMuteOnMiss = 1, storedFrameRate = 0;
        int storedNoteAnimation = 20, storedStarAnimation = 20;
        int storedGemSize = 100;
        int storedSongDisplay = 1, storedLyrics = 1;
        int storedNoteStarPowerColors = 1;
        int storedMeasureCountdown = 1;
        int storedHighwayShake = 1;
        int storedPracticeDelay = 2000;
        int storedPracticeStarPower = 1;
        int storedSongSortField = 0;
        int storedDefaultSearchBySort = 0;
        int storedMenuBackground = 0;
        int storedMenuBackgroundAnimation = 2;
        int storedAllowDuplicateSongs = 0;
        int storedNoFail = 1;
        float storedStrumDebounce=5.0f;
        float storedMenuVolume = 0.5f, storedPreviewVolume = 0.5f, storedMissVolume = 0.5f, storedMissSoundVolume=1.0f;
        float storedInstrumentVolumes[7]{ 1, 1, 1, 1, 1, 1, 1 };
        int stemFields = 0;
        int storedHorizontalStrum = 0, storedVibration = 1;
        unsigned storedBindings[5]{};
        unsigned storedStarPowerBinding = PAD_TOUCH_PAD;
        if (fscanf(file, "%31s %u", magic, &version) != 2 || strcmp(magic, "CHPS5SET") != 0 ||
            (version < 1 || version > 28)) {
            fclose(file);
            return;
        }
        int fields = 0;
        if (version == 1) {
            fields = fscanf(file,
                "%f %d %f %f %f %f %f %f %d %f %d %d %d %f %u %u %u %u %u",
                &storedMenuScale, &storedShowFps, &storedMaster, &storedMusic,
                &storedEffects, &storedAudioOffset, &storedNoteSpeed, &storedHitWindow,
                &storedGamepad, &storedVideoOffset, &storedFrameRate,
                &storedHorizontalStrum, &storedVibration, &storedRumble,
                &storedBindings[0], &storedBindings[1], &storedBindings[2],
                &storedBindings[3], &storedBindings[4]);
            storedShowHitWindow = 0;
        } else if (version == 2) {
            fields = fscanf(file,
                "%f %d %f %f %f %f %f %f %d %d %f %d %d %d %f %u %u %u %u %u",
                &storedMenuScale, &storedShowFps, &storedMaster, &storedMusic,
                &storedEffects, &storedAudioOffset, &storedNoteSpeed, &storedHitWindow,
                &storedGamepad, &storedShowHitWindow, &storedVideoOffset,
                &storedFrameRate, &storedHorizontalStrum, &storedVibration, &storedRumble,
                &storedBindings[0], &storedBindings[1], &storedBindings[2],
                &storedBindings[3], &storedBindings[4]);
        } else if (version == 3) {
            fields = fscanf(file,
                "%f %d %f %f %f %f %f %f %d %d %d %f %d %d %d %f %u %u %u %u %u",
                &storedMenuScale, &storedShowFps, &storedMaster, &storedMusic,
                &storedEffects, &storedAudioOffset, &storedNoteSpeed, &storedHitWindow,
                &storedGamepad, &storedShowHitWindow, &storedMuteOnMiss, &storedVideoOffset,
                &storedFrameRate, &storedHorizontalStrum, &storedVibration, &storedRumble,
                &storedBindings[0], &storedBindings[1], &storedBindings[2],
                &storedBindings[3], &storedBindings[4]);
        } else if (version == 4) {
            fields = fscanf(file,
                "%f %d %f %f %f %f %f %f %d %d %d %f %d %d %d %f %u %u %u %u %u %d",
                &storedMenuScale, &storedShowFps, &storedMaster, &storedMusic,
                &storedEffects, &storedAudioOffset, &storedNoteSpeed, &storedHitWindow,
                &storedGamepad, &storedShowHitWindow, &storedMuteOnMiss, &storedVideoOffset,
                &storedFrameRate, &storedHorizontalStrum, &storedVibration, &storedRumble,
                &storedBindings[0], &storedBindings[1], &storedBindings[2],
                &storedBindings[3], &storedBindings[4], &storedNoteAnimation);
        } else {
            fields = fscanf(file,
                "%f %d %f %f %f %f %f %f %d %d %d %f %d %d %d %f %u %u %u %u %u %d %f",
                &storedMenuScale, &storedShowFps, &storedMaster, &storedMusic,
                &storedEffects, &storedAudioOffset, &storedNoteSpeed, &storedHitWindow,
                &storedGamepad, &storedShowHitWindow, &storedMuteOnMiss, &storedVideoOffset,
                &storedFrameRate, &storedHorizontalStrum, &storedVibration, &storedRumble,
                &storedBindings[0], &storedBindings[1], &storedBindings[2],
                &storedBindings[3], &storedBindings[4], &storedNoteAnimation, &storedMenuVolume);
            if (version >= 7) {
                const int previewFields = fscanf(file, "%f", &storedPreviewVolume);
                if (previewFields != 1) fields = -1;
            }
            if (version >= 8) {
                const int muteFields = fscanf(file, "%f", &storedMissVolume);
                if (muteFields != 1) fields = -1;
            }
            if (version >= 6) {
                stemFields = fscanf(file, "%f %f %f %f %f %f %f",
                    &storedInstrumentVolumes[0], &storedInstrumentVolumes[1],
                    &storedInstrumentVolumes[2], &storedInstrumentVolumes[3],
                    &storedInstrumentVolumes[4], &storedInstrumentVolumes[5],
                    &storedInstrumentVolumes[6]);
            }
            if (version >= 9 && fscanf(file, "%d", &storedStarAnimation) != 1)
                fields = -1;
            if (version >= 10 && fscanf(file, "%u", &storedStarPowerBinding) != 1)
                fields = -1;
            if (version >= 11 && fscanf(file, "%d", &storedGemSize) != 1)
                fields = -1;
            if (version >= 12 && fscanf(file, "%d", &storedSongDisplay) != 1)
                fields = -1;
            if (version >= 13 && fscanf(file, "%d", &storedNoteStarPowerColors) != 1)
                fields = -1;
            if (version >= 14 && fscanf(file, "%d", &storedMeasureCountdown) != 1)
                fields = -1;
            if (version >= 15 && fscanf(file, "%d", &storedHighwayShake) != 1)
                fields = -1;
            if (version >= 16 && fscanf(file, "%d", &storedPracticeDelay) != 1)
                fields = -1;
            if (version >= 17 && fscanf(file, "%d", &storedPracticeStarPower) != 1)
                fields = -1;
            if (version >= 18 && fscanf(file, "%d", &storedSongSortField) != 1)
                fields = -1;
            if (version >= 19 && fscanf(file, "%d", &storedDefaultSearchBySort) != 1)
                fields = -1;
            if (version >= 20 && fscanf(file, "%d", &storedMenuBackground) != 1)
                fields = -1;
            if (version >= 21 && fscanf(file, "%d", &storedMenuBackgroundAnimation) != 1)
                fields = -1;
            if (version >= 22 && fscanf(file, "%d", &storedAllowDuplicateSongs) != 1)
                fields = -1;
            if (version >= 23 && fscanf(file, "%d", &storedNoFail) != 1)
                fields = -1;
            if (version >= 25 && fscanf(file, "%f", &storedStrumDebounce) != 1)
                fields = -1;
            if (version >= 26 && fscanf(file, "%d", &storedLyrics) != 1) fields = -1;
            if (version >= 27 && fscanf(file, "%f", &storedMissSoundVolume) != 1) fields = -1;
        }
        fclose(file);
        if (fields != (version == 1 ? 19 : (version == 2 ? 20 :
            (version == 3 ? 21 : (version == 4 ? 22 : 23)))) ||
            (version >= 6 && stemFields != 7) ||
            !std::isfinite(storedMenuScale) || storedMenuScale < 0.8f || storedMenuScale > 1.2f ||
            (storedShowFps != 0 && storedShowFps != 1) ||
            !std::isfinite(storedMaster) || storedMaster < 0.0f || storedMaster > 1.0f ||
            !std::isfinite(storedMusic) || storedMusic < 0.0f || storedMusic > 1.0f ||
            !std::isfinite(storedMenuVolume) || storedMenuVolume < 0.0f || storedMenuVolume > 1.0f ||
            !std::isfinite(storedPreviewVolume) || storedPreviewVolume < 0.0f || storedPreviewVolume > 1.0f ||
            !std::isfinite(storedMissSoundVolume) || storedMissSoundVolume<0 || storedMissSoundVolume>1 ||
            !std::isfinite(storedMissVolume) || storedMissVolume < 0.0f || storedMissVolume > 1.0f ||
            !std::isfinite(storedEffects) || storedEffects < 0.0f || storedEffects > 1.0f ||
            !std::isfinite(storedAudioOffset) || storedAudioOffset < -2000.0f || storedAudioOffset > 2000.0f ||
            !std::isfinite(storedNoteSpeed) || storedNoteSpeed < 0.5f || storedNoteSpeed > 2.0f ||
            !std::isfinite(storedStrumDebounce) || storedStrumDebounce<0 || storedStrumDebounce>50 ||
            !std::isfinite(storedHitWindow) || storedHitWindow < 35.0f || storedHitWindow > 150.0f ||
            (storedGamepad != 0 && storedGamepad != 1) ||
            (storedShowHitWindow != 0 && storedShowHitWindow != 1) ||
            (storedSongDisplay != 0 && storedSongDisplay != 1) ||
            (storedLyrics != 0 && storedLyrics != 1) ||
            (storedNoteStarPowerColors != 0 && storedNoteStarPowerColors != 1) ||
            (storedMeasureCountdown != 0 && storedMeasureCountdown != 1) ||
            (storedHighwayShake != 0 && storedHighwayShake != 1) ||
            storedPracticeDelay < 500 || storedPracticeDelay > 5000 || storedPracticeDelay % 100 != 0 ||
            (storedPracticeStarPower != 0 && storedPracticeStarPower != 1) ||
            storedSongSortField < 0 || storedSongSortField > 7 ||
            (storedDefaultSearchBySort != 0 && storedDefaultSearchBySort != 1) ||
            storedMenuBackground < 0 || storedMenuBackground > 13 ||
            storedMenuBackgroundAnimation < 0 || storedMenuBackgroundAnimation > 2 ||
            (storedAllowDuplicateSongs != 0 && storedAllowDuplicateSongs != 1) ||
            (storedNoFail != 0 && storedNoFail != 1) ||
            (storedMuteOnMiss != 0 && storedMuteOnMiss != 1) ||
            !std::isfinite(storedVideoOffset) || storedVideoOffset < -2000.0f || storedVideoOffset > 2000.0f ||
            storedFrameRate < 0 || storedFrameRate > 2 ||
            storedNoteAnimation < 1 || storedNoteAnimation > 60 ||
            storedStarAnimation < 1 || storedStarAnimation > 60 ||
            storedGemSize < -300 || storedGemSize > 300 ||
            (storedHorizontalStrum != 0 && storedHorizontalStrum != 1) ||
            (storedVibration != 0 && storedVibration != 1) ||
            !std::isfinite(storedRumble) || storedRumble < 0.0f || storedRumble > 1.0f)
            return;
        for (float volume : storedInstrumentVolumes)
            if (!std::isfinite(volume) || volume < 0.0f || volume > 1.0f) return;

        constexpr uint32_t validFretButtons[] = { PAD_CROSS, PAD_CIRCLE, PAD_TRIANGLE,
                                                   PAD_SQUARE, PAD_L1, PAD_R1 };
        for (int i = 0; i < 5; ++i) {
            bool valid = false;
            for (uint32_t button : validFretButtons) valid |= storedBindings[i] == button;
            if (!valid) return;
            for (int j = 0; j < i; ++j)
                if (storedBindings[i] == storedBindings[j]) return;
        }
        bool validStarPowerButton = false;
        for (uint32_t button : validFretButtons)
            validStarPowerButton |= storedStarPowerBinding == button;
        validStarPowerButton |= storedStarPowerBinding == PAD_TOUCH_PAD;
        if (!validStarPowerButton) return;
        for (unsigned binding : storedBindings)
            if (storedStarPowerBinding == binding) return;

        // Version 24 corrects the built-in five-fret layout to match the
        // rhythm_vdi bridge. Preserve custom mappings; migrate only the old
        // factory default.
        constexpr unsigned oldDefaultBindings[] = {
            PAD_CROSS, PAD_CIRCLE, PAD_TRIANGLE, PAD_SQUARE, PAD_L1
        };
        constexpr unsigned guitarBridgeBindings[] = {
            PAD_CROSS, PAD_CIRCLE, PAD_SQUARE, PAD_TRIANGLE, PAD_L1
        };
        bool oldFactoryDefault = version < 24;
        for (int i = 0; i < 5; ++i)
            oldFactoryDefault &= storedBindings[i] == oldDefaultBindings[i];
        if (oldFactoryDefault)
            for (int i = 0; i < 5; ++i) storedBindings[i] = guitarBridgeBindings[i];

        menuScale = storedMenuScale;
        showFps = storedShowFps != 0;
        masterVolume = storedMaster;
        musicVolume = storedMusic;
        menuVolume = storedMenuVolume;
        previewVolume = storedPreviewVolume;
        missVolume = storedMissVolume;missSoundVolume=storedMissSoundVolume;
        for (size_t i = 0; i < instrumentVolumes.size(); ++i)
            instrumentVolumes[i] = storedInstrumentVolumes[i];
        effectsVolume = storedEffects;
        audioOffsetMs = storedAudioOffset;
        noteSpeedScale = storedNoteSpeed;
        noteAnimationFps = storedNoteAnimation;
        starAnimationFps = storedStarAnimation;
        gemSizePercent = storedGemSize;
        showSongDisplay = storedSongDisplay != 0;
        showLyrics = storedLyrics != 0;
        noteStarPowerColors = storedNoteStarPowerColors != 0;
        measureCountdown = storedMeasureCountdown != 0;
        highwayShakeEnabled = storedHighwayShake != 0;
        practiceStartDelayMs = storedPracticeDelay;
        practiceStarPowerEnabled = storedPracticeStarPower != 0;
        songSortField = static_cast<SongSortField>(storedSongSortField);
        defaultSearchByCurrentSort = storedDefaultSearchBySort != 0;
        menuBackgroundIndex = storedMenuBackground;
        menuBackgroundAnimation = storedMenuBackgroundAnimation;
        allowDuplicateSongs = storedAllowDuplicateSongs != 0;
        noFail = storedNoFail != 0;
        hitWindowMs = version<28 && storedHitWindow==70.0f?80.0f:storedHitWindow;
        strumDebounceMs=storedStrumDebounce;
        session.setStrumDebounce(strumDebounceMs/1000.0);
        gamepadMode = storedGamepad != 0;
        showHitWindow = storedShowHitWindow != 0;
        muteOnMiss = storedMuteOnMiss != 0;
        videoOffsetMs = storedVideoOffset;
        frameRateLimit = storedFrameRate;
        horizontalStrum = storedHorizontalStrum != 0;
        controllerVibration = storedVibration != 0;
        rumbleStrength = storedRumble;
        for (int i = 0; i < 5; ++i) fretBindings[i] = storedBindings[i];
        starPowerBinding = storedStarPowerBinding;
        session.setHitWindow(hitWindowMs / 1000.0);
    }

    void saveSettings() const {
        char record[384];
        const int size = snprintf(record, sizeof(record),
            "CHPS5SET 28\n%.9g %d %.9g %.9g %.9g %.9g %.9g %.9g %d %d %d %.9g %d %d %d %.9g %u %u %u %u %u %d %.9g %.9g\n%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %d %u %d %d %d %d %d %d %d %d %d %d %d %d %d %.9g %d %.9g\n",
            menuScale, showFps ? 1 : 0, masterVolume, musicVolume, effectsVolume,
            audioOffsetMs, noteSpeedScale, hitWindowMs, gamepadMode ? 1 : 0,
            showHitWindow ? 1 : 0, muteOnMiss ? 1 : 0,
            videoOffsetMs, frameRateLimit, horizontalStrum ? 1 : 0,
            controllerVibration ? 1 : 0, rumbleStrength, fretBindings[0],
            fretBindings[1], fretBindings[2], fretBindings[3], fretBindings[4], noteAnimationFps, menuVolume, previewVolume,
            missVolume,
            instrumentVolumes[0], instrumentVolumes[1], instrumentVolumes[2], instrumentVolumes[3],
            instrumentVolumes[4], instrumentVolumes[5], instrumentVolumes[6], starAnimationFps,
            starPowerBinding, gemSizePercent, showSongDisplay ? 1 : 0,
            noteStarPowerColors ? 1 : 0, measureCountdown ? 1 : 0,
            highwayShakeEnabled ? 1 : 0, practiceStartDelayMs,
            practiceStarPowerEnabled ? 1 : 0, static_cast<int>(songSortField),
            defaultSearchByCurrentSort ? 1 : 0, menuBackgroundIndex, menuBackgroundAnimation,
            allowDuplicateSongs ? 1 : 0, noFail ? 1 : 0, strumDebounceMs, showLyrics ? 1 : 0, missSoundVolume);
        if (size <= 0 || static_cast<size_t>(size) >= sizeof(record)) return;

        constexpr char temporary[] = CH_USERDATA "/clonehero-settings.tmp";
        constexpr char current[] = CH_USERDATA "/clonehero-settings.cfg";
        FILE* file = fopen(temporary, "wb");
        if (!file) return;
        bool complete = fwrite(record, 1, static_cast<size_t>(size), file) == static_cast<size_t>(size);
        if (complete) complete = fflush(file) == 0 && fsync(fileno(file)) == 0;
        if (fclose(file) != 0) complete = false;
        if (!complete || rename(temporary, current) != 0) remove(temporary);
    }

    void resetSettings() {
        noteSpeedScale = 1.5f;
        gemSizePercent = 100;
        practiceStartDelayMs = 2000;
        practiceStarPowerEnabled = true;
        noteAnimationFps = 20;
        starAnimationFps = 20;
        practiceRate = 1.0f;
        practiceCountdownRemaining = 0.0;
        hitWindowMs = 80.0f;
        strumDebounceMs=5.0f;session.setStrumDebounce(.005);
        videoOffsetMs = audioOffsetMs = 0.0f;
        masterVolume = effectsVolume = menuVolume = previewVolume = missVolume = 0.5f;missSoundVolume=1.0f;
        musicVolume = 1.0f;
        instrumentVolumes.fill(1.0f);
        menuScale = 1.0f;
        rumbleStrength = 0.65f;
        frameRateLimit = 0;
        gamepadMode = showFps = showHitWindow = horizontalStrum = false;
        showSongDisplay = true;
        showLyrics = true;
        noteStarPowerColors = true;
        measureCountdown = true;
        highwayShakeEnabled = true;
        noFail = true;
        highwayShakeTime = 0.0f;
        muteOnMiss = true;
        missMuted = false;
        controllerVibration = true;
        fretBindings = {{ PAD_CROSS, PAD_CIRCLE, PAD_SQUARE, PAD_TRIANGLE, PAD_L1 }};
        starPowerBinding = PAD_TOUCH_PAD;
        setSongSortField(SongSortField::Song);
        defaultSearchByCurrentSort = false;
        menuBackgroundIndex = 0;
        menuBackgroundAnimation = 2;
        allowDuplicateSongs = false;
        session.setHitWindow(hitWindowMs / 1000.0);
        vibratePlayer(gameplayPlayer,0.0f, 0.0f);
        rumbleActive = false;
        saveSettings();
    }

    void adjustSettingsValue(int direction) {
        if (direction == 0) return;
        const float step = direction > 0 ? 1.0f : -1.0f;
        switch (settingsCategory) {
        case 0: // General
            if (settingsSelection == 5) cycleSongSort(direction);
            else if (settingsSelection == 6) defaultSearchByCurrentSort = !defaultSearchByCurrentSort;
            else if (settingsSelection == 7) allowDuplicateSongs = !allowDuplicateSongs;
            else if (settingsSelection == 8) menuScale = std::clamp(menuScale + step * 0.05f, 0.8f, 1.2f);
            break;
        case 1: // Audio; values are ready for the native mixer.
            if (settingsSelection == 0) masterVolume = std::clamp(masterVolume + step * 0.05f, 0.0f, 1.0f);
            else if (settingsSelection == 13) missSoundVolume=std::clamp(missSoundVolume+step*0.05f,0.0f,1.0f);
            else if (settingsSelection == 1) missVolume = std::clamp(missVolume + step * 0.05f, 0.0f, 1.0f);
            else if (settingsSelection == 2) effectsVolume = std::clamp(effectsVolume + step * 0.05f, 0.0f, 1.0f);
            else if (settingsSelection == 3) musicVolume = std::clamp(musicVolume + step * 0.05f, 0.0f, 1.0f);
            else if (settingsSelection == 4) menuVolume = std::clamp(menuVolume + step * 0.05f, 0.0f, 1.0f);
            else if (settingsSelection == 5) previewVolume = std::clamp(previewVolume + step * 0.05f, 0.0f, 1.0f);
            else if (settingsSelection >= 6 && settingsSelection <= 12) {
                const size_t instrument = static_cast<size_t>(settingsSelection - 6);
                instrumentVolumes[instrument] = std::clamp(instrumentVolumes[instrument] + step * 0.05f, 0.0f, 1.0f);
            }
            break;
        case 2: // Video; PC SettingsMenu initializes General, Audio, Video, Gameplay.
            if (settingsSelection == 0) noteAnimationFps = std::clamp(noteAnimationFps + (direction > 0 ? 1 : -1), 1, 60);
            else if (settingsSelection == 1) starAnimationFps = std::clamp(starAnimationFps + (direction > 0 ? 1 : -1), 1, 60);
            else if (settingsSelection == 2) showFps = !showFps;
            else if (settingsSelection == 3) showSongDisplay = !showSongDisplay;
            else if (settingsSelection == 4) noteStarPowerColors = !noteStarPowerColors;
            else if (settingsSelection == 5) highwayShakeEnabled = !highwayShakeEnabled;
            else if (settingsSelection == 6) menuBackgroundIndex = (menuBackgroundIndex + (direction > 0 ? 1 : 13)) % 14;
            else if (settingsSelection == 7) menuBackgroundAnimation = (menuBackgroundAnimation + (direction > 0 ? 1 : 2)) % 3;
            else if (settingsSelection == 8) {
                const int choices=displayTiming.supports120()?3:2;
                frameRateLimit=(displayTiming.effectiveChoice(frameRateLimit)+(direction>0?1:choices-1))%choices;
            } else if(settingsSelection==9) showLyrics=!showLyrics;
            break;
        case 3: // Gameplay
            if (settingsSelection == 0) noteSpeedScale = std::clamp(noteSpeedScale + step * 0.1f, 0.5f, 2.0f);
            else if (settingsSelection == 1) gemSizePercent = std::clamp(gemSizePercent + direction * 5, -300, 300);
            else if (settingsSelection == 2) {
                hitWindowMs = std::clamp(hitWindowMs + step * 5.0f, 35.0f, 150.0f);
                session.setHitWindow(hitWindowMs / 1000.0);
            } else if (settingsSelection == 3) showHitWindow = !showHitWindow;
            else if (settingsSelection == 4) muteOnMiss = !muteOnMiss;
            else if (settingsSelection == 5) gamepadMode = !gamepadMode;
            else if (settingsSelection == 6) measureCountdown = !measureCountdown;
            else if (settingsSelection == 7)
                practiceStartDelayMs = std::clamp(practiceStartDelayMs + direction * 100, 500, 5000);
            else if (settingsSelection == 8) practiceStarPowerEnabled = !practiceStarPowerEnabled;
            else if (settingsSelection == 9) noFail = !noFail;
            else if (settingsSelection == 10) {strumDebounceMs=std::clamp(strumDebounceMs+step,0.0f,50.0f);session.setStrumDebounce(strumDebounceMs/1000.0);}
            break;
        case 4: // Native controller options absent from the PC build.
            if (settingsSelection == 0) horizontalStrum = !horizontalStrum;
            else if (settingsSelection == 1) {
                controllerVibration = !controllerVibration;
                if (!controllerVibration) { vibratePlayer(gameplayPlayer,0.0f, 0.0f); rumbleActive = false; }
            } else if (settingsSelection == 2) rumbleStrength = std::clamp(rumbleStrength + step * 0.1f, 0.0f, 1.0f);
            break;
        }
        saveSettings();
    }

    // PC SettingsRenderer has distinct dropdown, bool and numeric-slider controls.
    // Guitar input stages choices/sliders until Green confirms; Red discards them.
    int settingsChoiceCount() const noexcept {
        if (settingsCategory==0 && settingsSelection==5) return 8;
        if (settingsCategory==2 && settingsSelection==6) return 14;
        if (settingsCategory==2 && (settingsSelection==7 || settingsSelection==8)) return 3;
        if (settingsCategory==4 && settingsSelection==0) return 2;
        return 0;
    }
    int currentSettingsChoice() const noexcept {
        if (settingsCategory==0) return static_cast<int>(songSortField);
        if (settingsCategory==4) return horizontalStrum ? 1 : 0;
        if (settingsSelection==6) return menuBackgroundIndex;
        return settingsSelection==7 ? menuBackgroundAnimation : displayTiming.effectiveChoice(frameRateLimit);
    }
    const char* settingsChoiceName(int choice) const noexcept {
        static constexpr const char* backgrounds[]={
            "Surfer", "Rainbow", "Surfer Alt", "Classic", "Dark", "Light", "Autumn", "Alien",
            "Blue Rays", "Grains", "Groovy", "Pastel Burst", "Spray", "Black"};
        static constexpr const char* animations[]={"Static", "UV Scroll", "Surfer Animated"};
        static constexpr const char* limits[]={"Auto", "60 FPS", "120 FPS"};
        if(settingsCategory==0) return songSortFieldName(static_cast<SongSortField>(choice));
        if(settingsCategory==4) return choice ? "D-pad Left / Right" : "D-pad Up / Down";
        if(settingsSelection==6) return backgrounds[choice];
        return settingsSelection==7 ? animations[choice] : limits[choice];
    }
    bool settingsChoiceEnabled(int choice) const noexcept {
        return settingsCategory!=2 || settingsSelection!=8 || displayTiming.choiceEnabled(choice);
    }
    bool settingsIsSlider() const noexcept {
        return settingsCategory==1 || (settingsCategory==0 && settingsSelection==8) ||
            (settingsCategory==2 && settingsSelection<2) ||
            (settingsCategory==3 && (settingsSelection<3 || settingsSelection==7 || settingsSelection==10)) ||
            (settingsCategory==4 && settingsSelection==2);
    }
    float currentSettingsSlider() const noexcept {
        if(settingsCategory==0) return menuScale;
        if(settingsCategory==1) {
            switch(settingsSelection) {
            case 0: return masterVolume; case 1: return missVolume; case 2: return effectsVolume;
            case 3: return musicVolume; case 4: return menuVolume; case 5: return previewVolume; case 13: return missSoundVolume;
            default: return instrumentVolumes[static_cast<size_t>(settingsSelection-6)];
            }
        }
        if(settingsCategory==2) return settingsSelection==0 ? noteAnimationFps : starAnimationFps;
        if(settingsCategory==4) return rumbleStrength;
        switch(settingsSelection) {
        case 0: return noteSpeedScale; case 1: return gemSizePercent;
        case 2: return hitWindowMs; case 10: return strumDebounceMs; default: return practiceStartDelayMs;
        }
    }
    void settingsSliderRange(float& low,float& high,float& step) const noexcept {
        low=0; high=1; step=.05f;
        if(settingsCategory==0) { low=.8f; high=1.2f; }
        else if(settingsCategory==2) { low=1; high=60; step=1; }
        else if(settingsCategory==4) step=.1f;
        else if(settingsCategory==3) {
            switch(settingsSelection) {
            case 0: low=.5f; high=2; step=.1f; break;
            case 1: low=-300; high=300; step=5; break;
            case 2: low=35; high=150; step=5; break;
            case 10: low=0; high=50; step=1; break;
            default: low=500; high=5000; step=100; break;
            }
        }
    }
    void moveSettingsEditor(int direction) noexcept {
        if(settingsEditor==SettingsEditor::Choices) {
            const int count=settingsChoiceCount();
            const int next=std::clamp(settingsChoice+direction,0,count-1);
            if(settingsChoiceEnabled(next))settingsChoice=next;
        } else if(settingsEditor==SettingsEditor::Slider) {
            float low,high,step;
            settingsSliderRange(low,high,step);
            settingsSliderValue=std::clamp(settingsSliderValue+direction*step,low,high);
        }
    }
    void confirmSettingsEditor() {
        if(settingsEditor==SettingsEditor::Choices) {
            if(!settingsChoiceEnabled(settingsChoice))return;
            if(settingsCategory==0) setSongSortField(static_cast<SongSortField>(settingsChoice));
            else if(settingsCategory==4) horizontalStrum=settingsChoice!=0;
            else if(settingsSelection==6) menuBackgroundIndex=settingsChoice;
            else if(settingsSelection==7) menuBackgroundAnimation=settingsChoice;
            else frameRateLimit=settingsChoice;
        } else if(settingsEditor==SettingsEditor::Slider) {
            const float value=settingsSliderValue;
            if(settingsCategory==0) menuScale=value;
            else if(settingsCategory==1) {
                switch(settingsSelection) {
                case 0: masterVolume=value; break; case 1: missVolume=value; break;
                case 2: effectsVolume=value; break; case 3: musicVolume=value; break;
                case 4: menuVolume=value; break; case 5: previewVolume=value; break; case 13: missSoundVolume=value; break;
                default: instrumentVolumes[static_cast<size_t>(settingsSelection-6)]=value; break;
                }
            } else if(settingsCategory==2) {
                if(settingsSelection==0) noteAnimationFps=static_cast<int>(std::round(value));
                else starAnimationFps=static_cast<int>(std::round(value));
            } else if(settingsCategory==4) rumbleStrength=value;
            else if(settingsSelection==0) noteSpeedScale=value;
            else if(settingsSelection==1) gemSizePercent=static_cast<int>(std::round(value));
            else if(settingsSelection==2) { hitWindowMs=value; session.setHitWindow(value/1000.0); }
            else if(settingsSelection==10) {strumDebounceMs=value;session.setStrumDebounce(value/1000.0);}
            else practiceStartDelayMs=static_cast<int>(std::round(value));
        }
        settingsEditor=SettingsEditor::None;
        saveSettings();
    }

    void activateSettingsValue() {
        if (settingsCategory == 0 && settingsSelection == 1) {
            storageNeedsScan=true;
            return;
        }
        if(settingsCategory==0 && (settingsSelection==2 || settingsSelection==3)){beginSongCopy(settingsSelection==2);return;}
        if(settingsCategory==0 && settingsSelection==4){beginSongRemoval();return;}
        if(settingsCategory==0 && settingsSelection==10){openUpdateDialog();return;}
        if(settingsCategory==0 && settingsSelection==11){autoUpdateChecks=!autoUpdateChecks;saveUpdateChecks();return;}
        if (settingsCategory == 0 && settingsSelection == 9) {
            resetSettings();
            return;
        }

        if(settingsEditor!=SettingsEditor::None) { confirmSettingsEditor(); return; }
        if(settingsChoiceCount()>0) {
            settingsChoice=currentSettingsChoice();
            settingsEditor=SettingsEditor::Choices;
        } else if(settingsIsSlider()) {
            settingsSliderValue=currentSettingsSlider();
            settingsEditor=SettingsEditor::Slider;
        } else if(!(settingsCategory==0 && settingsSelection==0)) adjustSettingsValue(1);
    }

    int pausePageRowCount() const {
        switch (pausePage) {
        case PausePage::Main: return playMode == PlayMode::Practice ? 10 : 7;
        case PausePage::Options: return 2;
        case PausePage::Volume: return 12;
        case PausePage::Calibration: return 4;
        case PausePage::SongSettings: return 3;
        case PausePage::PracticeSection: return 6;
        case PausePage::Sections: return static_cast<int>(practiceMarks.size())+1;
        case PausePage::ConfirmQuit: return 2;
        }
        return 0;
    }

    void adjustPauseValue(int direction) {
        if (direction == 0) return;
        const float step = direction > 0 ? 0.05f : -0.05f;
        if (pausePage == PausePage::Volume) {
            switch (pauseSelection) {
            case 0: masterVolume = std::clamp(masterVolume + step, 0.0f, 1.0f); break;
            case 1: instrumentVolumes[0] = std::clamp(instrumentVolumes[0] + step, 0.0f, 1.0f); break;
            case 2: instrumentVolumes[1] = std::clamp(instrumentVolumes[1] + step, 0.0f, 1.0f); break;
            case 3: instrumentVolumes[2] = std::clamp(instrumentVolumes[2] + step, 0.0f, 1.0f); break;
            case 4: instrumentVolumes[3] = std::clamp(instrumentVolumes[3] + step, 0.0f, 1.0f); break;
            case 5: instrumentVolumes[4] = std::clamp(instrumentVolumes[4] + step, 0.0f, 1.0f); break;
            case 6: instrumentVolumes[5] = std::clamp(instrumentVolumes[5] + step, 0.0f, 1.0f); break;
            case 7: musicVolume = std::clamp(musicVolume + step, 0.0f, 1.0f); break;
            case 8: instrumentVolumes[6] = std::clamp(instrumentVolumes[6] + step, 0.0f, 1.0f); break;
            case 10: effectsVolume = std::clamp(effectsVolume + step, 0.0f, 1.0f); break;
            case 11: missSoundVolume=std::clamp(missSoundVolume+step,0.0f,1.0f);break;
            default: return;
            }
        } else if (pausePage == PausePage::Calibration) {
            if (pauseSelection == 0)
                audioOffsetMs = std::clamp(audioOffsetMs + (direction > 0 ? 1.0f : -1.0f), -2000.0f, 2000.0f);
            else if (pauseSelection == 1)
                videoOffsetMs = std::clamp(videoOffsetMs + (direction > 0 ? 1.0f : -1.0f), -2000.0f, 2000.0f);
            else if(pauseSelection==3) showHitWindow=!showHitWindow;
            else return;
        } else if (pausePage == PausePage::SongSettings && pauseSelection == 0) {
            const double candidate = chartOffsetSeconds + (direction > 0 ? 0.001 : -0.001);
            if (!std::isfinite(candidate) || std::abs(candidate) > 60.0) return;
            const double previousTrackStart = trackStartOffsetSeconds;
            chartOffsetSeconds = candidate;
            trackStartOffsetSeconds = chartOffsetSeconds - readSongDelaySeconds(activeChartPath);
            // Rebase the paused chart timeline against the same audio position.
            // This applies the offset immediately without restarting the song
            // or discarding the current score and combo.
            rhythm::ClockConfig clockConfig{};
            clockConfig.audioReferenceOffset = -trackStartOffsetSeconds;
            const double preservedTimeline = songClock.gameplayTime() -
                (trackStartOffsetSeconds - previousTrackStart);
            songClock = rhythm::AudioClock(clockConfig);
            songClock.reset(preservedTimeline);
            saveSongOffsetOverride(activeChartPath, chartOffsetSeconds);
        } else if ((pausePage == PausePage::PracticeSection && pauseSelection == 2) ||
                   (pausePage == PausePage::Main && playMode==PlayMode::Practice && pauseSelection==1)) {
            practiceRate = std::clamp(practiceRate + (direction > 0 ? 0.1f : -0.1f), 0.5f, 1.5f);
        } else return;
        saveSettings();
#if !defined(PS5_HOST_REFERENCE)
        applyAudioGains();
#endif
    }

    void seekPracticePosition(double chartTime, bool resume) {
        if(!gameplaySeatMask){seekActivePracticePosition(chartTime,resume);return;}
        for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
            selectGameplaySeat(seat);seekActivePracticePosition(chartTime,resume,seat==primaryGameplaySeat);
        }
        selectGameplaySeat(primaryGameplaySeat);gameplayStarHeld.fill(false);
    }
    void seekActivePracticePosition(double chartTime,bool resume,bool shared=true) {
        if (!std::isfinite(chartTime)) chartTime = 0.0;
        chartTime = std::max(0.0, chartTime);
        session.beginSection(chartTime);
        drumSession.beginSection(chartTime);drumPadEdges.prime(0);
        drumFxClock=0;drumHitAt.fill(-1e30);drumPhraseHitAt.fill(-1e30);drumFretOffset.fill(0);for(auto& motion:drumFretMotion)motion.reset();
        for (auto& note : notes) {
            note.hit = note.missed = note.sustainDropped = false;
            note.heldUntil = 0.0;
            note.sustainScoreStartBeat = 0.0;
            note.sustainPointsAwarded = 0;
            note.sustainGrace = {};
        }
        for (auto& phrase : starPhrases) {
            phrase.hitCount = 0;
            phrase.failed = phrase.awarded = false;
        }
        score = combo = bestCombo = hits = misses = 0;
        soloBonusTotal=0;fretBonusTotal=0;resultStats={};resultsScroll=0;resultsSelection=0;
        for(auto& solo:solos) solo.reset();
        gameplayHealth = 0.5f;
        songFailed = false;
        starPower.reset(chartResolutionTicks);
        missMuted = false;
        inputPrimed = false;activeSustainNotes.clear();
        wasAdvancing = false;
        previousButtons = 0;
        const double offset = static_cast<double>(audioOffsetMs) / 1000.0;
        if(shared) {
            rhythm::ClockConfig clockConfig{};
            clockConfig.audioReferenceOffset = -trackStartOffsetSeconds;
            songClock = rhythm::AudioClock(clockConfig);
            songClock.reset(chartTime - offset);multiplayerQaTime=chartTime;
        }
        songTime = chartTime;
        for(auto& solo:solos) solo.reset(chart.notes[solo.firstHead].time>=chartTime);
        renderTime = songClock.renderTime() + (videoOffsetMs + audioOffsetMs) / 1000.0;
#if !defined(PS5_HOST_REFERENCE)
        practiceCountdownRemaining = resume && playMode == PlayMode::Practice
            ? static_cast<double>(practiceStartDelayMs) / 1000.0 : 0.0;
        if (shared && audioOutputStarted && songAudio.ready()) {
            const double audioPosition = chartTime - offset + trackStartOffsetSeconds;
            songAudio.seekSeconds(std::max(0.0, audioPosition));
            if (resume && screen == Screen::Gameplay && practiceCountdownRemaining <= 0.0)
                (void)songAudio.play();
        }
#else
        practiceCountdownRemaining = resume && playMode == PlayMode::Practice
            ? static_cast<double>(practiceStartDelayMs) / 1000.0 : 0.0;
#endif
    }

    void resumeGameplay() {
        ui.visible=false;
#if !defined(PS5_HOST_REFERENCE)
        if(resumeRewind.begin(songClock.gameplayTime(),songAudio.positionSeconds(),playMode==PlayMode::Practice)) {
            songAudio.pause();songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::RewindHighway);
        }
#else
        resumeRewind.begin(songClock.gameplayTime(),songClock.gameplayTime(),playMode==PlayMode::Practice);
#endif
    }
    void leaveGameplay(bool mainMenu) {
        resumeRewind.reset();
        screen=mainMenu?Screen::MainMenu:Screen::SongSelect;
        playing=false;ui.visible=true;pausePage=PausePage::Main;pauseSelection=0;
#if !defined(PS5_HOST_REFERENCE)
        songAudio.pause();menuPreviewAudioChartPath.clear();
#endif
    }

    void confirmPauseAction(PauseAction action) {
        pendingPauseAction=action;confirmationReturnRow=pauseSelection;
        pausePage=PausePage::ConfirmQuit;pauseSelection=1;
    }

    void activatePauseSelection() {
        switch (pausePage) {
        case PausePage::Main:
            if(playMode==PlayMode::Practice) {
                switch(pauseSelection) {
                case 0: resumeGameplay();break;
                case 1: seekPracticePosition(practiceSectionASet?practiceSectionA:0.0,true);ui.visible=false;break;
                case 2: practiceSectionA=std::max(0.0,songTime);practiceSectionASet=true;
                    if(practiceSectionBSet && practiceSectionB<=practiceSectionA) practiceSectionBSet=false;break;
                case 3: if(practiceSectionASet && songTime>practiceSectionA+.05) {practiceSectionB=songTime;practiceSectionBSet=true;}break;
                case 4: practiceSectionASet=practiceSectionBSet=false;break;
                case 5: pausePage=PausePage::Sections;pauseSelection=0;break;
                case 6: confirmPauseAction(PauseAction::Quickplay);break;
                case 7: confirmPauseAction(PauseAction::NewSong);break;
                case 8: pausePage=PausePage::Options;pauseSelection=0;break;
                case 9: confirmPauseAction(PauseAction::Quit);break;
                }
            } else {
                switch(pauseSelection) {
                case 0: resumeGameplay();break;
                case 1: confirmPauseAction(PauseAction::Restart);break;
                case 2: confirmPauseAction(PauseAction::NewSong);break;
                case 3: confirmPauseAction(PauseAction::Practice);break;
                case 4: pausePage=PausePage::Options;pauseSelection=0;break;
                case 5: pausePage=PausePage::SongSettings;pauseSelection=0;break;
                case 6: confirmPauseAction(PauseAction::Quit);break;
                }
            }
            break;
        case PausePage::ConfirmQuit:
            if(pauseSelection!=0) {pausePage=PausePage::Main;pauseSelection=confirmationReturnRow;}
            else {
                pausePage=PausePage::Main;pauseSelection=0;
                switch(pendingPauseAction) {
                case PauseAction::Quit: leaveGameplay(true);break;
                case PauseAction::NewSong: leaveGameplay(false);break;
                case PauseAction::Restart: restartSong();ui.visible=false;break;
                case PauseAction::Practice: playMode=PlayMode::Practice;pausePage=PausePage::Sections;break;
                case PauseAction::Quickplay: playMode=PlayMode::Quickplay;practiceRate=1.0f;practiceSectionASet=practiceSectionBSet=false;restartSong();ui.visible=false;break;
                }
            }
            break;
        case PausePage::Sections:
            if(pauseSelection==static_cast<int>(practiceMarks.size())) {pausePage=PausePage::Main;pauseSelection=5;}
            else if(pauseSelection>=0 && pauseSelection<static_cast<int>(practiceMarks.size())) {
                practiceSectionA=practiceMarks[pauseSelection].time;practiceSectionASet=true;
                practiceSectionBSet=pauseSelection+1<static_cast<int>(practiceMarks.size());
                if(practiceSectionBSet) practiceSectionB=practiceMarks[pauseSelection+1].time;
                seekPracticePosition(practiceSectionA,true);ui.visible=false;
            }
            break;
        case PausePage::Options:
            if (pauseSelection == 0) { pausePage = PausePage::Volume; pauseSelection = 0; }
            else if (pauseSelection == 1) { pausePage = PausePage::Calibration; pauseSelection = 0; }
            else { pausePage = PausePage::Main; pauseSelection = playMode==PlayMode::Practice?8:4; }
            break;
        case PausePage::Volume:
            adjustPauseValue(1);
            break;
        case PausePage::Calibration:
            adjustPauseValue(1);
            break;
        case PausePage::SongSettings:
            if (pauseSelection == 0) adjustPauseValue(1);
            break;
        case PausePage::PracticeSection:
            if (pauseSelection == 0) {
                practiceSectionA = std::max(0.0, songTime);
                practiceSectionASet = true;
                if (practiceSectionBSet && practiceSectionB <= practiceSectionA) practiceSectionBSet = false;
                std::snprintf(practiceSectionMessage, sizeof(practiceSectionMessage),
                    "A set at %.2f seconds", practiceSectionA);
            } else if (pauseSelection == 1) {
                const double position = std::max(0.0, songTime);
                if (!practiceSectionASet || position <= practiceSectionA + 0.05) {
                    std::snprintf(practiceSectionMessage, sizeof(practiceSectionMessage),
                        "Set A first; B must be later than A.");
                } else {
                    practiceSectionB = position;
                    practiceSectionBSet = true;
                    std::snprintf(practiceSectionMessage, sizeof(practiceSectionMessage),
                        "B set at %.2f seconds", practiceSectionB);
                }
            } else if (pauseSelection == 3) {
                const double target = practiceSectionASet ? practiceSectionA : 0.0;
                seekPracticePosition(target, true);
                ui.visible = false;
                pausePage = PausePage::Main;
                pauseSelection = 0;
            } else if (pauseSelection == 4) {
                practiceSectionASet = practiceSectionBSet = false;
                practiceSectionA = practiceSectionB = 0.0;
                std::snprintf(practiceSectionMessage, sizeof(practiceSectionMessage), "A/B section cleared.");
            } else if (pauseSelection == 5) {
                pausePage = PausePage::Main;
                pauseSelection = 4;
            }
            break;
        }
    }

    // PC prefab camera, with Z mirrored into the native scene and uniform world scale.
    float camPitch = pc_guitar::cameraPitch;
    float camHeight = pc_guitar::cameraHeight * pc_guitar::worldScale;
    float camZ = pc_guitar::cameraZ * pc_guitar::worldScale;
    float camFov = pc_guitar::cameraFov;

    static constexpr float HIGHWAY_FAR = -pc_guitar::farLimit * pc_guitar::worldScale, HIGHWAY_NEAR = 1.0f;
    // Note cleanup stops at z=1; reusing it for the board left a 63px gap
    // at 1080p with the recovered PC camera. Extend the floor beyond the
    // viewport to the PC near-fade start, independently of note lifetime.
    static constexpr float BOARD_NEAR = pc_guitar::worldScale;
    static constexpr float BOARD_L = -2.6f, BOARD_R = 2.6f;
    static constexpr float BOARD_TILE = 4.0f;        // world units per board-texture repeat
    static constexpr float laneX[5] = { -2.0f, -1.0f, 0.0f, 1.0f, 2.0f };
    static constexpr const auto& fretColor = pc_guitar::laneColors;

    // ---- song / scroll state ----
    struct TempoSegment {
        double startSec{0.0};
        double startBeat{0.0};
        double bpm{120.0};
        int64_t startTick{};
    };
    std::vector<TempoSegment> tempoSegments;
    rhythm::PcStarPowerClock starPowerClock;
    int chartResolutionTicks{192};

    struct Note {
        double timeSec, sustainSec;
        double beatTime{0.0}, sustainBeats{0.0};
        double sustainScoreStartBeat{0.0};
        unsigned sustainPointsAwarded{0};
        int lane; // 0..4 = G R Y B O, 5 = open
        bool hit = false, missed = false;
        double heldUntil = 0;
        size_t chordIndex = 0;
        bool sustainDropped = false;
        bool tap = false, hopo = false, starPower = false, chord = false;
        rhythm::PcSustainGrace sustainGrace;
        int64_t tick{},lengthTicks{};
        bool extended{},disjoint{};
        bool sustainScoreOwner{true};uint16_t sustainGroupMask{};
        int starPhraseIndex{-1};
        uint16_t drumMask{};uint32_t drumFlags{};
    };
    // PC phrases award one of four rounded chartResolution*7.5 tick units.
    struct StarPhrase {
        double startSec{0.0}, endSec{0.0};
        unsigned gemCount{0}, hitCount{0};
        bool failed{false}, awarded{false};
    };
    std::vector<Note> notes;
    // Only live/recent tails participate in sub-frame sustain settlement.
    // Keep source note order for identical score/SP boundary arithmetic.
    std::vector<size_t> activeSustainNotes;
    std::vector<StarPhrase> starPhrases;
    rhythm::PcStarPower starPower;
    std::vector<rhythm::PcSolo> solos;
    uint64_t soloBonusTotal{};
    uint64_t fretBonusTotal{};
    int songHopoThreshold{65};
    std::string activeChartPath;
    bool chartFileLoaded{ false };
    std::string playerSelectionMessage;
    rhythm::SongStartGate songStartGate;
    bool gameplayAudioPending{},gameplayReadyPending{};
    bool loadStallQa{},loadStallInjected{};
    std::string gameplayAudioChart,gameplayAudioMusic;
    double gameplayChartLoadMs{},gameplayAudioLoadMs{},gameplayReadyAt{};
    bool gameplayAudioCacheHit{};
#if !defined(PS5_HOST_REFERENCE)
    rhythm::AudioStemMixer songAudio;
    rhythm::AudioStemMixer previewAudioStage;
    bool audioOutputStarted{ false };
    pthread_t previewAudioThread{};
    std::atomic<bool> previewAudioWorkerDone{false};
    bool previewAudioThreadActive{false};
    std::atomic<bool> previewAudioCancel{};
    bool audioDelayQa{};bool previewAudioWorkerDelay{};
    bool previewAudioWorkerLoaded{false};
    bool previewAudioWorkerGameplay{},previewAudioWorkerForMenu{};
    double previewAudioWorkerAuthoredStart{},previewAudioWorkerRandom{};
    std::string previewAudioWorkerMusic;
    double previewAudioWorkerMs{};
    std::string previewAudioWorkerChartPath,previewAudioWorkerAudioPath;
#else
    rhythm::HostReferenceAudio songAudio;
#endif
    struct LocalSong {
        std::string title, artist, album, genre, year, charter, source, chartPath, albumArtPath, loadingPhrase;
        std::string audioChartPath,originalChartPath,externalRoot;
        uint16_t chartedInstrumentMask{0};
        std::array<uint8_t,10> chartDifficulties{};
        std::array<int,10> instrumentRatings{{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
        uint64_t dateAdded{};
        uint64_t chartBytes{}, chartFingerprint{};
        bool chartFingerprintValid{false};
        double previewStartSeconds{ 0.0 };
        double durationSeconds{ 0.0 };
    };
    std::vector<LocalSong> localSongs;
    std::unordered_map<uint64_t,rhythm::SavedScore> songBestScores;
    bool songScoresLoaded{};
    void loadSongScoreCache() {
        if(songScoresLoaded)return;
        songScoresLoaded=true;
        DIR* directory=opendir(CH_USERDATA "/scores");if(!directory)return;
        while(auto* entry=readdir(directory)) {
            unsigned long long key{};int instrument{},difficulty{},used{};
            if(std::sscanf(entry->d_name,"%16llx-%d-%d.cfg%n",&key,&instrument,&difficulty,&used)!=3 ||
               entry->d_name[used] || instrument<0 || instrument>7 || difficulty<0 || difficulty>3)continue;
            const auto path=std::string(CH_USERDATA "/scores/")+entry->d_name;
            FILE* file=std::fopen(path.c_str(),"rb");rhythm::SavedScore record;
            const bool valid=rhythm::readSavedScore(file,record);if(file)std::fclose(file);
            if(!valid)continue;
            record.instrument=instrument;record.difficulty=difficulty;
            auto found=songBestScores.find(key);
            if(found==songBestScores.end() || rhythm::betterSavedScore(record,found->second))songBestScores[key]=std::move(record);
        }
        closedir(directory);
    }
    const rhythm::SavedScore* songBest(const LocalSong& song) const {
        const auto at=songBestScores.find(songOffsetKey(song.chartPath));
        return at==songBestScores.end()?nullptr:&at->second;
    }
    LocalSong gameplaySongInfo;bool gameplaySongInfoValid{};
    pthread_t storageThread{};bool storageThreadActive{},storageNeedsScan{true};
    enum class StorageJob {Scan,Copy,Preview,Remove};StorageJob storageJob{StorageJob::Scan};
    std::atomic<bool> storageDone{false},storageCancel{false};
    std::atomic<unsigned> storageProgress{},storageTotal{};
    std::atomic<bool> storageScanTestHold{false};
    std::atomic<unsigned> storageUsbFound{},storageUsbLoaded{},storageUsbDuplicates{};
    bool storagePopupActive{},storagePopupComplete{};
    unsigned storagePopupFound{},storagePopupLoaded{},storagePopupDuplicates{},storagePopupFailed{};
    std::vector<std::string> externalSongRoots,storageScanRoots,storageExternalSnapshot;
    std::vector<LocalSong> storageSongs;
    std::string storageCopyFrom,storageCopyTo,storageResultText,storageWorkerError;
    rhythm::storage::CopyResult storageCopyResult;
    struct RemovalChoice {std::string folder,title,artist;bool bundled{};};
    std::vector<RemovalChoice> removalChoices;
    std::vector<uint8_t> removalSelected;
    std::vector<std::string> removedBundledSongs,storageRemovalFolders;
    rhythm::storage::RemoveResult storageRemovalResult;
    int removalCursor{},removalPage{}; // list, confirmation, progress, result
    unsigned removalCount() const {return unsigned(std::count(removalSelected.begin(),removalSelected.end(),uint8_t(1)));}
    void beginSongRemoval() {
        if(storageThreadActive){controllerToast="A library scan is running. Please try again shortly.";controllerToastUntil=ImGui::GetTime()+3;return;}
        removalChoices.clear();removalSelected.clear();removalCursor=removalPage=0;storageResultText.clear();
        const auto bundled=getAssetPath()+"songs";
        std::unordered_map<std::string,bool> seen;
        for(const auto& song:localSongs) {
            if(!song.externalRoot.empty())continue;
            const auto& original=song.originalChartPath.empty()?song.chartPath:song.originalChartPath;
            const auto slash=original.find_last_of('/');if(slash==std::string::npos)continue;
            const auto folder=original.substr(0,slash);
            const bool isBundled=folder.rfind(bundled+"/",0)==0;
            if(!isBundled && folder.rfind(internalSongRoot()+"/",0)!=0)continue;
            if(!seen.emplace(folder,true).second)continue;
            removalChoices.push_back({folder,song.title,song.artist,isBundled});removalSelected.push_back(0);
        }
        screen=Screen::SongRemove;
    }
    void confirmSongRemoval() {
        if(storageThreadActive || !removalCount())return;
        storageRemovalFolders.clear();
        for(size_t i=0;i<removalChoices.size();++i)if(removalSelected[i])storageRemovalFolders.push_back(removalChoices[i].folder);
#if !defined(PS5_HOST_REFERENCE)
        songAudio.pause();
#endif
        menuPreviewAudioChartPath.clear();menuPreviewChartPath.clear();menuMusicSelection=-1;
        startStorageJob(StorageJob::Remove);if(storageThreadActive)removalPage=2;
    }
    double storageNextPoll{};int storageDrive{};bool storageLiveQa{};
    enum class SongCopyPage {Confirm,Progress,Result};SongCopyPage songCopyPage{SongCopyPage::Confirm};
    bool songCopyToConsole{true};int songCopyChoice{1};std::string songCopyRoot;
    rhythm::storage::CopyPlan songCopyPlan;bool songCopyPlanReady{},songCopyPlanToConsole{};std::string songCopyPlanRoot;
    bool storageQa{},storageQaMounted{};
    rhythm::updates::ApplicationUpdate applicationUpdater{rhythm::updates::applicationVersion,CH_USERDATA "/updates"};
    bool updateDialog{},autoUpdateChecks{true},updateCheckStarted{},updateOfferPending{},updateLiveQa{},updateRuntimeQa{};
    rhythm::updates::UpdateFormat updateFormat{rhythm::updates::UpdateFormat::Folder};
    rhythm::updates::UpdateState lastUpdateState{rhythm::updates::UpdateState::Idle};
    void saveUpdateChecks() {
        const char* path=CH_USERDATA "/update-checks.cfg";
        const char* temporary=CH_USERDATA "/update-checks.cfg.tmp";
        FILE* file=std::fopen(temporary,"wb");bool ok=false;
        if(file){ok=std::fprintf(file,"ENCORE_UPDATES 1 %d\n",autoUpdateChecks?1:0)>0 && std::fflush(file)==0 && fsync(fileno(file))==0;if(std::fclose(file))ok=false;if(ok)ok=std::rename(temporary,path)==0;}
        if(!ok){controllerToast="Could not save automatic update checks.";controllerToastUntil=ImGui::GetTime()+4;}
    }
    void openUpdateDialog() {
        if(storageThreadActive){controllerToast="A song storage task is running. Please try again shortly.";controllerToastUntil=ImGui::GetTime()+3;return;}
        updateDialog=true;updateCheckStarted=true;
        updateFormat=rhythm::updates::UpdateFormat::Folder;
        if(applicationUpdater.state()==rhythm::updates::UpdateState::Idle)(void)applicationUpdater.check();
    }
    void updateApplication() {
        applicationUpdater.poll();
        if(!updateCheckStarted && autoUpdateChecks && (!ps5.frameBudget || updateLiveQa) && screen==Screen::MainMenu && menuIntroFinished) {
            updateCheckStarted=true;(void)applicationUpdater.check();
        }
        auto state=applicationUpdater.state();
        if(updateRuntimeQa && state==rhythm::updates::UpdateState::Ready){
            std::string error;if(!applicationUpdater.install(error))rhythm::logDiagnostic("updates runtime QA install: %s",error.c_str());
            state=applicationUpdater.state();
        }
        if(state==rhythm::updates::UpdateState::RestartReady){
#if !defined(PS5_HOST_REFERENCE)
            encore_restart_application_runtime();
#endif
            quit=true;
        }
        if(state!=lastUpdateState) {
            const auto info=applicationUpdater.snapshot();
            rhythm::logDiagnostic("updates state=%d version=%u message=%s",int(state),info.release.version,info.message.c_str());
            if(state==rhythm::updates::UpdateState::Available && !updateDialog)updateOfferPending=true;
            lastUpdateState=state;
        }
        bool playerPopup=false;for(int seat=0;seat<PAD_PLAYERS;++seat)playerPopup|=profileMenu.open(seat);
        if(updateOfferPending && screen==Screen::MainMenu && !playerPopup && !instrumentKeyboard.open && !searchKeyboardOpen && !storageThreadActive){updateOfferPending=false;openUpdateDialog();}
    }
    chorus::Downloader chorusDownloader;
    std::string downloaderQuery;
    int downloaderKey{0};
    enum class SearchKeyboardTarget { None, Encore, LocalSongs, Profile };
    rhythm::InstrumentKeyboard instrumentKeyboard;
    rhythm::MenuRepeat keyboardRepeat;
    int keyboardPlayer{};
    bool keyboardRawQa{};
    bool keyboardGridQa{},keyboardGridNavigation{};
    std::array<uint32_t,PAD_PLAYERS> keyboardAssistPrevious{};
    std::array<rhythm::MenuStick,PAD_PLAYERS> keyboardAssistSticks;
    std::array<rhythm::MenuRepeat,PAD_PLAYERS> keyboardAssistRepeat;
    SearchKeyboardTarget searchKeyboardTarget{SearchKeyboardTarget::None};
    // Native IME may append composition text beyond the committed text limit.
    std::array<uint16_t,4096> searchKeyboardBuffer{};
    std::string songSearchImeError;
    bool searchKeyboardOpen{false};
#if !defined(PS5_HOST_REFERENCE)
    // Keep every IME input alive until termination has completed.
    rhythm::native_ime::Panel nativeKeyboard;
    bool nativeImePreferred{true};
    Ps5ImeParam searchKeyboardParam{};
    Ps5ImeResult searchKeyboardResult{};
    int searchKeyboardResultCode{};
    bool searchKeyboardStopping{};
    // Bounded QA isolation only: observe system closure before either API call.
    bool keyboardCloseIsolation{};
    uint64_t keyboardResultAfter{},keyboardTermAfter{};
    int keyboardCloseStage{};
#endif
    bool keyboardTestSubmitted{false};
    std::string keyboardTestQuery;
    std::string downloaderImeError;
    bool downloaderScriptSearchRequested{false};
    int downloaderResult{0};
    bool downloaderResultsFocus{false};
    bool downloaderFilterFocus{false};
    int downloaderFilterSelection{0};
    int downloaderInstrument{0};
    int downloaderDifficulty{0};
    int downloaderSort{0};
    int downloaderDrumType{2};
    int downloaderSearchField{0};
    bool downloaderReviewedDrums{false};
    bool downloaderSortDescending{false};
    std::array<bool, chorus::Downloader::MaxResults> downloaderMarked{};
    std::array<size_t, chorus::Downloader::MaxResults> downloaderQueue{};
    size_t downloaderQueueCount{0};
    size_t downloaderQueuePosition{0};
    bool downloaderSawComplete{false};
    std::string songSearchQuery;
    bool defaultSearchByCurrentSort{false};
    // Matches PC SongSelectSearch's category order, omitting Playlist because
    // the native library does not yet have playlists.
    enum class SongSearchField : uint8_t { Song, Artist, Album, Charter, Genre, Source };
    SongSearchField songSearchField{SongSearchField::Song};
    // Native sort subset backed by PC SongSelect metadata/localization. The
    // PC integer sort_filter has 27 values whose complete mapping is unknown.
    enum class SongSortField : uint8_t { Song, Artist, Album, Genre, Year, Charter, Length, DateAdded };
    SongSortField songSortField{SongSortField::Song};
    uint64_t songListRevision{0};
    uint64_t songSectionCacheRevision{UINT64_MAX};
    int songSectionCacheField{-1};
    std::string songSectionCacheQuery;
    std::vector<size_t> songSectionCounts;
    mutable uint64_t songFilterCacheRevision{UINT64_MAX};
    mutable int songFilterCacheField{-1};
    mutable std::string songFilterCacheQuery;
    mutable std::vector<size_t> songFilterCache;
    static SongSearchField searchFieldForSongSort(SongSortField field) noexcept {
        switch (field) {
        case SongSortField::Artist: return SongSearchField::Artist;
        case SongSortField::Album: return SongSearchField::Album;
        case SongSortField::Genre: return SongSearchField::Genre;
        case SongSortField::Charter: return SongSearchField::Charter;
        case SongSortField::Song:
        case SongSortField::Year:
        case SongSortField::Length:
        case SongSortField::DateAdded:
            return SongSearchField::Song;
        }
        return SongSearchField::Song;
    }
    int songSearchKey{0};
    double songTime = 0.0;
    bool   playing  = false;
    static constexpr float SPEED_Z = 19.26f;         // units/sec: 26 over ~1.35 s travel
    static constexpr float LEAD_IN = 3.0f;           // count-in seconds before chart t=0
    static constexpr uint32_t MAX_TEX_VERTS = 16384; // 4 layers/note during dense solos

    // Exported CloneHeroPS5 pipeline owns chord judgement and timing.
    rhythm::SongChart chart;
    rhythm::GuitarSession session{chart};
    rhythm::DrumSession drumSession{chart};
    rhythm::pc_drums::PadEdges drumPadEdges;
    bool chartProDrums{false};
    double drumFxClock{};
    std::array<double,6> drumHitAt{{-1e30,-1e30,-1e30,-1e30,-1e30,-1e30}};
    std::array<double,6> drumPhraseHitAt{{-1e30,-1e30,-1e30,-1e30,-1e30,-1e30}};
    std::array<bool,6> drumHitActive{};
    std::array<unsigned,6> drumHitDynamics{};
    std::array<rhythm::PcDrumFretMotion,5> drumFretMotion;
    std::array<float,5> drumFretOffset{};
    const rhythm::InputEvaluator& gameplayEvaluator() const noexcept {return chart.kind==rhythm::ChartKind::Drums?drumSession.evaluator():session.evaluator();}
    rhythm::AudioClock songClock{rhythm::ClockConfig{}};
    double renderTime = -LEAD_IN;
    bool inputPrimed = false, wasConnected = false, wasAdvancing = false;
    std::vector<rhythm::RenderNote> visibleNotes;
    uint32_t frets = 0, previousButtons = 0;
    uint64_t score = 0;
    unsigned combo = 0, bestCombo = 0, hits = 0, misses = 0;
    uint64_t bestScore{ 0 };
    uint64_t chartBaseScore{ 0 };
    std::vector<size_t> chordNoteOffsets;
    int chartInstrument{ 0 }, chartDifficulty{ 3 };
    bool newHighScore{ false };
    std::array<float, 7> hitGlow{};
    double guitarFxClock{};
    std::array<double,7> guitarHitAt{{-1e30,-1e30,-1e30,-1e30,-1e30,-1e30,-1e30}};
    std::array<bool,7> guitarHitActive{},guitarHitPhrase{};
    std::array<double,5> guitarPhraseHitAt{{-1e30,-1e30,-1e30,-1e30,-1e30}};
    std::array<float,5> guitarEmitRemainder{};
    std::array<bool,5> guitarSustaining{};
    rhythm::GuitarParticles guitarParticles;
    std::array<bool, PAD_PLAYERS> playerJoined{};
    std::array<bool, PAD_PLAYERS> playerReady{};
    rhythm::ProfileMenu profileMenu;
    // Join-order colors; orange remains available for vocals.
    static ImU32 guestNameInk(int seat) {
        static constexpr ImU32 colors[]{IM_COL32(80,225,95,255),IM_COL32(255,85,85,255),
            IM_COL32(255,225,75,255),IM_COL32(95,160,255,255)};
        return colors[std::clamp(seat,0,3)];
    }
    rhythm::MenuProfile activeProfile{"Guest"};
    bool activeGuest{true};
    int gameplayPlayer{ 0 };
    rhythm::controls::PlayerSeats playerSeats;
    std::array<pad,8> sourcePads{};
    std::array<uint32_t,8> sourcePrevious{};
    uint32_t sourceConnectedMask{};
    int primarySource{};
    bool slotInputQa{};std::array<bool,8> qaSourceConnected{};
    std::string controllerToast;double controllerToastUntil{};
    rhythm::controls::Ps5UsbInput usbControllers;
    rhythm::midi::Ps5UsbMidi usbMidi;
    std::array<rhythm::midi::UsbStatus,4> midiSources{};
    std::array<rhythm::midi::StrikeBatch,4> midiBatches{};
    struct MidiLink {
        int source{-1};uint64_t generation{};
        rhythm::midi::Profile profile;
        std::string playerKey;
    };
    std::array<MidiLink,4> midiLinks{};
    std::array<uint64_t,4> midiIgnored{};
    rhythm::midi::SetupWizard midiWizard;
    int midiSetupSource{-1},midiNavSlot{-1},midiChoice{};
    bool midiTypePage{true},midiSavedMapping{},midiQa{},midiDiscoveryQa{};
    Screen midiReturnScreen{Screen::MainMenu};
    rhythm::midi::UsbDecoder midiQaDecoder;
    uint64_t midiSetupGeneration{};
    uint32_t midiNavHeld{};
    rhythm::MenuPressFilter midiMenuFilter;
    std::string midiMessage;
    int midiSourceForSeat(int seat) const {
        if(seat<0 || seat>=4)return -1;
        const auto& link=midiLinks[seat];
        if(link.source<0 || link.source>=4)return -1;
        const auto& source=midiSources[link.source];
        return source.present && source.generation==link.generation?link.source:-1;
    }
    static std::string midiPlayerKey(const rhythm::MenuProfile* profile,bool saved) {
        if(!profile || !saved)return {};
        uint64_t hash=14695981039346656037ull;
        for(unsigned char c:profile->name){hash^=c;hash*=1099511628211ull;}
        char key[24];std::snprintf(key,sizeof(key),"%016llx",static_cast<unsigned long long>(hash));return key;
    }
    std::string midiProfilePath(unsigned source,const std::string& player={}) const {
        char path[256];const auto& d=midiSources[source];
        std::snprintf(path,sizeof(path),CH_USERDATA "/controllers/midi-%04x-%04x-%02x-%02x%s%s.json",
            d.vid,d.pid,d.interface,d.endpoint,player.empty()?"":"-player-",player.c_str());return path;
    }
    bool loadMidiMapping(unsigned source,const std::string& player,rhythm::midi::Profile& result) const {
        std::string error;rhythm::midi::Profile mapped;
        if(!rhythm::midi::loadProfile(midiProfilePath(source,player).c_str(),mapped,error) ||
           mapped.vid!=midiSources[source].vid || mapped.pid!=midiSources[source].pid)return false;
        result=std::move(mapped);return true;
    }
    void beginMidiSetup(unsigned source,int navSlot=-1,bool remap=false) {
        if(source>=4 || !midiSources[source].present)return;
        midiReturnScreen=screen==Screen::MidiSetup?Screen::MainMenu:screen;
        midiSetupSource=int(source);midiSetupGeneration=midiSources[source].generation;
        midiNavSlot=navSlot;midiTypePage=!remap;midiChoice=0;midiNavHeld=0;
        midiMenuFilter.reset();midiMessage.clear();
        const auto& device=midiSources[source];midiWizard.begin(device.vid,device.pid,"USB MIDI drums");
        std::string error;
        midiSavedMapping=loadMidiMapping(source,{},midiWizard.draft);
        if(navSlot>=0){
            const int seat=playerSeats.seatOfSlot[navSlot];
            if(seat>=0 && midiSourceForSeat(seat)==int(source))midiWizard.draft=midiLinks[seat].profile;
        }
        if(remap){midiWizard.begin(device.vid,device.pid,"USB MIDI drums");(void)usbMidi.approve(source,midiSetupGeneration);}
        screen=Screen::MidiSetup;previousButtons=0;
    }
    void pollMidiSources() {
        midiSources=usbMidi.snapshot(&midiBatches);
        for(unsigned source=0;source<4;++source){
            if(!midiSources[source].present)midiIgnored[source]=0;
            if(midiBatches[source].dropped)rhythm::logDiagnostic("MIDI overflow source=%u dropped=%u",source,midiBatches[source].dropped);
        }
        for(int seat=0;seat<4;++seat)profileMenu.slots[seat].midiAvailable=midiSourceForSeat(seat)>=0 && profileMenu.profile(seat) && profileMenu.profile(seat)->controller>=2;
        for(int seat=0;seat<playerSeats.count;++seat){
            const int source=midiSourceForSeat(seat);
            auto& link=midiLinks[seat];auto* profile=profileMenu.profile(seat);
            if(source<0){
                if(link.source>=0 && screen==Screen::Gameplay && !ui.visible){
                    ui.visible=true;pauseOwner=seat;pausePage=PausePage::Main;pauseSelection=0;
                    controllerToast="MIDI kit disconnected - reconnect from the main menu";controllerToastUntil=ImGui::GetTime()+5;
                    rhythm::logDiagnostic("MIDI disconnected seat=%d",seat);
                }
                continue;
            }
            const auto key=midiPlayerKey(profile,profileMenu.slots[seat].profile>=0);
            if(key!=link.playerKey){
                std::string error;rhythm::midi::Profile mapped;
                if((!key.empty() && loadMidiMapping(source,key,mapped)) || loadMidiMapping(source,{},mapped))link.profile=std::move(mapped);
                link.playerKey=key;
                if(profile){profile->controller=link.profile.mapping.layout()==rhythm::midi::Layout::Pro4?4:3;profileMenu.dirty=true;}
            }else if(profile && profile->controller!=(link.profile.mapping.layout()==rhythm::midi::Layout::Pro4?4:3)){
                // Changing the instrument explicitly leaves MIDI mode.
                midiIgnored[source]=midiSources[source].generation;link=MidiLink{};
            }
        }
        if(screen!=Screen::MainMenu || searchKeyboardOpen || instrumentKeyboard.open || updateDialog)return;
        if(std::any_of(profileMenu.slots.begin(),profileMenu.slots.end(),[](const auto& slot){return slot.page!=rhythm::ProfileMenu::Page::Closed;}))return;
        for(unsigned source=0;source<4;++source){
            const auto& d=midiSources[source];if(!d.present || midiIgnored[source]==d.generation)continue;
            bool linked=false;for(int seat=0;seat<playerSeats.count;++seat)linked|=midiSourceForSeat(seat)==int(source);
            if(!linked){beginMidiSetup(source);break;}
        }
    }
    void handleMidiSetup() {
        if(midiSetupSource<0 || midiSetupSource>=4 || !midiSources[midiSetupSource].present ||
           midiSources[midiSetupSource].generation!=midiSetupGeneration){screen=midiReturnScreen;midiSetupSource=-1;return;}
        // A native companion remains the only menu source. A kit never emits a
        // menu button or consumes an scePad slot; each kit keeps its own queue.
        if(midiTypePage)for(int slot=0;slot<4;++slot)if((sourceConnectedMask&(1u<<slot)) && sourcePads[slot].pressed){
            if(midiNavSlot!=slot){midiNavSlot=slot;midiNavHeld=0;}break;
        }
        if(midiNavSlot<0)for(int slot=0;slot<4;++slot)if(sourceConnectedMask&(1u<<slot)){midiNavSlot=slot;break;}
        if(midiNavSlot<0 || !(sourceConnectedMask&(1u<<midiNavSlot)))return;
        auto input=sourcePads[midiNavSlot];
        if(midiQa && ps5.frameBudget)readPlayerInput(0,&input);
        if(input.left_y<-.5f)input.held|=PAD_UP;if(input.left_y>.5f)input.held|=PAD_DOWN;
        const auto edges=(input.held&~midiNavHeld)|input.pressed;midiNavHeld=input.held;
        const auto pressed=midiMenuFilter.update(edges,menuInputTime);
        if(pressed&PAD_CIRCLE){midiIgnored[midiSetupSource]=midiSetupGeneration;screen=midiReturnScreen;previousButtons=input.held;return;}
        const auto page=midiWizard.page;
        const int count=midiTypePage?2:page==rhythm::midi::SetupWizard::Page::Review?3:2;
        if(pressed&PAD_UP)midiChoice=(midiChoice+count-1)%count;
        if(pressed&PAD_DOWN)midiChoice=(midiChoice+1)%count;
        const auto nowUs=uint64_t(now_seconds()*1000000.0);
        if(midiTypePage){
            if(pressed&PAD_CROSS){
                if(midiChoice==1){midiIgnored[midiSetupSource]=midiSetupGeneration;screen=midiReturnScreen;previousButtons=input.held;return;}
                const int seat=playerSeats.seatOfSlot[midiNavSlot];
                const auto key=seat>=0?midiPlayerKey(profileMenu.profile(seat),profileMenu.slots[seat].profile>=0):std::string{};
                if(!key.empty()){
                    std::string error;rhythm::midi::Profile mapped;
if(loadMidiMapping(midiSetupSource,key,mapped)){midiWizard.draft=std::move(mapped);midiSavedMapping=true;}
                }
                midiTypePage=false;midiChoice=0;
                if(midiSavedMapping)midiWizard.page=rhythm::midi::SetupWizard::Page::Review;
                if(!midiQa && !usbMidi.approve(midiSetupSource,midiSetupGeneration))midiMessage="Device changed. Reconnect the kit.";
            }
            return;
        }
        if(page==rhythm::midi::SetupWizard::Page::Capture || page==rhythm::midi::SetupWizard::Page::Soft || page==rhythm::midi::SetupWizard::Page::Hard){
            if((pressed&PAD_SQUARE) && page!=rhythm::midi::SetupWizard::Page::Capture)midiWizard.skipVelocity(nowUs);
            else for(unsigned n=0;n<midiBatches[midiSetupSource].count;++n)
                midiWizard.sample(midiBatches[midiSetupSource].strikes[n]);
            midiChoice=0;return;
        }
        if(!(pressed&PAD_CROSS))return;
        if(page==rhythm::midi::SetupWizard::Page::Layout)midiWizard.selectLayout(midiChoice==0?rhythm::midi::Layout::Pro4:rhythm::midi::Layout::FiveLane);
        else if(page==rhythm::midi::SetupWizard::Page::KickPads)midiWizard.selectKickPads(midiChoice==1,nowUs);
        else if(page==rhythm::midi::SetupWizard::Page::OfferVelocity)midiWizard.chooseVelocity(midiChoice==0,nowUs);
        else if(page==rhythm::midi::SetupWizard::Page::Review){
            if(midiChoice==2){const auto d=midiSources[midiSetupSource];midiWizard.begin(d.vid,d.pid,"USB MIDI drums");midiChoice=0;return;}
            if(!midiSources[midiSetupSource].streaming){midiMessage="Waiting for MIDI input. Reconnect a class-compliant USB MIDI 1.0 kit.";return;}
            int seat=playerSeats.seatOfSlot[midiNavSlot];
            if(seat<0 && playerSeats.count>=4){midiMessage="Four players are already joined. Leave a slot free for this kit.";return;}
            const auto key=seat<0?std::string{}:midiPlayerKey(profileMenu.profile(seat),profileMenu.slots[seat].profile>=0);
            if(midiChoice==1 && key.empty()){midiMessage="Choose a saved player profile first, or save for this device.";return;}
            if(mkdir(CH_USERDATA "/controllers",0777) && errno!=EEXIST){midiMessage="Could not create mapping storage.";return;}
            std::string error;
            if(midiChoice==0 && !rhythm::midi::saveProfile(midiProfilePath(midiSetupSource).c_str(),midiWizard.draft,error)){midiMessage=error;return;}
            if(!key.empty() && !rhythm::midi::saveProfile(midiProfilePath(midiSetupSource,key).c_str(),midiWizard.draft,error)){midiMessage=error;return;}
            if(seat<0)seat=playerSeats.join(midiNavSlot);
            if(seat<0){midiMessage="No player seat is available.";return;}
            for(int other=0;other<4;++other)if(other!=seat && midiLinks[other].source==midiSetupSource)midiLinks[other]=MidiLink{};
            midiLinks[seat]={midiSetupSource,midiSetupGeneration,midiWizard.draft,key};
            const int type=midiWizard.draft.mapping.layout()==rhythm::midi::Layout::Pro4?4:3;
            auto* profile=profileMenu.profile(seat);profileMenu.slots[seat].guest.controller=type;
            if(profile){profile->controller=type;profileMenu.dirty=true;}
            else profileMenu.slots[seat].page=rhythm::ProfileMenu::Page::Profiles;
            playerInstrument[seat]=3;playerReady[seat]=false;playerPreviousButtons[seat]=input.held;menuPressFilters[seat].reset();
            rhythm::logDiagnostic("MIDI attached source=%d seat=%d layout=%d",midiSetupSource,seat,type);
            screen=midiReturnScreen;controllerToast="MIDI drums connected - use DualSense for menus";controllerToastUntil=ImGui::GetTime()+4;previousButtons=input.held;
        }
        midiChoice=0;
    }
    std::array<rhythm::controls::UsbSample,4> usbInputs;
    std::array<rhythm::controls::RawInputBatch,4> usbInputBatches;
    double lastJudgementInputTime{};
    bool rawStarHeld{};
    unsigned rawInputDrops{};
    rhythm::InputDiagnostics inputDiagnostics;
    void flushInputDiagnostics() {
#if !defined(PS5_HOST_REFERENCE)
        if(!inputDiagnostics.dirty)return;
        if(FILE* f=std::fopen("/app0/controller-timing.txt","w")) {
            std::fprintf(f,"raw_usb %d bot %d window_ms %.3f audio_offset_ms %.3f video_offset_ms %.3f\n",
                activeControllerProfile.source==rhythm::controls::Source::RawHid,activeProfile.bot,hitWindowMs,audioOffsetMs,videoOffsetMs);
            inputDiagnostics.write(f);std::fclose(f);inputDiagnostics.dirty=false;
        }
#endif
    }
    std::array<int, PAD_PLAYERS> playerInstrument{}; // PC PlayerSelectionMenu default: Lead Guitar
    std::array<int, PAD_PLAYERS> playerDifficulty{{3,3,3,3}}; // Easy..Expert, scene default Expert
    std::array<rhythm::play_menu::Page, PAD_PLAYERS> playerPage{};
    std::array<int, PAD_PLAYERS> playerMenuRow{};
    std::array<int, PAD_PLAYERS> selectionProfileId{}, selectionController{};
    std::array<uint8_t,8> selectedSongDifficulties{};
    std::array<uint8_t, PAD_PLAYERS> playerModifierMask{};
    std::array<uint32_t, PAD_PLAYERS> playerPreviousButtons{};
    std::array<uint32_t, 5> fretBindings{{ PAD_CROSS, PAD_CIRCLE, PAD_SQUARE, PAD_TRIANGLE, PAD_L1 }};
    uint32_t starPowerBinding{ PAD_TOUCH_PAD };
    std::vector<TexVertex> spriteScratch, boardScratch, sustainScratch;


    // Each parked engine refers to this object's stable chart address. Chart
    // contents and mutable engine state MUST move together; no judgement runs
    // while a different seat's chart is selected. GPU resources are per seat,
    // while textures, pipelines, swapchain and the audio clock remain shared.
#define CH_GAMEPLAY_FIELDS(X) \
    X(notes) \
    X(activeSustainNotes) \
    X(starPhrases) \
    X(starPower) \
    X(solos) \
    X(soloBonusTotal) \
    X(fretBonusTotal) \
    X(songHopoThreshold) \
    X(chartFileLoaded) \
    X(chart) \
    X(drumPadEdges) \
    X(chartProDrums) \
    X(drumFxClock) \
    X(drumHitAt) \
    X(drumHitActive) \
    X(drumPhraseHitAt) \
    X(drumHitDynamics) \
    X(drumFretMotion) \
    X(drumFretOffset) \
    X(inputPrimed) \
    X(wasConnected) \
    X(wasAdvancing) \
    X(visibleNotes) \
    X(frets) \
    X(score) \
    X(combo) \
    X(bestCombo) \
    X(hits) \
    X(misses) \
    X(bestScore) \
    X(chartBaseScore) \
    X(chordNoteOffsets) \
    X(chartInstrument) \
    X(chartDifficulty) \
    X(newHighScore) \
    X(hitGlow) \
    X(guitarFxClock) \
    X(guitarHitAt) \
    X(guitarHitActive) \
    X(guitarHitPhrase) \
    X(guitarPhraseHitAt) \
    X(guitarEmitRemainder) \
    X(guitarSustaining) \
    X(guitarParticles) \
    X(activeProfile) \
    X(activeGuest) \
    X(activeControllerProfile) \
    X(lastJudgementInputTime) \
    X(rawStarHeld) \
    X(rawInputDrops) \
    X(inputDiagnostics) \
    X(gameplayHealth) \
    X(songFailed) \
    X(missMuted) \
    X(rumbleActive) \
    X(rumbleStopTime) \
    X(noteSpeedScale) \
    X(gamepadMode) \
    X(highwayShakeTime) \
    X(meterAttachScale) \
    X(stringAttachScale) \
    X(spHighwayAlpha) \
    X(spHighwayTime) \
    X(spHighwayActive) \
    X(tempoSegments) \
    X(starPowerClock) \
    X(chartResolutionTicks) \
    X(beatLines) \
    X(measureTimes) \
    X(countdownRanges) \
    X(uniformData) \
    X(gameplayEye) \
    X(spriteScratch) \
    X(boardScratch) \
    X(sustainScratch) \
    X(meterScratch) \
    X(meterDraws) \
    X(spriteDraws) \
    X(gameplayDraws) \
    X(texVertexCount) \
    X(sustainVertexCount) \
    X(boardVertexCount) \
    X(highwayStaticCount) \
    X(highwayVertexCount) \
    X(beatVertexCount) \
    X(resultsSelection) \
    X(resultsScroll) \
    X(resultStats) \
    X(uniformBuffers) \
    X(texBuffers) \
    X(sustainBuffers) \
    X(boardBuffers) \
    X(meterBuffers) \
    X(atlasSets) \
    X(drumAtlasSets) \
    X(sixFretSets) \
    X(guitarFxSets) \
    X(boardSets) \
    X(boardStyleIds) \
    X(meterSets) \
    X(highwaySets) \
    X(beatSets)
    struct ParkedGameplay {
#define CH_DECLARE_FIELD(name) decltype(VulkanExample::name) name{};
        CH_GAMEPLAY_FIELDS(CH_DECLARE_FIELD)
#undef CH_DECLARE_FIELD
        rhythm::GuitarSession guitar;
        rhythm::DrumSession drums;
        explicit ParkedGameplay(rhythm::SongChart& activeChart):guitar(activeChart),drums(activeChart){}
    };
    std::array<std::unique_ptr<ParkedGameplay>,PAD_PLAYERS> parkedGameplay;
    bool multiplayerQa{},multiplayerQaLibraryLoaded{};
    double multiplayerQaTime{-LEAD_IN};
    unsigned gameplaySeatMask{};
    int primaryGameplaySeat{},activeGameplaySeat{},pauseOwner{};
    std::array<uint32_t,PAD_PLAYERS> gameplayMenuHeld{};
    std::array<bool,PAD_PLAYERS> gameplayStarHeld{};
    void swapGameplay(ParkedGameplay& parked) noexcept {
#define CH_SWAP_FIELD(name) std::swap(name,parked.name);
        CH_GAMEPLAY_FIELDS(CH_SWAP_FIELD)
#undef CH_SWAP_FIELD
        session.swapRuntimeState(parked.guitar);
        drumSession.swapRuntimeState(parked.drums);
    }
#undef CH_GAMEPLAY_FIELDS
    void selectGameplaySeat(int seat) {
        if(seat==activeGameplaySeat){gameplayPlayer=seat;return;}
        if(!parkedGameplay[activeGameplaySeat])parkedGameplay[activeGameplaySeat]=std::make_unique<ParkedGameplay>(chart);
        swapGameplay(*parkedGameplay[activeGameplaySeat]);
        if(!parkedGameplay[seat])parkedGameplay[seat]=std::make_unique<ParkedGameplay>(chart);
        swapGameplay(*parkedGameplay[seat]);
        activeGameplaySeat=gameplayPlayer=seat;
    }
    unsigned gameplayPlayerCount() const noexcept {return std::max(1,std::popcount(gameplaySeatMask));}
    unsigned gameplayOrdinal() const noexcept {return std::popcount(gameplaySeatMask&((1u<<gameplayPlayer)-1));}
    VkViewport gameplayViewport() const {
        const auto view=rhythm::pcPlayerViewport(gameplayPlayerCount(),gameplayOrdinal(),float(width)/height);
        return {view.x*width,view.y*height,view.width*width,view.height*height,0.0f,1.0f};
    }
    void ensureGameplayBuffers() {
        if(uniformBuffers[0].buffer)return;
        prepareUniformBuffers();buildGeometry();allocateGameplayDescriptors();
    }
    void destroyParkedGameplayBuffers() {
        for(auto& parked:parkedGameplay)if(parked) {
            for(auto* buffers:{&parked->uniformBuffers,&parked->texBuffers,&parked->sustainBuffers,&parked->boardBuffers,&parked->meterBuffers})
                for(auto& buffer:*buffers)if(buffer.buffer)buffer.destroy();
        }
    }

    static constexpr const char* profileStoragePath=CH_USERDATA "/clonehero-profiles.json";
    void probeControllerAccess() {
#if !defined(PS5_HOST_REFERENCE)
        FILE* log=std::fopen("/app0/controller-access.txt","w");
        if(!log)return;
        const auto scan=[&](const char* phase) {
            unsigned nodes=0,opened=0;errno=0;
            DIR* dir=opendir("/dev");const int directoryError=errno;
            if(dir) {
                while(auto* entry=readdir(dir)) if(std::strncmp(entry->d_name,"ugen",4)==0) {
                    ++nodes;const std::string path=std::string("/dev/")+entry->d_name;
                    const int fd=open(path.c_str(),O_RDWR|O_NONBLOCK);
                    std::fprintf(log,"%s node %s accessible %d errno %d\n",phase,entry->d_name,fd>=0,fd<0?errno:0);
                    if(fd>=0){
                        ++opened;struct usb_device_descriptor descriptor{};
                        const int result=ioctl(fd,USB_GET_DEVICE_DESC,&descriptor);
                        std::fprintf(log,"%s descriptor_result %d vendor %04x product %04x\n",entry->d_name,result,
                            descriptor.idVendor[0]|(descriptor.idVendor[1]<<8),descriptor.idProduct[0]|(descriptor.idProduct[1]<<8));
                        const bool reportProbe=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="probe-report";});
                        if(result==0 && reportProbe && (descriptor.idVendor[0]|(descriptor.idVendor[1]<<8))==0x12ba && (descriptor.idProduct[0]|(descriptor.idProduct[1]<<8))==0x0100) {
                            uint8_t report[27]{};usb_ctl_request request{};request.ucr_data=report;
                            request.ucr_request.bmRequestType=0xa1;request.ucr_request.bRequest=1;
                            request.ucr_request.wValue[1]=1;request.ucr_request.wLength[0]=sizeof(report);
                            errno=0;const int readResult=ioctl(fd,USB_DO_REQUEST,&request);
                            std::fprintf(log,"direct_get_report result %d errno %d length %u frets %02x menu %02x hat %02x\n",readResult,readResult?errno:0,unsigned(request.ucr_actlen),report[0],report[1],report[2]);
                        }
                        close(fd);
                    }
                }
                closedir(dir);
            }
            std::fprintf(log,"%s dev_directory %d errno %d ugen_nodes %u accessible %u\n",phase,dir!=nullptr,dir?0:directoryError,nodes,opened);
            return opened;
        };
        unsigned accessible=scan("before");
        // Measured native title link surface: these payload SDK helpers are
        // not public system-module exports and require elfldr's kernel bootstrap.
        std::fprintf(log,"payload_credential_helpers 0\n");
        std::fprintf(log,"elevate unavailable: payload helper imports rejected by native title link surface\n");
        std::fprintf(log,"backend %s\n",accessible?"raw_usb_accessible":"bridge_required");
        std::fclose(log);
#endif
    }

    void loadProfiles() {
#if !defined(PS5_HOST_REFERENCE)
        // On this title's filesystem stat can report ENOTDIR for a path that
        // fopen reads successfully. Check the actual file, not stat metadata.
        FILE* existing=fopen(profileStoragePath,"rb");
        if(existing) {
            fclose(existing);
            if(!profileMenu.load(profileStoragePath))
                fprintf(stderr,"[profiles] Could not read saved profiles\n");
        } else if((errno==ENOENT || errno==ENOTDIR) &&
                  (profileMenu.load(CH_USERDATA "/clonehero-profiles.cfg") || profileMenu.load("/download0/clonehero-profiles.cfg"))) {
            profileMenu.dirty=true;
        }
#endif
    }
    bool saveProfiles() {
#if !defined(PS5_HOST_REFERENCE)
        if(!profileMenu.dirty) return true;
        if(mkdir(CH_USERDATA "",0777)!=0 && errno!=EEXIST) return false;
        if(!profileMenu.save(profileStoragePath)) {
            fprintf(stderr,"[profiles] Save failed: %s\n",strerror(errno));
            rhythm::logDiagnostic("ERROR profile save errno=%d",errno);
            return false;
        }
        return true;
#else
        profileMenu.dirty=false;
        return true;
#endif
    }
    rhythm::controls::Mapping effectiveMapping(const rhythm::MenuProfile* profile,int type) const {
        auto mapping=profile?profile->mappings[type]:rhythm::controls::defaults(type);
        if(type!=0 && !mapping.custom)return rhythm::controls::defaults(type);
        if(type==0 && !mapping.custom) {
            for(unsigned lane=0;lane<5;++lane) mapping.keys[lane]=fretBindings[lane];
            mapping.keys[5]=horizontalStrum?PAD_RIGHT:PAD_UP;
            mapping.keys[6]=horizontalStrum?PAD_LEFT:PAD_DOWN;
            mapping.keys[8]=starPowerBinding;
        }
        return mapping;
    }
    uint32_t menuButtons(int player,uint32_t raw) const {
        const auto* profile=profileMenu.profile(player);
        if(!profile || midiLinks[player].source>=0)return raw;
        const int type=std::clamp(profile->controller,0,4);
        if(!ps5.frameBudget && player<playerSeats.count && playerSeats.slotOfSeat[player]>=4)return raw;
        const auto mapping=effectiveMapping(profile,type);
        constexpr uint32_t canonical[]{PAD_CROSS,PAD_CIRCLE,PAD_SQUARE,PAD_TRIANGLE,PAD_L1};
        constexpr unsigned drumKeys[]{4,1,2,3,0};
        uint32_t assigned=0;
        for(unsigned i=0;i<rhythm::controls::count(type);++i)assigned|=mapping.keys[i];
        uint32_t out=raw&~(PAD_CROSS|PAD_CIRCLE|PAD_SQUARE|PAD_TRIANGLE|PAD_L1|assigned);
        out|=raw&(PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT|PAD_OPTIONS);
        if(raw&mapping.keys[rhythm::controls::count(type)-2])out|=PAD_OPTIONS;
        if(raw&mapping.keys[rhythm::controls::count(type)-1])out|=PAD_TOUCH_PAD;
        if(type<2){
            if(raw&mapping.keys[rhythm::controls::count(type)-4])out|=PAD_UP;
            if(raw&mapping.keys[rhythm::controls::count(type)-3])out|=PAD_DOWN;
        }
        for(unsigned i=0;i<5;++i) {
            const unsigned proKeys[]{6,1,2,4,0},fiveKeys[]{1,2,3,4,0},sixKeys[]{3,4,5,0,1};
            const auto key=mapping.keys[type==3?fiveKeys[i]:type==4?proKeys[i]:(type>=2?drumKeys[i]:type==1?sixKeys[i]:i)];
            if(raw&key)out|=canonical[i];
        }
        if(type==3 && (raw&mapping.keys[5]))out|=PAD_L1;
        if(type==4){
            if(raw&mapping.keys[3])out|=PAD_SQUARE;
            if(raw&mapping.keys[5])out|=PAD_TRIANGLE;
            if(raw&mapping.keys[7])out|=PAD_CROSS;
        }
        // Fixed OS hat/Options navigation always remains an escape path.
        return out;
    }
    bool drumMenuNavigation() const {
        const auto* profile=profileMenu.profile(0);
        if(profile && profile->controller>=2)return true;
        const int source=playerSeats.count?playerSeats.slotOfSeat[0]:primarySource;
        return source>=4 && source<8 && usbInputs[source-4].deviceProfile &&
            usbInputs[source-4].deviceProfile->instrument.find("drums_")==0;
    }
    bool remapHelperReserved(int source) const {
        for(const auto& remap:playerRemaps)if(remap.active && remap.assistSource==source)return true;
        return false;
    }
    void openControllerRemap(int player,int requestedType=-1) {
        if(const int source=midiSourceForSeat(player);source>=0 && requestedType<0){
            beginMidiSetup(source,playerSeats.slotOfSeat[player],true);return;
        }
        auto& remap=playerRemaps[player];
        const auto* profile=profileMenu.profile(player);if(!profile && requestedType<0)return;
        const int type=requestedType>=0?requestedType:std::clamp(profile->controller,0,4);
        if(!rhythm::controls::controllerTypeEnabled(type)) {profileMenu.slots[player].message="6-Fret Guitar: Coming Soon";return;}
        const auto* usb=playerUsb(player);
        remap.raw=usb!=nullptr;remap.calibrated=false;remap.calibrationAt=ImGui::GetTime();
        remap.assistSource=-1;
        if(remap.raw)for(int i=0;i<PAD_PLAYERS;++i)if((sourceConnectedMask&(1u<<i)) && playerSeats.seatOfSlot[i]<0 && !remapHelperReserved(i)){remap.assistSource=i;break;}
        remapPlayer=player;remap.active=true;remap.message.clear();remap.captureFilter={};
        profileMenu.slots[player].page=rhythm::ProfileMenu::Page::Options;
        remap.navigation=effectiveMapping(profile,type);
        remap.wizard.begin(type,remap.navigation);
        if(remap.raw) {
            remap.navigation=rhythm::controls::defaults(0); // Native assistant: fixed DualSense controls.
            remap.draft=playerControllerProfile(player,type);
            if(remap.draft.source!=rhythm::controls::Source::RawHid || remap.draft.instrument!=rhythm::controls::instrumentName(type)) {
                remap.draft=rhythm::controls::nativeProfile(type,rhythm::controls::defaults(type),usb->vid,usb->pid);
                remap.draft.source=rhythm::controls::Source::RawHid;remap.draft.name="USB rhythm controller";
                for(auto& binding:remap.draft.bindings)binding.input={};
            }
            auto ordered=rhythm::controls::nativeProfile(type,rhythm::controls::defaults(type),usb->vid,usb->pid);
            ordered.source=rhythm::controls::Source::RawHid;ordered.name=remap.draft.name;ordered.reportLength=remap.draft.reportLength;
            ordered.schema=remap.draft.schema;ordered.reportId=remap.draft.reportId;
            ordered.menuX=remap.draft.menuX;ordered.menuY=remap.draft.menuY;
            ordered.menuHat=remap.draft.menuHat;ordered.menuHatMask=remap.draft.menuHatMask;
            for(auto& b:ordered.bindings) {
                auto old=std::find_if(remap.draft.bindings.begin(),remap.draft.bindings.end(),[&](const auto& item){return b.action==item.action;});
                if(old!=remap.draft.bindings.end())b=*old;else b.input={};
            }
            remap.draft=std::move(ordered);
            remap.draft.vid=usb->vid;remap.draft.pid=usb->pid;
        }
        remap.waitForRelease=true;
        remap.previousButtons=0;playerMenuRepeat[player].reset();
    }
    void closeControllerRemap(int player,uint32_t buttons) {
        auto& remap=playerRemaps[player];
        playerPreviousButtons[player]=menuButtons(player,buttons);
        remap.active=false;remap.message.clear();remap.assistSource=-1;
        profileMenu.slots[player].page=profileMenu.joined(player)?rhythm::ProfileMenu::Page::Options:rhythm::ProfileMenu::Page::Profiles;
    }
    bool remapReviewEnabled(int player,int row) const {
        const auto& remap=playerRemaps[player];
        if(row==0)return profileMenu.profile(player)!=nullptr;
        if(row==1)return remap.raw;
        if(row==3 && remap.raw){const auto* u=playerUsb(player);return u && u->deviceProfile && u->deviceProfile->instrument==rhythm::controls::instrumentName(remap.wizard.type);}
        return true;
    }
    bool timingCalibrationRunning() const {
        for(const auto& c:playerCalibrations)if(c.phase==PlayerCalibration::Phase::WaitingAudio || c.phase==PlayerCalibration::Phase::Timing)return true;
        return false;
    }
    void syncCalibrationAudio() {
#if !defined(PS5_HOST_REFERENCE)
        songAudio.calibrationTone.enable(timingCalibrationRunning(),masterVolume*.65f);
#endif
    }
    bool calibrationReference(uint64_t stamp,double& time,uint64_t& epoch) const {
#if !defined(PS5_HOST_REFERENCE)
        return songAudio.calibrationTone.timeAt(stamp,time,epoch);
#else
        (void)stamp;(void)time;(void)epoch;return false;
#endif
    }
    void closePlayerCalibration(int player,uint32_t held,const char* message="") {
        playerCalibrations[player].phase=PlayerCalibration::Phase::None;
        auto& slot=profileMenu.slots[player];slot.page=rhythm::ProfileMenu::Page::Calibration;slot.row=0;slot.message=message;
        playerPreviousButtons[player]=menuButtons(player,held);playerMenuRepeat[player].reset();syncCalibrationAudio();
    }
    void beginPlayerCalibration(int player,bool velocity=false) {
        auto* profile=profileMenu.profile(player);if(!profile)return;
        auto& slot=profileMenu.slots[player];slot.message.clear();
        const int source=profile->controller>=2?midiSourceForSeat(player):-1;
        if(velocity && source<0){slot.message="Connect and map a MIDI kit first.";return;}
#if !defined(PS5_HOST_REFERENCE)
        if(!velocity && (!audioOutputStarted || masterVolume<=0)) {slot.message="Enable audio and raise Master Volume first.";return;}
#else
        if(!velocity){slot.message="Timing test needs console audio output.";return;}
#endif
        auto& c=playerCalibrations[player];c={};c.type=std::clamp(profile->controller,0,4);c.profileId=slot.profile;
        c.source=source;c.generation=source>=0?midiSources[source].generation:0;
        c.phase=velocity?PlayerCalibration::Phase::Velocity:PlayerCalibration::Phase::WaitingAudio;
        c.waitRelease=true;c.lastAudioAt=uint64_t(now_seconds()*1000000);slot.page=rhythm::ProfileMenu::Page::Calibration;
        if(velocity)c.velocity.beginVelocity(midiLinks[player].profile,uint64_t(now_seconds()*1000000));
        syncCalibrationAudio();
    }
    void savePlayerCalibration(int player,uint32_t held) {
        auto& c=playerCalibrations[player];auto* profile=profileMenu.profile(player);if(!profile)return;
        auto& slot=profileMenu.slots[player];
        if(c.phase==PlayerCalibration::Phase::TimingReview) {
            if(!c.timing.valid){c.message="Timing varied too much. Retry the test.";return;}
            const float previous=profile->inputOffsetsMs[c.type];const bool dirty=profileMenu.dirty;
            const float suggested=float(std::round(c.timing.offsetMs+audioOffsetMs));
            if(!std::isfinite(suggested) || std::abs(suggested)>rhythm::MaxPlayerOffsetMs){c.message="Offset out of range. Check Audio Offset in Settings.";return;}
            profile->inputOffsetsMs[c.type]=suggested;
            profileMenu.dirty|=slot.profile>=0;
            if(!saveProfiles()){profile->inputOffsetsMs[c.type]=previous;profileMenu.dirty=dirty;c.message="Could not save. Previous timing is unchanged.";return;}
        } else if(c.phase==PlayerCalibration::Phase::VelocityReview) {
            if(midiSourceForSeat(player)!=c.source || midiSources[c.source].generation!=c.generation){c.message="The MIDI kit changed. Start again.";return;}
            const auto key=midiPlayerKey(profile,slot.profile>=0);
            if(!key.empty() && !rhythm::midi::saveProfile(midiProfilePath(c.source,key).c_str(),c.velocity.draft,c.message))return;
            midiLinks[player].profile=c.velocity.draft;
        } else return;
        closePlayerCalibration(player,held,slot.profile>=0?"Calibration saved for this player.":"Guest calibration applied for this session.");
    }
    void updatePlayerCalibration(int player,const pad& input) {
        auto& c=playerCalibrations[player];using Phase=PlayerCalibration::Phase;
        const auto* profile=profileMenu.profile(player);auto& slot=profileMenu.slots[player];
        if(!profile || slot.profile!=c.profileId || profile->controller!=c.type){closePlayerCalibration(player,input.held,"Profile changed; restart calibration.");return;}
        if(c.source>=0 && (midiSourceForSeat(player)!=c.source || midiSources[c.source].generation!=c.generation)){
            closePlayerCalibration(player,input.held,"MIDI kit disconnected. Previous settings kept.");return;
        }
        const auto held=menuButtons(player,input.held);
        uint32_t edges=held&~c.previousButtons;c.previousButtons=held;
        const auto menuEdges=filterMenuPresses(player,edges);
        if(c.waitRelease){if(!held)c.waitRelease=false;return;}
        if(menuEdges&(PAD_CIRCLE|PAD_OPTIONS)){closePlayerCalibration(player,input.held,"Calibration cancelled.");return;}
        if(c.phase==Phase::TimingReview || c.phase==Phase::VelocityReview){
            const unsigned controls=playerMenuRepeat[player].update(held&(PAD_UP|PAD_DOWN),menuEdges,float(frameTimer),2304+int(c.phase),true);
            if((menuEdges|controls)&PAD_UP)c.row=(c.row+2)%3;
            if((menuEdges|controls)&PAD_DOWN)c.row=(c.row+1)%3;
            if(menuEdges&PAD_CROSS){
                if(c.row==0)savePlayerCalibration(player,input.held);
                else if(c.row==1)beginPlayerCalibration(player,c.phase==Phase::VelocityReview);
                else closePlayerCalibration(player,input.held,"Calibration cancelled.");
            }
            return;
        }
        const auto nowUs=uint64_t(now_seconds()*1000000.0);
        if(c.phase==Phase::Velocity){
            if(menuEdges&PAD_SQUARE)c.velocity.skipVelocity(nowUs);
            for(unsigned i=0;i<midiBatches[c.source].count;++i)c.velocity.sample(midiBatches[c.source].strikes[i]);
            if(c.velocity.page==rhythm::midi::SetupWizard::Page::Review){c.phase=Phase::VelocityReview;c.row=0;}
            return;
        }
        double now=0;uint64_t epoch=0;
        if(!calibrationReference(nowUs,now,epoch)){
            if(nowUs>c.lastAudioAt+2000000)closePlayerCalibration(player,input.held,"Audio unavailable. Previous calibration kept.");
            return;
        }
        c.lastAudioAt=nowUs;
        if(c.phase==Phase::WaitingAudio || c.audioEpoch!=epoch){
            const bool restarted=c.audioEpoch!=0;
            c.phase=Phase::Timing;c.audioEpoch=epoch;c.timing.begin(now);c.previousTap=0;
            if(restarted)c.message="Audio restarted; timing test reset.";
        }
        const auto sample=[&](uint64_t stamp){double at=0;uint64_t generation=0;
            if(calibrationReference(stamp,at,generation) && generation==c.audioEpoch)c.timing.sample(at);
        };
        if(c.source>=0){
            for(unsigned i=0;i<midiBatches[c.source].count;++i)
                midiLinks[player].profile.mapping.apply(midiBatches[c.source].strikes[i],[&](const auto& strike){sample(strike.receivedUs);});
        }else{
            const auto mapping=playerControllerProfile(player,c.type);
            const auto tap=[&](const rhythm::controls::InstrumentState& state,uint64_t stamp){
                const unsigned value=c.type>=2?unsigned(state.pads&1):unsigned(state.frets&1)|(state.strumUp?2u:0u)|(state.strumDown?4u:0u);
                if(value&~c.previousTap)sample(stamp);c.previousTap=value;
            };
            const auto* usb=playerUsb(player);
            if(usb && mapping.source==rhythm::controls::Source::RawHid){
                const auto& batch=usbInputBatches[playerSeats.slotOfSeat[player]-4];
                for(unsigned i=0;i<batch.count;++i){const auto& report=batch.reports[i];tap(rhythm::controls::applyProfile(mapping,{},report.bytes.data(),report.length),report.receivedUs);}
            }else{
                rhythm::controls::NativeSample native{input.held,{input.l2*255,input.r2*255,input.left_x*127+128,input.left_y*127+128,input.right_x*127+128,input.right_y*127+128}};
                tap(rhythm::controls::applyProfile(mapping,native,nullptr,0),nowUs);
            }
        }
        c.timing.update(now);
        if(c.timing.state==rhythm::TimingCalibration::State::Review){c.phase=Phase::TimingReview;c.row=c.timing.valid?0:1;syncCalibrationAudio();}
    }

    void updateControllerRemap(int player,uint32_t pressed,uint32_t buttons) {
        auto& remap=playerRemaps[player];
        using Page=rhythm::controls::Remap::Page;
        if(remap.raw && !remap.calibrated) {
            const auto* u=playerUsb(player);
            if((pressed&PAD_OPTIONS) || (pressed&PAD_CIRCLE)) {closeControllerRemap(player,buttons);return;}
            if(u && u->streaming && u->length && ImGui::GetTime()-remap.calibrationAt>=1.0) {
                remap.capture.begin(u->report.data(),u->length);remap.draft.reportLength=u->length;
                remap.calibrated=true;remap.waitForRelease=false;
            }
            return;
        }
        if(remap.waitForRelease) {if(buttons==0)remap.waitForRelease=false;return;}
        const uint32_t green=remap.raw?PAD_CROSS:remap.navigation.keys[rhythm::controls::greenIndex(remap.wizard.type)];
        const uint32_t red=remap.raw?PAD_CIRCLE:remap.navigation.keys[rhythm::controls::redIndex(remap.wizard.type)];
        if(remap.wizard.page==Page::Capture) {
            const auto action=rhythm::controls::action(remap.wizard.type,remap.wizard.index);
            const bool captureStart=std::strcmp(action,"Start")==0;
            const bool assistantEscape=remap.raw && remap.assistSource>=0 && (sourcePads[remap.assistSource].held&PAD_OPTIONS);
            if((pressed&PAD_OPTIONS) && (!captureStart || assistantEscape)) {remap.wizard.page=Page::Review;remap.wizard.row=0;return;}
            if(remap.raw) {
                const auto* u=playerUsb(player);if(!u)return;
                const bool hat=std::strcmp(action,"Strum Up")==0 || std::strcmp(action,"Strum Down")==0;
                rhythm::controls::Locator candidate;
                if(remap.capture.capture(u->report.data(),u->length,hat,candidate,remap.message,(u->deviceProfile && u->deviceProfile->instrument==remap.draft.instrument)?u->deviceProfile.get():nullptr)) {
                    if(!remap.captureFilter.accept(candidate,menuInputTime))return;
                    bool duplicate=false;
                    for(unsigned i=0;i<remap.draft.bindings.size();++i)if(i!=remap.wizard.index && rhythm::controls::sameLocator(remap.draft.bindings[i].input,candidate)) {
                        remap.candidate=candidate;remap.wizard.duplicate=i;remap.wizard.page=Page::Conflict;duplicate=true;break;
                    }
                    if(!duplicate) {remap.draft.bindings[remap.wizard.index].input=candidate;remap.wizard.advance();if(remap.wizard.page==Page::Review && !remapReviewEnabled(player,remap.wizard.row))remap.wizard.row=1;}
                }
            } else for(uint32_t button:rhythm::controls::buttons) if(pressed&button) {remap.wizard.capture(button);break;}
        } else if(remap.wizard.page==Page::Conflict) {
            if(pressed&red) remap.wizard.retryConflict();
            else if(pressed&green) {
                if(remap.raw) {
                    auto& current=remap.draft.bindings[remap.wizard.index].input;
                    if(current.mask || current.kind==rhythm::controls::Kind::Hat) {
                        std::swap(current,remap.draft.bindings[remap.wizard.duplicate].input);remap.wizard.page=Page::Capture;remap.wizard.advance();
                    } else remap.wizard.retryConflict();
                } else remap.wizard.swap();
            }
            else if(pressed&PAD_OPTIONS){remap.wizard.page=Page::Review;remap.wizard.row=0;}
        } else {
            if(pressed&red) {closeControllerRemap(player,buttons);return;}
            if(pressed&PAD_UP)do{remap.wizard.row=(remap.wizard.row+4)%5;}while(!remapReviewEnabled(player,remap.wizard.row));
            if(pressed&PAD_DOWN)do{remap.wizard.row=(remap.wizard.row+1)%5;}while(!remapReviewEnabled(player,remap.wizard.row));
            if(pressed&green) {
                if(!remapReviewEnabled(player,remap.wizard.row))return;
                if(remap.wizard.row==0 || remap.wizard.row==1) {
                    auto* profile=profileMenu.profile(player);
                    if(remap.wizard.row==1) {
                        if(!remap.raw){remap.message="Device identity is unavailable. Save to the player profile.";return;}
                        for(const char* dir:{CH_USERDATA "",CH_USERDATA "/controllers"})
                            if(mkdir(dir,0777) && errno!=EEXIST){remap.message="Could not create controller storage.";return;}
                        char path[120];
                        const auto* device=playerUsb(player);
                        if(device && device->channel>=0)std::snprintf(path,sizeof(path),CH_USERDATA "/controllers/%04x_%04x_ch%d.json",remap.draft.vid,remap.draft.pid,device->channel);
                        else std::snprintf(path,sizeof(path),CH_USERDATA "/controllers/%04x_%04x.json",remap.draft.vid,remap.draft.pid);
                        if(!rhythm::controls::saveProfile(path,remap.draft,remap.message))return;
                        if(const auto* u=playerUsb(player)) {
                            const int slot=playerSeats.slotOfSeat[player]-4;
                            if(slot>=0)ignoredControllers[slot]=(uint32_t(u->vid)<<16)|u->pid;
                        }
                        closeControllerRemap(player,buttons);return;
                    }
                    if(!profile){remap.message="Choose Save Device, then select a player profile.";return;}
                    if(!remap.raw && !rhythm::controls::valid(remap.wizard.draft,remap.wizard.type)){remap.message="Mapping is incomplete. Retry setup.";return;}
                    if(remap.raw) {rhythm::controls::ControllerProfile check;if(!rhythm::controls::parseProfile(rhythm::controls::writeProfile(remap.draft),check,remap.message)){remap.message="Some inputs are not mapped. Retry setup to finish them.";return;}}
                    const auto before=profile->mappings[remap.wizard.type];const auto beforeRaw=profile->rawMappings;
                    if(remap.raw)profile->setRawMapping(remap.draft);
                    else {profile->mappings[remap.wizard.type]=remap.wizard.draft;profile->mappings[remap.wizard.type].custom=true;}
                    const bool oldDirty=profileMenu.dirty;profileMenu.dirty|=profileMenu.slots[player].profile>=0;
                    if(!saveProfiles()) {profile->mappings[remap.wizard.type]=before;profile->rawMappings=beforeRaw;profileMenu.dirty=oldDirty;remap.message="Could not save. Your previous mapping is unchanged.";}
                    else closeControllerRemap(player,buttons);
                } else if(remap.wizard.row==2) {remap.wizard.begin(remap.wizard.type,remap.wizard.draft);if(remap.raw){remap.calibrated=false;remap.calibrationAt=ImGui::GetTime();}}
                else if(remap.wizard.row==3) {
                    if(remap.raw) {const auto* u=playerUsb(player);if(u && u->deviceProfile)remap.draft=*u->deviceProfile;else remap.message="No verified default is available. Retry setup.";}
                    else {remap.wizard.draft=rhythm::controls::defaults(remap.wizard.type);remap.message="Defaults staged. Select Save Player to apply.";}
                } else closeControllerRemap(player,buttons);
            }
        }
    }

    std::string& keyboardError(SearchKeyboardTarget target) {
        return target==SearchKeyboardTarget::Profile?profileMenu.slots[keyboardPlayer].message:
            target==SearchKeyboardTarget::LocalSongs?songSearchImeError:downloaderImeError;
    }
    void primeKeyboardInputs() {
        for(int player=0;player<PAD_PLAYERS;++player) {
            const int source=player<playerSeats.count?playerSeats.slotOfSeat[player]:-1;
            playerPreviousButtons[player]=player==keyboardPlayer?previousButtons:
                source>=0?menuButtons(player,sourcePads[source].held):0;
            playerMenuRepeat[player].reset();
        }
        if(keyboardPlayer!=0)previousButtons=playerPreviousButtons[0];
        menuRepeat.reset();keyboardRepeat.reset();
        for(int slot=0;slot<PAD_PLAYERS;++slot){keyboardAssistRepeat[slot].reset();keyboardAssistPrevious[slot]=sourcePads[slot].held;keyboardAssistSticks[slot].reset();}
    }
    void cancelTextKeyboard() {
        primeKeyboardInputs();
        instrumentKeyboard.open=false;searchKeyboardTarget=SearchKeyboardTarget::None;
        if(keyboardPlayer>=0 && keyboardPlayer<PAD_PLAYERS && profileMenu.slots[keyboardPlayer].page==rhythm::ProfileMenu::Page::Name)
            profileMenu.back(keyboardPlayer);
    }
    int searchKeyboardStatus() const {
#if !defined(PS5_HOST_REFERENCE)
        return nativeImePreferred?nativeKeyboard.status():sceImeDialogGetStatus();
#else
        return 0;
#endif
    }
    void abortSearchKeyboard() {
#if !defined(PS5_HOST_REFERENCE)
        if(nativeImePreferred)nativeKeyboard.cancel();
        else (void)sceImeDialogAbort();
#endif
    }
    bool openSearchKeyboard(SearchKeyboardTarget target,int player=0) {
        if(searchKeyboardOpen || instrumentKeyboard.open)return false;
        keyboardPlayer=player;
        rhythm::logDiagnostic("keyboard open target=%d owner=%d",int(target),player);
        std::string& keyboardError=this->keyboardError(target);
        const std::string& initialQuery=target==SearchKeyboardTarget::Profile?profileMenu.slots[player].name:
            target==SearchKeyboardTarget::LocalSongs?songSearchQuery:downloaderQuery;
        const int source=player<playerSeats.count?playerSeats.slotOfSeat[player]:primarySource;
        bool instrumentSource=source>=4;
#if defined(PS5_NATIVE_INSTRUMENT_INPUT) && !defined(PS5_HOST_REFERENCE)
        instrumentSource|=source>=0 && source<PAD_PLAYERS && pad_player_instrument(source);
#endif
        if((!ps5.frameBudget && instrumentSource) || keyboardRawQa) {
            instrumentKeyboard.begin(initialQuery,target==SearchKeyboardTarget::Profile?24:96);
            keyboardGridNavigation=true;
            searchKeyboardTarget=target;keyboardRepeat.reset();keyboardError.clear();
            for(int slot=0;slot<PAD_PLAYERS;++slot){keyboardAssistPrevious[slot]=sourcePads[slot].held;keyboardAssistRepeat[slot].reset();}
            return true;
        }
#if !defined(PS5_HOST_REFERENCE)
        static std::array<bool,2> modulesLoaded{};
        bool& moduleLoaded=modulesLoaded[nativeImePreferred?1:0];
        static bool commonDialogInitialized = false;
        if (!moduleLoaded) {
            // Each API has its own module; loading ImeDialog does not load
            // the app-side IME exports used by the native panel.
            const int loaded = sceSysmoduleLoadModule(nativeImePreferred?0x95:0x96);
            if (loaded < 0) {
                char error[96];
                std::snprintf(error, sizeof(error), "Could not load PS5 keyboard (%08X).", static_cast<unsigned>(loaded));
                keyboardError = error;
                return false;
            }
            moduleLoaded = true;
        }
        if (!nativeImePreferred && !commonDialogInitialized) {
            const int initialized = sceCommonDialogInitialize();
            if (initialized != 0 && static_cast<uint32_t>(initialized) != 0x80b80002u) {
                char error[112];
                std::snprintf(error, sizeof(error), "PS5 dialog service failed (%08X).", static_cast<unsigned>(initialized));
                keyboardError = error;
                return false;
            }
            commonDialogInitialized = true;
        }
        std::fill(searchKeyboardBuffer.begin(), searchKeyboardBuffer.end(), 0);
        size_t out = 0;
        for (size_t i = 0; i < initialQuery.size() && out < 96;) {
            const uint8_t c = static_cast<uint8_t>(initialQuery[i++]);
            uint32_t cp = c;
            unsigned extra = 0;
            if ((c & 0xe0u) == 0xc0u) { cp = c & 0x1fu; extra = 1; }
            else if ((c & 0xf0u) == 0xe0u) { cp = c & 0x0fu; extra = 2; }
            else if ((c & 0xf8u) == 0xf0u) { cp = c & 0x07u; extra = 3; }
            for (unsigned n = 0; n < extra && i < initialQuery.size(); ++n)
                cp = (cp << 6) | (static_cast<uint8_t>(initialQuery[i++]) & 0x3fu);
            if (cp <= 0xffffu) searchKeyboardBuffer[out++] = static_cast<uint16_t>(cp);
            else if (out + 1 < 96) {
                cp -= 0x10000u;
                searchKeyboardBuffer[out++] = static_cast<uint16_t>(0xd800u + (cp >> 10));
                searchKeyboardBuffer[out++] = static_cast<uint16_t>(0xdc00u + (cp & 0x3ffu));
            }
        }
        int32_t userId = 0;
        const int userResult = sceUserServiceGetForegroundUser(&userId);
        if (userResult < 0) {
            char error[104];
            std::snprintf(error, sizeof(error), "No active PS5 user for keyboard (%08X).", static_cast<unsigned>(userResult));
            keyboardError = error;
            return false;
        }
        static const uint16_t title[] = {'S','e','a','r','c','h',' ','C','h','o','r','u','s',' ','E','n','c','o','r','e',0};
        static const uint16_t placeholder[] = {'E','n','t','e','r',' ','s','o','n','g',' ','o','r',' ','a','r','t','i','s','t',0};
        searchKeyboardParam={};
        auto& param=searchKeyboardParam;
        param.userId = userId;
        param.type = 0;
        // Let the OS select its configured languages and standard enter label,
        // matching the default request in the native PS5 keyboard reference.
        param.supportedLanguages = 0;
        param.enterLabel = 0;
        param.maxTextLength = target==SearchKeyboardTarget::Profile?24:96;
        param.inputTextBuffer = searchKeyboardBuffer.data();
        param.horizontalAlignment = 1;
        param.verticalAlignment = 1;
        param.placeholder = placeholder;
        static constexpr char16_t localTitle[]=u"Search Installed Songs";
        static constexpr char16_t localPlaceholder[]=u"Enter search text";
        param.title = target==SearchKeyboardTarget::LocalSongs?reinterpret_cast<const uint16_t*>(localTitle):title;
        if(target==SearchKeyboardTarget::LocalSongs) param.placeholder=reinterpret_cast<const uint16_t*>(localPlaceholder);
        static constexpr char16_t profileTitle[]=u"Profile Name";
        if(target==SearchKeyboardTarget::Profile) {
            param.title=reinterpret_cast<const uint16_t*>(profileTitle);
            param.placeholder=nullptr;
        }
        nativeKeyboard.api={sceImeParamInit,sceImeOpen,sceImeUpdate,sceImeClose};
        const int opened = nativeImePreferred?nativeKeyboard.open(userId,searchKeyboardBuffer.data(),param.maxTextLength):
            sceImeDialogInit(&param, nullptr);
        if (opened != 0) {
            char error[104];
            std::snprintf(error, sizeof(error), "PS5 keyboard could not open (%08X).", static_cast<unsigned>(opened));
            keyboardError = error;
            return false;
        }
        searchKeyboardOpen = true;
        searchKeyboardStopping=false;
        searchKeyboardTarget=target;
        keyboardError.clear();
        return true;
#else
        keyboardError = "PS5 keyboard is unavailable in the host reference build.";
        return false;
#endif
    }
    std::string searchKeyboardText() const {
        std::string query;
        for (size_t i = 0; i < searchKeyboardBuffer.size() && searchKeyboardBuffer[i] != 0; ++i) {
            uint32_t cp = searchKeyboardBuffer[i];
            if (cp >= 0xd800u && cp <= 0xdbffu && i + 1 < searchKeyboardBuffer.size()) {
                const uint32_t low = searchKeyboardBuffer[i + 1];
                if (low >= 0xdc00u && low <= 0xdfffu) { cp = 0x10000u + ((cp - 0xd800u) << 10) + (low - 0xdc00u); ++i; }
            }
            if (cp < 0x80u) query.push_back(static_cast<char>(cp));
            else if (cp < 0x800u) {
                query.push_back(static_cast<char>(0xc0u | (cp >> 6)));
                query.push_back(static_cast<char>(0x80u | (cp & 0x3fu)));
            } else if (cp < 0x10000u) {
                query.push_back(static_cast<char>(0xe0u | (cp >> 12)));
                query.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3fu)));
                query.push_back(static_cast<char>(0x80u | (cp & 0x3fu)));
            } else {
                query.push_back(static_cast<char>(0xf0u | (cp >> 18)));
                query.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3fu)));
                query.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3fu)));
                query.push_back(static_cast<char>(0x80u | (cp & 0x3fu)));
            }
        }
        return query;
    }
    void applySearchKeyboardText(SearchKeyboardTarget target,std::string query) {
        if(target==SearchKeyboardTarget::Profile) {
            profileMenu.slots[keyboardPlayer].name=std::move(query);
            submitProfileName(keyboardPlayer);
        } else if(target==SearchKeyboardTarget::Encore) downloaderQuery=std::move(query);
        else if(target==SearchKeyboardTarget::LocalSongs) {
            songSearchQuery=std::move(query);songSearchImeError.clear();
            const auto& matches=filteredSongIndices();
            if(!matches.empty()) songSelection=static_cast<int>(matches.front());
            screen=Screen::SongSelect;
        }
    }
    SearchKeyboardTarget pollSearchKeyboard() {
#if !defined(PS5_HOST_REFERENCE)
        if (!searchKeyboardOpen) return SearchKeyboardTarget::None;
        if(nativeImePreferred) {
            const auto result=nativeKeyboard.poll();
            if(result==rhythm::native_ime::Panel::Waiting)return SearchKeyboardTarget::None;
            searchKeyboardResultCode=result==rhythm::native_ime::Panel::Failed?nativeKeyboard.error():0;
            searchKeyboardResult.endStatus=result==rhythm::native_ime::Panel::Accepted?0:1;
        } else {
        const int status=sceImeDialogGetStatus();
        if(!searchKeyboardStopping) {
            if(status!=2)return SearchKeyboardTarget::None;
            if(ps5.frameBudget && keyboardCloseIsolation) {
                if(keyboardCloseStage==0) {
                    keyboardCloseStage=1;
                    keyboardResultAfter=ps5.framesDrawn+300;
                    keyboardTermAfter=ps5.framesDrawn+600;
                }
                if(ps5.framesDrawn<keyboardResultAfter)return SearchKeyboardTarget::None;
            }
            searchKeyboardResult={};
            searchKeyboardResultCode=sceImeDialogGetResult(&searchKeyboardResult);
            searchKeyboardStopping=true;
            if(ps5.frameBudget && keyboardCloseIsolation)keyboardCloseStage=2;
        }
        // Do not reuse the UTF-16 buffer or permit another Init while the
        // common-dialog service still owns this request.
        if(status!=0) {
            if(status!=2)return SearchKeyboardTarget::None;
            if(ps5.frameBudget && keyboardCloseIsolation && ps5.framesDrawn<keyboardTermAfter)
                return SearchKeyboardTarget::None;
            const int terminated=sceImeDialogTerm();
            if(terminated<0 || sceImeDialogGetStatus()!=0)return SearchKeyboardTarget::None;
        }
        if(ps5.frameBudget && keyboardCloseIsolation)keyboardCloseStage=3;
        }
        const auto result=searchKeyboardResult;
        const int resultCode=searchKeyboardResultCode;
        const auto target=searchKeyboardTarget;
        searchKeyboardStopping=false;
        searchKeyboardOpen = false;
        searchKeyboardTarget=SearchKeyboardTarget::None;
        // QA can fill the shared result buffer, then close the real OS dialog.
        // Normal launches always honor the actual system accept/cancel result.
        const bool testAccepted=ps5.frameBudget && keyboardTestSubmitted;
        keyboardTestSubmitted=false;
        if(testAccepted) {
            searchKeyboardBuffer.fill(0);
            for(size_t i=0;i<std::min<size_t>(96,keyboardTestQuery.size());++i)
                searchKeyboardBuffer[i]=static_cast<unsigned char>(keyboardTestQuery[i]);
            keyboardTestQuery.clear();
        }
        if(resultCode<0 || (result.endStatus!=0 && !testAccepted)) {
            cancelTextKeyboard();return SearchKeyboardTarget::None;
        }
        auto query=searchKeyboardText();
        applySearchKeyboardText(target,std::move(query));
        primeKeyboardInputs();
        return target;
#else
        return SearchKeyboardTarget::None;
#endif
    }
    void updateInstrumentKeyboard(uint32_t pressed) {
        int result=0;
        if(pressed&PAD_TOUCH_PAD)result=-1;
        else if(pressed&PAD_OPTIONS)result=1;
        else {
            // Physical directions always follow the visible grid. Guitar
            // strum supplies Up/Down; yellow/blue provide horizontal movement.
            if(pressed&PAD_UP)instrumentKeyboard.row(-1);
            if(pressed&PAD_DOWN)instrumentKeyboard.row(1);
            if(pressed&(PAD_LEFT|PAD_SQUARE))instrumentKeyboard.move(-1);
            if(pressed&(PAD_RIGHT|PAD_TRIANGLE))instrumentKeyboard.move(1);
            if(pressed&PAD_CIRCLE)instrumentKeyboard.erase();
            if(pressed&PAD_L1)instrumentKeyboard.upper=!instrumentKeyboard.upper;
            if(pressed&PAD_CROSS)result=instrumentKeyboard.select();
        }
        if(result<0)cancelTextKeyboard();
        else if(result>0) {
            const auto target=searchKeyboardTarget;
            applySearchKeyboardText(target,instrumentKeyboard.text);
            if(target==SearchKeyboardTarget::Profile && profileMenu.slots[keyboardPlayer].page==rhythm::ProfileMenu::Page::Name)return;
            primeKeyboardInputs();
            instrumentKeyboard.open=false;searchKeyboardTarget=SearchKeyboardTarget::None;
            if(target==SearchKeyboardTarget::Encore)startEncoreSearch();
        }
    }
    bool updateKeyboardAssist() {
        bool connected=false;
        const int ownerSlot=keyboardPlayer<playerSeats.count?playerSeats.slotOfSeat[keyboardPlayer]:-1;
        for(int slot=0;slot<PAD_PLAYERS && instrumentKeyboard.open;++slot) {
            struct pad helper{};bool present=false;
            if(ps5.frameBudget) {if(slot==1 && keyboardPlayer!=1)present=readPlayerInput(1,&helper);}
            else if(slot!=ownerSlot && (sourceConnectedMask&(1u<<slot))){helper=sourcePads[slot];present=true;}
            auto& stick=keyboardAssistSticks[slot];
            if(!present){keyboardAssistPrevious[slot]=0;keyboardAssistRepeat[slot].reset();stick.reset();continue;}
            connected=true;stick.update(helper.left_x,helper.left_y,true);
            if(stick.x<0)helper.held|=PAD_LEFT;if(stick.x>0)helper.held|=PAD_RIGHT;
            if(stick.y<0)helper.held|=PAD_UP;if(stick.y>0)helper.held|=PAD_DOWN;
            uint32_t edges=keyboardAssistFilters[slot].update((helper.held&~keyboardAssistPrevious[slot])|helper.pressed,menuInputTime);
            keyboardAssistPrevious[slot]=helper.held;
            const uint32_t direction=helper.held&(PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT);
            edges|=keyboardAssistRepeat[slot].update(direction && !(direction&(direction-1))?direction:0,edges,
                static_cast<float>(frameTimer),int(searchKeyboardTarget));
            if(edges){keyboardGridNavigation=true;updateInstrumentKeyboard(edges);}
        }
        return connected;
    }
    bool beginLocalSongSearch() {
        if(songSearchQuery.empty()) songSearchField=defaultSearchByCurrentSort
            ?searchFieldForSongSort(songSortField):SongSearchField::Song;
        return openSearchKeyboard(SearchKeyboardTarget::LocalSongs);
    }
    void openSongSearchOptions() {
        songSearchKey=static_cast<int>(songSearchField);
        songSearchImeError.clear();screen=Screen::SongSearch;
    }
    void acceptSongSearchOption() {
        if(songSearchKey<6) {
            songSearchField=static_cast<SongSearchField>(songSearchKey);
            (void)openSearchKeyboard(SearchKeyboardTarget::LocalSongs);
        } else {
            if(songSearchKey==6) {
                songSearchQuery.clear();songSearchImeError.clear();
                const auto& matches=filteredSongIndices();
                if(!matches.empty()) songSelection=static_cast<int>(matches.front());
            }
            screen=Screen::SongSelect;
        }
    }
    void startEncoreSearch() {
        if (downloaderQuery.empty()) { downloaderImeError = "Enter a song or artist to search."; return; }
        downloaderImeError.clear();
        static constexpr const char* instruments[] = {"guitar", "guitarcoop", "rhythm", "bass", "drums", "keys", "guitarghl", "guitarcoopghl", "rhythmghl", "bassghl", nullptr};
        static constexpr const char* difficulties[] = {"expert", "hard", "medium", "easy", nullptr};
        static constexpr const char* sorts[] = {"name", "artist", "album", "genre", "year", "charter", "length", "modifiedTime"};
        static constexpr const char* drumTypes[] = {"fourLane", "fourLanePro", "fiveLane"};
        static constexpr const char* searchFields[] = {nullptr, "name", "artist", "album", "charter", "genre", "year"};
        chorus::SearchOptions options{};
        options.instrument = instruments[downloaderInstrument];
        options.difficulty = difficulties[downloaderDifficulty];
        options.sortField = sorts[downloaderSort];
        options.sortDescending = downloaderSortDescending;
        options.drumType = drumTypes[downloaderDrumType];
        options.drumsReviewed = downloaderReviewedDrums;
        options.searchField = searchFields[downloaderSearchField];
        (void)chorusDownloader.search(downloaderQuery.c_str(), 1, options);
        downloaderResult = 0;
        downloaderMarked.fill(false);
        downloaderQueueCount = downloaderQueuePosition = 0;
        downloaderResultsFocus = true;
        downloaderFilterFocus = false;
    }
    void submitProfileName(int player) {
        if(profileMenu.submitName(player,[&] {return saveProfiles();})) {
            playerJoined[player]=true;
            playerReady[player]=false;
        }
    }
    void profileNameKey(int player) {
        if(profileMenu.slots[player].key==39) submitProfileName(player);
        else profileMenu.nameKey(player);
    }

    void removePlayerSeat(int seat) {
        if(screen==Screen::Gameplay) {ui.visible=true;pausePage=PausePage::Main;pauseSelection=0;}
        const auto shift=[&](auto& values) {
            for(int i=seat;i+1<playerSeats.count;++i)values[i]=std::move(values[i+1]);
            values[playerSeats.count-1]={};
        };
        shift(profileMenu.slots);shift(playerJoined);shift(playerReady);shift(playerPage);shift(playerMenuRow);
        shift(playerInstrument);shift(playerDifficulty);shift(playerModifierMask);shift(playerPreviousButtons);
        shift(selectionProfileId);shift(selectionController);shift(playerRemaps);shift(playerCalibrations);shift(midiLinks);syncCalibrationAudio();
        shift(menuPressFilters);shift(menuSticks);shift(playerMenuRepeat);
        remapPlayer=std::clamp(remapPlayer>seat?remapPlayer-1:remapPlayer,0,PAD_PLAYERS-1);
        playerDifficulty[playerSeats.count-1]=3;
        if(gameplayPlayer>seat)--gameplayPlayer;
        else if(gameplayPlayer==seat)gameplayPlayer=0;
        playerSeats.erase(seat);
        if(screen==Screen::Gameplay && playerSeats.count)
            activeControllerProfile=playerControllerProfile(gameplayPlayer,activeProfile.controller);
        previousButtons=playerSeats.count?menuButtons(0,sourcePads[playerSeats.slotOfSeat[0]].held):0;
    }
    static uint32_t canonicalControllerButtons(const rhythm::controls::InstrumentState& state,bool drums,bool six=false) {
        uint32_t out=0;constexpr uint32_t colors[]{PAD_CROSS,PAD_CIRCLE,PAD_SQUARE,PAD_TRIANGLE,PAD_L1};
        constexpr unsigned pads[]{4,1,2,3,0},sixKeys[]{3,4,5,0,1};
        const uint16_t menuPads=rhythm::pc_drums::inputMask(state.pads,false,false);
        for(unsigned i=0;i<5;++i)if((drums?menuPads:state.frets)&(1u<<(drums?pads[i]:six?sixKeys[i]:i)))out|=colors[i];
        if(drums && (state.pads&256))out|=PAD_L1;
        if(state.strumUp)out|=PAD_UP;if(state.strumDown)out|=PAD_DOWN;
        if(state.start)out|=PAD_OPTIONS;if(state.starPower)out|=PAD_TOUCH_PAD;return out;
    }
    static void normalizeFiveLaneInput(rhythm::controls::InstrumentState& input) {
        input.pads=rhythm::pc_drums::fiveLaneInput(input.pads);
        input.velocityKnown=rhythm::pc_drums::fiveLaneInput(input.velocityKnown);
        input.velocities[5]=input.velocities[2];input.velocities[7]=input.velocities[8];
    }
    static uint32_t menuHatButtons(const rhythm::controls::ControllerProfile& profile,const uint8_t* report,unsigned size) {
        const auto menu=rhythm::controls::rawMenuControls(profile,report,size);
        const auto directions=menu.directions;
        uint32_t buttons=0;constexpr uint32_t colors[]{PAD_CROSS,PAD_CIRCLE,PAD_SQUARE,PAD_TRIANGLE,PAD_L1};
        for(unsigned i=0;i<5;++i)if(menu.colors&(1u<<i))buttons|=colors[i];
        if(menu.start)buttons|=PAD_OPTIONS;if(menu.select)buttons|=PAD_TOUCH_PAD;
        return buttons | ((directions&rhythm::controls::MenuHatUp)?PAD_UP:0) |
            ((directions&rhythm::controls::MenuHatDown)?PAD_DOWN:0) |
            ((directions&rhythm::controls::MenuHatLeft)?PAD_LEFT:0) |
            ((directions&rhythm::controls::MenuHatRight)?PAD_RIGHT:0);
    }
    const rhythm::controls::UsbSample* playerUsb(int player) const {
        if(player>=playerSeats.count)return nullptr;
        const int slot=playerSeats.slotOfSeat[player];return slot>=4?&usbInputs[slot-4]:nullptr;
    }
    rhythm::controls::ControllerProfile playerControllerProfile(int player,int type) const {
        const auto* usb=playerUsb(player);const auto* profile=profileMenu.profile(player);
        if(usb && usb->deviceProfile) {
            if(profile)if(const auto* custom=profile->rawMapping(type,usb->vid,usb->pid))return *custom;
            if(usb->deviceProfile->instrument==rhythm::controls::instrumentName(type))return *usb->deviceProfile;
        }
        return rhythm::controls::nativeProfile(type,effectiveMapping(profile,type));
    }
    void pollControllerSources() {
#if defined(PS5_NATIVE_INSTRUMENT_INPUT) && !defined(PS5_HOST_REFERENCE)
        char nativeEvent[256];
        while(pad_diagnostic(nativeEvent,sizeof(nativeEvent)))rhythm::logDiagnostic("%s",nativeEvent);
#endif
        if(ps5.frameBudget && !slotInputQa)return;
        const uint32_t before=sourceConnectedMask;
        sourceConnectedMask=0;
        if(slotInputQa) {
            for(auto& pad:sourcePads)pad={};
            for(const auto& press:ps5Presses) {
                if(press.frame!=ps5.framesDrawn || press.program.rfind("cloneheronative.slot",0)!=0)continue;
                const int slot=std::atoi(press.program.c_str()+std::strlen("cloneheronative.slot"))-1;if(slot<0 || slot>=8)continue;
                if(press.action.rfind("multiplayer-library:",0)==0)installMultiplayerQaLibrary(press.action.substr(20));
                else if(press.action=="connect")qaSourceConnected[slot]=true;
                else if(press.action=="disconnect")qaSourceConnected[slot]=false;
                else sourcePads[slot].held|=rhythm::controls::nativeButton(press.action);
            }
            for(int i=0;i<8;++i)if(qaSourceConnected[i])sourceConnectedMask|=1u<<i;
        }
        else for(int slot=0;slot<PAD_PLAYERS;++slot) {
            // pad_player derives pressed from the caller's previous held mask.
            // Preserve it across reads so held buttons remain held, not presses.
            if(pad_player(slot,&sourcePads[slot])) {
                if(sourcePads[slot].l2>.5f)sourcePads[slot].held|=0x100;
                if(sourcePads[slot].r2>.5f)sourcePads[slot].held|=0x200;
                sourceConnectedMask|=1u<<slot;
            } else sourcePads[slot]={};
        }
        if(!slotInputQa) {
            usbInputs=usbControllers.snapshot(&usbInputBatches);
            for(unsigned i=0;i<usbInputBatches.size();++i)if(usbInputBatches[i].dropped)
                rhythm::logDiagnostic("ERROR usb history overflow source=%u dropped=%u",i,usbInputBatches[i].dropped);
        }
        if(!slotInputQa)for(unsigned i=0;i<usbInputs.size();++i) {
            const auto& usb=usbInputs[i];sourcePads[i+4]={};
            if(!usb.streaming)continue;
            sourceConnectedMask|=1u<<(i+4);
            if(!usb.deviceProfile)continue;
            const int seat=playerSeats.seatOfSlot[i+4];const auto* profile=seat>=0?profileMenu.profile(seat):nullptr;
            const rhythm::controls::ControllerProfile* mapping=usb.deviceProfile.get();
            if(profile) {
                if(const auto* custom=profile->rawMapping(std::clamp(profile->controller,0,4),usb.vid,usb.pid))mapping=custom;
            }
            const auto state=rhythm::controls::applyProfile(*mapping,{},usb.report.data(),usb.length);
            const bool drums=mapping->instrument.find("drums_")==0;
            const bool menuNavigation=screen!=Screen::Gameplay || ui.visible;
            sourcePads[i+4].held=canonicalControllerButtons(state,drums,mapping->instrument=="guitar_6fret") |
                (menuNavigation?menuHatButtons(*usb.deviceProfile,usb.report.data(),usb.length):0);
            if(menuNavigation) {
                if(mapping->menuX>=0 && unsigned(mapping->menuX)<usb.length)sourcePads[i+4].left_x=(int(usb.report[mapping->menuX])-128)/127.0f;
                if(mapping->menuY>=0 && unsigned(mapping->menuY)<usb.length)sourcePads[i+4].left_y=(int(usb.report[mapping->menuY])-128)/127.0f;
            }
            auto previous=sourcePrevious[i+4];
            for(unsigned n=0;n<usbInputBatches[i].count;++n) {
                const auto& report=usbInputBatches[i].reports[n];
                const auto changed=rhythm::controls::applyProfile(*mapping,{},report.bytes.data(),report.length);
                const auto held=canonicalControllerButtons(changed,drums,mapping->instrument=="guitar_6fret") |
                    (menuNavigation?menuHatButtons(*usb.deviceProfile,report.bytes.data(),report.length):0);
                sourcePads[i+4].pressed|=held&~previous;previous=held;
            }
            sourceConnectedMask|=1u<<(i+4);
        }
        if(sourceConnectedMask!=before) {
            rhythm::logDiagnostic("controllers native_mask=%u sources=%u seats=%d",pad_players(),sourceConnectedMask,playerSeats.count);
            controllerToast=(sourceConnectedMask&~before)?"Controller connected":"Controller disconnected";
            controllerToastUntil=ImGui::GetTime()+3.0;
        }
        if(screen!=Screen::Gameplay && screen!=Screen::Results)
        for(int seat=playerSeats.count-1;seat>=0;--seat)
            if(!(sourceConnectedMask&(1u<<playerSeats.slotOfSeat[seat])) &&
               !(playerRemaps[seat].active && playerRemaps[seat].raw && playerUsb(seat) && playerUsb(seat)->present))removePlayerSeat(seat);
        const bool joinScreen=!updateDialog && !searchKeyboardOpen && !instrumentKeyboard.open && screen!=Screen::Gameplay && screen!=Screen::ControllerRemap && screen!=Screen::Results && screen!=Screen::SongCopyConfirm && screen!=Screen::SongRemove && screen!=Screen::MidiSetup;
        for(int slot=0;slot<8;++slot) {
            const uint32_t fresh=sourcePads[slot].pressed|(sourcePads[slot].held&~sourcePrevious[slot]);
            sourcePrevious[slot]=sourcePads[slot].held;
            if(joinScreen && !remapHelperReserved(slot) && (sourceConnectedMask&(1u<<slot)) && playerSeats.seatOfSlot[slot]<0 && (fresh&PAD_OPTIONS)) {
                const int seat=playerSeats.join(slot);
                if(seat>=0) {
                    playerPreviousButtons[seat]=0;menuPressFilters[seat].reset();menuPressFrames[seat]=0;
                    if(slot>=4 && usbInputs[slot-4].deviceProfile)
                        for(int type=0;type<5;++type)if(usbInputs[slot-4].deviceProfile->instrument==rhythm::controls::instrumentName(type))profileMenu.slots[seat].guest.controller=type;
                }
            }
        }
        if(screen==Screen::MainMenu && !searchKeyboardOpen && !instrumentKeyboard.open && !profileMenu.open(0))for(unsigned i=0;i<usbInputs.size();++i) {
            const auto& u=usbInputs[i];if(!u.present){ignoredControllers[i]=0;continue;}
            const uint32_t key=(uint32_t(u.vid)<<16)|u.pid;
            if(u.unknown && ignoredControllers[i]!=key){unknownController=int(i);unknownControllerChoice=0;screen=Screen::ControllerSetup;break;}
        }
        primarySource=0;
        for(int slot=0;slot<8;++slot)if(sourceConnectedMask&(1u<<slot)){primarySource=slot;break;}
    }
    void releaseUnassignedSeats() {
        if((ps5.frameBudget && !slotInputQa) || screen==Screen::Gameplay || screen==Screen::ControllerRemap)return;
        for(int seat=playerSeats.count-1;seat>=0;--seat)
            if(!profileMenu.joined(seat) && !profileMenu.open(seat))removePlayerSeat(seat);
    }
    void vibratePlayer(int player,float large,float small) {
        const int slot=ps5.frameBudget?player:(player<playerSeats.count?playerSeats.slotOfSeat[player]:primarySource);
        if(slot<PAD_PLAYERS)pad_player_vibrate(slot,large,small);
    }
    void applyMenuStick(int player,struct pad* input) {
        auto& stick=menuSticks[player];
        if((screen==Screen::Gameplay && !ui.visible) || (playerRemaps[player].active && playerRemaps[player].wizard.page==rhythm::controls::Remap::Page::Capture)) {stick.reset();return;}
        stick.update(input->left_x,input->left_y,screen!=Screen::MainMenu || instrumentKeyboard.open);
        if(stick.x<0)input->held|=PAD_LEFT;
        if(stick.x>0)input->held|=PAD_RIGHT;
        if(stick.y<0)input->held|=PAD_UP;
        if(stick.y>0)input->held|=PAD_DOWN;
    }
    bool readPlayerInput(int player,struct pad* input) {
        // Extend the title's existing scripted test-run input to each card.
        // A plain launch always reads scePadReadState through pad_player.
        if(ps5.frameBudget) {
            const std::string program="cloneheronative.p"+std::to_string(player+1);
            bool scripted=false;
            *input={};
            for(const auto& press:ps5Presses) {
                if(press.program!=program) continue;
                scripted=true;
                if(press.frame!=ps5.framesDrawn) continue;
                if(player==0 && press.action.rfind("multiplayer:",0)==0)beginMultiplayerQa(press.action.substr(12));
                else if(player==0 && press.action.rfind("multiplayer-library:",0)==0)installMultiplayerQaLibrary(press.action.substr(20));
                else if(player==0 && multiplayerQa && press.action=="multiplayer-fail-enabled")noFail=false;
                else if(player==0 && multiplayerQa && press.action.rfind("multiplayer-health:",0)==0) {
                    int seat=0;float health=.5f;
                    if(std::sscanf(press.action.c_str()+19,"%d:%f",&seat,&health)==2 && seat>=0 && seat<PAD_PLAYERS && (gameplaySeatMask&(1u<<seat))) {
                        const int restore=activeGameplaySeat;selectGameplaySeat(seat);gameplayHealth=health;selectGameplaySeat(restore);
                    }
                }
                else if(player==0 && press.action.rfind("replay:",0)==0) beginGameplayReplay(press.action.substr(7));
                else if(player==0 && (press.action=="profile-render" || press.action=="profile-unbatched")) {
                    renderProfileQa=true;renderBatching=press.action!="profile-unbatched";
                }
#if !defined(PS5_HOST_REFERENCE)
                else if(player==0 && press.action=="audio-source-loss")songAudio.sourceUnavailable();
                else if(player==0 && press.action=="audio-load-delay")audioDelayQa=true;
#else
                else if(player==0 && (press.action=="audio-source-loss" || press.action=="audio-load-delay"))
                    std::fprintf(stderr,"host reference: native audio QA unavailable\n");
#endif
                else if(player==0 && press.action=="highway-effects")activeProfile.highwayEffects=true;
                else if(player==0 && press.action.rfind("pulse-qa:",0)==0 && screen==Screen::Gameplay) {
                    const int kind=std::atoi(press.action.c_str()+9);
                    gameplayHealth=kind==4?.2f:.8f;combo=kind==2?30:0;
                    solos.clear();if(kind==3)solos.push_back({0,0,renderTime-1,renderTime+10});
                    chartBaseScore=100;score=kind==1?1000:0;soloBonusTotal=fretBonusTotal=0;
                }
                else if(player==0 && press.action.rfind("sp-color:",0)==0)activeProfile.starPowerColor=std::clamp(std::atoi(press.action.c_str()+9),0,5);
                else if(player==0 && press.action.rfind("note-color:",0)==0)activeProfile.noteColor=std::clamp(std::atoi(press.action.c_str()+11),0,9);
                else if(player==0 && press.action=="purple-taps")activeProfile.purpleTapNotes=true;
                else if(player==0 && press.action=="lane-effects")activeProfile.laneHitEffects=true;
                else if(player==0 && press.action=="kick-purple")activeProfile.purpleKick=true;
                else if(player==0 && press.action.rfind("preview:",0)==0) {
                    const std::string path=press.action.substr(8);
                    // Owned synthetic fixtures live outside the user's song library.
                    // Never register them in persistent library/date caches.
                    if(path.rfind("/app0/preview-qa-20261116/",0)==0 &&
                       std::none_of(localSongs.begin(),localSongs.end(),[&](const LocalSong& song){return song.chartPath==path;})) {
                        LocalSong fixture;
                        fixture.chartPath=path;
                        fixture.title="Preview audio test";
                        fixture.artist="Audio QA";fixture.audioChartPath=path;
                        const auto folder=path.substr(0,path.find_last_of('/'));
                        readSongMetadata(folder,path,fixture);
                        localSongs.push_back(std::move(fixture));
                    }
                    for(size_t i=0;i<localSongs.size();++i) if(localSongs[i].chartPath==path) {
                        menuMusicSelection=static_cast<int>(i);
                        menuPreviewAudioChartPath.clear();
                        screen=Screen::MainMenu;
                        break;
                    }
                }
                else if(player==0 && press.action=="midi-setup-qa") {
                    midiQa=true;midiSources[0]={true,true,0xfffd,0xfffc,0,0x81,1};sourceConnectedMask|=1;
                    beginMidiSetup(0,0);midiBatches[0].count=0;
                }
                else if(player==0 && midiQa && press.action.rfind("midi-hit:",0)==0) {
                    unsigned note=0,velocity=0;
                    if(std::sscanf(press.action.c_str()+9,"%u:%u",&note,&velocity)==2 && note<=127 && velocity<=127){
                        const uint8_t packet[]{9,0x99,uint8_t(note),uint8_t(velocity)};
                        midiQaDecoder.feed(packet,uint64_t(now_seconds()*1000000.0),[&](const auto& event){
                            auto& batch=midiBatches[0];if(batch.count<batch.strikes.size())batch.strikes[batch.count++]=event;
                        });
                    }
                }
                else if(player==0 && press.action=="controller-setup-qa") {
                    unknownController=0;unknownControllerChoice=0;usbInputs[0].present=usbInputs[0].unknown=true;
                    usbInputs[0].vid=0xffff;usbInputs[0].pid=0xfffe;screen=Screen::ControllerSetup;
                }
                else if(player==0 && press.action=="scan-release")storageScanTestHold=false;
                else if(player==0 && press.action=="storage-mount") {storageQa=true;storageQaMounted=true;storageNextPoll=0;storageNeedsScan=true;}
                else if(player==0 && press.action=="storage-unmount") {storageQa=true;storageQaMounted=false;storageNextPoll=0;}
                else if(player==0 && (press.action=="storage-menu" || press.action=="storage-export")) {storageQa=true;beginSongCopy(press.action=="storage-menu");}
                else if(player==0 && press.action=="audio-test") {
                    // Controlled miss/overstrum audio test; never persisted.
                    noFail=true;muteOnMiss=true;missVolume=0;
                }
                else if(press.action=="imgui-menu") {
                    // Reproduce the framework's physical Square hold in QA.
                    auto& io=ImGui::GetIO();io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;
                    io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
                    io.NavInputs[ImGuiNavInput_Menu]=1.0f;
                }
#if !defined(PS5_HOST_REFERENCE)
                else if(press.action=="ime-kernel-dump") {
                    // Read-only, bounded diagnostics; never used by normal launches.
                    const int fd=open("/dev/klog",O_RDONLY|O_NONBLOCK);
                    const int openedError=fd<0?errno:0;
                    if(FILE* log=std::fopen("/app0/ime-kernel-test.raw","ab")) {
                        std::fprintf(log,"\nIME kernel capture frame %llu open_errno %d\n",
                            static_cast<unsigned long long>(ps5.framesDrawn),openedError);
                        if(fd>=0) {
                            char bytes[16384];
                            for(unsigned chunk=0;chunk<256;++chunk) {
                                const auto n=read(fd,bytes,sizeof(bytes));
                                if(n<=0)break;
                                std::fwrite(bytes,1,static_cast<size_t>(n),log);
                            }
                        }
                        std::fclose(log);
                    }
                    if(fd>=0)close(fd);
                }
                else if(press.action=="ime-direct")nativeImePreferred=true;
                else if(press.action=="ime-dialog")nativeImePreferred=false;
                else if(press.action=="ime-isolate-close")keyboardCloseIsolation=true;
                else if(press.action=="ime-abort" && searchKeyboardOpen && !keyboardTestSubmitted) abortSearchKeyboard();
                else if(press.action.rfind("ime-submit:",0)==0 && searchKeyboardOpen && !keyboardTestSubmitted) {
                    // Save text until the OS dialog has stopped writing its buffer.
                    keyboardTestQuery=press.action.substr(11);
                    keyboardTestSubmitted=true;abortSearchKeyboard();
                }
#endif
                else if(press.action=="options") input->held|=PAD_OPTIONS;
                else if(press.action=="cross") input->held|=PAD_CROSS;
                else if(press.action=="circle") input->held|=PAD_CIRCLE;
                else if(press.action=="up") input->held|=PAD_UP;
                else if(press.action=="down") input->held|=PAD_DOWN;
                else if(press.action=="left") input->held|=PAD_LEFT;
                else if(press.action=="right") input->held|=PAD_RIGHT;
                else if(press.action.rfind("hat:",0)==0) {
                    // Bounded native regression: run the shipped guitar report
                    // map through the same decoder used by the live USB poller.
                    rhythm::controls::ControllerProfile profile;std::string error;
                    if(rhythm::controls::loadProfile((getAssetPath()+"controller_profiles/12ba_0100.json").c_str(),profile,error)) {
                        uint8_t report[27]{};report[2]=static_cast<uint8_t>(std::atoi(press.action.c_str()+4));
                        input->held|=menuHatButtons(profile,report,sizeof(report));
                    }
                }
                else if(press.action=="stick-up") input->left_y=-1;
                else if(press.action=="stick-down") input->left_y=1;
                else if(press.action=="stick-left") input->left_x=-1;
                else if(press.action=="stick-right") input->left_x=1;
                else if(press.action=="triangle") input->held|=PAD_TRIANGLE;
                else if(press.action=="square") input->held|=PAD_SQUARE;
                else if(press.action=="l1") input->held|=PAD_L1;
                else if(press.action=="r1") input->held|=PAD_R1;
                else if(press.action=="touchpad") input->held|=PAD_TOUCH_PAD;
                else if(screen==Screen::Downloader && press.action.rfind("search:",0)==0) {
                    downloaderQuery=press.action.substr(7);
                    downloaderScriptSearchRequested=true;
                }
                else if(press.action.size()==1) {
                    const char character=static_cast<char>(std::toupper(static_cast<unsigned char>(press.action[0])));
                    const char* keyValues="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
                    const char* key=std::strchr(keyValues,character);
                    if(key) {
                        const int index=static_cast<int>(key-keyValues);
                        if(screen==Screen::Downloader) downloaderKey=index;
                    }
                }
            }
            if(scripted) {applyMenuStick(player,input);return true;}
        }
        const int slot=player<playerSeats.count?playerSeats.slotOfSeat[player]:(player==0 && playerSeats.count==0?primarySource:-1);
        const auto& remap=playerRemaps[player];
        const bool assisted=remap.active && remap.raw && remap.assistSource>=0 && (sourceConnectedMask&(1u<<remap.assistSource));
        if(slot<0 || (!(sourceConnectedMask&(1u<<slot)) && !assisted)){*input={};menuSticks[player].reset();return false;}
        *input=sourcePads[slot];
        if(assisted) {
            const auto& helper=sourcePads[remap.assistSource];
            input->held|=helper.held;
            input->pressed|=helper.pressed;
            if(remap.wizard.page!=rhythm::controls::Remap::Page::Capture &&
               (std::abs(helper.left_x)>.35f || std::abs(helper.left_y)>.35f)) {
                input->left_x=helper.left_x;input->left_y=helper.left_y;
            }
        }
        applyMenuStick(player,input);
        return true;
    }

    // Test-run only: feed the real gameplay path logical guitar states, then
    // replay that identical stream in the original PC engine. No save writes.
    struct GameplayReplayFrame {double time{};unsigned frets{},up{},down{},sp{};std::array<int,8> velocities{{-1,-1,-1,-1,-1,-1,-1,-1}};};
    std::array<std::vector<GameplayReplayFrame>,PAD_PLAYERS> multiplayerQaInputs;
    bool multiplayerQaReplay{},multiplayerQaAdvanced{};
    size_t multiplayerQaStep{},multiplayerQaSample{};
    std::vector<GameplayReplayFrame> gameplayReplay;
    size_t gameplayReplayIndex{};
    unsigned gameplayReplayBatch{1}; // Bounded QA only; never changes normal playback.
    unsigned gameplayReplayPcModifiers{};
    unsigned gameplayReplayInstrument{},gameplayReplaySeed{},gameplayReplayBot{};
    FILE* gameplayReplayTrace{};
    bool gameplayReplayAttempted{false},gameplayReplayCanonicalPads{};
    void beginGameplayReplay(const std::string& path) {
        if(!ps5.frameBudget || gameplayReplayAttempted) return;
        gameplayReplayAttempted=true;
        FILE* file=std::fopen((path+".replay").c_str(),"rb");
        if(!file) {notify_fail("missing gameplay replay");return;}
        double previous=-1;char line[512];
        while(std::fgets(line,sizeof(line),file)) {
            GameplayReplayFrame frame;
            const int fields=std::sscanf(line,"%lf %u %u %u %u %d %d %d %d %d %d %d %d",&frame.time,&frame.frets,&frame.up,&frame.down,&frame.sp,
                &frame.velocities[0],&frame.velocities[1],&frame.velocities[2],&frame.velocities[3],
                &frame.velocities[4],&frame.velocities[5],&frame.velocities[6],&frame.velocities[7]);
            if((fields!=5 && fields!=13) || !std::isfinite(frame.time) || frame.time<=previous || frame.frets>511 || gameplayReplay.size()>=120000)break;
            gameplayReplay.push_back(frame);previous=frame.time;
        }
        std::fclose(file);
        if(gameplayReplay.empty()) {notify_fail("empty gameplay replay");return;}
        unsigned replayModifiers=0;gameplayReplayPcModifiers=0;gameplayReplayInstrument=gameplayReplaySeed=gameplayReplayBot=0;
        unsigned replayLength=static_cast<unsigned>(activeProfile.highwayLength),replayLefty=0,replayController=99,canonicalPads=0;
        if(FILE* options=std::fopen((path+".replay-options").c_str(),"rb")) {
            (void)std::fscanf(options,"%u %u %u %u %u %u %u %u %u %u",&replayModifiers,&gameplayReplayPcModifiers,&gameplayReplayInstrument,&gameplayReplaySeed,&gameplayReplayBot,&replayLength,&replayLefty,&replayController,&canonicalPads,&gameplayReplayBatch);std::fclose(options);
        }
        gameplayReplayBatch=std::clamp(gameplayReplayBatch,1u,64u);
        if(replayController>4)replayController=gameplayReplayInstrument==3?2:0;
        gameplayReplayCanonicalPads=gameplayReplayInstrument==3 && canonicalPads!=0;
        if(!gameplayReplayCanonicalPads && std::any_of(gameplayReplay.begin(),gameplayReplay.end(),[](const auto& frame){return frame.frets>63;})) {
            gameplayReplay.clear();notify_fail("invalid guitar replay mask");return;
        }
        std::string replayChart=path;
        if(path.size()>4&&(path.substr(path.size()-4)==".zip"||path.substr(path.size()-4)==".sng")) {
            std::string error;
            const std::string stage=path+".stage",destination=path+".song";
            chorus::discardSongPackageStage(stage); // This test fixture owns the path.
            const bool madeStage=mkdir(stage.c_str(),0775)==0;
            if(!madeStage)error=std::string("stage mkdir: ")+std::strerror(errno);
            const bool installed=madeStage&&chorus::installSongPackage(path,stage,destination,error);
            if(FILE* status=std::fopen("/app0/package-replay-result.txt","wb")) {
                std::fprintf(status,"%s %s\n",installed?"PASS":"FAIL",error.c_str());std::fclose(status);
            }
            if(!installed){notify_fail("package replay install: "+error);gameplayReplay.clear();return;}
            replayChart=destination+"/notes.chart";
        }
        selectGameplaySeat(0);primaryGameplaySeat=0;gameplaySeatMask=1;
        loadChart(replayChart,int(std::min(gameplayReplayInstrument,3u)),3,static_cast<uint8_t>(replayModifiers),false,replayController);
        if(!chartFileLoaded) {gameplayReplay.clear();return;}
        activeProfile.bot=gameplayReplayBot!=0;activeProfile.controller=replayController;
        activeProfile.lefty=replayLefty!=0;activeProfile.highwayLength=static_cast<int>(std::clamp(replayLength,20u,1000u));gamepadMode=false;noFail=true;
        playMode=PlayMode::Quickplay;practiceCountdownRemaining=0;
        playerModifierMask[0]=static_cast<uint8_t>(replayModifiers);gameplayPlayer=0;
        screen=Screen::Gameplay;playing=true;ui.visible=false;
        restartSong();
        songClock.reset(gameplayReplay.front().time);
        songAudio.seekSeconds(std::max(0.0,gameplayReplay.front().time+trackStartOffsetSeconds));
        for(unsigned i=0;i<std::min(gameplayReplaySeed,4u);++i)starPower.awardPhrase();
        gameplayReplayTrace=std::fopen("/app0/gameplay-replay.tsv","wb");
        if(gameplayReplayTrace) {
            std::fprintf(gameplayReplayTrace,"# threshold %d\n# base_score %llu\n",songHopoThreshold,static_cast<unsigned long long>(chartBaseScore));
            std::fprintf(gameplayReplayTrace,"# chart_end %.17g %.17g\n",globalChartEndSeconds,authoredSongEndSeconds);
            for(const auto& segment:tempoSegments)std::fprintf(gameplayReplayTrace,"# tempo %lld %.17g %.17g\n",static_cast<long long>(segment.startTick),segment.startSec,segment.bpm);
            for(const auto& segment:starPowerClock.segments)std::fprintf(gameplayReplayTrace,"# power %.17g %.17g %.17g %.17g\n",segment.time,segment.tick,segment.bpm,segment.factor);
            for(const auto& range:solos)std::fprintf(gameplayReplayTrace,"# solo %zu %zu %.17g %.17g\n",range.firstHead,range.lastHead,range.startTime,range.endTime);
            if(replayChart!=path)std::fprintf(gameplayReplayTrace,"# archive_audio %d %.9g\n",songAudio.ready()?1:0,songAudio.durationSeconds());
            if(chart.kind==rhythm::ChartKind::Drums)for(size_t i=0;i<chart.notes.size();++i)std::fprintf(gameplayReplayTrace,"# drum %zu %.17g %u %u\n",i,chart.notes[i].time,chart.notes[i].frets,chart.notes[i].drumFlags);
            for(const auto& n:notes) std::fprintf(gameplayReplayTrace,"# gem %lld %lld %u %d %d %d %d\n",(long long)n.tick,(long long)n.lengthTicks,n.lane,n.extended,n.disjoint,n.hopo,n.tap);
            std::fprintf(gameplayReplayTrace,"time\tfrets\tup\tdown\tsp\tscore\tstreak\thits\tmisses\tsolo\tghosts\tghostCharge\tspTicks\tspActive\tordinary\tfretBonus\thealth\trank\tprogress\n");
        }
    }
    uint32_t connectedPlayerMask() const {
        if(multiplayerQa && !slotInputQa)return gameplaySeatMask;
        uint32_t mask=ps5.frameBudget && !slotInputQa?pad_players():0;
        if(!ps5.frameBudget || slotInputQa) {
            for(int seat=0;seat<playerSeats.count;++seat)
                if(sourceConnectedMask&(1u<<playerSeats.slotOfSeat[seat]))mask|=1u<<seat;
            if(playerSeats.count==0 && sourceConnectedMask)mask=1;
        }
        if(ps5.frameBudget && !slotInputQa) for(int player=0;player<PAD_PLAYERS;++player) {
            const std::string program="cloneheronative.p"+std::to_string(player+1);
            for(const auto& press:ps5Presses)
                if(press.program==program) {mask|=1u<<player;break;}
        }
        return mask;
    }

    [[nodiscard]] unsigned currentScoreMultiplier() const noexcept {
        return rhythm::pc_scoring::multiplier(combo,starPower.active());
    }

    struct BandScore {uint64_t total{},ordinary{},base{};};
    BandScore gameplayBandScore() const noexcept {
        BandScore band;
        const unsigned seats=gameplaySeatMask?gameplaySeatMask:(1u<<activeGameplaySeat);
        for(int seat=0;seat<PAD_PLAYERS;++seat)if(seats&(1u<<seat)) {
            if(seat==activeGameplaySeat) {
                band.total+=score;band.ordinary+=score-soloBonusTotal-fretBonusTotal;band.base+=chartBaseScore;
            } else if(parkedGameplay[seat]) {
                const auto& p=*parkedGameplay[seat];
                band.total+=p.score;band.ordinary+=p.score-p.soloBonusTotal-p.fretBonusTotal;band.base+=p.chartBaseScore;
            }
        }
        return band;
    }
    [[nodiscard]] unsigned currentStarRank() const noexcept {return scoreStarRank(score-soloBonusTotal-fretBonusTotal,chartBaseScore);}
    static unsigned scoreStarRank(uint64_t ordinaryScore,uint64_t chartBaseScore) noexcept {
        if(chartBaseScore==0)return 0;
        // StarProgress::Update (Ghidra 0x1800FF630) creates thresholds from
        // the PC metadata fractions and caps the visible star count at seven.
        static constexpr double thresholds[] = {0.1, 0.5, 1.0, 2.0, 2.8, 3.6, 4.4};
        unsigned rank = 0;
        for (unsigned i = 0; i < std::size(thresholds); ++i)
            if (ordinaryScore >= static_cast<uint64_t>(std::ceil(static_cast<float>(thresholds[i]) * static_cast<float>(chartBaseScore)))) rank = i + 1;
        return rank;
    }

    [[nodiscard]] float starProgressToNextRank() const noexcept {return scoreStarProgress(score-soloBonusTotal-fretBonusTotal,chartBaseScore);}
    static float scoreStarProgress(uint64_t ordinaryScore,uint64_t chartBaseScore) noexcept {
        if(chartBaseScore==0)return 0;
        static constexpr double thresholds[] = {0.1, 0.5, 1.0, 2.0, 2.8, 3.6, 4.4};
        const unsigned rank = scoreStarRank(ordinaryScore,chartBaseScore);
        const double lower = rank == 0 ? 0.0 : std::ceil(static_cast<float>(thresholds[rank - 1]) * static_cast<float>(chartBaseScore));
        if (rank >= 7) return 1.0f;
        const double upper = std::ceil(static_cast<float>(thresholds[rank]) * static_cast<float>(chartBaseScore));
        return static_cast<float>(std::clamp((static_cast<double>(ordinaryScore) - lower) / std::max(1.0, upper - lower), 0.0, 1.0));
    }

    void awardCompletedStarPhrases() {
        // BaseEngine 0x1820F2790 awards StarPowerEnd at the successful head,
        // rather than waiting for the authored phrase range (or its tail) to end.
        for(auto& phrase:starPhrases) {
            if(phrase.awarded || phrase.failed || !phrase.gemCount || phrase.hitCount!=phrase.gemCount) continue;
            phrase.awarded=true;
            if(playMode==PlayMode::Practice && !practiceStarPowerEnabled) continue;
            const float previous=starPower.fill();
            starPower.awardPhrase();
            resultStats.barsFilled+=std::max(0.0f,starPower.fill()-previous);
            songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarPowerAwarded);
            if(!starPower.active() && previous<0.5f && starPower.fill()>=0.5f)
                songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarAvailable);
        }
    }

    void awardSustainThrough(Note& note,int64_t through,bool active) {
        if(!note.hit || note.sustainDropped || note.sustainBeats<=0 || !note.sustainGrace.scoring()) return;
        const auto target=rhythm::pc_scoring::sustainPoints(note.lengthTicks,
            through-note.tick,chartResolutionTicks);
        if(target>note.sustainPointsAwarded) {
            // Equal-length chord frets share one PC runtime sustain object.
            // Different-length groups retain independent payout/hold state.
            if(note.sustainScoreOwner) {
                resultStats.award(target-note.sustainPointsAwarded,combo,active,true);
                score+=uint64_t(target-note.sustainPointsAwarded)*rhythm::pc_scoring::multiplier(combo,active);
            }
            note.sustainPointsAwarded=target;
        }
    }
    void settleSustainsForHead(size_t head,double time) {
        const auto& next=chart.notes[head];
        const unsigned mask=next.frets|(next.open?(1u<<guitarLaneCount()):0u);
        const auto tick=chartTickAt(time);
        for(const auto index:activeSustainNotes) {
            auto& tail=notes[index];
            if(!tail.hit || tail.sustainDropped || tail.lengthTicks<=0 || tail.sustainPointsAwarded>=rhythm::pc_scoring::sustainPoints(tail.lengthTicks,tail.lengthTicks,chartResolutionTicks)) continue;
            // Guitar hit 0x180FCC230 updates every existing tail through the
            // current raw tick before advancing streak/multiplier, including
            // extended tails which survive this head. At the quarter-beat
            // completion boundary this also settles their final ceil payout.
            awardSustainThrough(tail,tick,starPower.active());
            if(!rhythm::pcHeadStopsTail(next.extended,mask,1u<<tail.lane)) continue;
            tail.sustainDropped=true;
            tail.heldUntil=std::min(time,tail.timeSec+tail.sustainSec);
        }
    }

    [[nodiscard]] double chartTimeAtBeat(double beat) const noexcept {
        if(tempoSegments.empty())return beat*.5;
        const auto at=std::upper_bound(tempoSegments.begin(),tempoSegments.end(),beat,
            [](double b,const TempoSegment& s){return b<s.startBeat;});
        const auto& segment=at==tempoSegments.begin()?tempoSegments.front():*(at-1);
        return segment.startSec+(beat-segment.startBeat)*60.0/segment.bpm;
    }
    bool activateStarPower(double time) {
        if(!starPower.available())return false;
        // The PC settles the input update's tail ticks before deployment;
        // a head judged at this input still receives the deployed multiplier.
        advanceSustainScore(time,true);
        if(!starPower.activatePosition(starPowerClock.position(time),time))return false;
        ++resultStats.activations;resultStats.spStart=time;resultStats.lastSpSample=time;
        resultStats.spEnd=starPowerEndTime();
        return true;
    }
    [[nodiscard]] double starPowerEndTime() const noexcept {
        return starPowerClock.afterTicks(starPower.activationTime(),
            starPower.expirationTick()-starPower.activationTick());
    }
    void advanceSustainScore(double time, bool includeEndpoint) {
        if(starPower.active()) {
            resultStats.spEnd=starPowerEndTime();
            resultStats.advanceSp(time,true,resultStats.spEnd);
        }
        // Exclude only a tick that begins exactly at the input timestamp.
        // For a fractional-tick miss deadline the current floor tick is
        // already in the past; subtracting one loses a sustain point when
        // that input also changes the multiplier.
        auto tick=chartTickAt(includeEndpoint?time:std::nextafter(time,-INFINITY));
        if(!includeEndpoint && time<=0)--tick;
        const auto settle=[&](int64_t through,bool active) {
            for(const auto index:activeSustainNotes) {
                auto& note=notes[index];
                if(!note.hit || note.sustainDropped || note.sustainBeats<=0) continue;
                note.sustainGrace.advance(time,false);
                if(note.sustainGrace.dropped()) {note.sustainDropped=true;continue;}
                if(note.sustainGrace.scoring())awardSustainThrough(note,through,active);
            }
        };
        const auto powerTick=static_cast<int64_t>(starPowerClock.position(time));
        bool released=false;
        const double endTime=starPower.active()?starPowerEndTime():0.0;
        // BaseEngine.Update 0x1820F0840 judges input before its timer update.
        // At the EXACT deadline the head still gets SP; settleStep then ends
        // deployment. Earlier deadlines are settled before this input.
        if(starPower.active() && (time>endTime || (includeEndpoint && time==endTime))) {
            const auto end=starPower.expirationTick();
            const auto rawEnd=static_cast<int64_t>(std::ceil(chartBeatAt(endTime)*chartResolutionTicks));
            settle(rawEnd-1,true);released=starPower.advanceTicks(end);
        }
        settle(tick,starPower.active());
        released=starPower.advanceTicks(powerTick,false)||released;
        // SP has already advanced on its measure clock at the input timestamp.
        if(released) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarRelease);
    }

    void judgeSoloHead(size_t head,bool hit) {
        if(chartInstrument>=4) return;
        for(auto& solo:solos) {
            bool settle=true,endGroup=false;
            if(chartInstrument==3){
                endGroup=chart.notes[head].time==chart.notes[solo.lastHead].time;
                settle=endGroup;
                if(endGroup){
                    auto first=solo.lastHead;
                    while(first>solo.firstHead && chart.notes[first-1].time==chart.notes[solo.lastHead].time)--first;
                    for(auto i=first;i<=solo.lastHead;++i)
                        if(drumSession.evaluator().state(i)==rhythm::NoteState::Pending)settle=false;
                }
            }
            // BaseEngine 0x1820F2790 waits for every sibling of the final
            // chord. Drum dispatch visits children before their parent, so the
            // greatest array index alone is not the final judged head.
            const auto bonus=solo.judge(head,hit,settle,endGroup);
            score+=bonus;soloBonusTotal+=bonus;
            if(gameplayReplayTrace && head>=solo.firstHead && head<=solo.lastHead)
                std::fprintf(gameplayReplayTrace,"# solo_judge %d %zu %d %u %u %d %d %llu\n",chartInstrument,head,hit,solo.hits,solo.judged,solo.eligible,solo.awarded,static_cast<unsigned long long>(bonus));
        }
    }

    void applyPcHealthChange(bool hit) noexcept {
        // BaseEngine ctor / health update at 0x1820F3D80 / 0x1820F1290.
        gameplayHealth=rhythm::pc_scoring::healthAfter(gameplayHealth,hit,starPower.active());
    }

    [[nodiscard]] bool shouldFailSong() const noexcept {
        // BasePlayer 0x18036D7F0 bypasses zero-health failure in No Fail and
        // Practice; otherwise it fails at normalized health <= 0.
        return gameplayHealth <= 0.0f && !noFail && playMode != PlayMode::Practice;
    }

    [[nodiscard]] double chartBeatAt(double chartTime) const noexcept {
        if (tempoSegments.empty()) return chartTime * 2.0;
        const auto segment = std::upper_bound(tempoSegments.begin(), tempoSegments.end(), chartTime,
            [](double time, const TempoSegment& candidate) { return time < candidate.startSec; });
        const TempoSegment& active = segment == tempoSegments.begin()
            ? tempoSegments.front() : *(segment - 1);
        return active.startBeat + (chartTime - active.startSec) * active.bpm / 60.0;
    }
    [[nodiscard]] int64_t chartTickAt(double time) const noexcept {
        if(tempoSegments.empty())return rhythm::pcRawTick(time,0,0,chartResolutionTicks,120);
        // The source mapper uses the preceding tempo at its exact boundary.
        const auto at=std::lower_bound(tempoSegments.begin(),tempoSegments.end(),std::max(0.0,time),
            [](const TempoSegment& segment,double t){return segment.startSec<t;});
        const auto& segment=at==tempoSegments.begin()?tempoSegments.front():*(at-1);
        return rhythm::pcRawTick(time,segment.startSec,segment.startTick,chartResolutionTicks,segment.bpm);
    }

#if !defined(PS5_HOST_REFERENCE)
    void stopAudioOutput() {
        if (audioOutputStarted) audio_stop();
        audioOutputStarted = false;
    }

    void applyAudioGains() {
        const float content = screen == Screen::MainMenu ? menuVolume :
            (screen == Screen::SongSelect ? previewVolume : 1.0f);
        const bool menuPreview=screen==Screen::MainMenu;
        const float master = masterVolume * content *
            (menuPreview && menuPreviewMuted ? 0.0f : 1.0f);
        songAudio.setGain(master);
        songAudio.setPlaybackRate(screen == Screen::Gameplay && playMode == PlayMode::Practice
            ? practiceRate : 1.0f);
        songAudio.setUiGain(masterVolume * effectsVolume);
        songAudio.setMissSoundGain(missSoundVolume);
        const bool muted=screen==Screen::Gameplay && muteOnMiss && missMuted;
        const auto instrumentGain=[&](size_t stem,float volume) {
            bool assigned=false,audible=false;
            if(screen==Screen::Gameplay && gameplaySeatMask && muteOnMiss) {
                for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
                    const int instrument=seat==activeGameplaySeat?chartInstrument:parkedGameplay[seat]->chartInstrument;
                    const bool missed=seat==activeGameplaySeat?missMuted:parkedGameplay[seat]->missMuted;
                    if(rhythm::controlledStem(instrument,stem)){assigned=true;audible=audible || !missed;}
                }
            }
            const float gain=assigned?(!audible?missVolume:1.0f):rhythm::missStemGain(chartInstrument,stem,muted,missVolume);
            songAudio.setStemGain(stem,volume*gain);
        };
        instrumentGain(0,instrumentVolumes[0]); // guitar
        instrumentGain(1,instrumentVolumes[1]); // bass
        instrumentGain(2,instrumentVolumes[2]); // rhythm
        for(size_t stem=3;stem<=5;++stem) instrumentGain(stem,instrumentVolumes[3]);
        for(size_t stem=6;stem<=10;++stem) instrumentGain(stem,instrumentVolumes[4]);
        instrumentGain(11,instrumentVolumes[5]); // keys
        instrumentGain(12,musicVolume); // backing mix continues through misses
        instrumentGain(13,instrumentVolumes[6]); // crowd

    }

    void reloadSongAudio(const std::string& chartPath, const std::string& preferredName) {
        if (loadedAudioChartPath == chartPath && songAudio.ready() && !songAudio.streamFailed()) return;
        std::string audioChartPath=chartPath;
        for(const auto& song:localSongs)if(song.chartPath==chartPath && !song.audioChartPath.empty()){audioChartPath=song.audioChartPath;break;}
        const bool reusePreview=menuPreviewAudioChartPath==chartPath && songAudio.ready() && !songAudio.dedicatedPreview() && !songAudio.previewClip();
        stopAudioOutput();
        loadedAudioChartPath.clear();
        menuPreviewAudioChartPath.clear();
        // Only complete audio may be reused. A bounded preview is never
        // promoted to gameplay, even when it contains the selected song.
        bool loaded=reusePreview;
        if(reusePreview) songAudio.seekSeconds(0);
        else loaded=songAudio.loadGameplayStream(audioChartPath.c_str(),preferredName.c_str());
        if (loaded) loadedAudioChartPath = chartPath;
        if (!loaded) {
            fprintf(stderr, "[audio] no playable Opus/Vorbis/MP3/WAV stem beside %s\n", chartPath.c_str());
        }
        applyAudioGains();
        audioOutputStarted = audio_start(&rhythm::AudioStemMixer::audioFill, &songAudio);
        if (!audioOutputStarted)
            fprintf(stderr, "[audio] could not open the PS5 output device\n");
        else
            fprintf(stdout, "[audio] loaded %zu synchronized stems (%.2f seconds)\n",
                    songAudio.stemCount(), songAudio.durationSeconds());
    }

    void startGameplayAudioDecode() {
        if(!gameplayAudioPending || previewAudioThreadActive)return;
        previewAudioWorkerChartPath=gameplayAudioChart;
        previewAudioWorkerAudioPath=gameplayAudioChart;
        for(const auto& song:localSongs)if(song.chartPath==gameplayAudioChart && !song.audioChartPath.empty()) {
            previewAudioWorkerAudioPath=song.audioChartPath;break;
        }
        previewAudioWorkerMusic=gameplayAudioMusic;
        previewAudioWorkerDelay=audioDelayQa;
        previewAudioWorkerGameplay=true;
        previewAudioWorkerForMenu=false;previewAudioCancel=false;
        previewAudioWorkerLoaded=false;
        previewAudioWorkerDone.store(false,std::memory_order_release);
        if(pthread_create(&previewAudioThread,nullptr,&VulkanExample::previewAudioDecodeEntry,this)==0)
            previewAudioThreadActive=true;
        else {
            gameplayAudioPending=false;
            controllerToast="Could not start song audio decoder.";controllerToastUntil=ImGui::GetTime()+5;
        }
    }
    void requestGameplayAudio(const std::string& chartPath,const std::string& music) {
        if(gameplayAudioPending && gameplayAudioChart==chartPath)return;
        // Reuse only complete stems, never bounded preview buffers.
        if(!loadStallQa && ((loadedAudioChartPath==chartPath && songAudio.ready() && !songAudio.streamFailed()) ||
           (menuPreviewAudioChartPath==chartPath && songAudio.ready() && !songAudio.dedicatedPreview() && !songAudio.previewClip()))) {
            gameplayAudioPending=false;gameplayAudioCacheHit=true;reloadSongAudio(chartPath,music);return;
        }
        gameplayAudioCacheHit=false;gameplayAudioLoadMs=0;
        gameplayAudioChart=chartPath;gameplayAudioMusic=music;gameplayAudioPending=true;
        if(previewAudioThreadActive)previewAudioCancel=true;
        stopAudioOutput();songAudio.clear();loadedAudioChartPath.clear();menuPreviewAudioChartPath.clear();
        // Keep menu cues audible while the separate staging mixer decodes.
        // The current mixer now has only its unchanged UI sound players.
        applyAudioGains();audioOutputStarted=audio_start(&rhythm::AudioStemMixer::audioFill,&songAudio);
        // Reap an existing bounded preview job first, then prepare full audio
        // while the Ready clock remains frozen.
        startGameplayAudioDecode();
    }


#endif

    void restartSong() {
        rhythm::logDiagnostic("restart instrument=%d difficulty=%d seat=%d",chartInstrument,chartDifficulty,gameplayPlayer);
        resumeRewind.reset();
        if(!gameplaySeatMask){restartActiveGameplay();return;}
        for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
            selectGameplaySeat(seat);restartActiveGameplay(seat==primaryGameplaySeat);
        }
        selectGameplaySeat(primaryGameplaySeat);gameplayMenuHeld.fill(0);gameplayStarHeld.fill(false);
    }
    void restartActiveGameplay(bool shared=true) {
        if(shared) {
        multiplayerQaStep=multiplayerQaSample=0;
        multiplayerQaTime=multiplayerQaReplay?multiplayerQaInputs[primaryGameplaySeat].front().time:-LEAD_IN;
        songStartGate.arm();
        practiceCountdownRemaining = 0.0;
#if !defined(PS5_HOST_REFERENCE)
        songAudio.seekSeconds(std::max(0.0, trackStartOffsetSeconds));
#endif
        rhythm::ClockConfig clockConfig{};
        clockConfig.audioReferenceOffset = -trackStartOffsetSeconds;
        songClock = rhythm::AudioClock(clockConfig);
        songClock.reset(-LEAD_IN);
        }
        missMuted = false;
        badNoteVariant=0;
        #if !defined(PS5_HOST_REFERENCE)
        if(shared && screen==Screen::Gameplay && !gameplayAudioPending) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::NotesRippleUp);
#endif
        songTime = songClock.gameplayTime();
        renderTime = songClock.renderTime();
        session.reset();drumSession.reset();drumPadEdges.prime(0);
        drumFxClock=0;drumHitAt.fill(-1e30);drumPhraseHitAt.fill(-1e30);drumFretOffset.fill(0);for(auto& motion:drumFretMotion)motion.reset();
        stringAttachScale.fill(1);meterAttachScale.fill(1);
        inputPrimed = false;activeSustainNotes.clear();inputDiagnostics={};rawInputDrops=0;
        score = combo = bestCombo = hits = misses = 0;
        soloBonusTotal=0;fretBonusTotal=0;resultStats={};resultsScroll=0;resultsSelection=0;
        for(auto& solo:solos) solo.reset();
        gameplayHealth = 0.5f;
        songFailed = false;
        starPower.reset(chartResolutionTicks);
        for (auto& phrase : starPhrases) {
            phrase.hitCount = 0;
            phrase.failed = false;
            phrase.awarded = false;
        }
        newHighScore = false;
        hitGlow.fill(0);
        guitarFxClock=0;guitarHitAt.fill(-1e30);guitarPhraseHitAt.fill(-1e30);guitarEmitRemainder.fill(0);guitarSustaining.fill(false);guitarParticles.reset();
        highwayShakeTime = 0.0f;
        for (auto& n : notes) {
            n.hit = n.missed = false;
            n.heldUntil = 0;
            n.sustainDropped = false;
            n.sustainScoreStartBeat = 0.0;
            n.sustainPointsAwarded = 0;
            n.sustainGrace = {};
        }
    }

    void syncSongSelectionProfiles() {
        for(int player=0;player<PAD_PLAYERS;++player) {
            const auto* profile=profileMenu.profile(player);
            const int controller=songControllerType(player);
            const int id=profileMenu.slots[player].profile;
            playerJoined[player]=profileMenu.joined(player);
            if(id!=selectionProfileId[player] || controller!=selectionController[player]) {
                playerReady[player]=false;
                playerPage[player]=rhythm::play_menu::Page::Confirm;
                playerMenuRow[player]=0;
                playerInstrument[player]=controller==1?4:(controller>=2?3:0);
                if(!(selectedSongDifficulties[playerInstrument[player]]&(1u<<playerDifficulty[player])))
                    for(int d=3;d>=0;--d) if(selectedSongDifficulties[playerInstrument[player]]&(1u<<d)) {
                        playerDifficulty[player]=d;break;
                    }
                selectionProfileId[player]=id;selectionController[player]=controller;
            }
            if(!playerJoined[player] || profileMenu.open(player)) playerReady[player]=false;
            if(playerPage[player]==rhythm::play_menu::Page::Ready && !playerReady[player]) {
                playerPage[player]=rhythm::play_menu::Page::Confirm;playerMenuRow[player]=0;
            }
        }
    }

    void enterPreparedGameplay() {
        gameplayReadyPending=false;
        previousButtons=0;screen=Screen::Gameplay;playing=true;ui.visible=false;
        resultsSelection=settingsSelection=0;restartSong();
#if !defined(PS5_HOST_REFERENCE)
        if(FILE* file=std::fopen("/app0/song-load-status.txt","w")) {
            std::fprintf(file,"chart_ms %.3f audio_ms %.3f ready_wait_ms %.3f cache_hit %d stems %zu preview_only %d pcm_bytes %llu streaming %d underruns %llu\n",
                gameplayChartLoadMs,gameplayAudioLoadMs,std::max(0.0,now_seconds()-gameplayReadyAt)*1000,
                gameplayAudioCacheHit,songAudio.stemCount(),songAudio.previewClip()?1:0,
                static_cast<unsigned long long>(songAudio.pcmBytes()),songAudio.streaming()?1:0,static_cast<unsigned long long>(songAudio.streamUnderruns()));std::fclose(file);
        }
#endif
    }
    void beginMultiplayerQa(const std::string& path) {
        if(!ps5.frameBudget || multiplayerQa || path.rfind("/app0/multiplayer-qa-",0)!=0)return;
        FILE* options=std::fopen((path+".multi-options").c_str(),"r");if(!options)return;
        multiplayerQa=true;gameplaySeatMask=0;primaryGameplaySeat=0;pauseOwner=0;
        for(int seat=0;seat<PAD_PLAYERS;++seat) {
            unsigned instrument=0,difficulty=3,controller=0,bot=1,length=100,lefty=0,purple=0,style=0;
            char optionLine[256];
            float speed=1;
            if(!std::fgets(optionLine,sizeof(optionLine),options) || std::sscanf(optionLine,"%u %u %u %u %u %f %u %u %u",&instrument,&difficulty,&controller,&bot,&length,&speed,&lefty,&purple,&style)<8)break;
            if(instrument>7 || difficulty>3 || controller>4)break;
            selectGameplaySeat(seat);
            activeProfile={"Guest"};activeProfile.bot=bot;activeProfile.controller=controller;
            activeProfile.highwayLength=std::clamp(int(length),20,1000);activeProfile.speed=std::clamp(speed,.5f,2.0f);
            activeProfile.highwayStyle=std::min(int(style),int(rhythm::highwayStyles.size())-1);
            activeProfile.lefty=lefty;activeProfile.purpleKick=purple;activeProfile.displayName=true;activeGuest=true;
            profileMenu.slots[seat].guest=activeProfile;profileMenu.slots[seat].profile=-1;
            playerJoined[seat]=playerReady[seat]=true;playerInstrument[seat]=instrument;playerDifficulty[seat]=difficulty;playerModifierMask[seat]=0;
            loadChart(path,instrument,difficulty,0,false,controller,false);
            noteSpeedScale=activeProfile.speed;gamepadMode=false;
            activeControllerProfile=playerControllerProfile(seat,controller);
            ensureGameplayBuffers();gameplaySeatMask|=1u<<seat;
        }
        std::fclose(options);selectGameplaySeat(0);
        // Canonical logical inputs are accepted only by this bounded QA entry.
        // Every seat shares the same sample clock; normal input still uses its
        // own native/raw controller profile and the existing gameplay adapter.
        multiplayerQaReplay=false;
        for(int seat=0;seat<PAD_PLAYERS;++seat) {
            auto& frames=multiplayerQaInputs[seat];frames.clear();
            if(!(gameplaySeatMask&(1u<<seat)))continue;
            FILE* replay=std::fopen((path+".p"+std::to_string(seat+1)+".replay").c_str(),"r");
            if(!replay)continue;
            char line[512];double previous=-INFINITY;
            while(std::fgets(line,sizeof(line),replay)) {
                GameplayReplayFrame frame;
                const int fields=std::sscanf(line,"%lf %u %u %u %u %d %d %d %d %d %d %d %d",&frame.time,&frame.frets,&frame.up,&frame.down,&frame.sp,
                    &frame.velocities[0],&frame.velocities[1],&frame.velocities[2],&frame.velocities[3],&frame.velocities[4],&frame.velocities[5],&frame.velocities[6],&frame.velocities[7]);
                if((fields!=5 && fields!=13) || !std::isfinite(frame.time) || frame.time<=previous || frame.frets>255 || frames.size()>=36000)break;
                frames.push_back(frame);previous=frame.time;
            }
            std::fclose(replay);multiplayerQaReplay|=!frames.empty();
        }
        if(multiplayerQaReplay)for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
            const auto& reference=multiplayerQaInputs[0];const auto& frames=multiplayerQaInputs[seat];
            bool valid=!reference.empty() && frames.size()==reference.size();
            for(size_t i=0;valid && i<frames.size();++i)valid=frames[i].time==reference[i].time;
            if(!valid){multiplayerQaReplay=false;notify_fail("multiplayer QA clocks differ");break;}
        }
        if(slotInputQa && playerSeats.count==0)for(int seat=0;seat<PAD_PLAYERS;++seat)
            if(gameplaySeatMask&(1u<<seat))playerSeats.join(seat);
        gameplayAudioPending=gameplayReadyPending=false;playMode=PlayMode::Quickplay;noFail=true;
        songAudio.pause();songAudio.clear();loadedAudioChartPath.clear();
        screen=Screen::Gameplay;playing=true;ui.visible=false;restartSong();
    }

    void installMultiplayerQaLibrary(const std::string& path) {
        if(!ps5.frameBudget || path.rfind("/app0/multiplayer-qa-",0)!=0 || storageThreadActive)return;
        FILE* list=std::fopen(path.c_str(),"r");if(!list)return;
        std::vector<LocalSong> songs;char line[1024];
        while(songs.size()<8 && std::fgets(line,sizeof(line),list)) {
            const auto chartPath=trimText(line);if(chartPath.rfind("/app0/multiplayer-qa-",0)!=0)continue;
            if(FILE* chartFile=std::fopen(chartPath.c_str(),"r")) {
                std::fclose(chartFile);LocalSong song;song.chartPath=song.audioChartPath=song.originalChartPath=chartPath;
                const auto folder=chartPath.substr(0,chartPath.find_last_of('/'));song.title=folder.substr(folder.find_last_of('/')+1);
                readSongMetadata(folder,chartPath,song);songs.push_back(std::move(song));
            }
        }
        std::fclose(list);
        if(!songs.empty()){localSongs=std::move(songs);songSelection=0;menuMusicSelection=-1;++songListRevision;multiplayerQaLibraryLoaded=true;}
    }

    void tryStartSelectedSong() {
        for(const auto& remap:playerRemaps)if(remap.active)return;
        if(screen!=Screen::PlayerSelect || gameplayReadyPending) return;
        syncSongSelectionProfiles();
        // Do not launch while a player is editing or choosing a profile.
        for(int player=0;player<PAD_PLAYERS;++player) if(profileMenu.open(player)) return;
        const uint32_t connected=connectedPlayerMask();
        int chartPlayer=-1;
        for(int player=0;player<PAD_PLAYERS;++player) {
            if(!playerJoined[player] || !(connected&(1u<<player))) continue;
            if(!playerReady[player]) return;
            if(chartPlayer<0) chartPlayer=player;
        }
        if(chartPlayer<0) return;
        gameplaySeatMask=0;
        primaryGameplaySeat=chartPlayer;pauseOwner=chartPlayer;
        gameplayReadyAt=now_seconds();
        bool valid=true;
        for(int player=0;player<PAD_PLAYERS;++player) {
            if(!playerJoined[player] || !(connected&(1u<<player)))continue;
            selectGameplaySeat(player);
            const auto* profile=profileMenu.profile(player);
            const int instrument=playerInstrument[player];
            if(!instrumentAvailable(player,instrument)){valid=false;playerSelectionMessage="Choose a track for the connected guitar or drum kit.";break;}
            loadChart(activeChartPath,instrument,playerDifficulty[player],playerModifierMask[player],true,
                profile?profile->controller:0,player==chartPlayer);
            if(notes.empty()){valid=false;playerSelectionMessage="Player "+std::to_string(player+1)+": no notes for that instrument and difficulty.";break;}
            activeProfile=profile?*profile:rhythm::MenuProfile{"Guest"};
            activeGuest=profileMenu.slots[player].profile==-1;
            noteSpeedScale=activeProfile.speed;gamepadMode=activeProfile.gamepad;
            const int type=instrument==3?std::max(2,activeProfile.controller):instrument>=4?1:0;
            activeControllerProfile=playerControllerProfile(player,type);
            gameplaySeatMask|=1u<<player;
            ensureGameplayBuffers();
        }
        selectGameplaySeat(chartPlayer);
        gameplayChartLoadMs=(now_seconds()-gameplayReadyAt)*1000;
        if(valid) {
            playerSelectionMessage.clear();
            gameplaySongInfo=localSongs[static_cast<size_t>(songSelection)];gameplaySongInfoValid=true;
            if(gameplayAudioPending){gameplayReadyPending=true;playing=false;}
            else enterPreparedGameplay();
        } else {gameplaySeatMask=0;gameplayAudioPending=false;}

        if(screen==Screen::PlayerSelect && !gameplayReadyPending) {
            for(int player=0;player<PAD_PLAYERS;++player) {
                playerReady[player]=false;
                playerPage[player]=rhythm::play_menu::Page::Confirm;
            }
        }
    }

    int songControllerType(int player) const {
        if(const auto* usb=playerUsb(player);usb && usb->deviceProfile) {
            for(int type=0;type<5;++type)if(usb->deviceProfile->instrument==rhythm::controls::instrumentName(type))return type;
        }
        const auto* profile=profileMenu.profile(player);return profile?profile->controller:0;
    }
    bool instrumentAvailable(int player,int instrument) const {
        return rhythm::guitarTrackForController(instrument,songControllerType(player)) && selectedSongDifficulties[instrument];
    }
    unsigned guitarLaneCount() const {return chart.kind==rhythm::ChartKind::SixFret?6u:5u;}
    bool openLane(unsigned lane) const {return lane==guitarLaneCount();}
    float guitarLanePosition(unsigned lane) const {return chart.kind==rhythm::ChartKind::SixFret?pc_six_fret::laneX[lane%3]:laneX[lane];}
    bool selectionAvailable(int player) const {
        const int instrument=playerInstrument[player];
        return instrumentAvailable(player,instrument) && rhythm::play_menu::supported(instrument,selectedSongDifficulties[instrument],playerDifficulty[player]) &&
            (instrument!=3 || playerModifierMask[player]==0);
    }

    void acceptPlayerSelection(int player) {
        if(gameplayReadyPending)return;
        using rhythm::play_menu::Page;
        auto& page=playerPage[player];auto& row=playerMenuRow[player];
        playerSelectionMessage.clear();
        if(page==Page::Confirm) {
            if(row==0) {
                if(selectionAvailable(player)) { page=Page::Ready;playerReady[player]=true; }
                else playerSelectionMessage="Choose a supported instrument and difficulty with notes. Drum modifiers must be off.";
            } else if(row==1) {page=Page::Instrument;row=playerInstrument[player];}
            else if(row==2) {page=Page::Difficulty;row=3-playerDifficulty[player];}
            else {page=Page::Modifiers;row=0;}
        } else if(page==Page::Instrument) {
            if(instrumentAvailable(player,row)) {
                playerInstrument[player]=row;
                if(row==3) playerModifierMask[player]=0;
                if(!(selectedSongDifficulties[row]&(1u<<playerDifficulty[player])))
                    for(int d=3;d>=0;--d) if(selectedSongDifficulties[row]&(1u<<d)) {playerDifficulty[player]=d;break;}
                page=Page::Difficulty;row=3-playerDifficulty[player];
            }
        } else if(page==Page::Difficulty) {
            if(selectedSongDifficulties[playerInstrument[player]]&(1u<<(3-row))) {
                playerDifficulty[player]=3-row;page=Page::Confirm;row=0;
            }
        } else if(page==Page::Modifiers && playerInstrument[player]!=3) {
            playerModifierMask[player]=rhythm::play_menu::toggleModifier(playerModifierMask[player],row);
        }
    }

    void playerSelectionInput(int player,uint32_t pressed) {
        using rhythm::play_menu::Page;
        if(!playerJoined[player]) {
            if(player==0 && (pressed & PAD_CIRCLE)) screen=playerSelectReturn;
            return;
        }
        auto& page=playerPage[player];auto& row=playerMenuRow[player];
        if(pressed & PAD_CIRCLE) {
            if(page==Page::Confirm) screen=playerSelectReturn;
            else {page=Page::Confirm;row=0;playerReady[player]=false;}
            return;
        }
        const int count=rhythm::play_menu::rows(page);
        if(count) {
            if(pressed & PAD_UP)do {row=(row+count-1)%count;}while(page==Page::Instrument && !rhythm::guitarTrackForController(row,songControllerType(player)));
            if(pressed & PAD_DOWN)do {row=(row+1)%count;}while(page==Page::Instrument && !rhythm::guitarTrackForController(row,songControllerType(player)));
            if(pressed & PAD_CROSS) acceptPlayerSelection(player);
        }
    }

    bool openSongInPlayerSelect(int selection, Screen returnScreen) {
        if (localSongs.empty()) return false;
        const size_t index = static_cast<size_t>(std::clamp(selection, 0,
            static_cast<int>(localSongs.size() - 1)));
        songSelection = static_cast<int>(index);
        // Parse the selected gameplay chart once at Play. Prefetch audio on
        // the existing worker while players choose instruments/difficulties.
        activeChartPath = localSongs[index].chartPath;
        menuMusicSelection = static_cast<int>(index);
        playerSelectReturn = returnScreen;
        playerSelectionMessage.clear();
        selectedSongDifficulties.fill(0);
        // Library scanning cached the actual populated sections. Opening
        // player options must not synchronously re-read a large chart.
        constexpr int tracks[]{0,3,2,4,6,9,8,7};
        for(int track=0;track<8;++track)
            selectedSongDifficulties[track]=localSongs[index].chartDifficulties[tracks[track]];
        for (int player = 0; player < PAD_PLAYERS; ++player) {
            playerReady[player] = false;
            playerPage[player]=rhythm::play_menu::Page::Confirm;playerMenuRow[player]=0;
            const int controller=songControllerType(player);
            playerInstrument[player]=controller==1?4:controller>=2?3:0;
            if(playerInstrument[player]<8 && !(selectedSongDifficulties[playerInstrument[player]]&(1u<<playerDifficulty[player])))
                for(int d=3;d>=0;--d) if(selectedSongDifficulties[playerInstrument[player]]&(1u<<d)) {playerDifficulty[player]=d;break;}
            profileMenu.slots[player].page=rhythm::ProfileMenu::Page::Closed;
            selectionProfileId[player]=profileMenu.slots[player].profile;
            selectionController[player]=profileMenu.profile(player)?profileMenu.profile(player)->controller:0;
            playerJoined[player]=profileMenu.joined(player);
        }
        screen = Screen::PlayerSelect;
#if !defined(PS5_HOST_REFERENCE)
        if(!loadStallQa)requestGameplayAudio(activeChartPath,{});
#endif
        return true;
    }

    int resultsPageCount() const {return 3+std::max(1,(int(practiceMarks.size())+9)/10);}
    void performResultsAction(int action) {
        if(ps5.frameBudget && !gameplayReplay.empty()){gameplayReplay.clear();gameplayReplayIndex=0;}
        if(action==0) {screen=Screen::SongSelect;playing=false;return;}
        if(action==3) {
            const size_t count=localSongs.size(),start=count?size_t(std::rand())%count:0;
            int chosen=-1;
            const unsigned seats=gameplaySeatMask?gameplaySeatMask:(1u<<gameplayPlayer);
            std::array<std::string,PAD_PLAYERS> sections;
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(seats&(1u<<seat)) {
                const int instrument=seat==activeGameplaySeat?chartInstrument:parkedGameplay[seat]->chartInstrument;
                const int difficulty=seat==activeGameplaySeat?chartDifficulty:parkedGameplay[seat]->chartDifficulty;
                sections[seat]="["+std::string(rhythm::play_menu::difficulties[difficulty])+rhythm::play_menu::tracks[instrument]+"]";
            }
            // PC Random Song reloads the gameplay scene (0x1800A8730), so
            // every participant receives the new chart. Preflight all selected
            // parts before touching the completed match or starting audio work.
            for(size_t attempt=0;attempt<count;++attempt) {
                const size_t index=(start+attempt)%count;
                if(localSongs[index].chartPath==activeChartPath)continue;
                FILE* file=std::fopen(localSongs[index].chartPath.c_str(),"rb");if(!file)continue;
                char line[2048];unsigned inPart=0,available=0;
                while(std::fgets(line,sizeof(line),file)) {
                    const auto value=trimText(line);
                    if(!value.empty() && value.front()=='[') {
                        inPart=0;for(int seat=0;seat<PAD_PLAYERS;++seat)if((seats&(1u<<seat)) && value==sections[seat])inPart|=1u<<seat;
                    } else if(inPart){long tick,lane,len;
                        if(std::sscanf(value.c_str(),"%ld = N %ld %ld",&tick,&lane,&len)==3 &&
                           ((lane>=0&&lane<=5)||lane==7||lane==8)) {
                            for(int seat=0;seat<PAD_PLAYERS;++seat)if(inPart&(1u<<seat)) {
                                const int instrument=seat==activeGameplaySeat?chartInstrument:parkedGameplay[seat]->chartInstrument;
                                if((lane!=5 || instrument==3) && (lane!=8 || instrument>=4))available|=1u<<seat;
                            }
                        }
                    }
                    if((available&seats)==seats)break;
                }
                std::fclose(file);if((available&seats)==seats){chosen=int(index);break;}
            }
            if(chosen<0) {controllerToast="No other song supports every selected instrument and difficulty.";controllerToastUntil=ImGui::GetTime()+4;return;}
            songSelection=chosen;menuMusicSelection=chosen;
            gameplaySongInfo=localSongs[size_t(chosen)];gameplaySongInfoValid=true;
            const auto nextPath=gameplaySongInfo.chartPath;
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(seats&(1u<<seat)) {
                selectGameplaySeat(seat);
                const int instrument=chartInstrument,difficulty=chartDifficulty;
                loadChart(nextPath,instrument,difficulty,playerModifierMask[seat],true,activeProfile.controller,seat==primaryGameplaySeat);
                if(notes.empty()) {selectGameplaySeat(primaryGameplaySeat);leaveGameplay(false);controllerToast="A selected chart is unavailable. Please select another song.";controllerToastUntil=ImGui::GetTime()+4;return;}
                ensureGameplayBuffers();
            }
            selectGameplaySeat(primaryGameplaySeat);
            if(gameplayAudioPending){gameplayReadyPending=true;screen=Screen::PlayerSelect;playing=false;return;}
        }
        if(action==2)playMode=PlayMode::Practice;
        practiceSectionASet=practiceSectionBSet=false;practiceRate=1;
        screen=Screen::Gameplay;playing=true;ui.visible=false;restartSong();
    }

    void updateGameplay() {
        ++menuInputFrame;menuInputTime=now_seconds();
        static Screen loggedScreen=Screen::MainMenu;
        if(loggedScreen!=screen){rhythm::logDiagnostic("screen old=%d new=%d",int(loggedScreen),int(screen));loggedScreen=screen;}
        if(midiQa)for(auto& batch:midiBatches)batch.count=0;
        pollControllerSources();
        if(!ps5.frameBudget || midiDiscoveryQa)pollMidiSources();
        updateSongStorage();
        updateApplication();
        if(screen==Screen::MidiSetup){handleMidiSetup();return;}
        if(screen==Screen::SongSelect)loadSongScoreCache();
        bool participantPause=false;
        if(screen==Screen::Gameplay && gameplaySeatMask) {
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
                struct pad sampled{};
                if(!readPlayerInput(seat,&sampled))continue;
                const auto* profile=profileMenu.profile(seat);
                const int type=profile?profile->controller:0;
                const auto mapping=effectiveMapping(profile,type);
                const uint32_t start=playerUsb(seat)?PAD_OPTIONS:mapping.keys[rhythm::controls::count(type)-2];
                const auto edge=(sampled.held&~gameplayMenuHeld[seat])|sampled.pressed;
                gameplayMenuHeld[seat]=sampled.held;
                if(menuPressFilters[seat].update(edge&(PAD_OPTIONS|start),menuInputTime)) {
                    if(pauseOwner!=seat)previousButtons=0;
                    pauseOwner=seat;participantPause=true;
                }
            }
        }
        struct pad input{};
        const int inputPlayer = (instrumentKeyboard.open || searchKeyboardOpen)?keyboardPlayer:
            (screen == Screen::Gameplay ? (ui.visible?pauseOwner:gameplayPlayer) : 0);
        const bool connected = readPlayerInput(inputPlayer, &input);
        const uint32_t buttons = connected ? (((screen==Screen::Gameplay && !ui.visible) || screen==Screen::ControllerRemap)?input.held:menuButtons(inputPlayer,input.held)) : 0;
        const uint32_t rawEdges=(buttons&~previousButtons)|menuButtons(inputPlayer,input.pressed);
        const bool liveGameplay=screen==Screen::Gameplay && !ui.visible;
        const uint32_t keyboardEdges=playerRemaps[inputPlayer].active?rawEdges:
            liveGameplay?((rawEdges&~PAD_OPTIONS)|filterMenuPresses(inputPlayer,rawEdges&PAD_OPTIONS)):
            filterMenuPresses(inputPlayer,rawEdges);
        const bool keyboardBlockedFrame=searchKeyboardOpen || instrumentKeyboard.open;
        uint32_t pressed = keyboardBlockedFrame ? 0u : keyboardEdges;
        const bool menuNavigation=(screen!=Screen::Gameplay || ui.visible) && !playerRemaps[inputPlayer].active;
        const int navigationContext=int(screen)*128+int(settingsEditor)*4+(ui.visible?1:0);
        const uint32_t direction=buttons&(PAD_UP|PAD_DOWN);
        pressed|=menuRepeat.update(direction==(PAD_UP|PAD_DOWN)?0:direction,pressed,
            static_cast<float>(frameTimer),navigationContext,connected && menuNavigation && !keyboardBlockedFrame);
        previousButtons = buttons;
        if (screen == Screen::Downloader && downloaderScriptSearchRequested) {
            downloaderScriptSearchRequested = false;
            startEncoreSearch();
        }
        if (screen != Screen::Gameplay || ui.visible) {
            if (pressed & PAD_CROSS) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::Confirm);
            else if (pressed & PAD_CIRCLE) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::Cancel);
            else if (pressed & (PAD_UP | PAD_LEFT)) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::ScrollUp);
            else if (pressed & (PAD_DOWN | PAD_RIGHT)) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::ScrollDown);
        }
        chorusDownloader.update();
        const auto keyboardResult=pollSearchKeyboard();
        if(keyboardResult==SearchKeyboardTarget::Encore) startEncoreSearch();
        if(instrumentKeyboard.open) {
            uint32_t edges=keyboardEdges;
            const uint32_t direction=buttons&(PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT);
            const bool singleDirection=direction && !(direction&(direction-1));
            edges|=keyboardRepeat.update(singleDirection?direction:0,edges,
                static_cast<float>(frameTimer),int(searchKeyboardTarget),connected);
            if(edges)updateInstrumentKeyboard(edges);
            const bool assisted=instrumentKeyboard.open && updateKeyboardAssist();
            if(instrumentKeyboard.open && !connected && !assisted)cancelTextKeyboard();
#if !defined(PS5_HOST_REFERENCE)
            if(ps5.frameBudget)if(FILE* f=std::fopen("/app0/instrument-keyboard-test.txt",ps5.framesDrawn==0?"w":"a")) {
                std::fprintf(f,"frame %u open %d target %d owner %d key %d upper %d text %s\n",unsigned(ps5.framesDrawn),instrumentKeyboard.open,int(searchKeyboardTarget),keyboardPlayer,instrumentKeyboard.key,instrumentKeyboard.upper,instrumentKeyboard.text.c_str());std::fclose(f);
            }
#endif
            return;
        }
        if(keyboardBlockedFrame)return;
        if(screen==Screen::PlayerSelect && gameplayReadyPending) {
            bool cancel=(pressed&PAD_CIRCLE)!=0;
            for(int player=0;player<PAD_PLAYERS;++player) {
                struct pad p{};
                if(readPlayerInput(player,&p)) {
                    const auto held=menuButtons(player,p.held);
                    cancel=cancel || (filterMenuPresses(player,(held&~playerPreviousButtons[player])|menuButtons(player,p.pressed))&PAD_CIRCLE);
                    playerPreviousButtons[player]=held;
                }
            }
            if(cancel) {
                gameplayAudioPending=gameplayReadyPending=false;playing=false;
                playerSelectionMessage.clear();
                for(int player=0;player<PAD_PLAYERS;++player){playerReady[player]=false;playerPage[player]=rhythm::play_menu::Page::Confirm;}
                return;
            }
            pressed=0; // Loading cannot open pause/restart or consume gameplay actions.
        }
        if (screen == Screen::Downloader && chorusDownloader.state() == chorus::Downloader::State::Complete &&
            !downloaderSawComplete) {
            // The committed package is already normalized to one song root.
            // Update only that entry instead of rescanning the whole library.
            const std::string installedFolder = chorusDownloader.installedPath();
            if (!installedFolder.empty()) {
                const std::string installedChart = findChartFile(installedFolder);
                if (!installedChart.empty()) addSongChart(installedFolder, installedChart);
                sortLocalSongs();
            }
            downloaderSawComplete = true;
            if (downloaderQueuePosition < downloaderQueueCount) {
                const size_t next = downloaderQueue[downloaderQueuePosition++];
                if (chorusDownloader.download(next)) downloaderSawComplete = false;
                else downloaderQueuePosition = downloaderQueueCount;
            } else if (downloaderQueueCount != 0) {
                downloaderQueueCount = downloaderQueuePosition = 0;
            }
        }
        if(screen==Screen::ControllerSetup) {
            if(unknownController<0 || unknownController>=int(usbInputs.size()) ||
               !usbInputs[unknownController].present || !usbInputs[unknownController].unknown) {
                unknownController=-1;screen=Screen::MainMenu;return;
            }
            constexpr int order[]{0,2,4,3,1};
            if(pressed&PAD_UP)do {unknownControllerChoice=(unknownControllerChoice+5)%6;}while(unknownControllerChoice<5 && !rhythm::controls::controllerTypeEnabled(order[unknownControllerChoice]));
            if(pressed&PAD_DOWN)do {unknownControllerChoice=(unknownControllerChoice+1)%6;}while(unknownControllerChoice<5 && !rhythm::controls::controllerTypeEnabled(order[unknownControllerChoice]));
            if((pressed&PAD_CIRCLE) || ((pressed&PAD_CROSS) && unknownControllerChoice==5)) {
                const auto& u=usbInputs[unknownController];ignoredControllers[unknownController]=(uint32_t(u.vid)<<16)|u.pid;
                screen=Screen::MainMenu;
            } else if(pressed&PAD_CROSS) {
                const auto& u=usbInputs[unknownController];const int seat=playerSeats.join(unknownController+4);
                if(seat>=0) {
                    constexpr int order[]{0,2,4,3,1};
                    usbControllers.allow(u.vid,u.pid,order[unknownControllerChoice],u.channel);
                    screen=Screen::MainMenu;openControllerRemap(seat,order[unknownControllerChoice]);
                }
            }
            return;
        }
        bool profileInputConsumed=gameplayReadyPending;
        if (!gameplayReadyPending && (screen == Screen::MainMenu || screen == Screen::SongSelect || screen==Screen::SongSearch ||
            screen == Screen::PlayerSelect || screen == Screen::Settings || screen == Screen::SettingsDetail ||
            screen == Screen::Downloader)) {
            for (int player = 0; player < PAD_PLAYERS; ++player) {
                struct pad playerInput{};
                if (!readPlayerInput(player,&playerInput)) {
                    playerPreviousButtons[player] = 0;playerMenuRepeat[player].reset();
                    // Preserve the assigned profile across a temporary unplug.
                    if(playerCalibrations[player].active())closePlayerCalibration(player,0,"Controller disconnected. Previous settings kept.");
                    if(!playerRemaps[player].active)profileMenu.slots[player].page=rhythm::ProfileMenu::Page::Closed;
                    continue;
                }
                if(playerCalibrations[player].active()) {
                    if(player==0)profileInputConsumed=true;
                    if(!keyboardBlockedFrame && !searchKeyboardOpen && !instrumentKeyboard.open)updatePlayerCalibration(player,playerInput);
                    continue;
                }
                auto& remap=playerRemaps[player];
                if(remap.active) {
                    if(player==0)profileInputConsumed=true;
                    const uint32_t raw=playerInput.held;
                    uint32_t edges=(raw&~remap.previousButtons)|playerInput.pressed;
                    remap.previousButtons=raw;
                    if(remap.wizard.page==rhythm::controls::Remap::Page::Capture)
                        edges=menuPressFilters[player].update(edges,menuInputTime,.075,0,0);
                    else edges=filterMenuPresses(player,edges);
                    const uint32_t direction=raw&(PAD_UP|PAD_DOWN);
                    edges|=playerMenuRepeat[player].update(direction==(PAD_UP|PAD_DOWN)?0:direction,edges,
                        static_cast<float>(frameTimer),1024+int(remap.wizard.page),
                        remap.wizard.page!=rhythm::controls::Remap::Page::Capture && !keyboardBlockedFrame);
                    if(!keyboardBlockedFrame)updateControllerRemap(player,edges,raw);
                    continue;
                }
                playerInput.held=menuButtons(player,playerInput.held);
                uint32_t playerPressed = (keyboardBlockedFrame || searchKeyboardOpen || instrumentKeyboard.open) ? 0u : filterMenuPresses(player,(playerInput.held & ~playerPreviousButtons[player])|menuButtons(player,playerInput.pressed));
                const uint32_t direction=playerInput.held&(PAD_UP|PAD_DOWN);
                playerPressed|=playerMenuRepeat[player].update(direction==(PAD_UP|PAD_DOWN)?0:direction,playerPressed,
                    static_cast<float>(frameTimer),int(screen)*128+int(profileMenu.slots[player].page)*8+int(playerPage[player]),
                    !keyboardBlockedFrame && !searchKeyboardOpen && !instrumentKeyboard.open);
                playerPreviousButtons[player] = playerInput.held;
                if(screen==Screen::MainMenu || screen==Screen::SongSelect || screen==Screen::SongSearch ||
                   screen==Screen::Settings || screen==Screen::SettingsDetail || screen==Screen::Downloader ||
                   (screen==Screen::PlayerSelect && (profileMenu.open(player) || (playerPressed & PAD_OPTIONS)))) {
                    const bool wasOpen=profileMenu.open(player);
                    if(player==0) profileInputConsumed=wasOpen || (playerPressed & PAD_OPTIONS);
                    auto& slot=profileMenu.slots[player];
                    if((playerPressed & PAD_OPTIONS) && slot.page==rhythm::ProfileMenu::Page::Name)
                        submitProfileName(player);
                    else if(playerPressed & PAD_OPTIONS) profileMenu.start(player);
                    else if(wasOpen) {
                        if(playerPressed & PAD_CIRCLE) profileMenu.back(player);
                        else if(slot.page==rhythm::ProfileMenu::Page::Name) {
                            if(playerPressed & PAD_CROSS)(void)openSearchKeyboard(SearchKeyboardTarget::Profile,player);
                        } else {
                            if(playerPressed & PAD_UP) profileMenu.move(player,-1);
                            if(playerPressed & (PAD_DOWN | PAD_L1)) profileMenu.move(player,1);
                            if(playerPressed & PAD_LEFT) profileMenu.adjust(player,-1);
                            if(playerPressed & PAD_RIGHT) profileMenu.adjust(player,1);
                            if(slot.page==rhythm::ProfileMenu::Page::Calibration && profileMenu.rows(player)[slot.row]=="Input Offset"){
                                if(playerPressed&PAD_SQUARE)profileMenu.adjust(player,-1);
                                if(playerPressed&PAD_TRIANGLE)profileMenu.adjust(player,1);
                            }
                            if(playerPressed & PAD_CROSS) profileMenu.accept(player);
                        }
                    }
                    if(slot.page==rhythm::ProfileMenu::Page::Name && !searchKeyboardOpen && !instrumentKeyboard.open &&
                       (playerPressed&PAD_CROSS))(void)openSearchKeyboard(SearchKeyboardTarget::Profile,player);
                    playerJoined[player]=profileMenu.joined(player);
                    if(!playerJoined[player]) playerReady[player]=false;
                    continue;
                }
                profileInputConsumed=true;
                playerSelectionInput(player,playerPressed);
            }
            releaseUnassignedSeats();
            saveProfiles();
            for(int player=0;player<PAD_PLAYERS;++player) if(profileMenu.slots[player].remapRequested) {
                profileMenu.slots[player].remapRequested=false;
                openControllerRemap(player);if(player==0)profileInputConsumed=true;
            }
            for(int player=0;player<PAD_PLAYERS;++player){
                auto& slot=profileMenu.slots[player];
                if(slot.timingRequested || slot.velocityRequested){
                    const bool velocity=slot.velocityRequested;slot.timingRequested=slot.velocityRequested=false;
                    beginPlayerCalibration(player,velocity);if(player==0)profileInputConsumed=true;
                }
            }
            if(screen==Screen::PlayerSelect) tryStartSelectedSong();
        }
        if(screen==Screen::Results && gameplaySeatMask) {
            int sharedAction=-1;
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
                selectGameplaySeat(seat);struct pad resultInput{};
                if(readPlayerInput(seat,&resultInput)) {
                    const auto held=menuButtons(seat,resultInput.held);
                    auto edges=filterMenuPresses(seat,(held&~gameplayMenuHeld[seat])|menuButtons(seat,resultInput.pressed));
                    gameplayMenuHeld[seat]=held;
                    edges|=playerMenuRepeat[seat].update(held&(PAD_UP|PAD_DOWN),edges,float(frameTimer),int(Screen::Results),true);
                    if(edges&PAD_UP)resultsSelection=(resultsSelection+resultsPageCount()-1)%resultsPageCount();
                    if(edges&PAD_DOWN)resultsSelection=(resultsSelection+1)%resultsPageCount();
                    if(sharedAction<0) {
                        if(edges&(PAD_CROSS|PAD_CIRCLE))sharedAction=0;
                        else if(edges&PAD_SQUARE)sharedAction=1;
                        else if(edges&PAD_TRIANGLE)sharedAction=2;
                        else if(edges&PAD_L1)sharedAction=3;
                    }
                }
            }
            selectGameplaySeat(primaryGameplaySeat);
            // EndOfSong.Update 0x1800AA4F0 accepts each participant's controls.
            // Consume the winning action before a newly opened screen can see it.
            if(sharedAction>=0){performResultsAction(sharedAction);previousButtons=buttons;return;}
            pressed&=~(PAD_UP|PAD_DOWN);
        }
        if(updateDialog) {
            if(profileInputConsumed)return;
            using State=rhythm::updates::UpdateState;
            const auto state=applicationUpdater.state();
            if(pressed&PAD_CIRCLE){if(state==State::Checking || state==State::Downloading)applicationUpdater.cancel();if(state!=State::Installing)updateDialog=false;}
            if(pressed&PAD_CROSS) {
                if(state==State::Available)(void)applicationUpdater.download(updateFormat);
                else if(state==State::Ready) {
                    std::string error;if(!applicationUpdater.install(error)) {controllerToast=error;controllerToastUntil=ImGui::GetTime()+5;}
                } else if(state==State::Downloaded)updateDialog=false;
                else if(state==State::Idle || state==State::Current || state==State::Error || state==State::Cancelled)(void)applicationUpdater.check();
            }
            return;
        }
        if(screen==Screen::SongRemove) {
            if(profileInputConsumed)return;
            if(removalPage==0) {
                if(!removalChoices.empty()) {
                    if(pressed&PAD_UP)removalCursor=(removalCursor+int(removalChoices.size())-1)%int(removalChoices.size());
                    if(pressed&PAD_DOWN)removalCursor=(removalCursor+1)%int(removalChoices.size());
                    if(pressed&PAD_CROSS)removalSelected[removalCursor]^=1;
                }
                if((pressed&PAD_SQUARE) && removalCount())removalPage=1;
                if(pressed&PAD_CIRCLE)screen=Screen::SettingsDetail;
            } else if(removalPage==1) {
                if(pressed&PAD_CROSS)confirmSongRemoval();
                if(pressed&PAD_CIRCLE)removalPage=0;
            } else if(removalPage==2) {
                if(pressed&PAD_CIRCLE)storageCancel=true;
            } else if(pressed&(PAD_CROSS|PAD_CIRCLE))screen=Screen::SettingsDetail;
            return;
        }
        if(screen==Screen::SongCopyConfirm) {
            if(profileInputConsumed)return;
            if(songCopyPage==SongCopyPage::Progress) {
                if(pressed&PAD_CIRCLE)storageCancel=true;
            } else if(songCopyPage==SongCopyPage::Result) {
                if(pressed&(PAD_CROSS|PAD_CIRCLE))screen=Screen::SettingsDetail;
            } else {
                if(songCopyRoot.empty() && !externalSongRoots.empty()){storageDrive=0;songCopyRoot=externalSongRoots[0];storageResultText.clear();}
                if(pressed&(PAD_UP|PAD_DOWN))songCopyChoice=1-songCopyChoice;
                if(externalSongRoots.size()>1 && (pressed&(PAD_SQUARE|PAD_TRIANGLE))) {
                    storageDrive=(storageDrive+((pressed&PAD_SQUARE)?1:int(externalSongRoots.size())-1))%int(externalSongRoots.size());
                    songCopyRoot=externalSongRoots[storageDrive];storageResultText.clear();songCopyPlanReady=false;
                    if(storageThreadActive && storageJob==StorageJob::Preview)storageCancel=true;
                }
                if(pressed&PAD_CIRCLE)screen=Screen::SettingsDetail;
                else if(pressed&PAD_CROSS) {
                    if(songCopyChoice==1)screen=Screen::SettingsDetail;
                    else confirmSongCopy();
                }
            }
            return;
        }
        // Start owns this frame's pause transition. A fret held/pressed with
        // Start must not immediately activate Resume or Back in the new menu.
        const int mappingType=chart.kind==rhythm::ChartKind::Drums?(activeProfile.controller>=2?activeProfile.controller:2):(playerInstrument[gameplayPlayer]>=4?1:0);
        const auto gameplayMapping=effectiveMapping(&activeProfile,mappingType);
        const uint32_t mappedStarPower=gameplayMapping.keys[rhythm::controls::count(mappingType)-1];
        rhythm::controls::NativeSample nativeSample{buttons,{input.l2*255.0f,input.r2*255.0f,
            input.left_x*127.0f+128.0f,input.left_y*127.0f+128.0f,input.right_x*127.0f+128.0f,input.right_y*127.0f+128.0f}};
        const auto* usb=playerUsb(gameplayPlayer);
        auto logicalInstrument=rhythm::controls::applyProfile(activeControllerProfile,nativeSample,
            usb?usb->report.data():nullptr,usb?usb->length:0);
        if(mappingType==3)normalizeFiveLaneInput(logicalInstrument);
        const bool pauseToggled=screen==Screen::Gameplay && (participantPause || (ui.visible && (pressed & PAD_OPTIONS)));
        if (pauseToggled) {
            if(ui.visible)resumeGameplay();
            else {
                ui.visible=true;
#if !defined(PS5_HOST_REFERENCE)
                songAudio.pause();
#endif
            }
            if (ui.visible) {
                pausePage = PausePage::Main;
                pauseSelection = 0;
            }
        }
        // PC localization names the guitar Select action "Star Power"; the
        // supplied adapter translates Select to DualSense touchpad click.
        bool requestStarPower = screen == Screen::Gameplay && !ui.visible && !pauseToggled &&
            ((pressed & (usb?PAD_TOUCH_PAD:mappedStarPower)) || (activeProfile.bot && playing)) &&
            (playMode != PlayMode::Practice || practiceStarPowerEnabled);
        // DualSense fallback mappings for guitar and drums. CH drum
        // charts use lane 0 for kick then red/yellow/blue/green on lanes 1-4;
        // the renderer's five-lane order is green/red/yellow/blue/orange.
        frets = 0;
        const bool drumController = chart.kind==rhythm::ChartKind::Drums;
        if (screen == Screen::Gameplay && drumController) {
            constexpr unsigned visualLanes[]{4,1,2,3,0};
            for(unsigned pad=0;pad<5;++pad)if(logicalInstrument.pads&(1u<<pad))frets|=1u<<visualLanes[pad];
        } else if (screen == Screen::Gameplay) for (unsigned lane = 0; lane < guitarLaneCount(); ++lane) {
            if (logicalInstrument.frets & (1u<<lane)) frets |= 1u << lane;
        }
        if (screen == Screen::Gameplay && (pressed & PAD_R3)) restartSong();
        if (screen == Screen::Gameplay && playMode == PlayMode::Practice) {
            const uint32_t slower = horizontalStrum && mappingType!=1 ? PAD_L3 : PAD_LEFT;
            const uint32_t faster = horizontalStrum && mappingType!=1 ? PAD_R1 : PAD_RIGHT;
            if (pressed & slower) practiceRate = std::max(0.5f, practiceRate - 0.1f);
            if (pressed & faster) practiceRate = std::min(1.5f, practiceRate + 0.1f);
        }
        if (screen != Screen::Gameplay && !profileInputConsumed) {
            if (screen == Screen::MainMenu) {
                // Online stays visible as a coming-soon row, but cannot receive focus.
                if(mainSelection==2) mainSelection=3;
                if (pressed & PAD_UP) {
                    mainSelection = mainSelection == 6 ? 5 : (mainSelection + 5) % 6;
                    if(mainSelection==2) mainSelection=1;
                }
                if (pressed & PAD_DOWN) {
                    mainSelection = mainSelection == 6 ? 5 : (mainSelection + 1) % 6;
                    if(mainSelection==2) mainSelection=3;
                }
                if (pressed & PAD_L1) toggleMenuPreviewMute();
                if (pressed & PAD_TRIANGLE) toggleVersionNotes();
                if (pressed & PAD_SQUARE) toggleEncoreFeatures();
                if (pressed & PAD_TOUCH_PAD) {
                    playMode = PlayMode::Quickplay;
                    if (!openSongInPlayerSelect(menuMusicSelection, Screen::MainMenu))
                        screen = Screen::SongSelect;
                }
            } else if (screen == Screen::SongSelect) {
                const auto& filtered = filteredSongIndices();
                if (!filtered.empty()) {
                    auto current = std::find(filtered.begin(), filtered.end(), static_cast<size_t>(songSelection));
                    int visibleIndex = current == filtered.end() ? 0 : static_cast<int>(current - filtered.begin());
                    const int count = static_cast<int>(filtered.size());
                    if (pressed & PAD_UP) visibleIndex = (visibleIndex + count - 1) % count;
                    if (pressed & PAD_DOWN) visibleIndex = (visibleIndex + 1) % count;
                    songSelection = static_cast<int>(filtered[static_cast<size_t>(visibleIndex)]);
                }
                if(pressed & PAD_TRIANGLE) (void)beginLocalSongSearch();
                if(pressed & PAD_TOUCH_PAD) openSongSearchOptions();
                if (pressed & PAD_SQUARE) { cycleSongSort(1); saveSettings(); }
                else if (pressed & PAD_LEFT) { cycleSongSort(-1); saveSettings(); }
                else if (pressed & PAD_RIGHT) { cycleSongSort(1); saveSettings(); }
                if (pressed & PAD_L1) jumpSongSection(1);
                else if (pressed & PAD_R1) jumpSongSection(-1);
            } else if (screen == Screen::SongSearch) {
                if(pressed & PAD_UP) songSearchKey=(songSearchKey+7)%8;
                if(pressed & PAD_DOWN) songSearchKey=(songSearchKey+1)%8;
                if(pressed & PAD_TRIANGLE) acceptSongSearchOption();
            } else if (screen == Screen::Settings) {
                const int oldCategory=settingsCategorySelection;
                if (pressed & PAD_UP) settingsCategorySelection = (settingsCategorySelection + 5) % 6;
                if (pressed & PAD_DOWN) settingsCategorySelection = (settingsCategorySelection + 1) % 6;
                if(oldCategory!=settingsCategorySelection) {
                    if(settingsCategorySelection<5) settingsCategory=settingsCategorySelection;
                    settingsSelection=0;
                }
            } else if (screen == Screen::SettingsDetail) {
                if(settingsEditor!=SettingsEditor::None) {
                    if(pressed & PAD_UP) moveSettingsEditor(-1);
                    if(pressed & PAD_DOWN) moveSettingsEditor(1);
                    if(settingsEditor==SettingsEditor::Slider) {
                        if(pressed & (PAD_LEFT | PAD_SQUARE)) moveSettingsEditor(-1);
                        if(pressed & (PAD_RIGHT | PAD_TRIANGLE)) moveSettingsEditor(1);
                    }
                } else {
                    const int rowCount=settingsDetailRowCount();
                    if (pressed & PAD_UP) settingsSelection = (settingsSelection + rowCount - 1) % rowCount;
                    if (pressed & PAD_DOWN) settingsSelection = (settingsSelection + 1) % rowCount;
                }
            } else if (screen == Screen::Downloader) {
                // L1 is the guitar's orange fret on the supplied bridge. Let it
                // switch between query and results so the browser remains fully
                // navigable without a physical R1 shoulder button.
                if (pressed & PAD_L1) {
                    if (downloaderFilterFocus) {
                        downloaderFilterFocus = false;
                        downloaderResultsFocus = true;
                    } else {
                        downloaderResultsFocus = !downloaderResultsFocus;
                    }
                }
                if (pressed & PAD_R1) { downloaderResultsFocus = true; downloaderFilterFocus = false; }
                // Bridge Select is the DualSense touchpad click; Start stays with
                // the shared per-player profile cards on this screen.
                if (pressed & PAD_TOUCH_PAD) (void)openSearchKeyboard(SearchKeyboardTarget::Encore);
                if (pressed & PAD_TRIANGLE) {
                    downloaderFilterFocus = !downloaderFilterFocus;
                    downloaderResultsFocus = false;
                }
                if (downloaderResultsFocus && (pressed & PAD_SQUARE) && chorusDownloader.resultCount() > 0 &&
                    downloaderResult >= 0 && static_cast<size_t>(downloaderResult) < downloaderMarked.size()) {
                    downloaderMarked[static_cast<size_t>(downloaderResult)] = !downloaderMarked[static_cast<size_t>(downloaderResult)];
                }
                if (downloaderResultsFocus) {
                    const int count = static_cast<int>(chorusDownloader.resultCount());
                    if (count > 0 && (pressed & PAD_UP)) downloaderResult = (downloaderResult + count - 1) % count;
                    if (count > 0 && (pressed & PAD_DOWN)) {
                        if (downloaderResult + 1 < count) ++downloaderResult;
                        else if (chorusDownloader.hasMoreResults())
                            chorusDownloader.loadNextPage(downloaderQuery.c_str());
                    }
                } else if (downloaderFilterFocus) {
                    if (pressed & PAD_UP) downloaderFilterSelection = (downloaderFilterSelection + 5) % 6;
                    if (pressed & PAD_DOWN) downloaderFilterSelection = (downloaderFilterSelection + 1) % 6;
                    const int step = (pressed & PAD_RIGHT) ? 1 : ((pressed & PAD_LEFT) ? -1 : 0);
                    if (step != 0) {
                        if (downloaderFilterSelection == 0)do {downloaderInstrument = (downloaderInstrument + step + 11) % 11;}while(!rhythm::controls::browserInstrumentEnabled(downloaderInstrument));
                        else if (downloaderFilterSelection == 1) downloaderDifficulty = (downloaderDifficulty + step + 5) % 5;
                        else if (downloaderFilterSelection == 2) downloaderSort = (downloaderSort + step + 8) % 8;
                        else if (downloaderFilterSelection == 3) downloaderDrumType = (downloaderDrumType + step + 3) % 3;
                        else if (downloaderFilterSelection == 4) downloaderReviewedDrums = !downloaderReviewedDrums;
                        else downloaderSearchField = (downloaderSearchField + step + 7) % 7;
                    }
                    if (pressed & PAD_SQUARE) {
                        if (downloaderFilterSelection == 2) downloaderSortDescending = !downloaderSortDescending;
                        else if (downloaderFilterSelection == 4) downloaderReviewedDrums = !downloaderReviewedDrums;
                    }
                } else {
                    if (pressed & PAD_UP) downloaderKey = (downloaderKey + 30) % 40;
                    if (pressed & PAD_DOWN) downloaderKey = (downloaderKey + 10) % 40;
                    if (pressed & PAD_LEFT) downloaderKey = (downloaderKey + 39) % 40;
                    if (pressed & PAD_RIGHT) downloaderKey = (downloaderKey + 1) % 40;
                }
            } else if (screen == Screen::Results) {
                if(pressed&PAD_UP)resultsSelection=std::max(0,resultsSelection-1);
                if(pressed&PAD_DOWN)resultsSelection=std::min(resultsPageCount()-1,resultsSelection+1);
                if(pressed&PAD_SQUARE)performResultsAction(1);
                else if(pressed&PAD_TRIANGLE)performResultsAction(2);
                else if(pressed&PAD_L1)performResultsAction(3);
            }
            if ((pressed & PAD_CIRCLE) && !searchKeyboardOpen) {
                if(screen==Screen::MainMenu) {versionNotesExpanded=false;encoreFeaturesExpanded=false;}
                if (screen == Screen::SongSelect || screen == Screen::Online || screen == Screen::Settings) screen = Screen::MainMenu;
                else if (screen == Screen::SongSearch) screen = Screen::SongSelect;
                else if (screen == Screen::Downloader) {
                    chorusDownloader.reset();
                    downloaderQuery.clear();downloaderImeError.clear();downloaderCoverLoadedPath.clear();
                    downloaderResult=downloaderKey=downloaderFilterSelection=0;
                    downloaderResultsFocus=downloaderFilterFocus=downloaderScriptSearchRequested=false;
                    downloaderMarked.fill(false);downloaderQueueCount=downloaderQueuePosition=0;
                    downloaderSawComplete=false;
                    screen = Screen::MainMenu;
                }
                else if (screen == Screen::SettingsDetail) {
                    if(settingsEditor!=SettingsEditor::None) settingsEditor=SettingsEditor::None;
                    else screen=Screen::Settings;
                }
                else if (screen == Screen::Results) { screen = Screen::SongSelect; playing = false; }
            }
            if ((pressed & PAD_CROSS) && !searchKeyboardOpen) {
                if (screen == Screen::MainMenu) {
                    if (mainSelection == 0 || mainSelection == 1 || mainSelection == 3) {
                        playMode = mainSelection == 1 ? PlayMode::Versus : (mainSelection == 3 ? PlayMode::Practice : PlayMode::Quickplay);
                        screen = Screen::SongSelect;
                    } else if (mainSelection == 4) {
                        settingsCategorySelection = std::min(settingsCategorySelection, 4);
                        settingsCategory = settingsCategorySelection;
                        settingsSelection = 0;
                        settingsEditor=SettingsEditor::None;
                        screen = Screen::Settings;
                    }
                    else if (mainSelection == 5) screen = Screen::Downloader;
                    else if (mainSelection == 6) toggleVersionNotes();
                } else if (screen == Screen::SongSelect) {
                    if (!filteredSongIndices().empty())
                        (void)openSongInPlayerSelect(songSelection, Screen::SongSelect);
                } else if (screen == Screen::SongSearch) {
                    acceptSongSearchOption();
                } else if (screen == Screen::Downloader) {
                    if (downloaderResultsFocus && chorusDownloader.resultCount() > 0) {
                        downloaderQueueCount = downloaderQueuePosition = 0;
                        for (size_t i = 0; i < chorusDownloader.resultCount() && i < downloaderMarked.size(); ++i) {
                            if (downloaderMarked[i] && downloaderQueueCount < downloaderQueue.size())
                                downloaderQueue[downloaderQueueCount++] = i;
                        }
                        if (downloaderQueueCount == 0) {
                            if (chorusDownloader.download(static_cast<size_t>(downloaderResult))) downloaderSawComplete = false;
                        } else {
                            const size_t first = downloaderQueue[downloaderQueuePosition++];
                            if (chorusDownloader.download(first)) downloaderSawComplete = false;
                            else downloaderQueueCount = downloaderQueuePosition = 0;
                        }
                    } else startEncoreSearch();
                } else if (screen == Screen::Results) {
                    performResultsAction(0);
                } else if (screen == Screen::Settings) {
                    if (settingsCategorySelection == 5) screen = Screen::MainMenu;
                    else { settingsCategory = settingsCategorySelection; settingsSelection = 0; screen = Screen::SettingsDetail; }
                } else if (screen == Screen::SettingsDetail) {
                    if (settingsCategorySelection == 5) screen = Screen::MainMenu;
                    else activateSettingsValue();
                }
            }
        } else if (screen==Screen::Gameplay && ui.visible && !pauseToggled) {
            if (pressed & PAD_CIRCLE) {
                if (pausePage == PausePage::Main) resumeGameplay();
                else if (pausePage == PausePage::Options) { pausePage = PausePage::Main; pauseSelection = playMode==PlayMode::Practice?8:4; }
                else if (pausePage == PausePage::SongSettings) { pausePage = PausePage::Main; pauseSelection = 5; }
                else if (pausePage == PausePage::ConfirmQuit) { pausePage=PausePage::Main;pauseSelection=confirmationReturnRow; }
                else if (pausePage == PausePage::PracticeSection || pausePage == PausePage::Sections) { pausePage = PausePage::Main; pauseSelection = 0; }
                else { pausePage = PausePage::Options; pauseSelection = 0; }
            }
            const int rowCount = pausePageRowCount();
            if (rowCount > 0 && (pressed & PAD_UP)) pauseSelection = (pauseSelection + rowCount - 1) % rowCount;
            if (rowCount > 0 && (pressed & (PAD_DOWN | PAD_L1))) pauseSelection = (pauseSelection + 1) % rowCount;
            if (pressed & (PAD_LEFT | PAD_SQUARE)) adjustPauseValue(-1);
            if (pressed & (PAD_RIGHT | PAD_TRIANGLE)) adjustPauseValue(1);
            if (pressed & PAD_CROSS) activatePauseSelection();
        }
        if(screen==Screen::Gameplay && gameplaySeatMask && (connectedPlayerMask()&gameplaySeatMask)!=gameplaySeatMask) {
            ui.visible=true;pausePage=PausePage::Main;
            if(!(connectedPlayerMask()&(1u<<pauseOwner)))for(int seat=0;seat<PAD_PLAYERS;++seat)
                if((connectedPlayerMask()&gameplaySeatMask)&(1u<<seat)){pauseOwner=seat;previousButtons=0;break;}
        }
        if (screen != Screen::Gameplay) ui.visible = true;
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.frameBudget && (ps5.framesDrawn%30==0 || pressed)) {
            if(FILE* trace=std::fopen("/app0/play-menu-test.txt",ps5.framesDrawn==0?"w":"a")) {
                std::fprintf(trace,"frame %u screen %d pause %d row %d visible %d mode %d",
                    static_cast<unsigned>(ps5.framesDrawn),int(screen),int(pausePage),pauseSelection,ui.visible?1:0,int(playMode));
                for(int player=0;player<PAD_PLAYERS;++player)
                    std::fprintf(trace," p%d %d,%d,%d,%d,%d,%u",player+1,playerJoined[player]?1:0,int(playerPage[player]),playerMenuRow[player],playerInstrument[player],playerDifficulty[player],unsigned(playerModifierMask[player]));
                for(int player=0;player<PAD_PLAYERS;++player)
                    std::fprintf(trace," profile%d %d,%d,%d,%d",player+1,profileMenu.slots[player].profile,
                        int(profileMenu.slots[player].page),profileMenu.slots[player].row,playerReady[player]?1:0);
                std::fprintf(trace,"\n");std::fclose(trace);
            }
        }
#endif
        const bool replaying=gameplayReplayIndex<gameplayReplay.size();
        // PC GameManager.Start awaits preparation before starting its song
        // routine. Reap completed decode jobs without blocking the render loop.
#if !defined(PS5_HOST_REFERENCE)
        // Start once after preparation; sample completion, rather than a timer,
        // releases preview playback. Leaving the menu cancels the opening cue.
        if(!menuIntroStarted) {
            menuIntroStarted=true;
            if(screen==Screen::MainMenu && audioOutputStarted)
                songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::MenuIntro);
            rhythm::logDiagnostic("menu intro started playing=%d",songAudio.uiSoundPlaying(rhythm::AudioStemMixer::UiSound::MenuIntro));
        }
        if(!menuIntroFinished && (screen!=Screen::MainMenu || !audioOutputStarted ||
            !songAudio.uiSoundPlaying(rhythm::AudioStemMixer::UiSound::MenuIntro))) {
            songAudio.stopUiSound(rhythm::AudioStemMixer::UiSound::MenuIntro);
            menuIntroFinished=true;menuWidgetRevealAt=ImGui::GetTime();
            rhythm::logDiagnostic("menu intro finished; preview enabled");
        }
        if(!timingCalibrationRunning()){finishPreviewAudioDecode();startGameplayAudioDecode();}
        if(screen==Screen::Gameplay && songAudio.streamFailed() && !ui.visible){
            rhythm::logDiagnostic("ERROR gameplay audio source failed; pausing");
            playing=false;ui.visible=true;pausePage=PausePage::Main;pauseSelection=0;
            controllerToast="Song audio became unavailable. Reconnect storage and return to song selection.";
            controllerToastUntil=ImGui::GetTime()+8;
        }
#endif
        const bool injectLoadStall=loadStallQa && !loadStallInjected && screen==Screen::Gameplay && songStartGate.fresh;
        if(injectLoadStall){frameTimer=8.0f;loadStallInjected=true;}
        const double gameplayDelta=songStartGate.delta(frameTimer,now_seconds());
#if !defined(PS5_HOST_REFERENCE)
        if(injectLoadStall)if(FILE* file=std::fopen("/app0/song-start-test.txt","w")) {
            std::fprintf(file,"inherited_delta %.6f used_delta %.6f clock_before %.6f audio_position %.6f loading %d\n",
                frameTimer,gameplayDelta,songClock.gameplayTime(),songAudio.positionSeconds(),gameplayAudioPending);
            std::fclose(file);
        }
#endif
        if(screen==Screen::Gameplay && !ui.visible && resumeRewind.animated) {
            const bool ready=resumeRewind.advance(gameplayDelta);
            songClock.reset(resumeRewind.position);
#if !defined(PS5_HOST_REFERENCE)
            if(ready && songAudio.ready())songAudio.seekSeconds(std::max(0.0,resumeRewind.position+trackStartOffsetSeconds));
#else
            (void)ready;
#endif
        }
        const bool advancing = playing && screen == Screen::Gameplay && !ui.visible &&
            !gameplayAudioPending && !songStartGate.waiting && !resumeRewind.animated;
        if (advancing && practiceCountdownRemaining > 0.0) {
            practiceCountdownRemaining = std::max(0.0, practiceCountdownRemaining - gameplayDelta);
            if (practiceCountdownRemaining <= 0.0) {
#if !defined(PS5_HOST_REFERENCE)
                if (audioOutputStarted && songAudio.ready()) (void)songAudio.play();
#endif
            }
        }
        bool chartAdvancing = advancing && practiceCountdownRemaining <= 0.0;
        #if !defined(PS5_HOST_REFERENCE)
        if(songAudio.streamBuffering() && songClock.gameplayTime()>=0)chartAdvancing=false;
#endif
        const double frameAdvance = gameplayDelta * (screen == Screen::Gameplay && playMode == PlayMode::Practice ? practiceRate : 1.0f);
#if !defined(PS5_HOST_REFERENCE)
        if(screen==Screen::MainMenu && menuIntroFinished && !timingCalibrationRunning()) {
            if(menuMusicSelection>=0 && menuMusicSelection<static_cast<int>(localSongs.size())) {
                const LocalSong& current=localSongs[static_cast<size_t>(menuMusicSelection)];
                const bool completed=menuPreviewAudioChartPath==current.chartPath && previewPlayingForMenu &&
                    previewClipLength>0 && !songAudio.playing() &&
                    songAudio.positionSeconds()>=previewClipStart+previewClipLength-1.0/48000.0;
                const bool failed=std::find(menuPreviewFailedPaths.begin(),menuPreviewFailedPaths.end(),current.chartPath)!=menuPreviewFailedPaths.end();
                if(completed && ps5.frameBudget) {
                    FILE* trace=std::fopen("/app0/preview-test.txt","a");
                    if(trace) { std::fprintf(trace,"end elapsed %.6f path %s\n",
                        songAudio.positionSeconds()-previewClipStart,current.chartPath.c_str());std::fclose(trace); }
                }
                if(completed || failed) {
                    menuMusicSelection=chooseMenuPreviewSong();
                    menuPreviewAudioChartPath.clear();
                }
            }
            if(menuMusicSelection>=0 && menuMusicSelection<static_cast<int>(localSongs.size()))
                requestPreviewAudio(localSongs[static_cast<size_t>(menuMusicSelection)]);
        } else if (screen == Screen::SongSelect && !timingCalibrationRunning() && songSelection >= 0 &&
                   songSelection < static_cast<int>(localSongs.size())) {
            // Cursor settling plus worker decode keeps browsing responsive.
            if (previewQueuedSongSelection != songSelection) {
                previewQueuedSongSelection = songSelection;
                previewQueueChangedAt = ImGui::GetTime();
            } else if (ImGui::GetTime() - previewQueueChangedAt >= 0.45) {
                requestPreviewAudio(localSongs[static_cast<size_t>(songSelection)]);
            }
        }
        applyAudioGains();
        if (screen == Screen::MainMenu || screen == Screen::SongSelect) {
            const int selected=screen==Screen::MainMenu ? menuMusicSelection : songSelection;
            const bool pending=selected<0 || selected>=static_cast<int>(localSongs.size()) ||
                menuPreviewAudioChartPath!=localSongs[static_cast<size_t>(selected)].chartPath ||
                previewPlayingForMenu!=(screen==Screen::MainMenu);
            if(pending || (screen==Screen::MainMenu && !menuIntroFinished)) songAudio.pause();
            else if(audioOutputStarted && songAudio.ready() && !songAudio.playing()) {
                if(songAudio.positionSeconds()>=previewClipStart+previewClipLength-1.0/48000.0) {
                    if(screen==Screen::SongSelect) startPreviewPlayback(localSongs[static_cast<size_t>(selected)]);
                } else (void)songAudio.play();
            }
        } else if (!chartAdvancing) songAudio.pause();
        else if (audioOutputStarted && songClock.gameplayTime() >= std::max(0.0, -trackStartOffsetSeconds) && !songAudio.playing())
            (void)songAudio.play();
        if(ps5.frameBudget && ps5.framesDrawn%60==0) {
            FILE* trace=std::fopen("/app0/gameplay-audio-test.txt","a");
            if(trace) {std::fprintf(trace,"frame %u screen %d advance %d output %d stems %zu play %d clock %.3f pos %.3f duration %.3f gain %.3f music %.3f blocks %llu effects %llu loading %d\n",
                static_cast<unsigned>(ps5.framesDrawn),static_cast<int>(screen),chartAdvancing,audioOutputStarted,songAudio.stemCount(),songAudio.playing(),songClock.gameplayTime(),songAudio.positionSeconds(),songAudio.durationSeconds(),masterVolume,musicVolume,
                static_cast<unsigned long long>(songAudio.songBlocks()),static_cast<unsigned long long>(songAudio.effectBlocks()),gameplayAudioPending);std::fclose(trace);}
        }
        const std::optional<double> playbackPosition =
            audioOutputStarted && songAudio.playing()
                ? std::optional<double>{ songAudio.positionSeconds() }
                : std::nullopt;
        songClock.update(frameAdvance, playbackPosition, chartAdvancing, audioOutputStarted);
#else
        songClock.update(frameAdvance, std::nullopt, chartAdvancing, false);
#endif
        songTime = songClock.gameplayTime() + audioOffsetMs / 1000.0;
#if !defined(PS5_HOST_REFERENCE)
        if(injectLoadStall)if(FILE* file=std::fopen("/app0/song-start-test.txt","a")) {
            std::fprintf(file,"clock_after %.6f playing %d\n",songClock.gameplayTime(),songAudio.playing());std::fclose(file);
        }
#endif
        renderTime = songClock.renderTime() + (videoOffsetMs + audioOffsetMs) / 1000.0;
        if(screen!=Screen::Gameplay) {selectGameplaySeat(primaryGameplaySeat);return;}
        if(multiplayerQa) {
            multiplayerQaAdvanced=chartAdvancing;
            if(multiplayerQaReplay && chartAdvancing) {
                multiplayerQaSample=multiplayerQaStep;
                multiplayerQaTime=multiplayerQaInputs[primaryGameplaySeat][std::min(multiplayerQaStep,multiplayerQaInputs[primaryGameplaySeat].size()-1)].time;
            }
            else if(chartAdvancing)multiplayerQaTime+=1.0/60.0;
            songTime=renderTime=multiplayerQaTime;
        }
        if(replaying) songTime=renderTime=gameplayReplay[gameplayReplayIndex].time;
        else if(ps5.frameBudget && !gameplayReplay.empty())songTime=renderTime=gameplayReplay.back().time;
        if (chartAdvancing && playMode == PlayMode::Practice && practiceSectionASet &&
            practiceSectionBSet && practiceSectionB > practiceSectionA && songTime >= practiceSectionB) {
            seekPracticePosition(practiceSectionA, true);
            chartAdvancing = false;
        }
        const unsigned seats=gameplaySeatMask?gameplaySeatMask:(1u<<gameplayPlayer);
        bool failed=false;
        for(int seat=0;seat<PAD_PLAYERS;++seat)if(seats&(1u<<seat)) {
            selectGameplaySeat(seat);
            updateGameplaySeat(chartAdvancing && !resumeRewind.protectedAt(songClock.gameplayTime()) && !failed,gameplayDelta);
            failed=failed || songFailed;
        }
        // Library sweeps preserve every logical sample and engine step while
        // drawing only each batch endpoint. This is scoring/parser evidence,
        // not a display, audio synchronization or physical latency measurement.
        if(ps5.frameBudget && seats==1 && !multiplayerQa && chartAdvancing &&
           gameplayReplayBatch>1 && !gameplayReplay.empty()) {
            for(unsigned sample=1;sample<gameplayReplayBatch &&
                gameplayReplayIndex<gameplayReplay.size();++sample) {
                const double before=songTime;
                songTime=renderTime=gameplayReplay[gameplayReplayIndex].time;
                updateGameplaySeat(true,std::max(0.0,songTime-before));
            }
        }
        double audioEnd=0;
#if !defined(PS5_HOST_REFERENCE)
        if(songAudio.ready())audioEnd=songAudio.durationSeconds()-trackStartOffsetSeconds;
#endif
        const double finishTime=rhythm::pcSongFinishTime(globalChartEndSeconds,audioEnd,authoredSongEndSeconds);
        const double timeline=gameplayReplay.empty() && !multiplayerQaReplay?songClock.gameplayTime():songTime;
        // Engine probes intentionally keep sampling after the authored chart.
        // Normal launches always use the shared PC song-completion deadline.
        const bool replayPending=ps5.frameBudget && (gameplayReplayIndex<gameplayReplay.size() ||
            (multiplayerQaReplay && multiplayerQaStep+1<multiplayerQaInputs[primaryGameplaySeat].size()));
        if(failed || (chartAdvancing && !replayPending && timeline>=finishTime)) {
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(seats&(1u<<seat)) {
                selectGameplaySeat(seat);songFailed=failed;resultsSelection=0;resultsScroll=0;
                newHighScore=!failed && !activeProfile.bot && playMode!=PlayMode::Practice && score>bestScore;
                if(!failed && !activeProfile.bot && playMode!=PlayMode::Practice && score>=bestScore) {
                    bestScore=score;saveBestScore(activeChartPath,chartInstrument,chartDifficulty,bestScore);
                }
            }
            playing=false;screen=Screen::Results;ui.visible=true;
        }
        if(multiplayerQaReplay && chartAdvancing && screen==Screen::Gameplay)++multiplayerQaStep;
        selectGameplaySeat(primaryGameplaySeat);
#if !defined(PS5_HOST_REFERENCE)
        applyAudioGains();
#endif
    }

    void updateGameplaySeat(bool chartAdvancing,double gameplayDelta) {
        // Each engine advances on its calibrated timeline, including automatic
        // misses. Audio, highway rendering and the other seats keep shared time.
        struct RestoreTime {double& time;double value;~RestoreTime(){time=value;}} restore{songTime,songTime};
        if(!activeProfile.bot && gameplayReplay.empty() && !multiplayerQa)
            songTime=rhythm::playerJudgementTime(songTime,activeProfile.inputOffsetsMs[std::clamp(activeProfile.controller,0,4)],
                playMode==PlayMode::Practice?practiceRate:1.0);

        struct pad input{};const bool connected=readPlayerInput(gameplayPlayer,&input);
        const auto* usb=playerUsb(gameplayPlayer);
        const bool drumController=chart.kind==rhythm::ChartKind::Drums;
        rhythm::controls::NativeSample native{input.held,{input.l2*255.0f,input.r2*255.0f,
            input.left_x*127.0f+128.0f,input.left_y*127.0f+128.0f,input.right_x*127.0f+128.0f,input.right_y*127.0f+128.0f}};
        auto logicalInstrument=rhythm::controls::applyProfile(activeControllerProfile,native,usb?usb->report.data():nullptr,usb?usb->length:0);
        if(activeProfile.controller==3)normalizeFiveLaneInput(logicalInstrument);
        if(midiLinks[gameplayPlayer].source>=0){logicalInstrument.pads=0;logicalInstrument.velocityKnown=0;logicalInstrument.starPower=bool(input.held&PAD_TOUCH_PAD);}
        bool requestStarPower=(logicalInstrument.starPower && !gameplayStarHeld[gameplayPlayer]) || activeProfile.bot;
        gameplayStarHeld[gameplayPlayer]=logicalInstrument.starPower;
        requestStarPower=requestStarPower && chartAdvancing && (playMode!=PlayMode::Practice || practiceStarPowerEnabled);
        frets=logicalInstrument.frets;
        if(drumController) {
            frets=0;constexpr unsigned visualLanes[]{4,1,2,3,0,2,3,0};
            for(unsigned bit=0;bit<8;++bit)if(logicalInstrument.pads&(1u<<bit))frets|=1u<<visualLanes[bit];
        }
        for(auto& glow:hitGlow)glow=std::max(0.0f,glow-frameTimer*5.0f);
        rhythm::GuitarButtons guitar{static_cast<uint16_t>(frets),logicalInstrument.strumUp,logicalInstrument.strumDown};
        const bool replaying=gameplayReplayIndex<gameplayReplay.size();
        if(replaying) {
            const auto& frame=gameplayReplay[gameplayReplayIndex];frets=frame.frets;
            guitar={static_cast<uint16_t>(frets),frame.up!=0,frame.down!=0};requestStarPower=frame.sp!=0;
        }
        const auto* multiFrame=multiplayerQaReplay?&multiplayerQaInputs[gameplayPlayer][std::min(multiplayerQaSample,multiplayerQaInputs[gameplayPlayer].size()-1)]:nullptr;
        if(multiFrame) {
            frets=multiFrame->frets;guitar={static_cast<uint16_t>(frets),multiFrame->up!=0,multiFrame->down!=0};
            requestStarPower=multiFrame->sp!=0;logicalInstrument.pads=static_cast<uint16_t>(frets);
        }
        session.setGamepadMode(gamepadMode || drumController || (replaying && (gameplayReplayPcModifiers&0x100000)),replaying && (gameplayReplayPcModifiers&0x200000));
        session.setAutoStrum(replaying && (gameplayReplayPcModifiers&0x4000));
        session.setAllStrums(rhythm::allStrumsRequired(playerModifierMask[gameplayPlayer]));
        session.setNoFretGhosting((playerModifierMask[gameplayPlayer]&(1u<<6))!=0);
        session.setExactFrets(drumController);session.setPrecisionMode((playerModifierMask[gameplayPlayer]&(1u<<7))!=0);
        session.setStrumDebounce(strumDebounceMs/1000.0);
        session.setHitWindow(hitWindowMs/1000.0);
        if(chartAdvancing) {
            drumFxClock+=gameplayDelta;guitarFxClock+=gameplayDelta;guitarParticles.update(static_cast<float>(gameplayDelta));
        }
        const auto settleStep=[&](const rhythm::GuitarButtons& state,double stepTime) {
        advanceSustainScore(stepTime,true);
        for (const auto index:activeSustainNotes) {
            auto& n=notes[index];
            if (n.hit && !n.sustainDropped && n.sustainSec > 0 && n.heldUntil < n.timeSec + n.sustainSec) {
                const bool laneHeld = activeProfile.bot || gamepadMode ||
                    (drumController ? (n.lane==5 ? state.frets==0 : (state.frets&(1u<<n.lane))!=0) :
                     n.extended ? rhythm::pcExtendedTailHeld(n.sustainGroupMask,state.frets,guitarLaneCount()) :
                     rhythm::GuitarSession::sustainFretsMatch(chart.notes[n.chordIndex],state.frets,chart.kind));
                n.sustainGrace.sample(stepTime,laneHeld);
                n.sustainDropped=n.sustainGrace.dropped();
                if(laneHeld && !n.sustainDropped) {
                    const double until = std::min(stepTime, n.timeSec + n.sustainSec);
                    const double from = std::max(n.timeSec, n.heldUntil);
                    if (until > from) {
                        n.heldUntil = until;
                    }
                }
            }
        }
        // Recovery during release grace catches up at the current multiplier.
        advanceSustainScore(stepTime,true);
        };
        const auto processJudgements=[&](std::span<const rhythm::GuitarEvent> events,double stepTime,bool activate) {
            bool activationProcessed=false;
            for(const auto& event:events) {
                if(songFailed) break;
                inputDiagnostics.judgement(int(event.kind),event.time,int64_t(event.noteIndex),
                    event.noteIndex<chart.notes.size()?(event.time-chart.notes[event.noteIndex].time)*1000.0:0.0);
                advanceSustainScore(event.time,false);
                if(!activationProcessed && event.time>=stepTime) {
                    activationProcessed=true;
                    if(activate && activateStarPower(stepTime))
                        songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarDeployed);
                }
                if(event.kind!=rhythm::GuitarEventKind::Hit) {
                    if(event.kind==rhythm::GuitarEventKind::Ghost)++resultStats.ghosts;
                    if(event.kind==rhythm::GuitarEventKind::Overstrum)++resultStats.overstrums;
                    if(event.kind==rhythm::GuitarEventKind::Miss) {
                        judgeSoloHead(event.noteIndex,false);
                        // BaseEngine miss 0x1820EF950 breaks health once per
                        // chord parent, including disjoint child sustains.
                        applyPcHealthChange(false);
                        for(size_t i=chordNoteOffsets[event.noteIndex];i<chordNoteOffsets[event.noteIndex+1];++i) {
                            auto& note=notes[i];note.missed=true;
                            for(auto& phrase:starPhrases)
                                if(note.timeSec>=phrase.startSec && note.timeSec<phrase.endSec) phrase.failed=true;
                        }
                        if(muteOnMiss) missMuted=true;
                        if(highwayShakeEnabled) highwayShakeTime=0.30f;
                    } else {
                        applyPcHealthChange(false);
                        if(event.kind==rhythm::GuitarEventKind::Overstrum)
                            for(auto& phrase:starPhrases)
                                if(event.time>=phrase.startSec && event.time<phrase.endSec) phrase.failed=true;
                    }
                    if(combo>=10) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::LoseMultiplier);
                    combo=0;
                    if(event.kind==rhythm::GuitarEventKind::Ghost) songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::GhostDecrease);
                    else songAudio.playUiSound(static_cast<rhythm::AudioStemMixer::UiSound>(static_cast<size_t>(rhythm::AudioStemMixer::UiSound::BadNote1)+(badNoteVariant++%5)));
                    advanceSustainScore(event.time,true);
                    // Guitar overstrum dispatch retires held tails. A plain
                    // missed head still permits other extended tails to score.
                    if(event.kind==rhythm::GuitarEventKind::Overstrum)for(const auto index:activeSustainNotes) {
                        auto& tail=notes[index];
                        // Completed tails still supply the PC recent-fret mask.
                        if(!tail.hit || tail.sustainDropped || tail.sustainPointsAwarded>=
                           rhythm::pc_scoring::sustainPoints(tail.lengthTicks,tail.lengthTicks,chartResolutionTicks))continue;
                        tail.sustainDropped=true;
                        tail.heldUntil=std::min(event.time,tail.timeSec+tail.sustainSec);
                    }
                    if(shouldFailSong()) {
                        songFailed=true;playing=false;chartAdvancing=false;
                        screen=Screen::Results;ui.visible=true;resultsSelection=0;resultsScroll=0;newHighScore=false;
                        songAudio.pause();
                        hits=misses=0;
                        for(const auto& note:notes) {
                            if(note.hit) ++hits;
                            else if(note.missed) ++misses;
                        }
                    }
                    continue;
                }
                const rhythm::HitResult hit{event.noteIndex,event.time-chart.notes[event.noteIndex].time};

                const double hitTime=chart.notes[hit.noteIndex].time+hit.error;
                missMuted = false;
                applyPcHealthChange(true);
                if (controllerVibration && rumbleStrength > 0.0f) {
                    vibratePlayer(gameplayPlayer,rumbleStrength * 0.38f, rumbleStrength);
                    rumbleActive = true;
                    rumbleStopTime = now_seconds() + 0.045;
                }
                if(!drumController) settleSustainsForHead(hit.noteIndex,hitTime);
                const unsigned gems=static_cast<unsigned>(chordNoteOffsets[hit.noteIndex+1]-chordNoteOffsets[hit.noteIndex]);
                if(drumController){++combo;bestCombo=std::max(bestCombo,combo);score+=uint64_t(event.fretBonus)*rhythm::pc_scoring::multiplier(combo,starPower.active());}
                else score+=rhythm::pc_scoring::hit(gems,combo,bestCombo,starPower.active());
                const double authored=chart.notes[hit.noteIndex].time;
                const bool active=starPower.active();
                const bool positive=active && ((authored<=resultStats.spStart && resultStats.spStart<=hitTime) ||
                    (resultStats.spEnd<authored && hitTime<resultStats.spEnd));
                const double window=session.noteWindow(hit.noteIndex).early;
                const bool negative=!active && authored-window<=resultStats.lastSpEnd && resultStats.lastSpEnd<authored+window;
                resultStats.award(drumController?event.fretBonus:50u*gems,combo,active,false,positive,negative);
                if(!drumController) {score+=event.fretBonus;fretBonusTotal+=event.fretBonus;}
                for (size_t i = chordNoteOffsets[hit.noteIndex]; i < chordNoteOffsets[hit.noteIndex+1]; ++i) {
                    auto& n = notes[i];
                    n.hit = true;
                    if(n.sustainSec>0) {
                        const auto at=std::lower_bound(activeSustainNotes.begin(),activeSustainNotes.end(),i);
                        if(at==activeSustainNotes.end() || *at!=i)activeSustainNotes.insert(at,i);
                    }
                    n.heldUntil = std::max(hitTime, n.timeSec);
                    n.sustainScoreStartBeat = chartBeatAt(n.heldUntil);
                    n.sustainPointsAwarded = 0;
                    n.sustainGrace = {};
                    if(n.lane < hitGlow.size())hitGlow[n.lane]=1;
                    if(!drumController && n.lane<guitarLaneCount()+1) {
                        guitarHitAt[n.lane]=guitarFxClock;
                        guitarHitActive[n.lane]=starPower.active();guitarHitPhrase[n.lane]=false;
                        if(chart.kind!=rhythm::ChartKind::SixFret && n.lane<5 && n.sustainSec<=0.05) {
                            const auto& emitter=pc_guitar_fx::noteParticles[n.lane];
                            for(unsigned burst=0;burst<static_cast<unsigned>(emitter.burst);++burst)
                                guitarParticles.emit(emitter,n.lane,starPower.active(),false);
                        }
                    }
                    for (auto& phrase : starPhrases)
                        if (n.timeSec >= phrase.startSec && n.timeSec < phrase.endSec)
                            ++phrase.hitCount;
                }
                const bool wasSpReady=starPower.available(),wasSpActive=starPower.active();
                awardCompletedStarPhrases();
                // PC neck param3 is BaseEngine+0x45: successful phrase award,
                // not the note's membership in a star-power phrase.
                for(size_t i=chordNoteOffsets[hit.noteIndex];i<chordNoteOffsets[hit.noteIndex+1];++i) {
                    const auto& note=notes[i];
                    if(note.starPhraseIndex>=0) {
                        const auto& phrase=starPhrases[size_t(note.starPhraseIndex)];
                        if(phrase.awarded && !phrase.failed && note.lane<6){
                            if(drumController){
                                auto first=hit.noteIndex;const auto time=chart.notes[first].time;
                                while(first>0 && chart.notes[first-1].time==time)--first;
                                for(auto head=first;head<chart.notes.size() && chart.notes[head].time==time;++head){
                                    const unsigned lane=std::countr_zero(static_cast<unsigned>(chart.notes[head].frets));
                                    const unsigned pad=drumVisualPad(lane);
                                    if(pad<drumHitAt.size()){
                                        drumPhraseHitAt[pad]=drumFxClock;
                                        if(gameplayReplayTrace)std::fprintf(gameplayReplayTrace,"# drum_phrase %u %.9g %d\n",pad,drumFxClock,note.starPhraseIndex);
                                    }
                                }
                            } else {guitarHitPhrase[note.lane]=true;guitarPhraseHitAt[note.lane%5]=guitarFxClock;}
                        }
                    }
                }
                if(drumController && !wasSpReady && !wasSpActive && starPower.available())
                    drumSession.starPowerBecameAvailable(chartTickAt(event.time));
                judgeSoloHead(event.noteIndex,true);
                advanceSustainScore(event.time,true);
            }
            if(!songFailed) {
                advanceSustainScore(stepTime,false);
                if(!activationProcessed && activate && activateStarPower(stepTime))
                    songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarDeployed);
            }
        };
        if (!chartAdvancing || !wasAdvancing || !inputPrimed || connected != wasConnected) {
            session.prime(guitar);drumSession.prime();drumPadEdges.prime(replaying?(gameplayReplayCanonicalPads?static_cast<uint16_t>(frets):rhythm::pc_drums::fallbackPads(static_cast<uint16_t>(frets))):logicalInstrument.pads);
            inputPrimed = true;
            wasConnected = connected;
            lastJudgementInputTime=songTime;rawStarHeld=logicalInstrument.starPower;
        } else {
            const auto refreshSustains=[&](double stepTime) {
            uint16_t sustainMask=0,recentMask=0;
            for(const auto index:activeSustainNotes) {
                auto& n=notes[index];
                if(!n.hit || n.sustainDropped || !n.extended || n.lane>=guitarLaneCount() || n.lengthTicks<=0)continue;
                n.sustainGrace.advance(stepTime,false);
                if(n.sustainGrace.dropped()) {n.sustainDropped=true;continue;}
                if(n.sustainPointsAwarded<rhythm::pc_scoring::sustainPoints(n.lengthTicks,n.lengthTicks,chartResolutionTicks)) sustainMask|=1u<<n.lane;
                else if(stepTime<n.timeSec+n.sustainSec+0.10000000149011612) recentMask|=1u<<n.lane;
            }
            session.setSustainMasks(sustainMask,recentMask);
            };
            if(drumController) {
                const auto judgeDrums=[&](const rhythm::controls::InstrumentState& logical,double endpoint,bool deploy,bool bot,const rhythm::DrumInput* direct=nullptr) {
                    rhythm::DrumInput input;
                    input.strikes=direct || multiFrame || (replaying && gameplayReplayCanonicalPads)?logical.pads:drumPadEdges.sample(logical.pads);
                    if(multiFrame)input.velocities=multiFrame->velocities;
                    else if(replaying && gameplayReplayCanonicalPads)input.velocities=gameplayReplay[gameplayReplayIndex].velocities;
                    else for(unsigned bit=0;bit<8;++bit)if(logical.velocityKnown&(1u<<bit)) {
                        const int lane=rhythm::pc_drums::velocityLane(1u<<bit);
                        if(lane>=0)input.velocities[lane]=static_cast<int>(std::lround(std::clamp(logical.velocities[bit],0.0f,1.0f)*127));
                    }
                    if(direct)input=*direct;
                    drumSession.setOptions(chartProDrums,activeProfile.lefty,(playerModifierMask[gameplayPlayer]&0x80)!=0,
                        replaying && (gameplayReplayPcModifiers&0x10),100);
                    drumSession.update(input,endpoint,[&](double at){
                        advanceSustainScore(at,false);
                        if(at==endpoint && deploy && activateStarPower(at))
                            songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarDeployed);
                        drumSession.setStarPowerAvailable(starPower.available() &&
                            (playMode!=PlayMode::Practice || practiceStarPowerEnabled));
                    },[&](const rhythm::DrumEvent& event){
                        if(songFailed)return;
                        if(event.kind==rhythm::DrumEventKind::Activate){
                            if(activateStarPower(event.time))songAudio.playUiSound(rhythm::AudioStemMixer::UiSound::StarDeployed);
                            return;
                        }
                        if(event.kind==rhythm::DrumEventKind::Hit){
                            const unsigned lane=std::countr_zero(static_cast<unsigned>(chart.notes[event.noteIndex].frets));
                            const unsigned pad=drumVisualPad(lane);
                            if(pad<drumHitAt.size()){
                                drumHitAt[pad]=drumFxClock;drumHitActive[pad]=starPower.active();
                                const auto flags=chart.notes[event.noteIndex].drumFlags;
                                if(event.points>=100) {
                                    if(flags&rhythm::pc_drums::ghost)++resultStats.ghostPads;
                                    if(flags&rhythm::pc_drums::accent)++resultStats.accents;
                                }
                                drumHitDynamics[pad]=event.points>=100?((flags&rhythm::pc_drums::ghost)?1u:(flags&rhythm::pc_drums::accent)?2u:0u):0u;
                                if(pad)drumFretMotion[pad-1].strike(event.time);
                                else for(auto& motion:drumFretMotion)motion.strike(event.time);
                            }
                        }
                        const auto kind=event.kind==rhythm::DrumEventKind::Hit?rhythm::GuitarEventKind::Hit:
                            event.kind==rhythm::DrumEventKind::Miss?rhythm::GuitarEventKind::Miss:rhythm::GuitarEventKind::Overstrum;
                        const rhythm::GuitarEvent adapted{kind,event.noteIndex,event.time,event.points};
                        processJudgements(std::span<const rhythm::GuitarEvent>(&adapted,1),event.time,false);
                        drumSession.setStarPowerAvailable(starPower.available() &&
                            (playMode!=PlayMode::Practice || practiceStarPowerEnabled));
                    },bot);
                    settleStep(guitar,endpoint);
                    lastJudgementInputTime=endpoint;
                };
                auto endpoint=logicalInstrument;
                if(replaying)endpoint.pads=gameplayReplayCanonicalPads?static_cast<uint16_t>(frets):rhythm::pc_drums::fallbackPads(static_cast<uint16_t>(frets));
                bool sampledRaw=false;
                if(!replaying && usb && !activeProfile.bot && activeControllerProfile.source==rhythm::controls::Source::RawHid) {
                    const auto& batch=usbInputBatches[playerSeats.slotOfSeat[gameplayPlayer]-4];
                    const auto nowUs=static_cast<uint64_t>(now_seconds()*1000000.0);
                    if(batch.dropped) {
                        rawInputDrops+=batch.dropped;inputDiagnostics.dropped+=batch.dropped;drumSession.prime();drumPadEdges.prime(endpoint.pads);
                        lastJudgementInputTime=songTime;rawStarHeld=logicalInstrument.starPower;sampledRaw=true;
                    } else for(unsigned i=0;i<batch.count && !songFailed;++i) {
                        const auto& report=batch.reports[i];
                        auto changed=rhythm::controls::applyProfile(activeControllerProfile,{},report.bytes.data(),report.length);
                        if(activeProfile.controller==3)normalizeFiveLaneInput(changed);
                        const auto at=rhythm::controls::rawInputSongTime(report.receivedUs,nowUs,songTime,lastJudgementInputTime,
                            playMode==PlayMode::Practice?practiceRate:1.0);
                        const bool deploy=changed.starPower && !rawStarHeld && (playMode!=PlayMode::Practice || practiceStarPowerEnabled);
                        rawStarHeld=changed.starPower;
                        inputDiagnostics.sample(at,changed.pads,false,false,nowUs>report.receivedUs?nowUs-report.receivedUs:0);
                        judgeDrums(changed,at,deploy,false);sampledRaw=true;
                    }
                }
                const int midiSource=midiSourceForSeat(gameplayPlayer);
                if(!replaying && !activeProfile.bot && midiSource>=0){
                    // MIDI packets are independent strikes, not held pad states.
                    // Preserve timestamps and repeated Note On events, including
                    // two kick notes and consecutive strikes in the same frame.
                    endpoint.pads=0;endpoint.velocityKnown=0;
                    const auto& batch=midiBatches[midiSource];
                    const auto nowUs=uint64_t(now_seconds()*1000000.0);
                    for(unsigned i=0;i<batch.count && !songFailed;++i)
                        midiLinks[gameplayPlayer].profile.mapping.apply(batch.strikes[i],[&](const rhythm::midi::MappedStrike& event){
                            const auto strike=rhythm::midi::gameplayInput(event);
                            const auto at=rhythm::controls::rawInputSongTime(event.receivedUs,nowUs,songTime,lastJudgementInputTime,
                                playMode==PlayMode::Practice?practiceRate:1.0);
                            inputDiagnostics.sample(at,event.mask,false,false,nowUs>event.receivedUs?nowUs-event.receivedUs:0);
                            judgeDrums({},at,false,false,&strike);
                        });
                }
                if(!songFailed)judgeDrums(endpoint,songTime,!sampledRaw && requestStarPower,activeProfile.bot);
            } else if(activeProfile.bot) {
                refreshSustains(songTime);
                (void)session.updateBot(songTime);
                processJudgements(session.events(),songTime,requestStarPower);
                settleStep(guitar,songTime);
            } else {
                const auto judgeInput=[&](const rhythm::GuitarButtons& state,double at,bool deploy) {
                    (void)session.update(state,at,[&](const auto& sampled,double stepTime,bool before,std::span<const rhythm::GuitarEvent> events) {
                        if(songFailed)return;
                        if(before){advanceSustainScore(stepTime,false);refreshSustains(stepTime);}
                        else {processJudgements(events,stepTime,stepTime==at && deploy);settleStep(sampled,stepTime);}
                    });
                    lastJudgementInputTime=at;
                };
                bool sampledRaw=false;
                if(!replaying && usb && activeControllerProfile.source==rhythm::controls::Source::RawHid) {
                    const int source=playerSeats.slotOfSeat[gameplayPlayer]-4;
                    const auto& batch=usbInputBatches[source];
                    const auto nowUs=static_cast<uint64_t>(now_seconds()*1000000.0);
                    if(batch.dropped) {
                        rawInputDrops+=batch.dropped;inputDiagnostics.dropped+=batch.dropped;
                        // A discontinuous history cannot establish safe edges.
                        // Prime once rather than manufacture hits/overstrums.
                        session.prime(guitar);lastJudgementInputTime=songTime;
                        rawStarHeld=logicalInstrument.starPower;sampledRaw=true;
                    } else for(unsigned i=0;i<batch.count && !songFailed;++i) {
                        const auto& report=batch.reports[i];
                        const auto changed=rhythm::controls::applyProfile(activeControllerProfile,{},report.bytes.data(),report.length);
                        const auto at=rhythm::controls::rawInputSongTime(report.receivedUs,nowUs,songTime,lastJudgementInputTime,
                            playMode==PlayMode::Practice?practiceRate:1.0);
                        const bool deploy=changed.starPower && !rawStarHeld && (playMode!=PlayMode::Practice || practiceStarPowerEnabled);
                        rawStarHeld=changed.starPower;
                        inputDiagnostics.sample(at,changed.frets,changed.strumUp,changed.strumDown,nowUs>report.receivedUs?nowUs-report.receivedUs:0);
                        judgeInput({changed.frets,changed.strumUp,changed.strumDown},at,deploy);sampledRaw=true;
                    }
                }
                if(!songFailed)judgeInput(guitar,songTime,!sampledRaw && requestStarPower);
            }

        }
        if(chartAdvancing && !drumController && chart.kind!=rhythm::ChartKind::SixFret) {
            guitarSustaining.fill(false);
            for(const auto index:activeSustainNotes) {
                const auto& n=notes[index];
                if(n.hit && n.lane<5 && !n.sustainDropped && n.sustainSec>0.05 &&
                    songTime>=n.timeSec && songTime<n.timeSec+n.sustainSec)guitarSustaining[n.lane]=true;
            }
            for(unsigned lane=0;lane<5;++lane) {
                if(!guitarSustaining[lane]) {guitarEmitRemainder[lane]=0;continue;}
                const auto& emitter=pc_guitar_fx::sustainParticles[lane];
                guitarEmitRemainder[lane]+=frameTimer*emitter.rate*emitter.simulation;
                const unsigned count=static_cast<unsigned>(guitarEmitRemainder[lane]);
                guitarEmitRemainder[lane]-=count;
                for(unsigned i=0;i<std::min(count,32u);++i)guitarParticles.emit(emitter,lane,starPower.active(),true);
            }
        }
        if (rumbleActive && (!controllerVibration || now_seconds() >= rumbleStopTime)) {
            vibratePlayer(gameplayPlayer,0.0f, 0.0f);
            rumbleActive = false;
        }
        highwayShakeTime = std::max(0.0f, highwayShakeTime - static_cast<float>(frameTimer));
        wasAdvancing = chartAdvancing;
        if (!chartAdvancing) return;
        std::erase_if(activeSustainNotes,[&](size_t index) {
            const auto& n=notes[index];
            return n.sustainDropped || (songTime>n.timeSec+n.sustainSec+0.10000000149011612 &&
                n.sustainPointsAwarded>=rhythm::pc_scoring::sustainPoints(n.lengthTicks,n.lengthTicks,chartResolutionTicks));
        });
        if(activeProfile.bot) {
            frets=0;
            for(const auto& note:notes)
                if(note.hit && note.lane<guitarLaneCount() && songTime>=note.timeSec &&
                   songTime<=note.timeSec+std::max(note.sustainSec,0.075)) frets|=1u<<note.lane;
        }
        if(drumController)for(size_t pad=0;pad<(fiveLaneDrumView()?5u:4u);++pad)drumFretOffset[pad]=drumFretMotion[pad].sample(songTime);
        // End-of-song stats are counted per playable gem, not per chord event.
        // The session judgement groups simultaneous gems into one chord result.
        hits = misses = 0;
        for (const auto& n : notes) {
            if (n.hit) ++hits;
            else if (n.missed) ++misses;
        }
        if(replaying) {
            if(gameplayReplayTrace) {
                unsigned headHits=0,headMisses=0;
                for(size_t i=0;i<chart.notes.size();++i) {
                    headHits+=gameplayEvaluator().state(i)==rhythm::NoteState::Hit;
                    headMisses+=gameplayEvaluator().state(i)==rhythm::NoteState::Missed;
                }
                std::fprintf(gameplayReplayTrace,"%.17g\t%u\t%u\t%u\t%u\t%llu\t%u\t%u\t%u\t%llu\t%u\t%u\t%lld\t%d\t%llu\t%llu\t%.9g\t%u\t%.9g\n",
                    songTime,frets,guitar.strumUp,guitar.strumDown,requestStarPower,
                    (unsigned long long)score,combo,headHits,headMisses,(unsigned long long)soloBonusTotal,session.ghostCount(),session.fretCharge(),static_cast<long long>(starPower.remainingTicks()),starPower.active()?1:0,
                    (unsigned long long)(score-soloBonusTotal-fretBonusTotal),(unsigned long long)fretBonusTotal,gameplayHealth,currentStarRank(),starProgressToNextRank());
            }
            if(++gameplayReplayIndex==gameplayReplay.size()) {
                if(gameplayReplayTrace) {std::fclose(gameplayReplayTrace);gameplayReplayTrace=nullptr;}
                playing=false;
            }
        }
    }

    // PC NoteAnimator uses independent accumulators and advances the global
    // note/star frame counters by floor(accumulator / (1 / configured FPS)).
    // GameAssembly NoteAnimator::Start reads each interval as 1 / setting;
    // Update accumulates Unity deltaTime and wraps each counter by its frame
    // family length. Keep these clocks independent from chart/audio time.
    void updatePcNoteAnimations() noexcept {
        if (screen != Screen::Gameplay || ui.visible || !std::isfinite(frameTimer) || frameTimer <= 0.0)
            return;
        const auto advance = [this](double& remainder, size_t& frame, float& blend,
                                    int fps, size_t frameCount) {
            if (fps <= 0 || frameCount == 0) return;
            const double interval = 1.0 / static_cast<double>(fps);
            remainder += std::min(static_cast<double>(frameTimer), 0.25);
            const size_t steps = static_cast<size_t>(remainder / interval);
            if (steps != 0) {
                frame = (frame + steps % frameCount) % frameCount;
                remainder -= static_cast<double>(steps) * interval;
            }
            blend = static_cast<float>(std::clamp(remainder / interval, 0.0, 1.0));
        };
        advance(noteAnimationRemainder, noteAnimationFrame, noteAnimationBlend,
                noteAnimationFps, std::size(pc_guitar::bodyFrames));
        advance(starAnimationRemainder, starAnimationFrame, starAnimationBlend,
                starAnimationFps, std::size(pc_guitar::starBodyFrames));
    }

    void getEnabledFeatures() override
    {
        // Highway surfaces use the original PC texture import's 16x filtering.
        enabledFeatures.samplerAnisotropy=deviceFeatures.samplerAnisotropy;
    }

    VulkanExample() : VulkanExampleBase()
    {
        title = "CH: Encore";
        apiVersion = VK_API_VERSION_1_3;
        useDynamicRendering = true;
        requiresStencil = true;
        deviceCreatepNextChain = &baseDynamicRenderingFeatures;
        ps5.customOverlay = true;
        ps5.ownOverlay = true;
        ps5.optionsEnds = false;
    }

    ~VulkanExample()
    {
        rhythm::logDiagnostic("shutdown begin");
#if !defined(PS5_HOST_REFERENCE)
        if(searchKeyboardOpen && nativeImePreferred) {
            nativeKeyboard.cancel();
            for(int i=0;i<100 && nativeKeyboard.active();++i) {
                nativeKeyboard.poll();
                if(nativeKeyboard.active())platform_sleep_us(10000);
            }
            searchKeyboardOpen=false;
        }
        if(searchKeyboardOpen) {
            if(sceImeDialogGetStatus()==1)(void)sceImeDialogAbort();
            // Shutdown is outside the render loop. A bounded wait keeps the
            // dialog's pointers alive while cancellation reaches the service.
            for(unsigned wait=0;wait<100;++wait) {
                const int status=sceImeDialogGetStatus();
                if(status==0)break;
                if(status==2 && sceImeDialogTerm()>=0 && sceImeDialogGetStatus()==0)break;
                platform_sleep_us(10000);
            }
            searchKeyboardOpen=false;
        }
#endif
        usbMidi.stop();
        usbControllers.stop();
        storageCancel=true;if(storageThreadActive){(void)pthread_join(storageThread,nullptr);storageThreadActive=false;}
        if(gameplayReplayTrace) std::fclose(gameplayReplayTrace);
#if !defined(PS5_HOST_REFERENCE)
        if (previewAudioThreadActive) {
            previewAudioCancel=true;
            (void)pthread_join(previewAudioThread, nullptr);
            previewAudioThreadActive = false;
        }
        stopAudioOutput();
#endif
        if (device) {
            vkDeviceWaitIdle(device);
            if(renderTimestampPool)vkDestroyQueryPool(device,renderTimestampPool,nullptr);
            destroyParkedGameplayBuffers();
            vkDestroyPipeline(device, highwayGlowPipeline, nullptr);
            vkDestroyPipeline(device, texPipeline, nullptr);
            vkDestroyPipeline(device, sustainPipeline, nullptr);
            vkDestroyPipeline(device, boardPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            for (auto& b : texBuffers) b.destroy();
            for (auto& b : sustainBuffers) b.destroy();
            for (auto& b : boardBuffers) b.destroy();
            for (auto& b : uniformBuffers) b.destroy();
            for(auto pipeline:{meterPipeline,meterGlowPipeline,spMeterPipeline}) vkDestroyPipeline(device,pipeline,nullptr);
            for(auto& buffer:meterBuffers) buffer.destroy();
            destroyTex(meterTex);
            destroyTex(highwayTex);
            destroyTex(beatTex);
            destroyTex(atlasTex);destroyTex(drumAtlasTex);destroyTex(sixFretTex);destroyTex(guitarFxTex);
            destroyTex(boardTex);
            for(auto& texture:highwayStyleTextures)destroyTex(texture);
            for (auto& background : menuBackgroundTextures) destroyTex(background);
            for (auto& icon : downloaderInstrumentTextures) destroyTex(icon);
            for (auto& icon : chartTrackExtraTextures) destroyTex(icon);
            destroyTex(fiveLaneDrumsInstrumentTexture);
            destroyTex(rhythmInstrumentTexture);
            destroyTex(downloaderCoverTex);
            destroyTex(menuLogoTex);
            destroyTex(menuButtonTex);
            destroyTex(menuAlbumTex);
            destroyTex(menuPlayerPanelTex);
            for(auto& texture:menuDetailTextures) destroyTex(texture);
            for(auto& texture:sourceScoreTextures) destroyTex(texture);
            destroyTex(scorePanelTex);
            destroyTex(starPowerBaseTex);
            destroyTex(starPowerFillTex);
            destroyTex(failMeterBaseTex); destroyTex(failMeterUnderTex); destroyTex(hudArrowTex);
            for (auto& texture : failMeterGlowTextures) destroyTex(texture);
            destroyTex(multiplierBackgroundTex); destroyTex(multiplierOverlayTex); destroyTex(multiplierXTex);
            for (auto& texture : scoreGlyphTextures) destroyTex(texture);
            for (auto& texture : multiplierGlyphTextures) destroyTex(texture);
            for(auto& texture:resultsUiTextures)destroyTex(texture);
            destroyTex(starBadgeGoldTex); destroyTex(starBadgeGreyTex);
            destroyTex(starOutlineGoldTex); destroyTex(starOutlineGreyTex);
            destroyTex(starGlowTex); destroyTex(starProgressBackTex); destroyTex(starProgressFillTex);
        }
        rhythm::stopDiagnostics();
    }
    void destroyTex(Tex& t) {
        if (t.sampler) vkDestroySampler(device, t.sampler, nullptr);
        if (t.view)    vkDestroyImageView(device, t.view, nullptr);
        if (t.image)   vkDestroyImage(device, t.image, nullptr);
        if (t.mem)     vkFreeMemory(device, t.mem, nullptr);
    }

    // Source sliced texture: extend the shaft while preserving the rounded cap.
    void sustainQuad(std::vector<TexVertex>& v, float x, float rootZ, float length,
                     int state, const float* color, bool open = false) {
        const auto& region = open ? pc_guitar::openTailRegions[state == 1 ? 1 : 0] :
                                    pc_guitar::tailRegions[state];
        const float halfWidth = open ? pc_guitar::openTailHalfWidth :
            pc_guitar::tailWidth * pc_guitar::worldScale * 0.5f;
        rhythm::submitSustainSections(rootZ, length, pc_guitar::worldScale,
            pc_guitar::tailSliceHeight, region.topBorder, region.spriteHeight,
            HIGHWAY_FAR, HIGHWAY_NEAR, [&](const rhythm::TailSection& section) {
                if (v.size() + 6 > MAX_TEX_VERTS) return;
                const auto atlasV = [&](float t) { return region.uv[1] + t*(region.uv[3]-region.uv[1]); };
                const float nearV=atlasV(section.nearV), farV=atlasV(section.farV);
                const float positions[4][3] = {{x-halfWidth,0.015f,section.nearZ},
                    {x+halfWidth,0.015f,section.nearZ},{x+halfWidth,0.015f,section.farZ},
                    {x-halfWidth,0.015f,section.farZ}};
                const float uv[4][2] = {{region.uv[0],nearV},{region.uv[2],nearV},
                    {region.uv[2],farV},{region.uv[0],farV}};
                for (int i : {0,1,2,0,2,3})
                    v.push_back({{positions[i][0],positions[i][1],positions[i][2]},
                        {uv[i][0],uv[i][1]},{color[0],color[1],color[2],1}});
            });
    }

    bool loadRGBA(const std::string& path, std::vector<unsigned char>& out, int& w, int& h) {
        int n = 0; unsigned char* p = stbi_load(path.c_str(), &w, &h, &n, 4);
        if (!p) {
            fprintf(stderr, "STBI FAILED to load: %s\n", path.c_str());
            rhythm::logDiagnostic("image load failed path=%s reason=%s errno=%d",path.c_str(),stbi_failure_reason()?stbi_failure_reason():"unknown",errno);
            return false;
        }
        fprintf(stdout, "STBI SUCCESS loading %s (%dx%d)\n", path.c_str(), w, h);
        out.assign(p, p + (size_t)w * h * 4);
        stbi_image_free(p);
        return true;
    }

    ImTextureID playerInstrumentIcon(int instrument) const noexcept {
        switch (instrument) {
        case 0: return downloaderInstrumentIds[0];       // PC guitar.png
        case 1: return downloaderInstrumentIds[1];       // PC bass.png
        case 2: return rhythmInstrumentId;               // PC rhythm.png
        case 3: return downloaderInstrumentIds[2];       // PC drums.png
        case 4: return chartTrackExtraIds[1];             // PC ghl_guitar.png
        case 5: return chartTrackExtraIds[3];
        case 6: return chartTrackExtraIds[4];
        case 7: return chartTrackExtraIds[2];
        default: return ImTextureID{};
        }
    }

    ImTextureID playerControllerIcon(int controller) const noexcept {
        switch (controller) {
        case 0: return downloaderInstrumentIds[0];       // 5-fret guitar
        case 1: return chartTrackExtraIds[1];             // 6-fret guitar
        case 4: // Pro drums use the original drum-kit icon.
        case 2: return downloaderInstrumentIds[2];       // drums
        case 3: return fiveLaneDrumsInstrumentId;         // 5-lane drums
        default: return ImTextureID{};
        }
    }

    void createTexture(Tex& t, const unsigned char* rgba, int w, int h,
                       VkSamplerAddressMode addrU, VkSamplerAddressMode addrV,
                       VkFormat format=VK_FORMAT_R8G8B8A8_SRGB,bool highwaySampling=false)
    {
        uint32_t mipLevels=1;
        if(highwaySampling) {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(physicalDevice,format,&properties);
            constexpr VkFormatFeatureFlags required=VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            if((properties.optimalTilingFeatures&required)==required)
                mipLevels=1u+static_cast<uint32_t>(std::floor(std::log2(std::max(w,h))));
        }
        VkDeviceSize size = (VkDeviceSize)w * h * 4;
        vks::Buffer staging;
        VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &staging, size, (void*)rgba));

        VkImageCreateInfo ic = vks::initializers::imageCreateInfo();
        ic.imageType = VK_IMAGE_TYPE_2D; ic.format = format;
        ic.extent = { (uint32_t)w,(uint32_t)h,1 }; ic.mipLevels = mipLevels; ic.arrayLayers = 1;
        ic.samples = VK_SAMPLE_COUNT_1_BIT; ic.tiling = VK_IMAGE_TILING_OPTIMAL;
        ic.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        if(mipLevels>1)ic.usage|=VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        ic.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VK_CHECK_RESULT(vkCreateImage(device, &ic, nullptr, &t.image));

        VkMemoryRequirements mr; vkGetImageMemoryRequirements(device, t.image, &mr);
        VkMemoryAllocateInfo ma = vks::initializers::memoryAllocateInfo();
        ma.allocationSize = mr.size;
        ma.memoryTypeIndex = vulkanDevice->getMemoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &ma, nullptr, &t.mem));
        VK_CHECK_RESULT(vkBindImageMemory(device, t.image, t.mem, 0));

        VkCommandBuffer cmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1 };
        vks::tools::setImageLayout(cmd, t.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent = { (uint32_t)w,(uint32_t)h,1 };
        vkCmdCopyBufferToImage(cmd, staging.buffer, t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        int mipWidth=w,mipHeight=h;
        for(uint32_t level=1;level<mipLevels;++level) {
            VkImageSubresourceRange previous{VK_IMAGE_ASPECT_COLOR_BIT,level-1,1,0,1};
            vks::tools::setImageLayout(cmd,t.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,previous);
            const int nextWidth=std::max(1,mipWidth/2),nextHeight=std::max(1,mipHeight/2);
            VkImageBlit blit{};
            blit.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level-1,0,1};
            blit.srcOffsets[1]={mipWidth,mipHeight,1};
            blit.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level,0,1};
            blit.dstOffsets[1]={nextWidth,nextHeight,1};
            vkCmdBlitImage(cmd,t.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,t.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_LINEAR);
            mipWidth=nextWidth;mipHeight=nextHeight;
        }
        if(mipLevels>1) {
            VkImageSubresourceRange sources{VK_IMAGE_ASPECT_COLOR_BIT,0,mipLevels-1,0,1};
            vks::tools::setImageLayout(cmd,t.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,sources);
        }
        VkImageSubresourceRange last{VK_IMAGE_ASPECT_COLOR_BIT,mipLevels-1,1,0,1};
        vks::tools::setImageLayout(cmd,t.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,last);
        vulkanDevice->flushCommandBuffer(cmd, queue, true);
        staging.destroy();

        VkImageViewCreateInfo vi = vks::initializers::imageViewCreateInfo();
        vi.image = t.image; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = format;
        vi.subresourceRange = range;
        VK_CHECK_RESULT(vkCreateImageView(device, &vi, nullptr, &t.view));

        VkSamplerCreateInfo si = vks::initializers::samplerCreateInfo();
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.addressModeU = addrU; si.addressModeV = addrV; si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.maxLod = highwaySampling?float(mipLevels-1):1.0f;
        if(highwaySampling && enabledFeatures.samplerAnisotropy) {
            si.anisotropyEnable=VK_TRUE;
            si.maxAnisotropy=std::min(16.0f,deviceProperties.limits.maxSamplerAnisotropy);
        }
        si.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
#if !defined(PS5_HOST_REFERENCE)
        if(highwaySampling && ps5.frameBudget) {
            if(FILE* trace=std::fopen("/app0/highway-sampling-test.txt",&t==&boardTex?"w":"a")) {
                std::fprintf(trace,"style %d width %d height %d mips %u aniso %.1f\n",&t==&boardTex?0:int(&t-highwayStyleTextures.data())+1,w,h,mipLevels,si.anisotropyEnable?si.maxAnisotropy:1.0f);
                std::fclose(trace);
            }
        }
#endif
        VK_CHECK_RESULT(vkCreateSampler(device, &si, nullptr, &t.sampler));
    }

    bool prepareTextures()
    {
        // Source-derived atlas and geometry must be generated together.
        std::vector<unsigned char> apix; int aw = 0, ah = 0;
        if (!loadRGBA(rhythm::assets::currentRoot() + "pc_guitar/guitar_atlas.png", apix, aw, ah)
            || aw != pc_guitar::atlasWidth || ah != pc_guitar::atlasHeight) {
            notify_fail("PC guitar atlas missing or inconsistent with generated geometry");
            quit = true;
            return false;
        }
        createTexture(atlasTex, apix.data(), aw, ah,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FORMAT_R8G8B8A8_UNORM);

        std::vector<unsigned char> drumPixels;int drumW=0,drumH=0;
        if(!loadRGBA(rhythm::assets::currentRoot()+"pc_drums/drums_atlas.png",drumPixels,drumW,drumH) ||
            drumW!=pc_drums_art::atlasWidth || drumH!=pc_drums_art::atlasHeight){
            notify_fail("PC drum atlas missing or inconsistent with source geometry");quit=true;return false;
        }
        createTexture(drumAtlasTex,drumPixels.data(),drumW,drumH,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FORMAT_R8G8B8A8_UNORM);
        std::vector<unsigned char> sixPixels;int sixW=0,sixH=0;
        if(!loadRGBA(rhythm::assets::currentRoot()+"pc_six_fret/six_fret_atlas.png",sixPixels,sixW,sixH) || sixW!=pc_six_fret::atlasWidth || sixH!=pc_six_fret::atlasHeight)return false;
        createTexture(sixFretTex,sixPixels.data(),sixW,sixH,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FORMAT_R8G8B8A8_UNORM);
        std::vector<unsigned char> fxPixels;int fxW=0,fxH=0;
        if(!loadRGBA(rhythm::assets::currentRoot()+"pc_guitar_fx/guitar_fx_atlas.png",fxPixels,fxW,fxH) ||
           fxW!=pc_guitar_fx::atlasWidth || fxH!=pc_guitar_fx::atlasHeight) {
            notify_fail("Original PC guitar effect atlas missing");quit=true;return false;
        }
        createTexture(guitarFxTex,fxPixels.data(),fxW,fxH,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FORMAT_R8G8B8A8_UNORM);
        std::vector<unsigned char> highwayPixels;int highwayW=0,highwayH=0;
        if(!loadRGBA(rhythm::assets::currentRoot()+"pc_highway/highway_atlas.png",highwayPixels,highwayW,highwayH)) return false;
        createTexture(highwayTex,highwayPixels.data(),highwayW,highwayH,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FORMAT_R8G8B8A8_UNORM);
        std::vector<unsigned char> beatPixels;int beatW=0,beatH=0;
        if(!loadRGBA(rhythm::assets::currentRoot()+"pc_guitar/beatline.png",beatPixels,beatW,beatH)) return false;
        createTexture(beatTex,beatPixels.data(),beatW,beatH,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FORMAT_R8G8B8A8_UNORM);
        // Extracted PC highway only; never substitute painted artwork.
        std::vector<unsigned char> bpix; int bw = 0, bh = 0;
        if (!loadRGBA(rhythm::assets::currentRoot() + "Texture2D/highway_board.png", bpix, bw, bh)) {
            notify_fail("Extracted PC highway_board.png missing");
            quit = true;
            return false;
        }
        createTexture(boardTex, bpix.data(), bw, bh,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FORMAT_R8G8B8A8_UNORM,true);

        // Upload each original style once; all four players share these images.
        for(size_t style=1;style<rhythm::highwayStyles.size();++style) {
            if(!loadRGBA(rhythm::assets::currentRoot()+rhythm::highwayStyles[style].asset,bpix,bw,bh)) {
                notify_fail("Extracted highway style missing");quit=true;return false;
            }
            createTexture(highwayStyleTextures[style-1],bpix.data(),bw,bh,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_REPEAT,VK_FORMAT_R8G8B8A8_UNORM,true);
        }

        auto loadMenuImage = [&](Tex& tex, ImTextureID& id, const char* path, bool alphaMask=false) {
            std::vector<unsigned char> pixels; int w = 0, h = 0;
            const bool owned=std::strncmp(path,"branding/",9)==0;
            const std::string fullPath = (owned?getAssetPath():rhythm::assets::currentRoot()) + path;
            if (!loadRGBA(fullPath, pixels, w, h)) return false;
            if(&tex==&menuLogoTex) {
                // Keep the supplied PNG intact. Fit its visible alpha bounds,
                // so a square canvas with transparent padding is not stretched.
                int left=w,top=h,right=0,bottom=0;
                for(int y=0;y<h;++y) for(int x=0;x<w;++x)
                    if(pixels[(static_cast<size_t>(y)*w+x)*4+3]) {
                        left=std::min(left,x);top=std::min(top,y);
                        right=std::max(right,x+1);bottom=std::max(bottom,y+1);
                    }
                if(right<=left || bottom<=top) return false;
                menuLogoUV0=ImVec2(float(left)/w,float(top)/h);
                menuLogoUV1=ImVec2(float(right)/w,float(bottom)/h);
                menuLogoAspect=float(right-left)/float(bottom-top);
            }
            // Extracted monochrome UI glyphs use their original alpha silhouette
            // with the PC's white tint; the source file remains unchanged.
            if(alphaMask) for(size_t i=0;i<pixels.size();i+=4)
                pixels[i]=pixels[i+1]=pixels[i+2]=255;
            // PC PlayerSettings.m_ActiveColorSpace=0 (Gamma). Preserve source RGB
            // for gameplay sprites on the existing UNORM direct-display target.
            const bool gameplay=std::strncmp(path,"pc_score_hud/",13)==0 || std::strncmp(path,"pc_gameplay_hud/",16)==0;
            createTexture(tex, pixels.data(), w, h,
                          VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                          VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                          gameplay?VK_FORMAT_R8G8B8A8_UNORM:VK_FORMAT_R8G8B8A8_SRGB);
            id = ui.addTexture(tex.view, tex.sampler);
            return true;
        };
        static constexpr const char* menuBackgroundPaths[] = {
            "pc_menu/Surfer_19.png", "pc_menu/Rainbow_27.png", "pc_menu/SurferAlt_26.png",
            "pc_menu/Classic_13.png", "pc_menu/Dark_39.png", "pc_menu/Light_20.png",
            "pc_menu/Autumn_37.png", "pc_menu/Alien_25.png", "pc_menu/Blue_Rays_28.png",
            "pc_menu/Grains_21.png", "pc_menu/Groovy_45.png", "pc_menu/Pastel_Burst_23.png",
            "pc_menu/Spray_33.png"
        };
        bool backgroundsLoaded = true;
        for (size_t i = 0; i < std::size(menuBackgroundPaths); ++i)
            backgroundsLoaded = loadMenuImage(menuBackgroundTextures[i], menuBackgroundIds[i], menuBackgroundPaths[i]) && backgroundsLoaded;
        if (!backgroundsLoaded || !loadMenuImage(menuLogoTex, menuLogoId, "branding/menu-logo.png") ||
            !loadMenuImage(menuButtonTex, menuButtonId, "pc_menu/whiteBox_0_201.png") ||
            !loadMenuImage(menuPlayerPanelTex, menuPlayerPanelId, "pc_menu/whiteBoxTop_0_1389.png") ||
            !loadMenuImage(starPowerBaseTex, starPowerBaseId, "Sprite/power_meter_basepng.png") ||
            !loadMenuImage(starPowerFillTex, starPowerFillId, "Sprite/power_meter_fill.png")) {
            notify_fail("PC menu source artwork is missing from the app assets");
            quit = true;
            return false;
        }
        // Reserve a stable descriptor without requiring any particular song.
        // The preview remains blank until its selected song's cover is loaded.
        // This descriptor initially borrows the existing UI texture; it does not
        // own that texture, so menuAlbumTex stays empty until an actual cover loads.
        menuAlbumId=ui.addTexture(menuButtonTex.view,menuButtonTex.sampler);
        menuAlbumVisible=false;
        static constexpr const char* songSelectControlPaths[] = {
            "pc_menu/gtr-G-instrument-nav_211.png",
            "pc_menu/gtr-R-instrument-nav_215.png",
            "pc_menu/gtr-Y-instrument-nav_218.png",
            "pc_menu/gtr-B-instrument-nav_205.png",
            "pc_menu/gtr-O-instrument-nav_207.png",
            "pc_menu/007_select_216.png",
            "pc_menu/006_start.png"
        };
        for (size_t i = 0; i < std::size(songSelectControlPaths); ++i)
            if (!loadMenuImage(songSelectControlTextures[i], songSelectControlIds[i], songSelectControlPaths[i])) {
                notify_fail("PC Song Select control icons are missing from app assets");
                quit = true;
                return false;
            }
        static constexpr const char* detailPaths[]={
            "pc_menu/radiusbg_1609.png", "pc_menu/radiusfg_1586.png",
            "pc_menu/DropdownArrow.png", "pc_menu/information_74.png",
            "pc_menu/OvalNoOutline_1394.png", "pc_menu/ScrollBar_BG_1406.png",
            "pc_menu/radiusbg_1609.png", "pc_menu/target_1396.png"
        };
        for(size_t i=0;i<std::size(detailPaths);++i)
            if(!loadMenuImage(menuDetailTextures[i],menuDetailIds[i],detailPaths[i],i==0||i==2||i==4||i==6)) return false;
        static constexpr const char* resultPaths[]={"pc_menu/star_1410.png","pc_menu/whiteBoxBottom_0_1517.png",
            "pc_menu/whiteBoxTop_0_1389.png","pc_menu/Arrow_Down_1646.png","pc_menu/GoldCrown_1559.png",
            "pc_menu/PFC_Crown_1400.png","pc_menu/DynamicCrown_1665.png","pc_menu/ch_icon_1640.png"};
        for(size_t i=0;i<std::size(resultPaths);++i)
            if(!loadMenuImage(resultsUiTextures[i],resultsUiIds[i],resultPaths[i]))return false;
        static constexpr const char* browserInstrumentIconPaths[] = {
            "Sprite/guitar.png", "Sprite/bass.png", "Sprite/drums.png", "Sprite/keys.png"
        };
        for (size_t i = 0; i < std::size(browserInstrumentIconPaths); ++i)
            if (!loadMenuImage(downloaderInstrumentTextures[i], downloaderInstrumentIds[i], browserInstrumentIconPaths[i])) return false;
        static constexpr const char* chartTrackExtraPaths[] = {
            "Sprite/guitar_coop.png", "Sprite/ghl_guitar.png",
            "Sprite/ghl_guitar_coop_sq.png", "Sprite/ghl_bass.png", "Sprite/ghl_rhythm_sq.png"
        };
        for (size_t i = 0; i < std::size(chartTrackExtraPaths); ++i)
            if (!loadMenuImage(chartTrackExtraTextures[i], chartTrackExtraIds[i], chartTrackExtraPaths[i])) return false;
        if (!loadMenuImage(rhythmInstrumentTexture,rhythmInstrumentId,"Sprite/rhythm.png")) return false;
        if (!loadMenuImage(fiveLaneDrumsInstrumentTexture, fiveLaneDrumsInstrumentId,
                           "Sprite/drums_reg.png")) return false;
        if (!loadMenuImage(downloaderCoverTex, downloaderCoverId, "Sprite/guitar.png")) return false;
        for (int i = 0; i < 11; ++i) {
            const std::string path = "pc_score_hud/fnt_gh5score_strip14_" + std::to_string(i) + ".png";
            if (!loadMenuImage(scoreGlyphTextures[i], scoreGlyphIds[i], path.c_str())) return false;
        }
        for(size_t i=0;i<std::size(pc_score::layers);++i)
            if(!loadMenuImage(sourceScoreTextures[i],sourceScoreIds[i],pc_score::layers[i].path)) return false;
        if (!loadMenuImage(scorePanelTex, scorePanelId, "pc_score_hud/spr_gh5scorebar_strip2_0.png")) return false;
        if (!loadMenuImage(starBadgeGoldTex, starBadgeGoldId, "pc_score_hud/score_star_gold_back.png") ||
            !loadMenuImage(starBadgeGreyTex, starBadgeGreyId, "pc_score_hud/score_star_grey_back.png") ||
            !loadMenuImage(starOutlineGoldTex, starOutlineGoldId, "pc_score_hud/score_star_outline_gold.png") ||
            !loadMenuImage(starOutlineGreyTex, starOutlineGreyId, "pc_score_hud/score_star_outline_grey.png") ||
            !loadMenuImage(starGlowTex, starGlowId, "pc_score_hud/score_star_overlay.png") ||
            !loadMenuImage(starProgressBackTex, starProgressBackId, "pc_score_hud/progress_bg.png") ||
            !loadMenuImage(starProgressFillTex, starProgressFillId, "pc_score_hud/progress_overlay.png")) return false;
        std::vector<unsigned char> meterPixels;int meterW=0,meterH=0;
        if(!loadRGBA(rhythm::assets::currentRoot()+"pc_gameplay_hud/meter_atlas.png",meterPixels,meterW,meterH)) return false;
        createTexture(meterTex,meterPixels.data(),meterW,meterH,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_FORMAT_R8G8B8A8_UNORM);
        menuAlbumTexturePath.clear();
        return true;
    }

    void refreshResultsIcon() {
        const std::string key=gameplaySongInfo.source;
        if(resultsIconKey==key)return;
        resultsIconKey=key;
        const bool safe=!key.empty() && key.size()<80 && std::all_of(key.begin(),key.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';});
        std::vector<unsigned char> pixels;int w=0,h=0;
        bool loaded=safe && loadRGBA(rhythm::assets::currentRoot()+"Texture2D/"+key+".png",pixels,w,h);
        if(!loaded)loaded=loadRGBA(rhythm::assets::currentRoot()+"pc_menu/ch_icon_1640.png",pixels,w,h);
        if(!loaded)return;
        VK_CHECK_RESULT(vkDeviceWaitIdle(device));
        Tex replacement{};
        createTexture(replacement,pixels.data(),w,h,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        updateTextureDescriptor(resultsUiIds[7],replacement);destroyTex(resultsUiTextures[7]);resultsUiTextures[7]=replacement;
    }

    void updateMenuAlbumDescriptor(const Tex& texture) {
        updateTextureDescriptor(menuAlbumId, texture);
    }

    void updateTextureDescriptor(ImTextureID textureId, const Tex& texture) {
        VkDescriptorSet descriptorSet{};
        static_assert(sizeof(descriptorSet) <= sizeof(textureId));
        std::memcpy(&descriptorSet, &textureId, sizeof(descriptorSet));
        VkDescriptorImageInfo imageInfo{ .sampler = texture.sampler,
            .imageView = texture.view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkWriteDescriptorSet write = vks::initializers::writeDescriptorSet(
            descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageInfo);
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

#if !defined(PS5_HOST_REFERENCE)
    static std::string previewMusicStream(const std::string& chartPath) {
        std::string chartText;
        FILE* chartFile = std::fopen(chartPath.c_str(), "rb");
        if (chartFile) {
            if (std::fseek(chartFile, 0, SEEK_END) == 0) {
                const long length = std::ftell(chartFile);
                if (length > 0 && length <= 32 * 1024 * 1024 && std::fseek(chartFile, 0, SEEK_SET) == 0) {
                    chartText.resize(static_cast<size_t>(length));
                    if (std::fread(&chartText[0], 1, chartText.size(), chartFile) != chartText.size())
                        chartText.clear();
                }
            }
            std::fclose(chartFile);
        }
        std::string musicStream = "song.opus";
        const std::string section = chartSection(chartText, "Song");
        size_t lineStart = 0;
        while (lineStart < section.size()) {
            const size_t lineEnd = section.find('\n', lineStart);
            const std::string line = trimText(section.substr(lineStart,
                (lineEnd == std::string::npos ? section.size() : lineEnd) - lineStart));
            const size_t equal = line.find('=');
            if (equal != std::string::npos) {
                std::string key = trimText(line.substr(0, equal));
                std::transform(key.begin(), key.end(), key.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                std::string value = trimText(line.substr(equal + 1));
                if (key == "musicstream" && !value.empty()) {
                    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                        value = value.substr(1, value.size() - 2);
                    musicStream = value;
                    break;
                }
            }
            if (lineEnd == std::string::npos) break;
            lineStart = lineEnd + 1;
        }
        return musicStream;
    }

    static void* previewAudioDecodeEntry(void* context) noexcept {
        auto* self = static_cast<VulkanExample*>(context);
        const double started=now_seconds();
        if(self->previewAudioWorkerDelay)for(unsigned i=0;i<3000 && !self->previewAudioCancel.load(std::memory_order_acquire);++i)platform_sleep_us(1000);
        const std::string musicStream = self->previewAudioWorkerGameplay && !self->previewAudioWorkerMusic.empty()?
            self->previewAudioWorkerMusic:previewMusicStream(self->previewAudioWorkerChartPath);
        self->previewAudioWorkerLoaded = self->previewAudioWorkerGameplay?
            self->previewAudioStage.loadGameplayStream(self->previewAudioWorkerAudioPath.c_str(),musicStream.c_str(),&self->previewAudioCancel):
            self->previewAudioStage.loadPreviewClip(self->previewAudioWorkerAudioPath.c_str(),musicStream.c_str(),
                true,self->previewAudioWorkerAuthoredStart,self->previewAudioWorkerRandom,&self->previewAudioCancel);
        self->previewAudioWorkerMs=(now_seconds()-started)*1000;
        rhythm::logDiagnostic("audio decode gameplay=%d loaded=%d ms=%.2f",self->previewAudioWorkerGameplay,self->previewAudioWorkerLoaded,self->previewAudioWorkerMs);
        self->previewAudioWorkerDone.store(true, std::memory_order_release);
        return nullptr;
    }

    void startPreviewPlayback(const LocalSong& song) {
        stopAudioOutput();
        const double duration=songAudio.durationSeconds();
        previewPlayingForMenu=screen==Screen::MainMenu;
        rhythm::PreviewWindow window;
        if(songAudio.previewClip()) {
            window={songAudio.previewStartSeconds(),songAudio.previewLengthSeconds()};
            if(previewPlayingForMenu)menuPreviewHistory.remember(song.chartPath);
        } else if(previewPlayingForMenu) {
            window=rhythm::menuPreviewWindow(duration,songAudio.dedicatedPreview(),
                static_cast<double>(std::rand())/static_cast<double>(RAND_MAX));
            menuPreviewHistory.remember(song.chartPath);
        } else {
            window=rhythm::menuPreviewWindow(duration,songAudio.dedicatedPreview(),
                static_cast<double>(std::rand())/static_cast<double>(RAND_MAX));
        }
        window.start=std::round(window.start*48000.0)/48000.0;
        previewClipStart=window.start;
        previewClipLength=window.length;
        songAudio.setPlaybackWindow(window.start,window.length);
        menuPreviewAudioChartPath=song.chartPath;
        applyAudioGains();
        audioOutputStarted=audio_start(&rhythm::AudioStemMixer::audioFill,&songAudio);
        if(audioOutputStarted && (screen!=Screen::MainMenu || menuIntroFinished)) (void)songAudio.play();
        if(ps5.frameBudget) {
            FILE* trace=std::fopen("/app0/preview-test.txt","a");
            if(trace) {
                std::fprintf(trace,"preview_memory clip %d pcm_bytes %llu duration %.6f decode_ms %.3f fade_seconds %.3f fade_in_seconds %.3f\n",
                    songAudio.previewClip()?1:0,static_cast<unsigned long long>(songAudio.pcmBytes()),
                    songAudio.durationSeconds(),previewAudioWorkerMs,songAudio.previewFadeOutSeconds(),songAudio.previewFadeInSeconds());
                std::fprintf(trace,"preview %s start %.6f length %.6f stems %zu dedicated %d muted %d path %s\n",
                    previewPlayingForMenu?"menu":"list",window.start,window.length,songAudio.stemCount(),
                    songAudio.dedicatedPreview()?1:0,menuPreviewMuted?1:0,song.chartPath.c_str());
                std::fclose(trace);
            }
        }
        if(!audioOutputStarted) std::fprintf(stderr,"[audio] could not open preview output\n");
    }

    void requestPreviewAudio(const LocalSong& song) {
        if(song.chartPath==menuPreviewAudioChartPath && songAudio.ready()) {
            if(previewPlayingForMenu!=(screen==Screen::MainMenu))startPreviewPlayback(song);
            return; // Reuse the identical decoded widget/list excerpt across screens.
        }
        if(std::find(menuPreviewFailedPaths.begin(),menuPreviewFailedPaths.end(),song.chartPath)!=menuPreviewFailedPaths.end()) return;
        if (previewAudioThreadActive) return;
        previewAudioWorkerChartPath = song.chartPath;
        previewAudioWorkerGameplay=false;previewAudioWorkerDelay=audioDelayQa;previewAudioCancel=false;
        previewAudioWorkerForMenu=screen==Screen::MainMenu;
        previewAudioWorkerAuthoredStart=song.previewStartSeconds;
        previewAudioWorkerRandom=double(std::rand())/double(RAND_MAX);
        previewAudioWorkerAudioPath = song.audioChartPath.empty()?song.chartPath:song.audioChartPath;
        previewAudioWorkerLoaded = false;
        previewAudioWorkerDone.store(false, std::memory_order_release);
        songAudio.pause();
        const int result = pthread_create(&previewAudioThread, nullptr,
            &VulkanExample::previewAudioDecodeEntry, this);
        if (result != 0) {
            menuPreviewFailedPaths.push_back(song.chartPath);
            std::fprintf(stderr, "[audio] could not start background preview decoder (%d)\n", result);
            return;
        }
        previewAudioThreadActive = true;
    }

    void finishPreviewAudioDecode() {
        if(gameplayAudioPending && screen!=Screen::PlayerSelect){gameplayAudioPending=gameplayReadyPending=false;}
        if(previewAudioThreadActive) {
            bool wanted=false;
            if(previewAudioWorkerGameplay)wanted=gameplayAudioPending && screen==Screen::PlayerSelect && gameplayAudioChart==previewAudioWorkerChartPath;
            else {
                const int index=previewAudioWorkerForMenu?menuMusicSelection:songSelection;
                wanted=screen==(previewAudioWorkerForMenu?Screen::MainMenu:Screen::SongSelect) && index>=0 && index<int(localSongs.size()) && localSongs[size_t(index)].chartPath==previewAudioWorkerChartPath;
            }
            if(!wanted)previewAudioCancel=true;
        }
        if (!previewAudioThreadActive || !previewAudioWorkerDone.load(std::memory_order_acquire)) return;
        (void)pthread_join(previewAudioThread, nullptr);
        previewAudioThreadActive = false;
        if(previewAudioCancel.load(std::memory_order_acquire)) {previewAudioStage.clear();return;}
        if(previewAudioWorkerGameplay && !gameplayAudioPending){previewAudioStage.clear();return;}
        if(gameplayAudioPending) {
            if(screen!=Screen::PlayerSelect){gameplayAudioPending=gameplayReadyPending=false;}
        }
        if(gameplayAudioPending) {
            const bool reusable=previewAudioWorkerChartPath==gameplayAudioChart && previewAudioWorkerLoaded &&
                !previewAudioStage.dedicatedPreview() && !previewAudioStage.previewClip();
            if(reusable || (previewAudioWorkerGameplay && previewAudioWorkerChartPath==gameplayAudioChart)) {
                stopAudioOutput();songAudio.takeChartStemsFrom(previewAudioStage);
                loadedAudioChartPath=previewAudioWorkerLoaded?gameplayAudioChart:std::string{};
                menuPreviewAudioChartPath.clear();gameplayAudioPending=false;
                gameplayAudioLoadMs=previewAudioWorkerMs;gameplayAudioCacheHit=false;
                rhythm::logDiagnostic("gameplay audio ready loaded=%d stems=%zu decode_ms=%.2f",previewAudioWorkerLoaded,songAudio.stemCount(),gameplayAudioLoadMs);
                applyAudioGains();audioOutputStarted=audio_start(&rhythm::AudioStemMixer::audioFill,&songAudio);
                if(!previewAudioWorkerLoaded){controllerToast="No playable song audio was found.";controllerToastUntil=ImGui::GetTime()+5;}
                if(gameplayReadyPending)enterPreparedGameplay();
            } else previewAudioStage.clear();
            return;
        }
        const auto isSelected = [&](int index) {
            return index >= 0 && index < static_cast<int>(localSongs.size()) &&
                localSongs[static_cast<size_t>(index)].chartPath == previewAudioWorkerChartPath;
        };
        const bool stillSelected = (!previewAudioWorkerForMenu && screen == Screen::SongSelect && isSelected(songSelection)) ||
            (previewAudioWorkerForMenu && screen == Screen::MainMenu && isSelected(menuMusicSelection));
        if (!previewAudioWorkerLoaded || !stillSelected) {
            if (!previewAudioWorkerLoaded && stillSelected)
                menuPreviewFailedPaths.push_back(previewAudioWorkerChartPath);
            previewAudioStage.clear();
            return;
        }

        stopAudioOutput();
        songAudio.takeChartStemsFrom(previewAudioStage);
        loadedAudioChartPath.clear();
        menuPreviewAudioChartPath = previewAudioWorkerChartPath;
        const int index=screen==Screen::MainMenu ? menuMusicSelection : songSelection;
        startPreviewPlayback(localSongs[static_cast<size_t>(index)]);
    }

#endif

    int chooseMenuPreviewSong() const {
        std::vector<std::string> paths;
        std::vector<int> indices;
        for(size_t i=0;i<localSongs.size();++i) {
            if(std::find(menuPreviewFailedPaths.begin(),menuPreviewFailedPaths.end(),localSongs[i].chartPath)!=menuPreviewFailedPaths.end()) continue;
            paths.push_back(localSongs[i].chartPath);
            indices.push_back(static_cast<int>(i));
        }
        const int choice=menuPreviewHistory.choose(paths,static_cast<uint32_t>(std::rand()));
        return choice<0 ? -1 : indices[static_cast<size_t>(choice)];
    }

    void refreshMenuPreview() {
        if (localSongs.empty()) {
            menuPreviewChartPath.clear();
            menuPreviewAudioChartPath.clear();
            menuMusicSelection = -1;
            menuAlbumVisible = false;
            return;
        }
        if (menuMusicSelection < 0 || menuMusicSelection >= static_cast<int>(localSongs.size()))
            menuMusicSelection = chooseMenuPreviewSong();
        if(menuMusicSelection<0) { menuAlbumVisible=false; return; }
        const size_t index = static_cast<size_t>(menuMusicSelection);
        const LocalSong& song = localSongs[index];

        refreshSongAlbum(song);
    }

    void refreshSongAlbum(const LocalSong& song) {
        if (song.chartPath == menuPreviewChartPath && song.albumArtPath == menuPreviewAlbumPath) {
            menuAlbumVisible=!song.albumArtPath.empty() && song.albumArtPath==menuAlbumTexturePath;
            menuArtPendingChart.clear();menuArtPendingPath.clear();return;
        }
        // Coalesce rapid strum navigation: decode/upload art only after the
        // cursor settles. Never show the previous song's cover meanwhile.
        if(screen==Screen::SongSelect || screen==Screen::SongSearch) {
            if(menuArtPendingChart!=song.chartPath || menuArtPendingPath!=song.albumArtPath) {
                menuArtPendingChart=song.chartPath;menuArtPendingPath=song.albumArtPath;
                menuArtPendingSince=ImGui::GetTime();menuAlbumVisible=false;return;
            }
            if(ImGui::GetTime()-menuArtPendingSince<.10)return;
        } else {menuArtPendingChart.clear();menuArtPendingPath.clear();}
        menuPreviewChangedAt = ImGui::GetTime();
        menuPreviewChartPath = song.chartPath;
        menuPreviewAlbumPath = song.albumArtPath;
        if (song.albumArtPath.empty()) {
            menuAlbumVisible = false;
            return;
        }
        if (song.albumArtPath == menuAlbumTexturePath) {
            menuAlbumVisible = true;
            return;
        }

        std::vector<unsigned char> pixels;
        int width = 0, height = 0;
        if (!loadRGBA(song.albumArtPath, pixels, width, height) || width <= 0 || height <= 0) {
            menuAlbumVisible = false;
            return;
        }
        // The existing descriptor set remains stable. Wait before replacing its
        // referenced view so no in-flight frame can sample destroyed album art.
        VK_CHECK_RESULT(vkDeviceWaitIdle(device));
        Tex replacement{};
        createTexture(replacement, pixels.data(), width, height,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        updateMenuAlbumDescriptor(replacement);
        destroyTex(menuAlbumTex);
        menuAlbumTex = replacement;
        menuAlbumTexturePath = song.albumArtPath;
        menuAlbumVisible = true;
    }

    void buildGeometry()
    {
        // flat lane dividers / rails / strike bar (the board itself is textured now)
        for(auto& buffer:meterBuffers) {
            VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &buffer,512*sizeof(TexVertex)));
            VK_CHECK_RESULT(buffer.map());
        }
        meterScratch.reserve(512);meterDraws.reserve(32);
        // dynamic per-frame buffers (sprites + board), persistently mapped
        for (auto& b : texBuffers) {
            VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &b, (VkDeviceSize)MAX_TEX_VERTS*sizeof(TexVertex)));
            VK_CHECK_RESULT(b.map());
        }
        for (auto& b : sustainBuffers) {
            VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &b, (VkDeviceSize)MAX_TEX_VERTS*sizeof(TexVertex)));
            VK_CHECK_RESULT(b.map());
        }
        for (auto& b : boardBuffers) {
            VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &b, (VkDeviceSize)6*sizeof(TexVertex)));
            VK_CHECK_RESULT(b.map());
        }
    }

    // ---------- .chart parsing ----------
    static std::string chartSection(const std::string& t, const char* name) {
        size_t s = t.find(std::string("[") + name + "]");
        if (s == std::string::npos) return "";
        size_t o = t.find('{', s), c = t.find('}', o);
        if (o == std::string::npos || c == std::string::npos) return "";
        return t.substr(o + 1, c - o - 1);
    }
    template<class F> static void forEachEntry(const std::string& sec, F fn) {
        size_t i = 0, n = sec.size();
        while (i < n) {
            size_t e = sec.find('\n', i); if (e == std::string::npos) e = n;
            std::string line = sec.substr(i, e - i); i = e + 1;
            size_t eq = line.find('='); if (eq == std::string::npos) continue;
            long key = atol(line.c_str());
            std::string rest = line.substr(eq + 1);
            size_t p = rest.find_first_not_of(" \t\r");
            if (p != std::string::npos) fn(key, trimText(rest.substr(p)));
        }
    }

    static std::string trimText(std::string value) {
        const size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        const size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    static bool isDirectory(const std::string& path) {
        DIR* directory = opendir(path.c_str());
        if (!directory) return false;
        closedir(directory);
        return true;
    }

    std::string findChartFile(const std::string& folder) const {
        DIR* directory = opendir(folder.c_str());
        if (!directory) return {};
        std::vector<std::string> charts;
        while (dirent* entry = readdir(directory)) {
            const std::string name(entry->d_name);
            if (name == "." || name == "..") continue;
            if (name.size() >= 6) {
                std::string extension = name.substr(name.size() - 6);
                std::transform(extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (extension != ".chart") continue;
                const std::string candidate = folder + "/" + name;
                DIR* chartDirectory = opendir(candidate.c_str());
                if (chartDirectory) { closedir(chartDirectory); continue; }
                FILE* chart = std::fopen(candidate.c_str(), "rb");
                if (chart) {
                    std::fclose(chart);
                    charts.push_back(name);
                }
            }
        }
        closedir(directory);
        if (charts.empty()) return {};
        std::stable_sort(charts.begin(), charts.end(), [](const std::string& a, const std::string& b) {
            const bool aPreferred = a == "notes.chart", bPreferred = b == "notes.chart";
            return aPreferred != bPreferred ? aPreferred : a < b;
        });
        return folder + "/" + charts.front();
    }

    static std::string stripRichTextTags(const std::string& value) {
        std::string plain;
        plain.reserve(value.size());
        bool inTag = false;
        for (char c : value) {
            if (c == '<') { inTag = true; continue; }
            if (c == '>' && inTag) { inTag = false; continue; }
            if (!inTag) plain.push_back(c);
        }
        return trimText(plain);
    }

    void readSongMetadata(const std::string& libraryRoot, const std::string& chartPath, LocalSong& song) const {
        const auto catalog=rhythm::chartCatalog((song.chartPath.empty()?chartPath:song.chartPath).c_str());
        song.chartDifficulties=catalog.difficulties;
        song.chartedInstrumentMask|=catalog.instruments;
        const size_t chartSlash = chartPath.find_last_of('/');
        if (chartSlash == std::string::npos) return;
        // PC's Source search category reads chart metadata. Keep it separate
        // from the install path, which is not the song's source label.
        if (FILE* chart = fopen(chartPath.c_str(), "rb")) {
            bool inSongSection = false;
            char line[1024];
            while (fgets(line, sizeof(line), chart)) {
                std::string text = trimText(line);
                if (text.empty() || text[0] == ';' || text[0] == '#') continue;
                if (text.front() == '[' && text.back() == ']') {
                    std::string section = text.substr(1, text.size() - 2);
                    std::transform(section.begin(), section.end(), section.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (inSongSection) break;
                    inSongSection = section == "song";
                    continue;
                }
                if (!inSongSection) continue;
                const size_t equal = text.find('=');
                if (equal == std::string::npos) continue;
                std::string key = trimText(text.substr(0, equal));
                std::transform(key.begin(), key.end(), key.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (key != "source") continue;
                song.source = trimText(text.substr(equal + 1));
                if (song.source.size() >= 2 &&
                    ((song.source.front() == '"' && song.source.back() == '"') ||
                     (song.source.front() == '\'' && song.source.back() == '\'')))
                    song.source = song.source.substr(1, song.source.size() - 2);
                break;
            }
            fclose(chart);
        }
        std::string folder = chartPath.substr(0, chartSlash);
        for (;;) {
            if (song.albumArtPath.empty()) {
                static constexpr const char* imageNames[] = {
                    "album.jpg", "album.png", "album.jpeg", "cover.jpg", "cover.png", "cover.jpeg", "albumart.jpg"
                };
                for (const char* imageName : imageNames) {
                    const std::string imagePath = rhythm::caseFile(folder,imageName);
                    if (!imagePath.empty()) {
                        song.albumArtPath = imagePath;
                        break;
                    }
                }
            }
            FILE* file = fopen(rhythm::caseFile(folder,"song.ini").c_str(), "rb");
            if (file) {
                bool metadataRead = false;
                bool inSongSection = false;
                char line[1024];
                while (fgets(line, sizeof(line), file)) {
                    std::string text = trimText(line);
                    if (text.empty() || text[0] == ';' || text[0] == '#') continue;
                    if (text.front() == '[' && text.back() == ']') {
                        std::string section = text.substr(1, text.size() - 2);
                        std::transform(section.begin(), section.end(), section.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        inSongSection = section == "song";
                        continue;
                    }
                    if (!inSongSection) continue;
                    const size_t equal = text.find('=');
                    if (equal == std::string::npos) continue;
                    std::string key = trimText(text.substr(0, equal));
                    std::transform(key.begin(), key.end(), key.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    std::string value = trimText(text.substr(equal + 1));
                    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                        (value.front() == '\'' && value.back() == '\''))) {
                        value = value.substr(1, value.size() - 2);
                    }
                    if (key == "name" && !value.empty()) { song.title = value; metadataRead = true; }
                    else if (key == "artist" && !value.empty()) { song.artist = value; metadataRead = true; }
                    else if (key == "loading_phrase" && !value.empty()) song.loadingPhrase=stripRichTextTags(value);
                    else if (key == "album" && !value.empty()) { song.album = value; metadataRead = true; }
                    else if (key == "genre" && !value.empty()) { song.genre = value; metadataRead = true; }
                    else if (key == "charter" && !value.empty()) {
                        // SongEntry.Charter_StrippedTags is what the PC scene
                        // displays and searches. Charts can store per-letter
                        // TMP color/font tags in song.ini; keep only the
                        // charter's visible names for the native UI.
                        song.charter = stripRichTextTags(value);
                        metadataRead = true;
                    }
                    else if (key == "source" && !value.empty()) { song.source = value; metadataRead = true; }
                    else if (key == "year" && !value.empty()) { song.year = value; metadataRead = true; }
                    else if (key == "diff_guitar" || key == "diff_guitar_coop" ||
                             key == "diff_rhythm" || key == "diff_bass" ||
                             key == "diff_drums" || key == "diff_drums_real" ||
                             key == "diff_drums_real_ps" || key == "diff_keys" ||
                             key == "diff_guitarghl" || key == "diff_guitar_coop_ghl" ||
                             key == "diff_rhythmghl" || key == "diff_bassghl") {
                        // Follow the Encore/CH instrument ordering
                        // used by the PC result UI: guitar, co-op, rhythm,
                        // bass, drums, keys, then the 6-fret chart variants.
                        static constexpr const char* trackDifficultyKeys[] = {
                            "diff_guitar", "diff_guitar_coop", "diff_rhythm", "diff_bass",
                            "diff_drums", "diff_drums_real", "diff_drums_real_ps", "diff_keys",
                            "diff_guitarghl", "diff_guitar_coop_ghl", "diff_rhythmghl", "diff_bassghl"
                        };
                        static constexpr uint8_t trackIndexes[] = {0,1,2,3,4,4,4,5,6,7,8,9};
                        for (size_t track = 0; track < std::size(trackDifficultyKeys); ++track) {
                            if (key != trackDifficultyKeys[track]) continue;
                            char* end = nullptr;
                            const long level = std::strtol(value.c_str(), &end, 10);
                            if (end != value.c_str() && trimText(end ? end : "").empty() &&
                                level >= 0 && level <= 6)
                            {
                                song.chartedInstrumentMask |= static_cast<uint16_t>(1u << trackIndexes[track]);
                                song.instrumentRatings[trackIndexes[track]]=static_cast<int>(level);
                            }
                            break;
                        }
                    }
                    else if (key == "preview_start_time" && !value.empty()) {
                        char* end = nullptr;
                        const double milliseconds = std::strtod(value.c_str(), &end);
                        if (end != value.c_str() && trimText(end ? end : "").empty() &&
                            std::isfinite(milliseconds) && milliseconds >= 0.0 && milliseconds <= 3600000.0)
                            song.previewStartSeconds = milliseconds / 1000.0;
                    } else if (key == "song_length" && !value.empty()) {
                        char* end = nullptr;
                        const double milliseconds = std::strtod(value.c_str(), &end);
                        if (end != value.c_str() && trimText(end ? end : "").empty() &&
                            std::isfinite(milliseconds) && milliseconds > 0.0 && milliseconds <= 86400000.0)
                            song.durationSeconds = milliseconds / 1000.0;
                    }
                }
                fclose(file);
                if (metadataRead) return;
            }
            if (folder == libraryRoot || folder.size() <= libraryRoot.size() ||
                folder.compare(0, libraryRoot.size(), libraryRoot) != 0 || folder[libraryRoot.size()] != '/') break;
            const size_t parentSlash = folder.find_last_of('/');
            if (parentSlash == std::string::npos) break;
            folder.resize(parentSlash);
        }
    }

    uint64_t loadOrCreateSongDate(const std::string& chartPath) const {
        constexpr char directory[] = CH_USERDATA "/song-dates";
        (void)mkdir(directory, 0777);
        uint64_t key = 14695981039346656037ull;
        for (const unsigned char byte : rhythm::persistent::identity(chartPath)) {
            key ^= byte;
            key *= 1099511628211ull;
        }
        char path[160], temporary[176];
        std::snprintf(path, sizeof(path), "%s/%016llx.cfg", directory,
                      static_cast<unsigned long long>(key));
        std::snprintf(temporary, sizeof(temporary), "%s/%016llx.tmp", directory,
                      static_cast<unsigned long long>(key));

        FILE* file = std::fopen(path, "rb");
        bool hashCollision = false;
        if (file) {
            char header[96]{}, storedPath[8192]{};
            unsigned version = 0;
            unsigned long long timestamp = 0;
            const bool headerRead = std::fgets(header, sizeof(header), file) != nullptr;
            const bool pathRead = std::fgets(storedPath, sizeof(storedPath), file) != nullptr;
            std::fclose(file);
            if (headerRead && pathRead &&
                std::sscanf(header, "CHPS5DATE %u %llu", &version, &timestamp) == 2 &&
                version == 1 && timestamp > 0) {
                size_t length = std::strlen(storedPath);
                while (length && (storedPath[length - 1] == '\n' || storedPath[length - 1] == '\r'))
                    storedPath[--length] = '\0';
                if (rhythm::persistent::identity(chartPath) == rhythm::persistent::identity(storedPath)) return static_cast<uint64_t>(timestamp);
                hashCollision = true;
            }
        }

        const std::time_t now = std::time(nullptr);
        const uint64_t timestamp = now > 0 ? static_cast<uint64_t>(now) : 1;
        if (hashCollision || chartPath.empty() || chartPath.size() >= 4096 ||
            chartPath.find_first_of("\r\n") != std::string::npos) return timestamp;
        char record[8192];
        const int size = std::snprintf(record, sizeof(record), "CHPS5DATE 1 %llu\n%s\n",
            static_cast<unsigned long long>(timestamp), chartPath.c_str());
        if (size <= 0 || static_cast<size_t>(size) >= sizeof(record)) return timestamp;
        file = std::fopen(temporary, "wb");
        if (!file) return timestamp;
        bool complete = std::fwrite(record, 1, static_cast<size_t>(size), file) == static_cast<size_t>(size);
        if (complete) complete = std::fflush(file) == 0 && fsync(fileno(file)) == 0;
        if (std::fclose(file) != 0) complete = false;
        if (!complete || std::rename(temporary, path) != 0) (void)std::remove(temporary);
        return timestamp;
    }

    void scanSongDirectory(const std::string& folder, const std::string& libraryRoot, unsigned depth) {
        if (depth > 12 || localSongs.size() >= 512) return;
        const std::string chartPath = findChartFile(folder);
        if (!chartPath.empty()) {
            addSongChart(libraryRoot, chartPath);
            return;
        }
        DIR* directory = opendir(folder.c_str());
        if (!directory) return;
        std::vector<std::string> children;
        while (dirent* entry = readdir(directory)) {
            const std::string folderName(entry->d_name);
            if (folderName == "." || folderName == ".." || folderName.empty() || folderName[0] == '.') continue;
            const std::string child = folder + "/" + folderName;
            DIR* childDirectory = opendir(child.c_str());
            if (childDirectory) {
                closedir(childDirectory);
                children.push_back(child);
            }
        }
        closedir(directory);
        std::sort(children.begin(), children.end());
        for (const std::string& child : children) {
            if (localSongs.size() >= 512) break;
            scanSongDirectory(child, libraryRoot, depth + 1);
        }
    }

    void scanSongRoot(const std::string& root) {
        scanSongDirectory(root, root, 0);
    }

    void addSongChart(const std::string& libraryRoot, const std::string& chartPath) {
        if(rhythm::storage::bundleRemoved(chartPath.substr(0,chartPath.find_last_of('/')),getAssetPath()+"songs",removedBundledSongs))return;
        for (const LocalSong& song : localSongs)
            if (song.chartPath == chartPath) return;
        FILE* chart = fopen(chartPath.c_str(), "rb");
        if (!chart) return;
        fclose(chart);
        LocalSong song;
        song.chartPath = chartPath;
        if (chartFingerprint(chartPath, song.chartBytes, song.chartFingerprint))
            song.chartFingerprintValid = true;
        if (!allowDuplicateSongs && song.chartFingerprintValid) {
            for (const LocalSong& existing : localSongs) {
                if (!existing.chartFingerprintValid ||
                    existing.chartBytes != song.chartBytes ||
                    existing.chartFingerprint != song.chartFingerprint) continue;
                if (chartFilesEqual(existing.chartPath, chartPath)) return;
            }
        }
        const size_t slash = chartPath.find_last_of('/');
        const std::string folder = slash == std::string::npos ? chartPath : chartPath.substr(0, slash);
        const size_t folderSlash = folder.find_last_of('/');
        song.title = folderSlash == std::string::npos ? folder : folder.substr(folderSlash + 1);
        readSongMetadata(libraryRoot, chartPath, song);
        song.dateAdded = loadOrCreateSongDate(chartPath);
        localSongs.push_back(std::move(song));
        ++songListRevision;
    }

    bool chartFingerprint(const std::string& path, uint64_t& byteCount, uint64_t& fingerprint) const noexcept {
        FILE* file = std::fopen(path.c_str(), "rb");
        if (!file) return false;
        uint64_t count = 0;
        uint64_t hash = 14695981039346656037ull;
        std::array<unsigned char, 16384> bytes{};
        bool valid = true;
        for (;;) {
            const size_t got = std::fread(bytes.data(), 1, bytes.size(), file);
            if (got > 32u * 1024u * 1024u - std::min<uint64_t>(count, 32u * 1024u * 1024u)) {
                valid = false;
                break;
            }
            count += got;
            for (size_t i = 0; i < got; ++i) {
                hash ^= bytes[i];
                hash *= 1099511628211ull;
            }
            if (got < bytes.size()) {
                if (std::ferror(file)) valid = false;
                break;
            }
        }
        std::fclose(file);
        if (!valid) return false;
        byteCount = count;
        fingerprint = hash;
        return true;
    }

    bool chartFilesEqual(const std::string& first, const std::string& second) const noexcept {
        FILE* a = std::fopen(first.c_str(), "rb");
        if (!a) return false;
        FILE* b = std::fopen(second.c_str(), "rb");
        if (!b) { std::fclose(a); return false; }
        std::array<unsigned char, 16384> left{}, right{};
        bool equal = true;
        for (;;) {
            const size_t leftCount = std::fread(left.data(), 1, left.size(), a);
            const size_t rightCount = std::fread(right.data(), 1, right.size(), b);
            if (leftCount != rightCount ||
                std::memcmp(left.data(), right.data(), leftCount) != 0) {
                equal = false;
                break;
            }
            if (leftCount < left.size()) {
                equal = !std::ferror(a) && !std::ferror(b);
                break;
            }
        }
        std::fclose(a);
        std::fclose(b);
        return equal;
    }

    void sortLocalSongs() {
        const auto compareText = [](std::string a, std::string b) {
            std::transform(a.begin(), a.end(), a.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(b.begin(), b.end(), b.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return a < b;
        };
        std::stable_sort(localSongs.begin(), localSongs.end(), [&](const LocalSong& a, const LocalSong& b) {
            bool less = false, greater = false;
            const std::string* aText = &a.title;
            const std::string* bText = &b.title;
            switch (songSortField) {
            case SongSortField::Song: aText = &a.title; bText = &b.title; break;
            case SongSortField::Artist: aText = &a.artist; bText = &b.artist; break;
            case SongSortField::Album: aText = &a.album; bText = &b.album; break;
            case SongSortField::Genre: aText = &a.genre; bText = &b.genre; break;
            case SongSortField::Year: aText = &a.year; bText = &b.year; break;
            case SongSortField::Charter: aText = &a.charter; bText = &b.charter; break;
            case SongSortField::Length:
                less = a.durationSeconds < b.durationSeconds;
                greater = a.durationSeconds > b.durationSeconds;
                break;
            case SongSortField::DateAdded:
                less = a.dateAdded < b.dateAdded;
                greater = a.dateAdded > b.dateAdded;
                break;
            }
            if (aText != &a.title || bText != &b.title || songSortField == SongSortField::Song) {
                less = compareText(*aText, *bText);
                greater = compareText(*bText, *aText);
            }
            if (less != greater) return less;
            return compareText(a.title, b.title);
        });
        ++songListRevision;
    }

    static const char* songSortFieldName(SongSortField field) noexcept {
        switch (field) {
        case SongSortField::Song: return "SONG";
        case SongSortField::Artist: return "ARTIST";
        case SongSortField::Album: return "ALBUM";
        case SongSortField::Genre: return "GENRE";
        case SongSortField::Year: return "YEAR";
        case SongSortField::Charter: return "CHARTER";
        case SongSortField::Length: return "SONG LENGTH";
        case SongSortField::DateAdded: return "DATE ADDED";
        }
        return "SONG";
    }

    void setSongSortField(SongSortField field) {
        const std::string selectedPath = songSelection >= 0 &&
            songSelection < static_cast<int>(localSongs.size())
            ? localSongs[static_cast<size_t>(songSelection)].chartPath : std::string{};
        songSortField = field;
        sortLocalSongs();
        if (!selectedPath.empty()) {
            for (size_t i = 0; i < localSongs.size(); ++i) {
                if (localSongs[i].chartPath == selectedPath) {
                    songSelection = static_cast<int>(i);
                    return;
                }
            }
        }
        if (!localSongs.empty())
            songSelection = std::clamp(songSelection, 0, static_cast<int>(localSongs.size()) - 1);
    }

    void cycleSongSort(int direction) {
        if (direction == 0) return;
        constexpr int fieldCount = 8;
        int next = static_cast<int>(songSortField) + (direction > 0 ? 1 : fieldCount - 1);
        setSongSortField(static_cast<SongSortField>(next % fieldCount));
    }

    void jumpSongSection(int direction) {
        const auto& filtered = filteredSongIndices();
        if (filtered.size() < 2 || direction == 0) return;
        auto current = std::find(filtered.begin(), filtered.end(), static_cast<size_t>(songSelection));
        int index = current == filtered.end() ? 0 : static_cast<int>(current - filtered.begin());
        const auto sectionKey = [&](int at) {
            const LocalSong& song = localSongs[filtered[static_cast<size_t>(at)]];
            const std::string* label=&song.title;
            switch(songSortField) {
            case SongSortField::Artist:label=&song.artist;break;
            case SongSortField::Album:label=&song.album;break;
            case SongSortField::Genre:label=&song.genre;break;
            case SongSortField::Year:label=&song.year;break;
            case SongSortField::Charter:label=&song.charter;break;
            default:break;
            }
            for (unsigned char c : *label)
                if (std::isalnum(c)) return static_cast<char>(std::tolower(c));
            return '#';
        };
        const char initial = sectionKey(index);
        const int count = static_cast<int>(filtered.size());
        for (int step = 1; step < count; ++step) {
            const int candidate = (index + (direction > 0 ? step : count - step)) % count;
            if (sectionKey(candidate) != initial) {
                songSelection = static_cast<int>(filtered[static_cast<size_t>(candidate)]);
                return;
            }
        }
    }

    const std::vector<size_t>& filteredSongIndices() const {
        const int field = static_cast<int>(songSearchField);
        if (songFilterCacheRevision == songListRevision &&
            songFilterCacheField == field && songFilterCacheQuery == songSearchQuery)
            return songFilterCache;
        std::string needle = songSearchQuery;
        std::transform(needle.begin(), needle.end(), needle.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        songFilterCache.clear();
        songFilterCache.reserve(localSongs.size());
        for (size_t i = 0; i < localSongs.size(); ++i) {
            if (needle.empty()) { songFilterCache.push_back(i); continue; }
            const LocalSong& song = localSongs[i];
            const std::string* field = &song.title;
            switch (songSearchField) {
            case SongSearchField::Song: field = &song.title; break;
            case SongSearchField::Artist: field = &song.artist; break;
            case SongSearchField::Album: field = &song.album; break;
            case SongSearchField::Charter: field = &song.charter; break;
            case SongSearchField::Genre: field = &song.genre; break;
            case SongSearchField::Source: field = &song.source; break;
            }
            std::string haystack = *field;
            std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (haystack.find(needle) != std::string::npos) songFilterCache.push_back(i);
        }
        songFilterCacheQuery = songSearchQuery;
        songFilterCacheField = field;
        songFilterCacheRevision = songListRevision;
        return songFilterCache;
    }

    static const char* songSearchFieldName(SongSearchField field) noexcept {
        switch (field) {
        case SongSearchField::Song: return "SONG";
        case SongSearchField::Artist: return "ARTIST";
        case SongSearchField::Album: return "ALBUM";
        case SongSearchField::Charter: return "CHARTER";
        case SongSearchField::Genre: return "GENRE";
        case SongSearchField::Source: return "SOURCE";
        }
        return "SONG";
    }

    uint64_t songOffsetKey(const std::string& chartPath) const {
        uint64_t hash = 14695981039346656037ull;
        for (const unsigned char byte : rhythm::persistent::identity(chartPath)) {
            hash ^= byte;
            hash *= 1099511628211ull;
        }
        return hash;
    }

    bool loadBestScore(const std::string& chartPath, int instrument, int difficulty,
                       uint64_t& value) const {
        if (chartPath.empty()) return false;
        char path[192];
        std::snprintf(path, sizeof(path),
            CH_USERDATA "/scores/%016llx-%d-%d.cfg",
            static_cast<unsigned long long>(songOffsetKey(chartPath)), instrument, difficulty);
        FILE* file = std::fopen(path, "rb");
        if (!file) return false;
        rhythm::SavedScore record;
        const bool valid=rhythm::readSavedScore(file,record);
        std::fclose(file);
        if(valid)value=record.score;
        return valid;
    }

    void saveBestScore(const std::string& chartPath, int instrument, int difficulty,
                       uint64_t value) {
        if (ps5.frameBudget || chartPath.empty() || activeProfile.bot || playMode==PlayMode::Practice) return;
        uint64_t previous=0;
        if(loadBestScore(chartPath,instrument,difficulty,previous) && previous>value)return;
        constexpr char directory[] = CH_USERDATA "/scores";
        if (mkdir(directory, 0777) != 0 && errno != EEXIST) return;
        char path[192], temporary[208];
        const auto key = static_cast<unsigned long long>(songOffsetKey(chartPath));
        const int pathLength = std::snprintf(path, sizeof(path),
            "%s/%016llx-%d-%d.cfg", directory, key, instrument, difficulty);
        const int temporaryLength = std::snprintf(temporary, sizeof(temporary),
            "%s/%016llx-%d-%d.tmp", directory, key, instrument, difficulty);
        if (pathLength <= 0 || temporaryLength <= 0 ||
            static_cast<size_t>(pathLength) >= sizeof(path) ||
            static_cast<size_t>(temporaryLength) >= sizeof(temporary)) return;
        rhythm::SavedScore record;
        record.score=value;record.stars=currentStarRank();record.instrument=instrument;
        record.difficulty=difficulty;record.controller=activeProfile.controller;record.player=activeProfile.name;
        record.details=true;record.accuracyKnown=true;
        unsigned completedHeads=0;
        for(size_t head=0;head<chart.notes.size();++head)
            if(head+1<chordNoteOffsets.size() && chordNoteOffsets[head]<notes.size() && notes[chordNoteOffsets[head]].hit)++completedHeads;
        record.accuracyHundredths=chart.notes.empty()?0:static_cast<unsigned>(std::lround(10000.0*completedHeads/chart.notes.size()));
        if(record.player.empty())record.player="Guest";
        if(FILE* previousFile=std::fopen(path,"rb")) {
            rhythm::SavedScore old;const bool valid=rhythm::readSavedScore(previousFile,old);std::fclose(previousFile);
            if(valid && !rhythm::betterSavedScore(record,old))return;
        }
        FILE* file = std::fopen(temporary, "wb");
        if (!file) return;
        bool complete = rhythm::writeSavedScore(file,record);
        if (complete) complete = std::fflush(file) == 0 && fsync(fileno(file)) == 0;
        if (std::fclose(file) != 0) complete = false;
        if (!complete || std::rename(temporary, path) != 0) {(void)std::remove(temporary);return;}
        loadSongScoreCache();
        auto found=songBestScores.find(key);
        if(found==songBestScores.end() || rhythm::betterSavedScore(record,found->second))songBestScores[key]=std::move(record);
    }

    bool loadSongOffsetOverride(const std::string& chartPath, double& offset) const {
        if (chartPath.empty()) return false;
        char path[160];
        std::snprintf(path, sizeof(path), CH_USERDATA "/song-settings/%016llx.cfg",
                      static_cast<unsigned long long>(songOffsetKey(chartPath)));
        FILE* file = std::fopen(path, "rb");
        if (!file) return false;
        char magic[24]{};
        unsigned version = 0;
        double stored = 0.0;
        const bool valid = std::fscanf(file, "%23s %u %lf", magic, &version, &stored) == 3 &&
            std::strcmp(magic, "CHPS5SONG") == 0 && version == 1 && std::isfinite(stored) &&
            std::abs(stored) <= 60.0;
        std::fclose(file);
        if (!valid) return false;
        offset = stored;
        return true;
    }

    void saveSongOffsetOverride(const std::string& chartPath, double offset) const {
        if (chartPath.empty() || !std::isfinite(offset) || std::abs(offset) > 60.0) return;
        constexpr char directory[] = CH_USERDATA "/song-settings";
        (void)mkdir(directory, 0777);
        char path[160], temporary[176];
        const auto key = static_cast<unsigned long long>(songOffsetKey(chartPath));
        std::snprintf(path, sizeof(path), "%s/%016llx.cfg", directory, key);
        std::snprintf(temporary, sizeof(temporary), "%s/%016llx.tmp", directory, key);
        char record[96];
        const int size = std::snprintf(record, sizeof(record), "CHPS5SONG 1\n%.17g\n", offset);
        if (size <= 0 || static_cast<size_t>(size) >= sizeof(record)) return;
        FILE* file = std::fopen(temporary, "wb");
        if (!file) return;
        bool complete = std::fwrite(record, 1, static_cast<size_t>(size), file) == static_cast<size_t>(size);
        if (complete) complete = std::fflush(file) == 0 && fsync(fileno(file)) == 0;
        if (std::fclose(file) != 0) complete = false;
        if (!complete || std::rename(temporary, path) != 0) (void)std::remove(temporary);
    }

    std::string readSongIniValue(const std::string& chartPath,const char* wanted) const {
        const auto slash=chartPath.find_last_of('/');
        if(slash==std::string::npos) return {};
        std::string folder=chartPath.substr(0,slash);
        const std::array<std::string,3> roots{{getAssetPath()+"songs",CH_SONGS "",CH_LEGACY_SONGS}};
        for(const auto& root:roots) {
            if(folder.size()<root.size() || folder.compare(0,root.size(),root)!=0 ||
               (folder.size()>root.size() && folder[root.size()]!='/')) continue;
            for(;;) {
                if(FILE* file=std::fopen((folder+"/song.ini").c_str(),"rb")) {
                    bool inSong=false;char buffer[1024];
                    while(std::fgets(buffer,sizeof(buffer),file)) {
                        std::string line=trimText(buffer);
                        if(line.compare(0,3,"\xef\xbb\xbf")==0) line.erase(0,3);
                        if(line.empty() || line[0]==';' || line[0]=='#') continue;
                        if(line[0]=='[' && line.back()==']') {
                            auto section=line.substr(1,line.size()-2);
                            std::transform(section.begin(),section.end(),section.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                            inSong=section=="song";continue;
                        }
                        const auto equal=line.find('=');
                        if(!inSong || equal==std::string::npos) continue;
                        auto key=trimText(line.substr(0,equal));
                        std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                        if(key==wanted) {auto value=trimText(line.substr(equal+1));std::fclose(file);return value;}
                    }
                    std::fclose(file);
                }
                if(folder==root) break;
                const auto parent=folder.find_last_of('/');if(parent==std::string::npos) break;
                folder.resize(parent);
            }
            break;
        }
        return {};
    }

    double readSongDelaySeconds(const std::string& chartPath) const {
        const size_t chartSlash = chartPath.find_last_of('/');
        if (chartSlash == std::string::npos) return 0.0;
        std::string folder = chartPath.substr(0, chartSlash);
        const std::array<std::string, 3> roots{{getAssetPath() + "songs", CH_SONGS "",CH_LEGACY_SONGS}};
        for (const std::string& root : roots) {
            if (folder.size() < root.size() || folder.compare(0, root.size(), root) != 0 ||
                (folder.size() > root.size() && folder[root.size()] != '/')) continue;
            for (;;) {
                FILE* file = fopen(rhythm::caseFile(folder,"song.ini").c_str(), "rb");
                if (file) {
                    bool inSongSection = false;
                    char line[1024];
                    while (fgets(line, sizeof(line), file)) {
                        const std::string text = trimText(line);
                        if (text.empty() || text[0] == ';' || text[0] == '#') continue;
                        if (text.front() == '[' && text.back() == ']') {
                            std::string section = text.substr(1, text.size() - 2);
                            std::transform(section.begin(), section.end(), section.begin(),
                                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                            inSongSection = section == "song";
                            continue;
                        }
                        if (!inSongSection) continue;
                        const size_t equal = text.find('=');
                        if (equal == std::string::npos) continue;
                        std::string key = trimText(text.substr(0, equal));
                        std::transform(key.begin(), key.end(), key.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        if (key != "delay") continue;
                        const std::string value = trimText(text.substr(equal + 1));
                        char* end = nullptr;
                        const double milliseconds = std::strtod(value.c_str(), &end);
                        fclose(file);
                        return end != value.c_str() && trimText(end ? end : "").empty() &&
                            std::isfinite(milliseconds) ? milliseconds / 1000.0 : 0.0;
                    }
                    fclose(file);
                }
                if (folder == root || folder.size() <= root.size()) break;
                const size_t parentSlash = folder.find_last_of('/');
                if (parentSlash == std::string::npos) break;
                folder.resize(parentSlash);
            }
            break;
        }
        return 0.0;
    }

    std::string internalSongRoot() const {return storageQa?"/app0/storage-qa/internal":CH_SONGS "";}
    static bool copyCacheFile(const std::string& from,const std::string& to) {
        FILE* in=std::fopen(from.c_str(),"rb");if(!in)return false;
        FILE* out=std::fopen(to.c_str(),"wb");if(!out){std::fclose(in);return false;}
        std::array<char,65536> bytes;bool ok=true;
        for(;;){const auto n=std::fread(bytes.data(),1,bytes.size(),in);if(n && std::fwrite(bytes.data(),1,n,out)!=n){ok=false;break;}if(n<bytes.size()){ok=!std::ferror(in);break;}}
        if(std::fclose(out))ok=false;std::fclose(in);return ok;
    }
    static void* storageWorker(void* pointer) {
        auto* self=static_cast<VulkanExample*>(pointer);
        if(self->storageJob==StorageJob::Remove) {
            self->storageRemovalResult=rhythm::storage::removeSongs(self->storageRemovalFolders,self->internalSongRoot(),getAssetPath()+"songs",CH_USERDATA "/removed-bundled-songs.cfg",&self->storageCancel,
                [&](unsigned done,unsigned total){self->storageProgress=done;self->storageTotal=total;});
        } else if(self->storageJob==StorageJob::Copy) {
            self->storageCopyResult=rhythm::storage::copySongs(self->storageCopyFrom,self->storageCopyTo,&self->storageCancel,
                [&](unsigned done,unsigned total){self->storageProgress=done;self->storageTotal=total;});
        } else if(self->storageJob==StorageJob::Preview) {
            self->songCopyPlan=rhythm::storage::planCopy(self->storageCopyFrom,self->storageCopyTo,&self->storageCancel);
        } else {
            auto discovered=rhythm::storage::scan(self->storageScanRoots,&self->storageCancel);
            self->storageTotal=static_cast<unsigned>(discovered.size());
            const auto external=[&](const auto& file){return std::find(self->storageExternalSnapshot.begin(),self->storageExternalSnapshot.end(),file.root)!=self->storageExternalSnapshot.end();};
            self->storageUsbFound=static_cast<unsigned>(std::count_if(discovered.begin(),discovered.end(),external));
            std::unordered_multimap<uint64_t,size_t> indexed;
            for(const auto& file:discovered) {
                if(self->storageCancel)break;
                if(rhythm::storage::bundleRemoved(file.folder,getAssetPath()+"songs",self->removedBundledSongs))continue;
                ++self->storageProgress;
                // Bounded QA can hold a real scan mid-job for a deterministic
                // widget capture; plain launches never enable this latch.
                if(self->storageProgress.load()==std::max(1u,self->storageTotal.load()/2))
                    while(self->storageScanTestHold.load() && !self->storageCancel.load())usleep(2000);
                bool duplicate=false;
                const auto range=indexed.equal_range(file.hash);
                for(auto i=range.first;i!=range.second;++i) {
                    const auto& old=self->storageSongs[i->second];
                    if(old.chartBytes==file.bytes && rhythm::storage::equalFiles(old.originalChartPath,file.chart,&self->storageCancel)){duplicate=true;break;}
                }
                if(duplicate){if(external(file))++self->storageUsbDuplicates;continue;}
                LocalSong song;song.chartPath=file.chart;song.audioChartPath=file.chart;song.originalChartPath=file.chart;
                song.chartBytes=file.bytes;song.chartFingerprint=file.hash;song.chartFingerprintValid=true;
                const auto slash=file.folder.find_last_of('/');song.title=file.folder.substr(slash==std::string::npos?0:slash+1);
                const auto dot=file.chart.find_last_of('.');std::string extension=dot==std::string::npos?"":file.chart.substr(dot);
                std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return char(std::tolower(c));});
                if(extension==".mid" || extension==".midi") {
                    const std::string cache=CH_USERDATA "/external-charts";
                    (void)mkdir(CH_USERDATA "",0777);(void)mkdir(cache.c_str(),0777);
                    char key[80];std::snprintf(key,sizeof(key),"/%016llx-%llu",static_cast<unsigned long long>(file.hash),static_cast<unsigned long long>(file.bytes));
                    std::string directory=cache+key;unsigned collision=0;
                    while(rhythm::storage::directory(directory) && !rhythm::storage::equalFiles(file.chart,directory+"/original.mid"))directory=cache+key+"-"+std::to_string(++collision);
                    if(!rhythm::storage::directory(directory) && mkdir(directory.c_str(),0777))continue;
                    const auto sourceIni=rhythm::caseFile(file.folder,"song.ini");
                    const auto cachedIni=rhythm::caseFile(directory,"song.ini");
                    const bool sameIni=sourceIni.empty()?cachedIni.empty():
                        (!cachedIni.empty() && rhythm::storage::equalFiles(sourceIni,cachedIni,&self->storageCancel));
                    const auto cachedChart=directory+"/notes.chart";
                    if(!sameIni || !chorus::midiChartCacheCurrent(cachedChart)) {
                        if(!rhythm::storage::equalFiles(file.chart,directory+"/original.mid") &&
                           (!copyCacheFile(file.chart,directory+"/original.mid.tmp") ||
                            !rhythm::storage::equalFiles(file.chart,directory+"/original.mid.tmp") ||
                            std::rename((directory+"/original.mid.tmp").c_str(),(directory+"/original.mid").c_str())))continue;
                        // Stage metadata first, but commit it after conversion.
                        // A failed conversion must leave the old INI mismatch
                        // visible, forcing a retry instead of reusing stale notes.
                        if(!sourceIni.empty() && !copyCacheFile(sourceIni,directory+"/song.ini.tmp")) {
                            self->storageWorkerError+=file.chart+": Cannot cache song metadata.\n";continue;
                        }
                        std::string error;const auto temporary=directory+"/notes.chart.tmp";
                        if(!chorus::midiToChart(file.chart,temporary,error,&self->storageCancel)) {
                            self->storageWorkerError+=file.chart+": "+error+"\n";continue;
                        }
                        if(std::rename(temporary.c_str(),cachedChart.c_str())) {
                            self->storageWorkerError+=file.chart+": Cannot commit the imported chart.\n";continue;
                        }
                        const int metadataResult=sourceIni.empty()?
                            (cachedIni.empty()?0:std::remove(cachedIni.c_str())):
                            std::rename((directory+"/song.ini.tmp").c_str(),(directory+"/song.ini").c_str());
                        if(metadataResult){self->storageWorkerError+=file.chart+": Cannot commit song metadata.\n";continue;}
                    }
                    song.chartPath=cachedChart;
                }
                self->readSongMetadata(file.root,file.chart,song);
                song.dateAdded=self->loadOrCreateSongDate(file.chart);
                for(const auto& root:self->storageExternalSnapshot)if(file.root==root){song.externalRoot=root;break;}
                indexed.emplace(file.hash,self->storageSongs.size());self->storageSongs.push_back(std::move(song));
                if(external(file))++self->storageUsbLoaded;
            }
        }
        self->storageDone.store(true,std::memory_order_release);return nullptr;
    }
    void startStorageJob(StorageJob job) {
        if(storageThreadActive)return;
        storageJob=job;storageDone=false;storageCancel=false;storageProgress=storageTotal=0;storageWorkerError.clear();storageSongs.clear();
        if(job==StorageJob::Scan){storageUsbFound=0;storageUsbLoaded=0;storageUsbDuplicates=0;storagePopupActive=!externalSongRoots.empty();storagePopupComplete=false;}
        storageScanRoots={internalSongRoot()};
        if(!storageQa && rhythm::storage::directory(CH_LEGACY_SONGS))storageScanRoots.push_back(CH_LEGACY_SONGS);
        storageExternalSnapshot=externalSongRoots;
        storageScanRoots.insert(storageScanRoots.end(),externalSongRoots.begin(),externalSongRoots.end());
        storageThreadActive=pthread_create(&storageThread,nullptr,storageWorker,this)==0;
        if(!storageThreadActive)storageResultText="Could not start song storage worker.";
    }
    void publishStorageSongs(std::vector<LocalSong> songs) {
        const std::string selected=songSelection>=0 && songSelection<int(localSongs.size())?localSongs[songSelection].chartPath:"";
        const std::string preview=menuMusicSelection>=0 && menuMusicSelection<int(localSongs.size())?localSongs[menuMusicSelection].chartPath:"";
        localSongs=std::move(songs);sortLocalSongs();++songListRevision;
        songSelection=localSongs.empty()?0:std::min(songSelection,int(localSongs.size())-1);menuMusicSelection=-1;
        bool selectionFound=false;
        for(size_t i=0;i<localSongs.size();++i){if(localSongs[i].chartPath==selected){songSelection=int(i);selectionFound=true;}if(localSongs[i].chartPath==preview)menuMusicSelection=int(i);}
        if(screen==Screen::PlayerSelect && !selectionFound){screen=Screen::SongSelect;playerSelectionMessage="The selected song drive was removed.";}
        if(screen!=Screen::Gameplay && !menuPreviewAudioChartPath.empty() &&
           std::none_of(localSongs.begin(),localSongs.end(),[&](const auto& song){return song.chartPath==menuPreviewAudioChartPath;})) {
#if !defined(PS5_HOST_REFERENCE)
            songAudio.pause();
#endif
            menuPreviewAudioChartPath.clear();
        }
        if(menuMusicSelection<0 && screen==Screen::MainMenu){menuPreviewAudioChartPath.clear();menuPreviewChartPath.clear();}
        previewQueuedSongSelection=-1;
    }
    void writeStorageStatus() {
        FILE* log=std::fopen("/app0/song-storage-status.txt","w");if(!log)return;
        unsigned external=0;for(const auto& song:localSongs)if(!song.externalRoot.empty())++external;
        std::fprintf(log,"library total %zu external %u roots %zu busy %d\n",localSongs.size(),external,externalSongRoots.size(),storageThreadActive?1:0);
        for(int i=0;i<8;++i) {
            const auto root="/mnt/usb"+std::to_string(i)+"/SONGS";errno=0;DIR* directory=opendir(root.c_str());const int error=directory?0:errno;
            std::fprintf(log,"root %s readable %d errno %d\n",root.c_str(),directory!=nullptr,error);if(directory)closedir(directory);
        }
        for(const auto& song:localSongs)if(!song.externalRoot.empty())std::fprintf(log,"external title %s artist %s source %s chart %s\n",song.title.c_str(),song.artist.c_str(),song.originalChartPath.c_str(),song.chartPath.c_str());
        if(!storageThreadActive && !storageWorkerError.empty())std::fprintf(log,"import errors\n%s",storageWorkerError.c_str());
        std::fclose(log);
    }
    void updateSongStorage() {
        if(ps5.frameBudget && !storageQa && !storageLiveQa)return;
        if(storageThreadActive && storageDone.load(std::memory_order_acquire)) {
            (void)pthread_join(storageThread,nullptr);storageThreadActive=false;
            if(storageJob==StorageJob::Scan) {
                if(!storageCancel && storageExternalSnapshot==externalSongRoots) {
                    publishStorageSongs(std::move(storageSongs));
                    {
                        storagePopupActive=false;storagePopupComplete=true;
                        storagePopupFound=storageUsbFound.load();storagePopupLoaded=storageUsbLoaded.load();storagePopupDuplicates=storageUsbDuplicates.load();
                        storagePopupFailed=storagePopupFound-storagePopupLoaded-storagePopupDuplicates;
                    }
                    if(!storageWorkerError.empty())storageResultText="Some song charts could not be imported.";
                    else if(storageResultText=="Some song charts could not be imported.")storageResultText.clear();
                }
            } else if(storageJob==StorageJob::Remove) {
                removedBundledSongs=rhythm::storage::removedBundles(CH_USERDATA "/removed-bundled-songs.cfg");
                char text[220];std::snprintf(text,sizeof(text),"Removed: %u    Failed: %u%s",storageRemovalResult.removed+storageRemovalResult.hidden,storageRemovalResult.failed,storageRemovalResult.cancelled?"    Cancelled":"");
                storageResultText=text;
                if(!storageRemovalResult.error.empty())storageResultText+="\n"+storageRemovalResult.error;
                if(storageRemovalResult.cleanupFailed)storageResultText+="\nSome hidden files could not be erased.";
                storageNeedsScan=true;removalPage=3;
            } else if(storageJob==StorageJob::Preview) {
                if(screen==Screen::SongCopyConfirm && songCopyPage==SongCopyPage::Confirm &&
                   !storageCancel && songCopyPlanRoot==songCopyRoot && songCopyPlanToConsole==songCopyToConsole) songCopyPlanReady=true;
            } else {
                char result[160];std::snprintf(result,sizeof(result),"Copied: %u   Already installed: %u   Failed: %u%s",storageCopyResult.copied,storageCopyResult.duplicates,storageCopyResult.failed,storageCopyResult.cancelled?"   Cancelled":"");
                storageResultText=result;if(!storageCopyResult.error.empty())storageResultText+=" - "+storageCopyResult.error;
                storageNeedsScan=true;
                if(screen==Screen::SongCopyConfirm)songCopyPage=SongCopyPage::Result;
            }
            writeStorageStatus();
        }
        if(ImGui::GetTime()>=storageNextPoll) {
            const bool initialPoll=storageNextPoll==0;
            storageNextPoll=ImGui::GetTime()+1.0;std::vector<std::string> roots;
            if(storageQa){if(storageQaMounted)roots.push_back("/app0/storage-qa/USB/SONGS");}
            else for(int i=0;i<8;++i){const auto root="/mnt/usb"+std::to_string(i)+"/SONGS";if(rhythm::storage::directory(root))roots.push_back(root);}
            if(roots!=externalSongRoots) {
                const auto old=externalSongRoots;externalSongRoots=std::move(roots);storageNeedsScan=true;
#if !defined(PS5_HOST_REFERENCE)
                // Open files may remain readable briefly after removal. Stop
                // from the mount observer too, without waiting for an I/O error.
                if(screen==Screen::Gameplay && gameplaySongInfoValid && !gameplaySongInfo.externalRoot.empty() &&
                   std::find(externalSongRoots.begin(),externalSongRoots.end(),gameplaySongInfo.externalRoot)==externalSongRoots.end())songAudio.sourceUnavailable();
#endif
                const bool inserted=std::any_of(externalSongRoots.begin(),externalSongRoots.end(),[&](const auto& root){return std::find(old.begin(),old.end(),root)==old.end();});
                if(inserted){storagePopupActive=true;storagePopupComplete=false;storagePopupFound=storagePopupLoaded=storagePopupDuplicates=storagePopupFailed=0;}
                else if(externalSongRoots.empty())storagePopupActive=false;
                if(storageThreadActive && (storageJob==StorageJob::Scan || storageJob==StorageJob::Preview))storageCancel=true;
                if(screen==Screen::SongCopyConfirm && songCopyPage==SongCopyPage::Confirm) {
                    songCopyPlanReady=false;
                    if(std::find(externalSongRoots.begin(),externalSongRoots.end(),songCopyRoot)==externalSongRoots.end())songCopyRoot.clear();
                }
                std::vector<LocalSong> retained;retained.reserve(localSongs.size());
                for(auto& song:localSongs)if(song.externalRoot.empty() || std::find(externalSongRoots.begin(),externalSongRoots.end(),song.externalRoot)!=externalSongRoots.end())retained.push_back(song);
                if(retained.size()!=localSongs.size())publishStorageSongs(std::move(retained));
                writeStorageStatus();
            }else if(initialPoll)writeStorageStatus();
        }
        if(storageNeedsScan && !storageThreadActive) {storageNeedsScan=false;startStorageJob(StorageJob::Scan);}
        if(!storageThreadActive && screen==Screen::SongCopyConfirm && songCopyPage==SongCopyPage::Confirm && !songCopyPlanReady &&
           !songCopyRoot.empty() && std::find(externalSongRoots.begin(),externalSongRoots.end(),songCopyRoot)!=externalSongRoots.end()) {
            songCopyPlanRoot=songCopyRoot;songCopyPlanToConsole=songCopyToConsole;
            storageCopyFrom=songCopyToConsole?songCopyRoot:internalSongRoot();
            storageCopyTo=songCopyToConsole?internalSongRoot():songCopyRoot;
            startStorageJob(StorageJob::Preview);
        }
    }
    void beginSongCopy(bool toConsole) {
        screen=Screen::SongCopyConfirm;storagePopupActive=false;
        if(storageThreadActive && storageJob==StorageJob::Copy){songCopyPage=SongCopyPage::Progress;return;}
        songCopyToConsole=toConsole;songCopyPage=SongCopyPage::Confirm;songCopyChoice=1;storageResultText.clear();songCopyPlanReady=false;
        storageDrive=externalSongRoots.empty()?0:std::clamp(storageDrive,0,int(externalSongRoots.size())-1);
        songCopyRoot=externalSongRoots.empty()?"":externalSongRoots[storageDrive];
    }
    void confirmSongCopy() {
        if(!songCopyPlanReady || !songCopyPlan.error.empty() || !songCopyPlan.toCopy)return;
        if(std::find(externalSongRoots.begin(),externalSongRoots.end(),songCopyRoot)==externalSongRoots.end() ||
           !rhythm::storage::directory(songCopyRoot)) {
            storageResultText="Connect a USB drive with a SONGS folder to continue.";return;
        }
        if(storageThreadActive){storageResultText="A library scan is running. Please try again shortly.";return;}
        if(!rhythm::storage::directory(internalSongRoot()) && mkdir(internalSongRoot().c_str(),0777)) {
            storageResultText="Console song storage is unavailable.";return;
        }
        storageCopyFrom=songCopyToConsole?songCopyRoot:internalSongRoot();
        storageCopyTo=songCopyToConsole?internalSongRoot():songCopyRoot;
        startStorageJob(StorageJob::Copy);
        if(storageThreadActive){storageResultText.clear();songCopyPage=SongCopyPage::Progress;}
    }

    void scanLocalSongs() {
        menuPreviewFailedPaths.clear();
        localSongs.clear();
        scanSongRoot(CH_SONGS "");
        if(rhythm::storage::directory(CH_LEGACY_SONGS))scanSongRoot(CH_LEGACY_SONGS);
        sortLocalSongs();
    }

    void loadChart(const std::string& path, int instrument = 0, int difficulty = 3,
                   uint8_t modifierMask = 0,bool backgroundAudio=false,int controllerType=-1,bool prepareAudio=true)
    {
        if(prepareAudio)resumeRewind.reset();
        if(controllerType>=0)chartProDrums=instrument==3 && (controllerType==3 || controllerType==4);
        practiceRate = 1.0f;
        practiceCountdownRemaining = 0.0;
        practiceSectionA = practiceSectionB = 0.0;
        practiceSectionASet = practiceSectionBSet = false;
        practiceSectionMessage[0] = '\0';
        lyricPhrases.clear();
        notes.clear();activeSustainNotes.clear();activeSustainNotes.reserve(64);solos.clear();soloBonusTotal=0;fretBonusTotal=0;resultStats={};resultsScroll=0;resultsSelection=0;
        chordNoteOffsets.clear();
        chart.notes.clear();chart.drumFills.clear();chart.kind=instrument==3?rhythm::ChartKind::Drums:instrument>=4?rhythm::ChartKind::SixFret:rhythm::ChartKind::FiveFret;
        session.reset();drumSession.reset();drumPadEdges.prime(0);
        drumFxClock=0;drumHitAt.fill(-1e30);drumPhraseHitAt.fill(-1e30);drumFretOffset.fill(0);for(auto& motion:drumFretMotion)motion.reset();
        chartFileLoaded = false;
        activeChartPath = path;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { notify_fail("chart not found: " + path); return; }
        if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return; }
        long len = ftell(f);
        if (len <= 0 || len > 32 * 1024 * 1024 || fseek(f, 0, SEEK_SET) != 0) {
            fclose(f); notify_fail("invalid chart size"); return;
        }
        std::string text(len > 0 ? len : 0, '\0');
        if (fread(&text[0], 1, static_cast<size_t>(len), f) != static_cast<size_t>(len)) {
            fclose(f); notify_fail("incomplete chart read"); return;
        }
        fclose(f);
        chartFileLoaded = true;

        int resolution = 192;
        bool importedMidi=false;
        int importedHopoThreshold=0;
        long importedEndTick=-1;
        std::string musicStream = "song.opus";
        chartOffsetSeconds = 0.0;
        trackStartOffsetSeconds = 0.0;
        {
            const std::string song = chartSection(text, "Song");
            auto songField = [&](const char* wanted) {
                size_t lineStart = 0;
                while (lineStart < song.size()) {
                    size_t lineEnd = song.find('\n', lineStart);
                    if (lineEnd == std::string::npos) lineEnd = song.size();
                    const std::string line = trimText(song.substr(lineStart, lineEnd - lineStart));
                    const size_t equal = line.find('=');
                    if (equal != std::string::npos) {
                        std::string key = trimText(line.substr(0, equal));
                        std::transform(key.begin(), key.end(), key.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        std::string value = trimText(line.substr(equal + 1));
                        if (key == wanted) {
                            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                                value = value.substr(1, value.size() - 2);
                            return value;
                        }
                    }
                    lineStart = lineEnd + 1;
                }
                return std::string{};
            };
            importedMidi=songField("midiimport")=="1";
            importedHopoThreshold=std::clamp(std::atoi(songField("midihopothreshold").c_str()),0,65535);
            const auto midiEnd=songField("midiendtick");
            if(!midiEnd.empty())importedEndTick=std::strtol(midiEnd.c_str(),nullptr,10);
            const std::string resolutionText = songField("resolution");
            if (!resolutionText.empty()) resolution = std::atoi(resolutionText.c_str());
            const std::string streamText = songField("musicstream");
            if (!streamText.empty()) musicStream = streamText;
            const std::string offsetText = songField("offset");
            if (!offsetText.empty()) {
                char* end = nullptr;
                chartOffsetSeconds = std::strtod(offsetText.c_str(), &end);
                if (end == offsetText.c_str() || !trimText(end ? end : "").empty() || !std::isfinite(chartOffsetSeconds)) {
                    chartOffsetSeconds = 0.0;
                    notify_fail("invalid chart Offset; using zero");
                }
            }
        }
        double songOffsetOverride = 0.0;
        if (loadSongOffsetOverride(path, songOffsetOverride))
            chartOffsetSeconds = songOffsetOverride;
        trackStartOffsetSeconds = chartOffsetSeconds - readSongDelaySeconds(path);
        rhythm::ClockConfig clockConfig{};
        clockConfig.audioReferenceOffset = -trackStartOffsetSeconds;
        songClock = rhythm::AudioClock(clockConfig);
        songClock.reset(-LEAD_IN);
        if (resolution <= 0) resolution = 192;
        chartResolutionTicks = resolution;chart.resolution=resolution;

        std::vector<rhythm::BeatSignature> signatures{{0,4,4}};
        std::vector<std::pair<long,double>> bpms,midiTempos;
        forEachEntry(chartSection(text, "SyncTrack"), [&](long tick, const std::string& r) {
            if(r.rfind("TS ",0)==0) {
                unsigned numerator=4,power=2;
                const int fields=sscanf(r.c_str(),"TS %u %u",&numerator,&power);
                if(fields>=1 && numerator>0 && power<16) signatures.push_back({tick,numerator,1u<<power});
            }
            if(importedMidi && r.rfind("MPQN ",0)==0){
                const unsigned long tempo=std::strtoul(r.c_str()+5,nullptr,10);
                if(tempo>0&&tempo<=0xffffff)midiTempos.push_back({tick,60000000.0/static_cast<double>(tempo)});
            }
            if (!r.empty() && r[0] == 'B') { double bpm = atol(r.c_str() + 1) / 1000.0;
                if (bpm > 0) bpms.push_back({ tick, bpm }); }
        });
        if(importedMidi&&!midiTempos.empty())bpms=std::move(midiTempos);
        std::stable_sort(signatures.begin(),signatures.end(),[](const auto& a,const auto& b){return a.tick<b.tick;});
        std::stable_sort(bpms.begin(), bpms.end());
        if (bpms.empty() || bpms.front().first > 0) bpms.insert(bpms.begin(), { 0, 120.0 });

        tempoSegments.clear();
        tempoSegments.push_back({0.0, 0.0, bpms.front().second});
        double accumulatedSeconds = 0.0;
        double accumulatedBeats = 0.0;
        long previousTempoTick = 0;
        double activeBpm = bpms.front().second;
        for (size_t i = 1; i < bpms.size(); ++i) {
            const long tempoTick = bpms[i].first;
            if (tempoTick < previousTempoTick) continue;
            const long elapsedTicks = tempoTick - previousTempoTick;
            accumulatedSeconds += (elapsedTicks * 60.0) / (activeBpm * resolution);
            accumulatedBeats += static_cast<double>(elapsedTicks) / resolution;
            tempoSegments.push_back({accumulatedSeconds, accumulatedBeats, bpms[i].second,tempoTick});
            previousTempoTick = tempoTick;
            activeBpm = bpms[i].second;
        }

        auto tickToSec = [&](long tick)->double {
            auto at=std::upper_bound(tempoSegments.begin(),tempoSegments.end(),tick,
                [](long value,const TempoSegment& segment){return value<segment.startTick;});
            const auto& segment=at==tempoSegments.begin()?tempoSegments.front():*(at-1);
            return rhythm::pcTimeAtTick(tick,segment.startSec,segment.startTick,resolution,segment.bpm);
        };

        globalChartEndSeconds=0;authoredSongEndSeconds=-1;long globalLastTick=0;
        for(size_t at=0;(at=text.find('[',at))!=std::string::npos;) {
            const auto close=text.find(']',at);if(close==std::string::npos)break;
            const auto name=text.substr(at+1,close-at-1);at=close+1;
            if(name.rfind("Easy",0)!=0 && name.rfind("Medium",0)!=0 && name.rfind("Hard",0)!=0 && name.rfind("Expert",0)!=0)continue;
            const bool drums=name.ends_with("Drums"),ghl=name.find("GHL")!=std::string::npos;
            forEachEntry(chartSection(text,name.c_str()),[&](long tick,const std::string& entry){
                int fret=-1;long length=0;
                if(std::sscanf(entry.c_str(),"N %d %ld",&fret,&length)!=2 || tick<0 || length<0)return;
                const bool playable=(fret>=0 && fret<=4) || (drums?(fret==5 || fret==32):(fret==7 || (ghl && fret==8)));
                if(playable)globalLastTick=std::max(globalLastTick,tick+length);
            });
        }
        globalChartEndSeconds=tickToSec(globalLastTick);
        if(importedMidi && importedEndTick>=globalLastTick+resolution)authoredSongEndSeconds=tickToSec(importedEndTick);
        starPowerClock.build(resolution,bpms,signatures,tickToSec);
        // Song-wide vocal events use the same tempo map as every instrument.
        std::vector<rhythm::LyricEvent> lyricEvents;
        forEachEntry(chartSection(text,"Events"),[&](long tick,const std::string& entry) {
            if(tick<0 || entry.rfind("E ",0)!=0 || lyricEvents.size()>=100000)return;
            std::string event=trimText(entry.substr(2));
            if(event.size()>=2 && event.front()=='"' && event.back()=='"')event=event.substr(1,event.size()-2);
            if(event=="phrase_start")lyricEvents.push_back({tickToSec(tick),0,{}});
            else if(event=="phrase_end")lyricEvents.push_back({tickToSec(tick),1,{}});
            else if(event.rfind("lyric ",0)==0)lyricEvents.push_back({tickToSec(tick),2,event.substr(6)});
        });
        lyricPhrases=rhythm::pcLyricPhrases(std::move(lyricEvents));
        practiceMarks.clear();
        forEachEntry(chartSection(text,"Events"),[&](long tick,const std::string& event) {
            const size_t at=event.find("section ");
            if(at==std::string::npos || practiceMarks.size()>=2048) return;
            std::string name=event.substr(at+8);
            while(!name.empty() && (name.back()=='"' || name.back()=='\r')) name.pop_back();
            if(!name.empty()) practiceMarks.push_back({name,tickToSec(tick)});
        });
        std::stable_sort(practiceMarks.begin(),practiceMarks.end(),[](const auto& a,const auto& b){return a.time<b.time;});
        if(practiceMarks.empty() || practiceMarks.front().time>0.0) practiceMarks.insert(practiceMarks.begin(),{"Full Song",0.0});
        static constexpr const char* difficultyNames[] = { "Easy", "Medium", "Hard", "Expert" };
        static constexpr const char* instrumentTracks[] = { "Single", "DoubleBass", "DoubleRhythm", "Drums", "GHLGuitar", "GHLBass", "GHLRhythm", "GHLCoop" };
        if (instrument < 0 || instrument >= static_cast<int>(std::size(instrumentTracks))) instrument = 0;
        if (difficulty < 0 || difficulty >= static_cast<int>(std::size(difficultyNames))) difficulty = 3;
        chartInstrument = instrument;
        chartDifficulty = difficulty;
        rhythm::logDiagnostic("chart request key=%016llx instrument=%d difficulty=%d",static_cast<unsigned long long>(songOffsetKey(path)),instrument,difficulty);
        bestScore = 0;
        newHighScore = false;
        (void)loadBestScore(path, chartInstrument, chartDifficulty, bestScore);
        char diff[48];
        std::snprintf(diff, sizeof(diff), "%s%s", difficultyNames[difficulty], instrumentTracks[instrument]);
        const auto hopoFrequencyText=readSongIniValue(path,"hopo_frequency");
        char* hopoEnd=nullptr;
        const long frequency=std::strtol(hopoFrequencyText.c_str(),&hopoEnd,10);
        const int hopoFrequency=hopoEnd!=hopoFrequencyText.c_str() && trimText(hopoEnd).empty()
            ? static_cast<int>(std::clamp(frequency,0L,65535L)) : 0;
        auto eighthText=readSongIniValue(path,"eighthnote_hopo");
        std::transform(eighthText.begin(),eighthText.end(),eighthText.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        songHopoThreshold=rhythm::pcHopoThreshold(resolution,hopoFrequency,eighthText=="true" || eighthText=="1");
        if(importedMidi&&hopoFrequency<=0)songHopoThreshold=(eighthText=="true"||eighthText=="1")?resolution/2+1:resolution/3+1;
        if(importedMidi && importedHopoThreshold>0 && hopoFrequency<=0)songHopoThreshold=importedHopoThreshold;
        std::vector<std::pair<long,long>> parsedSolos;
        std::optional<long> soloStart;
        struct ParsedGem { long tick; int fret; int length; };
        std::vector<ParsedGem> parsedGems;
        std::vector<std::pair<long, long>> parsedStarPhrases;
        struct DrumRegion{long start,end;int type;};std::vector<DrumRegion> drumRegions;
        forEachEntry(chartSection(text, diff), [&](long tick, const std::string& r) {
            int fret = -1, lenTicks = 0;
            if (!r.empty() && r[0] == 'N' && sscanf(r.c_str(), "N %d %d", &fret, &lenTicks) == 2) {
                if (tick >= 0 && lenTicks >= 0 && fret >= 0 && (fret <= 7 || (instrument>=4 && fret==8) || (instrument==3 &&
                    (fret==32 || (fret>=34 && fret<=38) || (fret>=40 && fret<=44) || (fret>=66 && fret<=68)))))
                    parsedGems.push_back({tick, fret, lenTicks});
            } else if(r=="E \"solo\"" || r=="E solo") {
                if(tick>=0 && !soloStart) soloStart=tick;
            } else if(r=="E \"soloend\"" || r=="E soloend") {
                // PC chart events include the soloend tick. The private MIDI
                // cache retains note-off's exclusive endpoint instead.
                if(soloStart && tick>*soloStart) parsedSolos.push_back({*soloStart,tick+(!importedMidi && tick<std::numeric_limits<long>::max()?1:0)});
                soloStart.reset();
            } else if (!r.empty() && r[0] == 'S') {
                int phrase = 0, lenTicks = 0;
                if (sscanf(r.c_str(), "S %d %d", &phrase, &lenTicks) == 2 &&
                    tick >= 0 && lenTicks > 0) {
                    if(phrase==2)parsedStarPhrases.push_back({tick,tick+lenTicks});
                    if(instrument==3 && phrase>=64 && phrase<=66)drumRegions.push_back({tick,tick+lenTicks,phrase});
                }
            }
        });
        starPhrases.clear();
        starPhrases.reserve(parsedStarPhrases.size());
        for (const auto& phrase : parsedStarPhrases)
            starPhrases.push_back({tickToSec(phrase.first), tickToSec(phrase.second)});
        std::stable_sort(parsedGems.begin(), parsedGems.end(),
            [&](const ParsedGem& a, const ParsedGem& b) {return a.tick<b.tick || (instrument==3 && a.tick==b.tick && a.fret<b.fret);});
        // Coalesce duplicate lane events exactly once before chord grouping.
        // This includes multiple frets converted to Open by a MIDI PS phrase.
        size_t uniqueGems=0;long uniqueTick=-1;std::array<size_t,69> laneAt{};laneAt.fill(SIZE_MAX);
        for(size_t i=0;i<parsedGems.size();++i) {
            const auto gem=parsedGems[i];
            if(gem.tick!=uniqueTick){uniqueTick=gem.tick;laneAt.fill(SIZE_MAX);}
            auto& same=laneAt[size_t(gem.fret)];
            if(same!=SIZE_MAX)parsedGems[same].length=std::max(parsedGems[same].length,gem.length);
            else {same=uniqueGems;parsedGems[uniqueGems++]=gem;}
        }
        parsedGems.resize(uniqueGems);
        std::vector<long> forcedHopoTicks, tapTicks;
        for (const ParsedGem& gem : parsedGems) {
            if (gem.fret == 5) forcedHopoTicks.push_back(gem.tick);
            if (gem.fret == 6) tapTicks.push_back(gem.tick);
        }
        std::sort(forcedHopoTicks.begin(), forcedHopoTicks.end());
        forcedHopoTicks.erase(std::unique(forcedHopoTicks.begin(), forcedHopoTicks.end()), forcedHopoTicks.end());
        std::sort(tapTicks.begin(), tapTicks.end());
        tapTicks.erase(std::unique(tapTicks.begin(), tapTicks.end()), tapTicks.end());
        long previousPlayableTick = -1;
        unsigned previousPlayableMask = 0;
        for (size_t begin = 0; begin < parsedGems.size();) {
            size_t end = begin + 1;
            while (end < parsedGems.size() && parsedGems[end].tick == parsedGems[begin].tick) ++end;
            const long tick = parsedGems[begin].tick;
            size_t playableCount = 0;
            unsigned playableMask = 0;
            for (size_t i = begin; i < end; ++i) {
                if (instrument==3 ? (parsedGems[i].fret>5) : (parsedGems[i].fret == 5 || parsedGems[i].fret == 6)) continue;
                ++playableCount;
                playableMask |= parsedGems[i].fret==7 ? 1u : (2u<<(parsedGems[i].fret==8?5:parsedGems[i].fret));
            }
            // .chart threshold/config 0x1820C3020; classification 0x182170210.
            const bool naturalHopo = instrument!=3 && rhythm::naturalGuitarHopo(
                playableMask,previousPlayableMask,tick-previousPlayableTick,songHopoThreshold);
            const bool forcedHopo = std::binary_search(forcedHopoTicks.begin(), forcedHopoTicks.end(), tick);
            const bool tapMarker = std::binary_search(tapTicks.begin(), tapTicks.end(), tick);
            const bool inStarPower = std::any_of(parsedStarPhrases.begin(), parsedStarPhrases.end(),
                [&](const auto& phrase) { return tick >= phrase.first && tick < phrase.second; });
            for (size_t i = begin; i < end; ++i) {
                const ParsedGem& gem = parsedGems[i];
                if (instrument==3 ? (gem.fret>5) : (gem.fret == 5 || gem.fret == 6)) continue;
                const double t0 = tickToSec(gem.tick);
                const double sus = tickToSec(gem.tick + gem.length) - t0;
                Note note{};
                note.timeSec = t0;
                note.sustainSec = sus;
                if (instrument == 3 && gem.fret <= 5) {
                    // .chart drum lanes: 0 kick, 1 red, 2 yellow, 3 blue,
                    // 4 green. Place them on the five-lane highway's matching
                    // colors (green, red, yellow, blue, orange/kick).
                    static constexpr uint8_t drumLaneToHighway[]{4, 1, 2, 3, 0};
                    note.lane = drumLaneToHighway[std::min(gem.fret,4)];
                    note.drumMask=static_cast<uint16_t>(1u<<std::min(gem.fret,4));
                    // Normal drums fold authored cymbals to tom masks. No
                    // ordinary drum sustain awards guitar-tail points.
                    note.drumFlags=gem.fret==1?0x800000:gem.fret?0x400000:0;
                    for(size_t marker=begin;marker<end;++marker)if(parsedGems[marker].fret==gem.fret+64 && gem.fret>=2 && gem.fret<=4)
                        note.drumFlags=(note.drumFlags&~0x400000u)|rhythm::pc_drums::cymbal;
                    for(size_t marker=begin;chartProDrums && marker<end;++marker) {
                        if(parsedGems[marker].fret==gem.fret+33)note.drumFlags|=rhythm::pc_drums::accent;
                        if(parsedGems[marker].fret==gem.fret+39)note.drumFlags|=rhythm::pc_drums::ghost;
                    }
                    if(chartProDrums && (note.drumFlags&rhythm::pc_drums::cymbal))
                        note.drumMask=static_cast<uint16_t>(1u<<(gem.fret+3));
                    note.sustainSec=note.sustainBeats=0;note.lengthTicks=0;
                } else {
                    note.lane = instrument>=4 ? rhythm::sixFretChartLane(gem.fret) : gem.fret == 7 ? 5 : gem.fret;
                }
                note.beatTime = static_cast<double>(gem.tick) / resolution;
                note.sustainBeats = static_cast<double>(gem.length) / resolution;
                note.tick=gem.tick;note.lengthTicks=gem.length;
                if(instrument==3){note.sustainSec=note.sustainBeats=0;note.lengthTicks=0;}
                const auto head=rhythm::modifiedGuitarHead(
                    rhythm::chartGuitarHead(naturalHopo,forcedHopo,tapMarker),instrument==3?0:modifierMask);
                note.hopo=instrument!=3 && head==rhythm::GuitarHead::Hopo;
                note.tap=instrument!=3 && head==rhythm::GuitarHead::Tap;
                note.chord = playableCount > 1;
                // Modifier names and source flag values come from the PC enum;
                // the player screen stores them as compact UI-index bits.
                // Transform before chord masks are built so visuals and input
                // judgement both consume the modified chart.
                if (instrument != 3) {
                    if (modifierMask & (1u << 3)) note.lane = guitarLaneCount();    // AllOpens
                    if ((modifierMask & (1u << 5)) && note.lane < guitarLaneCount()) // MirrorMode
                        note.lane = instrument>=4 ? (note.lane/3)*3+2-note.lane%3 : 4-note.lane;
                }
                note.starPower = inStarPower;
                notes.push_back(note);
                if (inStarPower) {
                    for (auto& phrase : starPhrases)
                        if (t0 >= phrase.startSec && t0 < phrase.endSec) ++phrase.gemCount;
                }
            }
            if(playableCount) {
                previousPlayableMask=playableMask;
                previousPlayableTick=tick;
            }
            begin = end;
        }
        std::stable_sort(notes.begin(), notes.end(),
                  [](const Note& a, const Note& b) { return a.timeSec < b.timeSec; });
        if(instrument!=3) {
            std::vector<rhythm::PcSustainGem> sustainGems;
            sustainGems.reserve(notes.size());
            for(const auto& n:notes) sustainGems.push_back({n.tick,n.lengthTicks,1u<<n.lane});
            rhythm::normalizePcSustainLengths(sustainGems);
            rhythm::classifyPcSustains(sustainGems);
            for(size_t i=0;i<notes.size();++i) {
                notes[i].lengthTicks=sustainGems[i].length;
                notes[i].sustainBeats=double(notes[i].lengthTicks)/chartResolutionTicks;
                notes[i].sustainSec=tickToSec(notes[i].tick+notes[i].lengthTicks)-notes[i].timeSec;
                notes[i].extended=sustainGems[i].extended;
                notes[i].disjoint=sustainGems[i].disjoint;
            }
        }
        if(instrument==3 && controllerType==3) {
            for(size_t first=0;first<notes.size();) {
                size_t end=first+1;uint16_t chord=notes[first].drumMask;
                while(end<notes.size() && notes[end].tick==notes[first].tick)chord|=notes[end++].drumMask;
                constexpr int highwayLane[]{4,1,2,3,0,2,3,0};
                for(size_t i=first;i<end;++i) {
                    auto& note=notes[i];note.drumMask=rhythm::pc_drums::fiveLaneChartMask(note.drumMask,chord);
                    const unsigned lane=std::countr_zero(unsigned(note.drumMask));
                    if(lane<8)note.lane=highwayLane[lane];
                }
                first=end;
            }
        }
        if(instrument==3) {
            // PC marks roll masks after instrument compatibility conversion.
            // Keep equal-tick objects in canonical mask order for dual rolls.
            std::stable_sort(notes.begin(),notes.end(),[](const Note& a,const Note& b){return a.tick<b.tick || (a.tick==b.tick && a.drumMask<b.drumMask);});
            for(const auto& region:drumRegions)if(region.type>=65)
                rhythm::pc_drums::markRoll(notes,region.start,region.end,region.type==66,resolution);
        }
        // Equal-time gems form one chronological chord, as in the exported desktop adapter.
        for (auto& n : notes) {
            if(instrument==3){
                rhythm::Note gem{n.timeSec,0,n.drumMask};gem.drumFlags=n.drumFlags;
                chart.notes.push_back(gem);n.chordIndex=chart.notes.size()-1;continue;
            }
            if (chart.notes.empty() || chart.notes.back().time != n.timeSec)
                chart.notes.push_back({n.timeSec, n.sustainSec, 0});
            auto& chord = chart.notes.back();
            if (n.lane < guitarLaneCount()) chord.frets |= static_cast<uint16_t>(1u << n.lane);
            else chord.open = true;
            chord.duration = std::max(chord.duration, n.sustainSec);
            chord.tap |= n.tap;
            chord.hopo |= n.hopo;
            chord.chord |= n.chord;
            chord.extended |= n.extended;
            chord.disjoint |= n.disjoint;
            n.chordIndex = chart.notes.size() - 1;
        }
        if(instrument==3)for(const auto& region:drumRegions)if(region.type==64){
            const size_t chosen=rhythm::pc_drums::explicitFillHead(notes,region.start,region.end,resolution);
            if(chosen<chart.notes.size()){chart.notes[chosen].drumFlags|=rhythm::pc_drums::activation;chart.drumFills.push_back({region.start,chosen});}
        }
        if(instrument==3 && !notes.empty() && std::none_of(drumRegions.begin(),drumRegions.end(),[](const auto& region){return region.type==64;})){
            // 0x1820B9720 generates fills from measure ticks. Its nearest-head
            // helper starts the reverse search at the saved cursor when there
            // is no following head; retain that source edge case at song end.
            std::vector<long> measures;
            long last=0;for(const auto& gem:parsedGems)last=std::max(last,gem.tick+gem.length);
            rhythm::pcBeatTicks(resolution,last+resolution,signatures,[&](long tick,unsigned kind){if(kind==0)measures.push_back(tick);});
            size_t cursor=0;
            for(size_t measure=1;measure<measures.size();){
                const long target=measures[measure];
                size_t following=cursor;
                while(following<notes.size() && notes[following].tick<=target)++following;
                size_t nearest=cursor;
                if(following<notes.size()){
                    nearest=following;
                    if(following>cursor && target-notes[following-1].tick<notes[following].tick-target)nearest=following-1;
                }
                if(std::abs(notes[nearest].tick-target)>resolution/2+1){++measure;continue;}
                const long headTick=notes[nearest].tick;
                size_t chosen=nearest;
                for(size_t i=0;i<notes.size();++i)if(notes[i].tick==headTick && notes[i].drumMask>notes[chosen].drumMask)chosen=i;
                chart.notes[chosen].drumFlags|=rhythm::pc_drums::activation;
                chart.drumFills.push_back({target,chosen});cursor=nearest;measure+=4;
            }
        }
        chordNoteOffsets.assign(chart.notes.size() + 1, notes.size());
        for (size_t i = 0; i < notes.size(); ++i)
            if (i == 0 || notes[i].chordIndex != notes[i-1].chordIndex)
                chordNoteOffsets[notes[i].chordIndex] = i;
        for(size_t head=0;head<chart.notes.size();++head) {
            const auto begin=chordNoteOffsets[head],end=chordNoteOffsets[head+1];
            for(size_t i=begin;i<end;++i) {
                auto& note=notes[i];note.sustainScoreOwner=true;note.sustainGroupMask=0;
                for(size_t j=begin;j<end;++j)if(notes[j].lengthTicks==note.lengthTicks) {
                    note.sustainGroupMask|=1u<<notes[j].lane;
                    if(j<i)note.sustainScoreOwner=false;
                }
                for(size_t phrase=0;phrase<starPhrases.size();++phrase)
                    if(note.timeSec>=starPhrases[phrase].startSec && note.timeSec<starPhrases[phrase].endSec)note.starPhraseIndex=int(phrase);
            }
        }
        std::sort(parsedSolos.begin(),parsedSolos.end());
        for(const auto& range:parsedSolos) {
            size_t first=chart.notes.size(),last=first;
            for(size_t head=0;head<chart.notes.size();++head) {
                const auto tick=notes[chordNoteOffsets[head]].tick;
                if(tick>=range.second) break;
                if(tick>=range.first) {if(first==chart.notes.size()) first=head;last=head;}
            }
            if(first!=chart.notes.size()) {
                // PC converter 0x1820B8AC0 anchors the visible range to its
                // first head and stores the last head/tail span as float.
                const double start=chart.notes[first].time;
                const float duration=static_cast<float>(chart.notes[last].time+
                    static_cast<float>(chart.notes[last].duration)-start);
                solos.push_back({first,last,start,start+duration});
            }
        }
        beatLines.clear();measureTimes.clear();countdownRanges.clear();
        long lastBeatTick=0;
        for(const auto& gem:parsedGems) lastBeatTick=std::max(lastBeatTick,gem.tick+gem.length);
        rhythm::pcBeatTicks(resolution,lastBeatTick,signatures,[&](long tick,unsigned kind) {
            const auto time=tickToSec(tick);beatLines.push_back({time,kind});
            if(kind==0)measureTimes.push_back(time);
        });
        countdownRanges=rhythm::pcCountdownRanges(chart,measureTimes);
        int64_t qualifyingTailTicks=0;
        for(const auto& note:notes) {
            // BaseEngine sums original integer lengths before dividing once.
            // Reconstructing them from beat fractions can introduce a new ULP.
            const auto ticks=note.lengthTicks;
            if(note.sustainScoreOwner && rhythm::pc_scoring::countsTowardBase(ticks,chartResolutionTicks)) qualifyingTailTicks+=ticks;
        }
        chartBaseScore=rhythm::pc_scoring::baseScore(static_cast<unsigned>(notes.size()),qualifyingTailTicks,chartResolutionTicks);
        if(instrument==3){
            // BaseEngine 0x1820F0B30 includes cymbal weighting and authored
            // dynamics in the star-rank denominator, independent of hit velocity.
            chartBaseScore=0;
            for(const auto& gem:chart.notes)chartBaseScore+=(50+((gem.frets&224)?15:0))*
                ((gem.drumFlags&(rhythm::pc_drums::ghost|rhythm::pc_drums::accent))?2:1);
        }

        if (!rhythm::validateChart(chart)) {
            notify_fail("invalid gameplay chart");
        }
#if !defined(PS5_HOST_REFERENCE)
        if (prepareAudio && !chart.notes.empty()) {
            if(backgroundAudio)requestGameplayAudio(path,musicStream);
            else {gameplayAudioPending=false;reloadSongAudio(path,musicStream);}
        }
#endif
        session.reset();drumSession.reset();drumPadEdges.prime(0);
        drumFxClock=0;drumHitAt.fill(-1e30);drumPhraseHitAt.fill(-1e30);drumFretOffset.fill(0);for(auto& motion:drumFretMotion)motion.reset();
        rhythm::logDiagnostic("chart ready heads=%zu gems=%zu solos=%zu resolution=%d",chart.notes.size(),notes.size(),solos.size(),resolution);
        printf("[chart] res=%d bpmEvents=%zu notes=%zu\n",
               resolution, (size_t)bpms.size(), (size_t)notes.size());
        songTime = -LEAD_IN;
    }

    // ---------- per-frame sprite mesh: multi-layer notes + receptors ----------
    // Vertex positions and UVs come from the original PC sprites and prefab layers.
    void sourceLayer(std::vector<TexVertex>& tv, const pc_guitar::Layer& layer,
                     float x, float z, const float* color, bool noteScale, float yOffset = 0,
                     const pc_guitar::Layer* nextFrame = nullptr, float frameBlend = 0.0f,
                     float brightness = 1.0f) {
        if (tv.size() + 6 > MAX_TEX_VERTS) return;
        const float gemScale = noteScale ? static_cast<float>(gemSizePercent) / 100.0f : 1.0f;
        const float scaleX = pc_guitar::worldScale * (noteScale ? pc_guitar::noteScaleX * gemScale : 1.0f);
        const float scaleY = pc_guitar::worldScale * (noteScale ? pc_guitar::noteScaleY * gemScale : 1.0f);
        const float uv[4][2] = {{layer.uv[0], layer.uv[3]}, {layer.uv[2], layer.uv[3]},
                               {layer.uv[2], layer.uv[1]}, {layer.uv[0], layer.uv[1]}};
        const float uvNext[4][2] = {
            {nextFrame ? nextFrame->uv[0] : layer.uv[0], nextFrame ? nextFrame->uv[3] : layer.uv[3]},
            {nextFrame ? nextFrame->uv[2] : layer.uv[2], nextFrame ? nextFrame->uv[3] : layer.uv[3]},
            {nextFrame ? nextFrame->uv[2] : layer.uv[2], nextFrame ? nextFrame->uv[1] : layer.uv[1]},
            {nextFrame ? nextFrame->uv[0] : layer.uv[0], nextFrame ? nextFrame->uv[1] : layer.uv[1]}};
        for (int index : {0, 1, 2, 0, 2, 3}) {
            const auto& p = layer.positions[index];
            const float* tint = layer.laneTint ? color : layer.color;
            tv.push_back({{x + p[0] * scaleX, p[1] * scaleY + yOffset, z - p[2] * pc_guitar::worldScale},
                          {uv[index][0], uv[index][1]},
                          {tint[0] * brightness, tint[1] * brightness, tint[2] * brightness, layer.color[3]},
                          {uvNext[index][0], uvNext[index][1]}, nextFrame ? frameBlend : 0.0f});
        }
    }
    // PC GuitarNoteRenderer selects open separately (mask 0x20). Active SP
    // recolors notes and tails; phrase star sprites remain a separate choice.
    static constexpr float openNoteColor[3] = {0.65f, 0.0f, 1.0f};
    static constexpr float sixFretOpenColor[3] = {0.72f,0.73f,0.75f};
    static constexpr float phraseGlowColor[3] = {0.0f, 0.75f, 1.0f};
    static constexpr float deployedNoteColor[3] = {0.0f, 1.0f, 1.0f};
    static constexpr float droppedTailColor[3] = {0.35f, 0.35f, 0.35f};
    const float* selectedStarPowerColor() const {
        return rhythm::starPowerColors[std::clamp(activeProfile.starPowerColor,0,5)].data();
    }
    void customStarPowerVertices(std::vector<TexVertex>& vertices,size_t first) const {
        if(!activeProfile.starPowerColor)return;
        const auto* color=selectedStarPowerColor();
        tintNoteVertices(vertices,first,color);
    }
    static void tintNoteVertices(std::vector<TexVertex>& vertices,size_t first,const float* color) {
        for(size_t i=first;i<vertices.size();++i) {
            auto& v=vertices[i];const float intensity=std::max({v.color[0],v.color[1],v.color[2]});
            for(unsigned k=0;k<3;++k)v.color[k]=color[k]*intensity;
            const float blend=v.frameBlend<0?-v.frameBlend-1:v.frameBlend;
            v.frameBlend=-(1+blend);
        }
    }
    const float* gameplayNoteColor(int lane) const {
        if (starPower.active() && noteStarPowerColors) return activeProfile.starPowerColor?selectedStarPowerColor():deployedNoteColor;
        if(chart.kind==rhythm::ChartKind::SixFret) {static constexpr float white[]{.94f,.94f,.94f},black[]{.12f,.12f,.12f};return openLane(lane)?sixFretOpenColor:lane<3?white:black;}
        return lane == 5 ? openNoteColor : fretColor[lane];
    }
    bool starPhraseVisible(const Note& note) const {
        return note.starPower && (note.starPhraseIndex<0 || !starPhrases[size_t(note.starPhraseIndex)].failed);
    }
    void addNote(std::vector<TexVertex>& tv, const Note& note, float x, float z, const float* c) {
        const bool phraseArt=starPhraseVisible(note);
        const size_t first=tv.size();
        const auto tapAppearance=[&] {
            const bool sp=noteStarPowerColors && (starPower.active() || phraseArt) &&
                (playMode!=PlayMode::Practice || practiceStarPowerEnabled);
            if(activeProfile.noteColor && !sp) {
                const auto color=rhythm::regularNoteColor(activeProfile.noteColor,renderTime);
                tintNoteVertices(tv,first,color.data());
            }
            if(noteStarPowerColors && (starPower.active() || phraseArt) &&
                (playMode!=PlayMode::Practice || practiceStarPowerEnabled))customStarPowerVertices(tv,first);
            if(!note.tap || !activeProfile.purpleTapNotes)return;
            for(size_t i=first;i<tv.size();++i) {
                // Preserve the original lane/artwork hue; the fragment adds
                // only a light purple sheen after sampling the real sprite.
                auto& v=tv[i];v.color[3]*=.72f;v.frameBlend=v.frameBlend<0? v.frameBlend-2.0f:v.frameBlend+2.0f;
            }
        };
        const size_t noteCount = std::size(pc_guitar::bodyFrames);
        const size_t starCount = std::size(pc_guitar::starBodyFrames);
        if (note.lane == 5) {
            const bool showStarArt = phraseArt && noteStarPowerColors &&
                (playMode != PlayMode::Practice || practiceStarPowerEnabled);
            const auto& frames = showStarArt ? pc_guitar::starOpenBodyFrames : pc_guitar::openBodyFrames;
            const size_t count = showStarArt ? std::size(pc_guitar::starOpenBodyFrames) : std::size(pc_guitar::openBodyFrames);
            const size_t frame = showStarArt ? starAnimationFrame : noteAnimationFrame;
            const float blend = showStarArt ? starAnimationBlend : noteAnimationBlend;
            sourceLayer(tv, pc_guitar::openBase, x, z, c, true);
            sourceLayer(tv, pc_guitar::openBody, x, z, c, true);
            sourceLayer(tv, frames[frame % count], x, z, showStarArt ? phraseGlowColor : c, true, 0.0f,
                        &frames[(frame + 1) % count], blend);
            if (note.hopo) sourceLayer(tv, pc_guitar::openHopo, x, z, c, true);
            tapAppearance();return;
        }
        const pc_guitar::Layer& body = note.tap ? pc_guitar::tapBody : pc_guitar::standardBody;
        if (phraseArt && noteStarPowerColors &&
            (playMode != PlayMode::Practice || practiceStarPowerEnabled)) {
            const pc_guitar::Layer* starFrames = note.tap ? pc_guitar::starTapBodyFrames : pc_guitar::starBodyFrames;
            const size_t count = note.tap ? std::size(pc_guitar::starTapBodyFrames) : starCount;
            const size_t frame = starAnimationFrame % count;
            // PC note state 0x1801FE050 assigns the body star animation and
            // a separate blue-tinted starPowerBottomSprites animation.
            const size_t bottomCount=std::size(pc_guitar::starBottomFrames);
            const size_t bottomFrame=starAnimationFrame%bottomCount;
            sourceLayer(tv,pc_guitar::starBottomFrames[bottomFrame],x,z,phraseGlowColor,true,0.0f,
                &pc_guitar::starBottomFrames[(bottomFrame+1)%bottomCount],starAnimationBlend);
            sourceLayer(tv, starFrames[frame], x, z, c, true, 0.0f,
                        &starFrames[(frame + 1) % count], starAnimationBlend);
            const auto& cap = note.tap ? pc_guitar::starCap :
                (note.hopo ? pc_guitar::starHopoCap : pc_guitar::starCap);
            sourceLayer(tv, cap, x, z, c, true);
        } else {
            sourceLayer(tv, body, x, z, c, true);
            const size_t frame = noteAnimationFrame % noteCount;
            // PC state 0x1801FE050 sets TapBodySprite and clears Anim.
            // An ordinary body animation would conceal the tap's dark body.
            if(!note.tap)
                sourceLayer(tv, pc_guitar::bodyFrames[frame], x, z, c, true, 0.0f,
                            &pc_guitar::bodyFrames[(frame + 1) % noteCount], noteAnimationBlend);
            sourceLayer(tv, note.tap ? pc_guitar::standardCap :
                (note.hopo ? pc_guitar::hopoCap : pc_guitar::standardCap), x, z, c, true);
        }
        tapAppearance();
    }
    void sixFretLayer(std::vector<TexVertex>& vertices,pc_six_fret::Layer layer,float x,float z,const float* color,bool note=true) {
        const float size=note?float(gemSizePercent)/100.0f:1.0f;
        for(auto& p:layer.positions){p[0]*=note?pc_six_fret::scaleX*size:1.0f;p[1]*=note?pc_six_fret::scaleY*size:1.0f;}
        sourceLayer(vertices,layer,x,z,color,false);
    }
    void addSixFretNote(std::vector<TexVertex>& vertices,const Note& note,float z) {
        const uint32_t first=vertices.size();
        static constexpr float white[]{1,1,1},black[]{.055f,.055f,.055f};
        const bool enabled=noteStarPowerColors && (playMode!=PlayMode::Practice || practiceStarPowerEnabled);
        const bool phrase=starPhraseVisible(note) && enabled,active=starPower.active() && enabled;
        const auto* highlight=active?selectedStarPowerColor():phrase?phraseGlowColor:white;
        if(openLane(note.lane)) {
            sixFretLayer(vertices,pc_six_fret::openBody,0,z,active?highlight:sixFretOpenColor);
            sixFretLayer(vertices,pc_six_fret::openCap,0,z,active?highlight:sixFretOpenColor);
            if(note.hopo || note.tap)sixFretLayer(vertices,pc_six_fret::openHopo,0,z,highlight);
            if(phrase)sixFretLayer(vertices,pc_six_fret::openBolt,0,z,phraseGlowColor);
        } else {
            const unsigned column=note.lane%3,mask=chart.notes[note.chordIndex].frets;
            const bool bar=(mask&(1u<<column)) && (mask&(1u<<(column+3)));
            if(bar && note.lane>=3)return;
            const bool upper=note.lane>=3;const float x=pc_six_fret::laneX[column];
            const auto& body=bar?pc_six_fret::chordBody:upper?pc_six_fret::upBody:pc_six_fret::downBody;
            const auto& ring=bar?pc_six_fret::chordRing:upper?pc_six_fret::upRing:pc_six_fret::downRing;
            sixFretLayer(vertices,note.hopo||note.tap?pc_six_fret::hopoShadow:pc_six_fret::shadow,x,z,white);
            sixFretLayer(vertices,body,x,z,active?highlight:upper?black:white);
            if(bar){sixFretLayer(vertices,pc_six_fret::chordInsideTop,x,z,black);sixFretLayer(vertices,pc_six_fret::chordInsideBottom,x,z,white);}
            else sixFretLayer(vertices,upper?pc_six_fret::upInside:pc_six_fret::downInside,x,z,active?highlight:upper?black:white);
            sixFretLayer(vertices,ring,x,z,highlight);
            if(note.hopo || note.tap)sixFretLayer(vertices,bar?pc_six_fret::chordHopo:upper?pc_six_fret::upHopo:pc_six_fret::downHopo,x,z,highlight);
            if(phrase)sixFretLayer(vertices,pc_six_fret::bolt,x,z,phraseGlowColor);
        }
        if(activeProfile.noteColor && !active && !phrase) {
            const auto color=rhythm::regularNoteColor(activeProfile.noteColor,renderTime);tintNoteVertices(vertices,first,color.data());
        }
        if((active || phrase) && activeProfile.starPowerColor)customStarPowerVertices(vertices,first);
        if(note.tap && activeProfile.purpleTapNotes)for(size_t i=first;i<vertices.size();++i) {
            auto& v=vertices[i];v.color[3]*=.72f;v.frameBlend=v.frameBlend<0?v.frameBlend-2.0f:v.frameBlend+2.0f;
        }
        spriteDraws.push_back({pc_six_fret::noteKey,GameplayDrawKind::SixFret,first,uint32_t(vertices.size())-first});
    }
    void addSixFretReceptors(std::vector<TexVertex>& vertices) {
        static constexpr float white[]{1,1,1},black[]{.12f,.12f,.12f};
        for(unsigned column=0;column<3;++column) {
            const auto layers=column==0?pc_six_fret::receptor0:column==1?pc_six_fret::receptor1:pc_six_fret::receptor2;
            const auto keys=column==0?pc_six_fret::receptorKeys0:column==1?pc_six_fret::receptorKeys1:pc_six_fret::receptorKeys2;
            const unsigned count=column==0?std::size(pc_six_fret::receptor0):column==1?std::size(pc_six_fret::receptor1):std::size(pc_six_fret::receptor2);
            const float glow=std::max(hitGlow[column],hitGlow[column+3]);
            for(unsigned i=0;i<count;++i) {
                auto layer=layers[i];const bool held=(frets&((1u<<column)|(1u<<(column+3))))!=0;
                const auto first=uint32_t(vertices.size());
                const auto* color=glow>.05f && starPower.active()?selectedStarPowerColor():held && i==2 && (frets&(1u<<(column+3)))?black:white;
                layer.laneTint=held || glow>.05f;if(held || glow>.05f)layer.color[3]=1;
                sixFretLayer(vertices,layer,pc_six_fret::laneX[column],0,color,false);
                spriteDraws.push_back({keys[i],GameplayDrawKind::SixFret,first,uint32_t(vertices.size())-first});
            }
        }
    }

    bool fiveLaneDrumView() const {return chart.kind==rhythm::ChartKind::Drums && activeProfile.controller==3;}
    unsigned drumVisualPad(unsigned lane) const {
        return fiveLaneDrumView()?rhythm::fiveLaneDrumPad(lane):(lane>4?lane-3:lane);
    }
    void fiveLaneDrumLayer(pc_guitar::Layer& layer,unsigned source,unsigned pad) const {
        if(!fiveLaneDrumView() || !pad)return;
        const float oldX=rhythm::drumPadX(source,4,pc_drums_art::firstX);
        const float newX=rhythm::drumPadX(pad-1,5,pc_drums_art::firstX);
        for(auto& point:layer.positions)point[0]=newX+(point[0]-oldX)*.75f;
        if(pad==4) {
            // Reuse the source green cymbal/fret layers with the orange tint.
            const float hi=std::max({layer.color[0],layer.color[1],layer.color[2]});
            const float lo=std::min({layer.color[0],layer.color[1],layer.color[2]});
            if(hi-lo>.1f && layer.color[1]>layer.color[0] && layer.color[1]>layer.color[2]) {
                layer.color[0]=hi;layer.color[1]=lo+(hi-lo)*.274509804f;layer.color[2]=lo;
            }
        }
    }
    void addDrumNote(std::vector<TexVertex>& tv,const Note& note,float z) {
        const unsigned lane=std::countr_zero(static_cast<unsigned>(note.drumMask));
        if(lane>=8)return;
        const auto flags=drumSession.noteFlags(note.chordIndex);
        const unsigned dynamics=(flags&rhythm::pc_drums::ghost)?1u:(flags&rhythm::pc_drums::accent)?2u:0u;
        const bool useSP=noteStarPowerColors && (playMode!=PlayMode::Practice || practiceStarPowerEnabled);
        const auto& state=pc_drums_art::state(lane,dynamics,starPhraseVisible(note) && useSP,starPower.active() && useSP,starPower.available() && useSP);
        const unsigned pad=drumVisualPad(lane);
        const float x=pad?rhythm::drumPadX(pad-1,fiveLaneDrumView()?5:4,pc_drums_art::firstX)*pc_guitar::worldScale:0.0f;
        std::array<unsigned,4> order{{0,1,2,3}};
        std::stable_sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return state.layers[a].order<state.layers[b].order;});
        const auto first=static_cast<uint32_t>(tv.size());
        const float size=gemSizePercent/100.0f;
        for(auto index:order){
            const auto& visual=state.layers[index];if(visual.role<0)continue;
            auto sprite=pc_drums_art::sprite(visual.role,static_cast<unsigned>(noteAnimationFrame));
            auto next=pc_drums_art::sprite(visual.role,static_cast<unsigned>(noteAnimationFrame+1));
            for(auto* layer:{&sprite,&next}){
                for(auto& point:layer->positions){point[0]*=state.scale[0]*size*(fiveLaneDrumView() && pad?.75f:1.f);point[1]*=state.scale[1]*size*(fiveLaneDrumView() && pad?.75f:1.f);}
                std::copy_n(visual.color,4,layer->color);layer->laneTint=false;
                if(fiveLaneDrumView() && pad==4) {
                    auto palette=*layer;fiveLaneDrumLayer(palette,3,pad);
                    std::copy_n(palette.color,4,layer->color);
                }
                if(lane==0 && activeProfile.usesPurpleKick() && !(useSP && (starPhraseVisible(note) || starPower.active())) &&
                    (visual.role==24 || visual.role==26)) {
                    const auto color=rhythm::drumPurpleKick(visual.role==26);
                    std::copy(color.begin(),color.end(),layer->color);
                }
            }
            sourceLayer(tv,sprite,x,z,visual.color,false,0,&next,noteAnimationBlend);
        }
        if(useSP && (starPhraseVisible(note) || starPower.active()))customStarPowerVertices(tv,first);
        auto key=pc_drums_art::noteKey;
        // Keep all four children atomic in their source SortingGroup. Pool
        // index orders far notes first; kick groups precede the tom groups.
        const auto firstVisible=visibleNotes.empty()?note.chordIndex:visibleNotes.front().noteIndex;
        key.order=state.group-static_cast<int>(note.chordIndex-firstVisible);
        spriteDraws.push_back({key,GameplayDrawKind::Drums,first,static_cast<uint32_t>(tv.size())-first});
    }
    template<size_t Count> void addDrumFret(std::vector<TexVertex>& tv,unsigned pad,
        const pc_guitar::Layer (&layers)[Count],const pc_render_order::Key (&keys)[Count],unsigned target=0){
        static constexpr unsigned visualLane[]{1,2,3,0};
        const float hit=fiveLaneDrumView()?float(std::max(0.0,.12-(drumFxClock-drumHitAt[target]))):hitGlow[visualLane[pad]];
        for(size_t i=0;i<Count;++i){
            auto layer=layers[i];
            const float cameraFactor=rhythm::pcPlayerCamera(gameplayPlayerCount()).factor;
            for(unsigned corner=0;corner<4;++corner)for(unsigned axis=0;axis<3;++axis)
                layer.positions[corner][axis]+=pc_player_scaling::drums[pad][i][corner][axis]*cameraFactor;
            fiveLaneDrumLayer(layer,pad,target);
            if(layer.heldLight?hit<=0.0f:!layer.enabled)continue;
            // Drum frets pulse on strikes, not on held guitar-style buttons.
            layer.laneTint=false;
            const auto first=static_cast<uint32_t>(tv.size());
            const float dy=layer.movingHead?drumFretOffset[fiveLaneDrumView()?target-1:pad]*pc_guitar::worldScale:0.0f;
            sourceLayer(tv,layer,0,0,layer.color,false,dy);
            spriteDraws.push_back({keys[i],GameplayDrawKind::Drums,first,static_cast<uint32_t>(tv.size())-first});
        }
    }
    void addDrumReceptors(std::vector<TexVertex>& tv){
        if(fiveLaneDrumView()) {
            addDrumFret(tv,0,pc_drums_art::fret0,pc_drums_art::fretKeys0,1);
            addDrumFret(tv,1,pc_drums_art::fret1,pc_drums_art::fretKeys1,2);
            addDrumFret(tv,2,pc_drums_art::fret2,pc_drums_art::fretKeys2,3);
            addDrumFret(tv,3,pc_drums_art::fret3,pc_drums_art::fretKeys3,4);
            addDrumFret(tv,3,pc_drums_art::fret3,pc_drums_art::fretKeys3,5);
            return;
        }
        addDrumFret(tv,0,pc_drums_art::fret0,pc_drums_art::fretKeys0);
        addDrumFret(tv,1,pc_drums_art::fret1,pc_drums_art::fretKeys1);
        addDrumFret(tv,2,pc_drums_art::fret2,pc_drums_art::fretKeys2);
        addDrumFret(tv,3,pc_drums_art::fret3,pc_drums_art::fretKeys3);
    }
    void sourceEffectLayer(std::vector<TexVertex>& tv,const pc_guitar::Layer& layer,unsigned lane,bool drums,bool sp=false) {
        const auto first=tv.size();sourceLayer(tv,layer,0,0,layer.color,false);
        if(sp) {tintNoteVertices(tv,first,selectedStarPowerColor());return;}
        if(!activeProfile.laneHitEffects)return;
        std::array<float,3> color;
        if(drums && lane==0 && activeProfile.usesPurpleKick())color=rhythm::drumPurpleKick();
        else {
            static constexpr unsigned drumLane[]{4,1,2,3,0};
            const unsigned index=drums?(fiveLaneDrumView() && lane?lane%5:drumLane[std::min(lane,4u)]):lane;
            const float* tint=index<5?fretColor[index]:openNoteColor;
            std::copy_n(tint,3,color.begin());
        }
        const float energy=std::max({layer.color[0],layer.color[1],layer.color[2]});
        for(size_t i=first;i<tv.size();++i) {
            for(unsigned k=0;k<3;++k)tv[i].color[k]=color[k]*energy;
            // Negative blend tags optional luminance tint in the existing
            // sprite shader. It preserves source texture alpha and animation.
            tv[i].frameBlend=-1;
        }
    }
    void addDrumAnimation(std::vector<TexVertex>& tv,const pc_drums_art::Animation& animation,double age,bool active,unsigned pad,bool palette=true){
        if(age<0 || (animation.length>0 && age>=animation.length))return;
        const unsigned frame=(static_cast<unsigned>(age/animation.interval)+animation.start)%animation.count;
        auto layer=animation.frames[frame];layer.laneTint=false;
        // Drum phrase effects use Animator.Play() without a color argument
        // (0x1800F48D0). Its unused palette is zero; preserve renderer color.
        if(palette)std::copy_n(active?animation.active:animation.normal,4,layer.color);
        if(gameplayReplayTrace && !palette)std::fprintf(gameplayReplayTrace,"# drum_fx %u %.9g %.5g %.5g %.5g %.5g\n",pad,age,layer.color[0],layer.color[1],layer.color[2],layer.color[3]);
        fiveLaneDrumLayer(layer,rhythm::fiveLaneDrumSource(pad),pad);
        const auto first=static_cast<uint32_t>(tv.size());
        sourceEffectLayer(tv,layer,pad,true,active);
        spriteDraws.push_back({animation.key,GameplayDrawKind::Drums,first,static_cast<uint32_t>(tv.size())-first,animation.additive});
    }
    void addDrumHitEffects(std::vector<TexVertex>& tv){
        for(unsigned pad=0;pad<(fiveLaneDrumView()?6u:5u);++pad){
            const unsigned source=pad?(fiveLaneDrumView()?rhythm::fiveLaneDrumSource(pad):pad-1):4;
            const double age=drumFxClock-drumHitAt[pad];if(age<0 || age>2.0)continue;
            const bool active=drumHitActive[pad];
            if(pad){
                const auto& animation=drumHitDynamics[pad]==1?pc_drums_art::ghostFlames[source]:
                    drumHitDynamics[pad]==2?pc_drums_art::accentFlames[source]:pc_drums_art::flames[source];
                addDrumAnimation(tv,animation,age,active,pad);
            }else {
                const unsigned step=static_cast<unsigned>(age/pc_drums_art::kickInterval);
                if(step<6)for(const auto& fx:pc_drums_art::kickFx){
                    auto layer=fx.layer;layer.laneTint=false;
                    const auto* color=active?pc_drums_art::kickHitActive:pc_drums_art::kickHitNormal;
                    std::copy_n(color,4,layer.color);
                    if(!active && activeProfile.usesPurpleKick()) {const auto purple=rhythm::drumPurpleKick(true);std::copy(purple.begin(),purple.end(),layer.color);}
                    layer.color[3]-=step*pc_drums_art::kickAlphaStep;
                    const float blend=std::min(1.0f,step*pc_drums_art::kickScaleStep);
                    for(auto& p:layer.positions)for(unsigned k=0;k<3;++k)
                        p[k]=fx.pivot[k]+(p[k]-fx.pivot[k])*(fx.start[k]+(fx.end[k]-fx.start[k])*blend)/fx.original[k];
                    const auto first=static_cast<uint32_t>(tv.size());sourceEffectLayer(tv,layer,0,true,active);
                    spriteDraws.push_back({fx.key,GameplayDrawKind::Drums,first,static_cast<uint32_t>(tv.size())-first,fx.additive});
                }
            }
            if(noteStarPowerColors && (playMode!=PlayMode::Practice || practiceStarPowerEnabled)){
                const unsigned effect=source;
                const double phraseAge=drumFxClock-drumPhraseHitAt[pad];
                addDrumAnimation(tv,pc_drums_art::lightning[effect],phraseAge,starPower.active(),pad,false);
                addDrumAnimation(tv,pc_drums_art::lightning2[effect],phraseAge,starPower.active(),pad,false);
                if(pad)addDrumAnimation(tv,pc_drums_art::sparks[source],phraseAge,starPower.active(),pad,false);
            }
        }
    }
    void addGuitarAnimation(std::vector<TexVertex>& tv,const pc_guitar_fx::Animation& animation,double age,bool active,unsigned lane) {
        if(age<0 || (animation.length>0 && age>=animation.length))return;
        const unsigned frame=(static_cast<unsigned>(age/animation.interval)+animation.start)%animation.count;
        auto layer=animation.frames[frame];layer.laneTint=false;
        std::copy_n(active?animation.active:animation.normal,4,layer.color);
        const auto first=static_cast<uint32_t>(tv.size());sourceEffectLayer(tv,layer,lane,false,active || starPower.active());
        spriteDraws.push_back({animation.key,GameplayDrawKind::GuitarFx,first,static_cast<uint32_t>(tv.size())-first,animation.additive});
    }
    void addGuitarHitEffects(std::vector<TexVertex>& tv) {
        for(unsigned lane=0;lane<5;++lane) {
            const double age=guitarFxClock-guitarHitAt[lane];
            addGuitarAnimation(tv,pc_guitar_fx::flames[lane],age,guitarHitActive[lane],lane);
            const double phraseAge=guitarFxClock-guitarPhraseHitAt[lane];
            addGuitarAnimation(tv,pc_guitar_fx::lightning[lane],phraseAge,starPower.active(),lane);
            addGuitarAnimation(tv,pc_guitar_fx::lightning2[lane],phraseAge,starPower.active(),lane);
            addGuitarAnimation(tv,pc_guitar_fx::spSparks[lane],phraseAge,starPower.active(),lane);
            if(guitarSustaining[lane]) {
                addGuitarAnimation(tv,pc_guitar_fx::sparks[lane],age,starPower.active(),lane);
                // Keep the original overlay art; deployment applies the chosen
                // SP tint to this layer too, avoiding a red overlay on cyan fire.
                addGuitarAnimation(tv,pc_guitar_fx::sparksOverlay[lane],age,false,lane);
            }
        }
        const double openAge=guitarFxClock-guitarHitAt[5];
        const unsigned step=openAge>=0 && openAge<1 ? static_cast<unsigned>(openAge/pc_drums_art::kickInterval):6;
        if(step<6)for(const auto& fx:pc_guitar_fx::open) {
            auto layer=fx.layer;layer.laneTint=false;
            std::copy_n(guitarHitActive[5]?pc_guitar_fx::color338:pc_guitar_fx::color32c,4,layer.color);
            layer.color[3]-=step*pc_drums_art::kickAlphaStep;
            const float blend=std::min(1.0f,step*pc_drums_art::kickScaleStep);
            for(auto& p:layer.positions)for(unsigned k=0;k<3;++k)
                p[k]=fx.pivot[k]+(p[k]-fx.pivot[k])*(fx.start[k]+(fx.end[k]-fx.start[k])*blend)/fx.original[k];
            const auto first=static_cast<uint32_t>(tv.size());sourceEffectLayer(tv,layer,5,false,guitarHitActive[5]);
            spriteDraws.push_back({fx.key,GameplayDrawKind::GuitarFx,first,static_cast<uint32_t>(tv.size())-first,fx.additive});
        }
        const auto first=static_cast<uint32_t>(tv.size());
        guitarParticles.visit([&](const rhythm::GuitarParticles::Particle& particle) {
            // SP can start/end while this particle is still alive. Its birth
            // flag describes emission, not the current neck palette.
            const bool active=starPower.active();
            const auto& spec=particle.sustain?pc_guitar_fx::sustainParticles[particle.lane]:pc_guitar_fx::noteParticles[particle.lane];
            const float age=particle.age;
            glm::vec3 center;
            for(unsigned k=0;k<3;++k)center[k]=particle.origin[k]+particle.velocity[k]*age;
            center[1]-=.5f*9.81f*particle.gravity*age*age;
            center=glm::vec3(center.x,center.y,-center.z)*pc_guitar::worldScale;
            const auto facing=glm::normalize(gameplayEye-center);
            const auto right=glm::normalize(glm::cross(glm::vec3(0,1,0),facing));
            const auto up=glm::cross(facing,right);
            pc_guitar::Layer layer{};layer.laneTint=false;
            std::copy_n(spec.uv,4,layer.uv);
            std::copy_n(active?pc_guitar_fx::color340:pc_guitar_fx::color33c,4,layer.color);
            layer.color[3]*=std::clamp(1.0f-age/particle.life,0.0f,1.0f);
            constexpr float xy[4][2]={{-.5f,-.5f},{.5f,-.5f},{.5f,.5f},{-.5f,.5f}};
            for(unsigned i=0;i<4;++i) {
                const auto p=center+(right*xy[i][0]+up*xy[i][1])*particle.size*pc_guitar::worldScale;
                layer.positions[i][0]=p.x/pc_guitar::worldScale;
                layer.positions[i][1]=p.y/pc_guitar::worldScale;
                layer.positions[i][2]=-p.z/pc_guitar::worldScale;
            }
            sourceEffectLayer(tv,layer,particle.lane,false,active);
        });
        spriteDraws.push_back({pc_guitar_fx::noteParticles[0].key,GameplayDrawKind::GuitarFx,first,static_cast<uint32_t>(tv.size())-first,true});
    }

    void addReceptor(std::vector<TexVertex>& tv, int lane) {
        const float z = pc_guitar::receptorZ[lane] * pc_guitar::worldScale;
        const bool held = (frets & (1u << lane)) != 0;
        const float hit = hitGlow[lane];
        size_t layerIndex=0;
        for (auto layer : pc_guitar::receptors[lane]) {
            const float cameraFactor=rhythm::pcPlayerCamera(gameplayPlayerCount()).factor;
            for(unsigned corner=0;corner<4;++corner)for(unsigned axis=0;axis<3;++axis)
                layer.positions[corner][axis]+=pc_player_scaling::guitar[lane][layerIndex][corner][axis]*cameraFactor;
            const auto key=pc_render_order::receptors[lane][layerIndex++];
            // PC's headLight is the receptor's original lane-colored flash art.
            // The PC component enables it while held; also expose it briefly
            // after a judged hit so a quick fret tap remains visible to players.
            if (layer.heldLight ? (held || hit > 0.01f) : layer.enabled) {
                const float dy = held && layer.movingHead ?
                    -pc_guitar::heldHeadTravel * pc_guitar::worldScale : 0.0f;
                const float flash = layer.heldLight ? 1.0f + hit * 0.8f : 1.0f;
                const auto first=static_cast<uint32_t>(tv.size());
                sourceLayer(tv, layer, laneX[lane], z, fretColor[lane], false, dy,
                            nullptr, 0.0f, flash);
                spriteDraws.push_back({key,GameplayDrawKind::Atlas,first,static_cast<uint32_t>(tv.size())-first});
            }
        }
    }

    // SpNeckRenderer coroutine 0x1800B2120; prefab 23972: min .55, max 1, speed 1.
    void updateHighwayGlow() {
        const bool active=screen==Screen::Gameplay && starPower.active();
        if(active && !spHighwayActive) spHighwayTime=0.0f;
        if(screen!=Screen::Gameplay) { spHighwayAlpha=0; spHighwayTime=0; }
        else if(active) {
            spHighwayAlpha=0.55f+0.45f*(std::sin(spHighwayTime*6.283185482f)*0.5f+0.5f);
            if(!ui.visible) spHighwayTime+=std::max(0.0f,static_cast<float>(frameTimer));
        } else if(!ui.visible) spHighwayAlpha=std::max(0.0f,spHighwayAlpha-3.0f*static_cast<float>(frameTimer));
        spHighwayActive=active;
    }

    glm::vec3 healthArrowOffset() const {
        const glm::vec3 position=rhythm::pc_meter::position(pc_hud::healthPositions,gameplayHealth);
        return glm::vec3(position.x-pc_hud::healthPositions[1][0],
            position.y-pc_hud::healthPositions[1][1],-(position.z-pc_hud::healthPositions[1][2]))*pc_guitar::worldScale;
    }

    bool soloHighwayGlow() const noexcept {
        return activeProfile.highwayEffects && !starPower.active() && gameplayHealth>1.0f/3.0f &&
            std::any_of(solos.begin(),solos.end(),[&](const auto& range){return renderTime>=range.startTime && renderTime<range.endTime;});
    }
    void addHighwayLayer(std::vector<TexVertex>& tv,const pc_highway::Layer& layer) {
        const bool soloGlow=soloHighwayGlow();
        // Clip in world space before projection; the source sliced rails are 50 units long.
        // Interpolate UVs at the clip edges so their original bottom border stays intact.
        std::array<TexVertex,8> poly{};
        size_t count=4;
        for(unsigned i=0;i<4;++i) {
            TexVertex v{};
            std::copy_n(layer.vertices[i],3,v.pos);
            std::copy_n(layer.uv[i],2,v.uv);
            std::copy_n(layer.color,4,v.color);
            if(layer.pulse) {
                const float scale=layer.renderer==17407?1.0f:0.5f;
                constexpr float soloBlue[]{.12f,.32f,1.0f};
                for(unsigned k=0;k<3;++k) v.color[k]=(soloGlow?soloBlue[k]:activeProfile.starPowerColor?selectedStarPowerColor()[k]:pc_meter_art::spNeck[k])*scale;
                v.color[3]=soloGlow?.65f:spHighwayAlpha;
            }
            if(layer.health) {
                const unsigned color=gameplayHealth<=1.0f/3.0f?2:gameplayHealth<=2.0f/3.0f?1:0;
                std::copy_n(pc_hud::healthColors[color],4,v.color);
                const glm::vec3 offset=healthArrowOffset();
                for(unsigned k=0;k<3;++k) v.pos[k]+=offset[k];
            }
            if(layer.health) {
                const auto point=rhythm::highwayMeterPoint(glm::vec3(v.pos[0],v.pos[1],v.pos[2]),pc_hud::railMeterPlaneZ,gameplayEye);
                for(unsigned k=0;k<3;++k) v.pos[k]=point[k];
            }
            poly[i]=v;
        }
        for(unsigned side=0;side<2;++side) {
            std::array<TexVertex,8> clipped{};size_t clippedCount=0;
            const float boundary=side==0?BOARD_NEAR:HIGHWAY_FAR;
            for(size_t i=0;i<count;++i) {
                const auto& a=poly[i];const auto& b=poly[(i+1)%count];
                const bool insideA=side==0?a.pos[2]<=boundary:a.pos[2]>=boundary;
                const bool insideB=side==0?b.pos[2]<=boundary:b.pos[2]>=boundary;
                if(insideA) clipped[clippedCount++]=a;
                if(insideA!=insideB) {
                    const float t=(boundary-a.pos[2])/(b.pos[2]-a.pos[2]);TexVertex v=a;
                    for(unsigned k=0;k<3;++k) v.pos[k]=a.pos[k]+t*(b.pos[k]-a.pos[k]);
                    for(unsigned k=0;k<2;++k) v.uv[k]=a.uv[k]+t*(b.uv[k]-a.uv[k]);
                    clipped[clippedCount++]=v;
                }
            }
            poly=clipped;count=clippedCount;
        }
        for(size_t i=1;i+1<count;++i) {
            tv.push_back(poly[0]);tv.push_back(poly[i]);tv.push_back(poly[i+1]);
        }
    }

    void buildTexFrame(std::vector<TexVertex>& tv)
    {
        tv.clear();spriteDraws.clear();
        auto highwayLayer=[&](const auto& saved) {
            auto layer=saved;
            for(unsigned i=0;i<std::size(pc_highway::strings);++i)if(layer.renderer==pc_highway::strings[i].renderer) {
                if(chart.kind==rhythm::ChartKind::Drums || chart.kind==rhythm::ChartKind::SixFret)return;
                const auto& attach=pc_highway::strings[i];
                std::array<glm::vec3,4> vertices;
                for(unsigned corner=0;corner<4;++corner)vertices[corner]=glm::vec3(layer.vertices[corner][0],layer.vertices[corner][1],layer.vertices[corner][2]);
                const auto native=[](const float* point){return glm::vec3(point[0],point[1],-point[2])*pc_guitar::worldScale;};
                auto matrix=uniformData.viewProj;if(activeProfile.lefty)matrix[0]=-matrix[0];
                const auto viewport=gameplayViewport();
                vertices=rhythm::pcAttachedString(vertices,native(attach.top),native(attach.bottom),
                    glm::vec2(attach.scaleOffset[0],attach.scaleOffset[1]),matrix,glm::vec2(viewport.width,viewport.height),
                    gameplayPlayerCount(),stringAttachScale[i]);
                for(unsigned corner=0;corner<4;++corner)for(unsigned axis=0;axis<3;++axis)layer.vertices[corner][axis]=vertices[corner][axis];
            }
            const auto first=static_cast<uint32_t>(tv.size());
            addHighwayLayer(tv,layer);
            spriteDraws.push_back({pc_render_order::renderer(layer.renderer),GameplayDrawKind::Highway,
                first,static_cast<uint32_t>(tv.size())-first,layer.pulse});
        };
        for(const auto& layer:pc_highway::layers) if(!layer.pulse && !layer.health) highwayLayer(layer);
        highwayStaticCount=static_cast<uint32_t>(tv.size());
        for(const auto& layer:pc_highway::layers) if(layer.pulse && (spHighwayAlpha>0 || soloHighwayGlow())) highwayLayer(layer);
        highwayVertexCount=static_cast<uint32_t>(tv.size());
        // Source Beatline prefab sprite 1520: bounds, identity rotation,
        // BeatRenderer thickness/opacity and serialized beatZOffset=0.0125.
        const double earliest=renderTime-HIGHWAY_NEAR/currentSpeedZ();
        auto beat=std::lower_bound(beatLines.begin(),beatLines.end(),earliest,
            [](const BeatLine& b,double time){return b.time<time;});
        for(;beat!=beatLines.end();++beat) {
            const float z=-static_cast<float>(beat->time-renderTime)*currentSpeedZ()-0.0125f*pc_guitar::worldScale;
            if(z<HIGHWAY_FAR) break;
            if(z>HIGHWAY_NEAR) continue;
            const float thickness=beat->kind==0?2.2f:beat->kind==1?1.1f:0.55f;
            const float alpha=beat->kind==0?1.0f:beat->kind==1?0.7f:0.4f;
            const float x=0.4654545486f*pc_guitar::worldScale;
            const float lo=-0.0029090911f*thickness*pc_guitar::worldScale;
            const float hi=0.0116363643f*thickness*pc_guitar::worldScale;
            const float positions[4][3]={{-x,lo,z},{x,lo,z},{x,hi,z},{-x,hi,z}};
            const float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
            for(int i:{0,1,2,0,2,3}) tv.push_back({{positions[i][0],positions[i][1],positions[i][2]},
                {uv[i][0],uv[i][1]},{1,1,1,alpha}});
            if(tv.size()-highwayVertexCount>=1200) break; // PC pool: 200 beatline sprites.
        }
        beatVertexCount=static_cast<uint32_t>(tv.size());
        // Geometry batches are sorted by the PC's global sorting layer below.
        if(chart.kind==rhythm::ChartKind::Drums){addDrumReceptors(tv);addDrumHitEffects(tv);}
        else if(chart.kind==rhythm::ChartKind::SixFret)addSixFretReceptors(tv);
        else {for (int lane = 0; lane < 5; ++lane) addReceptor(tv, lane);addGuitarHitEffects(tv);}
        visibleNotes.clear();
        rhythm::submitVisibleNotes(chart, renderTime, -HIGHWAY_FAR / currentSpeedZ(),
            HIGHWAY_NEAR / currentSpeedZ(), [&](const rhythm::RenderNote& n) { visibleNotes.push_back(n); });
        // Chords arrive chronologically; paint in reverse order for transparent overlap.
        for (auto chord = visibleNotes.rbegin(); chord != visibleNotes.rend(); ++chord) {
            auto first = std::lower_bound(notes.begin(), notes.end(), chord->noteIndex,
                [](const Note& n, size_t index) { return n.chordIndex < index; });
            for (auto it = first; it != notes.end() && it->chordIndex == chord->noteIndex; ++it) {
                if (it->missed) continue;
                const float zHead = it->hit ? 0.0f : -(float)chord->timeUntilHit * currentSpeedZ();
                const float zTail = -(float)(it->timeSec + it->sustainSec - renderTime) * currentSpeedZ();
                if (zHead < HIGHWAY_FAR || zTail > HIGHWAY_NEAR) continue;
                if (tv.size() + 48 > MAX_TEX_VERTS) break;
                const float* c = gameplayNoteColor(it->lane);
                const float x = openLane(it->lane) ? 0.0f : guitarLanePosition(it->lane);
                if (!it->hit && zHead >= HIGHWAY_FAR && zHead <= HIGHWAY_NEAR) {
                    if(chart.kind==rhythm::ChartKind::Drums){addDrumNote(tv,*it,zHead);continue;}
                    if(chart.kind==rhythm::ChartKind::SixFret){addSixFretNote(tv,*it,zHead);continue;}
                    const auto firstVertex=static_cast<uint32_t>(tv.size());
                    addNote(tv, *it, x, zHead, c);
                    const auto layer=it->lane==5?pc_render_order::Layer::KickOpenNotes:pc_render_order::Layer::Notes;
                    spriteDraws.push_back({pc_render_order::at(layer),GameplayDrawKind::Atlas,
                        firstVertex,static_cast<uint32_t>(tv.size())-firstVertex});
                }
            }
        }
        texVertexCount = (uint32_t)tv.size();
    }

    void buildSustainFrame(std::vector<TexVertex>& vertices) {
        vertices.clear();
        for (auto chord = visibleNotes.rbegin(); chord != visibleNotes.rend(); ++chord) {
            auto first=std::lower_bound(notes.begin(),notes.end(),chord->noteIndex,
                [](const Note& n,size_t index) { return n.chordIndex<index; });
            for (auto n=first; n!=notes.end() && n->chordIndex==chord->noteIndex; ++n) {
                if (n->sustainSec<=0.05 || renderTime>=n->timeSec+n->sustainSec) continue;
                const float originalRoot=-(float)(n->timeSec-renderTime)*currentSpeedZ();
                const bool held=n->hit && !n->sustainDropped;
                const float root=held ? std::min(originalRoot,0.0f) : originalRoot;
                const float end=-(float)(n->timeSec+n->sustainSec-renderTime)*currentSpeedZ();
                const int state=(n->missed || n->sustainDropped) ? 2 : held ? 1 : 0;
                const float* color = state == 2 ? droppedTailColor : gameplayNoteColor(n->lane);
                const size_t firstVertex=vertices.size();
                sustainQuad(vertices,openLane(n->lane) ? 0.0f : guitarLanePosition(n->lane),root,root-end,state,color,openLane(n->lane));
                if(chart.kind!=rhythm::ChartKind::Drums && activeProfile.noteColor && state!=2 &&
                    !(starPower.active() && noteStarPowerColors)) {
                    const auto custom=rhythm::regularNoteColor(activeProfile.noteColor,renderTime);
                    for(size_t i=firstVertex;i<vertices.size();++i)for(unsigned k=0;k<3;++k)vertices[i].color[k]=custom[k];
                }
                if(n->tap && activeProfile.purpleTapNotes && state!=2)
                    for(size_t i=firstVertex;i<vertices.size();++i) {
                        vertices[i].color[3]*=.72f;vertices[i].frameBlend=2.0f;
                    }
            }
        }
        sustainVertexCount=static_cast<uint32_t>(vertices.size());
    }

    // ---------- per-frame board quad: textured, V scrolls with the song clock ----------
    void buildBoardFrame(std::vector<TexVertex>& bv)
    {
        bv.clear();
        auto Vof = [&](float z) { return (float)((renderTime * currentSpeedZ() - z) / BOARD_TILE); };
        float yB = 0.0f, zN = BOARD_NEAR, zF = HIGHWAY_FAR;
        float vN = Vof(zN), vF = Vof(zF);
        const bool solo=std::any_of(solos.begin(),solos.end(),[&](const auto& range){return renderTime>=range.startTime && renderTime<range.endTime;});
        const auto kind=rhythm::highwayPulse(activeProfile.highwayEffects,gameplayHealth,solo,combo,currentStarRank());
        const auto tint=rhythm::highwayPulseTint(kind,renderTime);
        const float w[4] = { tint[0],tint[1],tint[2],1 };
        bv.push_back({ {BOARD_L,yB,zN},{0,vN},{w[0],w[1],w[2],w[3]} });
        bv.push_back({ {BOARD_R,yB,zN},{1,vN},{w[0],w[1],w[2],w[3]} });
        bv.push_back({ {BOARD_R,yB,zF},{1,vF},{w[0],w[1],w[2],w[3]} });
        bv.push_back({ {BOARD_L,yB,zN},{0,vN},{w[0],w[1],w[2],w[3]} });
        bv.push_back({ {BOARD_R,yB,zF},{1,vF},{w[0],w[1],w[2],w[3]} });
        bv.push_back({ {BOARD_L,yB,zF},{0,vF},{w[0],w[1],w[2],w[3]} });
        boardVertexCount = (uint32_t)bv.size();
    }

    void notify_fail(const std::string& s) { printf("%s\n", s.c_str()); rhythm::logDiagnostic("failure: %s",s.c_str()); }

    void prepareUniformBuffers()
    {
        for (auto& b : uniformBuffers) {
            VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &b, sizeof(UniformData)));
            VK_CHECK_RESULT(b.map());
        }
    }

    void updateGameplayProjection()
    {
        const float lengthScale=static_cast<float>(activeProfile.highwayLength)/100.0f;
        const float farPlane=std::max(100.0f,-HIGHWAY_FAR*lengthScale+50.0f);
        glm::mat4 proj = glm::perspective(glm::radians(camFov), gameplayViewport().width/gameplayViewport().height, 0.1f, farPlane);
        proj[1][1] *= -1.0f;
        const float shake = highwayShakeEnabled && highwayShakeTime > 0.0f
            ? (highwayShakeTime / 0.30f) * 0.035f : 0.0f;
        const float shakePhase = static_cast<float>(now_seconds() * 57.0);
        const auto layout=rhythm::pcPlayerCamera(gameplayPlayerCount());
        glm::vec3 eye(sinf(shakePhase)*shake,
            camHeight+(layout.height-pc_guitar::cameraHeight)*pc_guitar::worldScale+cosf(shakePhase*.83f)*shake*.45f,
            camZ+(layout.z-pc_guitar::cameraZ)*pc_guitar::worldScale);
        // Meter reference projection is evaluated before the optional X mirror.
        gameplayEye=glm::vec3(activeProfile.lefty?-eye.x:eye.x,eye.y,eye.z);
        const float basePitch=std::atan2(pc_guitar::cameraHeight,pc_guitar::cameraZ+5.545f);
        float p=glm::radians(-camPitch)+basePitch-std::atan2(layout.height,layout.z+layout.targetZ);
        glm::vec3 fwd(0.0f, sinf(p), -cosf(p));
        glm::mat4 view = glm::lookAt(eye, eye + fwd, glm::vec3(0,1,0));
        // Native adaptation of the profile's percentage length: stretch depth
        // about the strike line. Lefty reverses the visible fret order without
        // changing chart masks, pitch, timing or scoring. Mirror Mode remains
        // a separate chart modifier.
        uniformData.viewProj=rhythm::highwayMatrix(proj*view,activeProfile.highwayLength,activeProfile.lefty);
        const auto screenY = [&](float pcZ) {
            const glm::vec4 p=uniformData.viewProj*glm::vec4(0,0,-pcZ*pc_guitar::worldScale,1);
            return p.y / p.w * 0.5f + 0.5f;
        };
        // Recovered shader fallback limits: close(-1,-1.25), far(7,6).
        uniformData.sustainFade=glm::vec4(screenY(-1),screenY(-1.25f),screenY(7),screenY(6));
    }

    void buildMeterFrame() {
        meterScratch.clear();meterDraws.clear();
        if(screen!=Screen::Gameplay) return;
        const unsigned healthBand=gameplayHealth<=1.0f/3?0:gameplayHealth<=2.0f/3?1:2;
        const unsigned multiplier=currentScoreMultiplier();
        const unsigned baseTier=std::min(4u,1u+combo/10u);
        const unsigned palette=starPower.active() && noteStarPowerColors?5:baseTier;
        auto attachCamera=uniformData.viewProj;if(activeProfile.lefty)attachCamera[0]=-attachCamera[0];
        const auto viewport=gameplayViewport();
        std::array<rhythm::PcAttachedMeterPose,2> attachedPoses;
        for(unsigned i=0;i<2;++i)attachedPoses[i]=rhythm::pcMeterAttachmentPose(pc_meter_attach::groups[i],
            attachCamera,glm::vec2(viewport.width,viewport.height),meterAttachScale[i]);
        glm::vec3 multiplierOrigin(0);float referenceWidth=0;
        for(const auto& layer:pc_meter_art::layers) {
            if(layer.renderer==18166)multiplierOrigin=(glm::vec3(layer.vertices[0][0],layer.vertices[0][1],layer.vertices[0][2])+glm::vec3(layer.vertices[2][0],layer.vertices[2][1],layer.vertices[2][2]))*.5f;
            if(layer.renderer==18205)referenceWidth=glm::length(glm::vec3(layer.vertices[1][0]-layer.vertices[0][0],layer.vertices[1][1]-layer.vertices[0][1],layer.vertices[1][2]-layer.vertices[0][2]));
        }
        const float multiplierScale=1+.04f*(gameplayPlayerCount()-1);
        const glm::vec3 multiplierMove(referenceWidth*(multiplierScale-1)*.65f,0,0);
        for(unsigned index:pc_meter_art::drawOrder) {
            const auto& layer=pc_meter_art::layers[index];const int id=layer.renderer;
            if((id==14434 && healthBand!=2)||(id==17423 && healthBand!=1)||(id==17190 && healthBand!=0)) continue;
            if((id==17000 || id==16093 || id==16336) && starPower.fill()<=0) continue;
            if(id==16336 && starPower.fill()<0.5f && !starPower.active()) continue;
            if((id==18156 || id==15969 || id==17098) && multiplier<2) continue;
            float color[4];std::copy_n(layer.color,4,color);
            const float* region=layer.uv;
            if(id==17000) std::copy_n(pc_meter_art::spFill,4,color);
            if(id==16093 || id==16336) std::copy_n(pc_meter_art::spArrow,4,color);
            if(id==14855) std::copy_n(pc_hud::healthColors[2-healthBand],4,color);
            if(id==18156 || id==15969) {
                region=id==18156?pc_meter_art::numbers[multiplier]:pc_meter_art::glows[multiplier];
                std::copy_n(id==18156?pc_meter_art::combo[palette]:pc_meter_art::comboGlow[palette],4,color);
                color[3]=0.85f; // ColorProfile assignment 0x1801BA640 / DAT_183077818.
            }
            if(index>=20) {
                const bool lit=index-20<rhythm::pc_meter::ticks(combo);
                std::copy_n(lit?pc_meter_art::combo[palette]:pc_meter_art::inactive,4,color);
                if(lit) color[3]=0.85f;
            }
            if(activeProfile.starPowerColor && (id==17000 || id==16093 || id==16336 ||
                (starPower.active() && (id==18156 || id==15969 || index>=20)))) {
                const float intensity=std::max({color[0],color[1],color[2]});
                for(unsigned k=0;k<3;++k)color[k]=selectedStarPowerColor()[k]*intensity;
            }
            const pc_meter_attach::Layer* attachment=nullptr;
            for(const auto& candidate:pc_meter_attach::layers)if(candidate.renderer==id){attachment=&candidate;break;}
            glm::vec3 arrowDelta(0);
            if(attachment) {
                const auto& group=pc_meter_attach::groups[attachment->group];
                const auto target=attachment->group==0?rhythm::pc_meter::position(pc_hud::healthPositions,gameplayHealth):
                    rhythm::pc_meter::position(pc_hud::spPositions,starPower.fill());
                arrowDelta=target-glm::vec3(group.arrowBase[0],group.arrowBase[1],group.arrowBase[2]);
            }
            const uint32_t first=static_cast<uint32_t>(meterScratch.size());
            const float uv[4][2]={{region[0],region[1]},{region[2],region[1]},{region[2],region[3]},{region[0],region[3]}};
            const float local[4][2]={{0,1},{1,1},{1,0},{0,0}};
            for(unsigned corner:{0,1,2,0,2,3}) {
                const auto& v=layer.vertices[corner];
                auto source=glm::vec3(v[0],v[1],v[2]);
                // Counter-reflect the complete xN lettering within the source
                // circle. The highway mirror then preserves both glyph shape
                // and x-before-number order on the left-handed meter.
                if(activeProfile.lefty && (id==18156 || id==15969 || id==17098))
                    for(const auto& circle:pc_meter_art::layers)if(circle.renderer==18166)
                        source.x=rhythm::pcMultiplierTextX(source.x,(circle.vertices[0][0]+circle.vertices[2][0])*.5f,true,id);
                if(id==15375 || id==18205 || id==18166 || id==17098 || id==18156 || id==15969)
                    source=multiplierOrigin+(source-multiplierOrigin)*multiplierScale+multiplierMove;
                const auto p=attachment?rhythm::pcAttachedMeterPoint(*attachment,attachedPoses[attachment->group],corner,arrowDelta):
                    id==14844 || index>=20?rhythm::highwayStreakPoint(source,gameplayEye):
                    rhythm::highwayMeterPoint(source,layer.plane,gameplayEye);
                meterScratch.push_back({{p.x,p.y,p.z},{uv[corner][0],uv[corner][1]},
                    {color[0],color[1],color[2],color[3]}, {local[corner][0],local[corner][1]},0});
            }
            meterDraws.push_back({first,layer.additive,id==17000,layer.fade,id==15969,id});
        }
    }

    void setupDescriptors()
    {
        std::vector<VkDescriptorPoolSize> poolSizes = {
            vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8 * PAD_PLAYERS * maxConcurrentFrames),
            vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 * PAD_PLAYERS * maxConcurrentFrames),
        };
        VkDescriptorPoolCreateInfo poolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 8 * PAD_PLAYERS * maxConcurrentFrames);
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        };
        VkDescriptorSetLayoutCreateInfo layout = vks::initializers::descriptorSetLayoutCreateInfo(bindings);
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layout, nullptr, &descriptorSetLayout));

        allocateGameplayDescriptors();
    }

    void allocateGameplayDescriptors() {
        VkDescriptorImageInfo meterInfo{meterTex.sampler,meterTex.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo guitarFxInfo{guitarFxTex.sampler,guitarFxTex.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo sixInfo{sixFretTex.sampler,sixFretTex.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo drumInfo{drumAtlasTex.sampler,drumAtlasTex.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo atlasInfo{ atlasTex.sampler, atlasTex.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkDescriptorImageInfo highwayInfo{highwayTex.sampler,highwayTex.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo beatInfo{beatTex.sampler,beatTex.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo boardInfo{ boardTex.sampler, boardTex.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkDescriptorSetAllocateInfo alloc = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
        for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device,&alloc,&drumAtlasSets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device,&alloc,&sixFretSets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device,&alloc,&guitarFxSets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device,&alloc,&meterSets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &alloc, &highwaySets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &alloc, &beatSets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &alloc, &atlasSets[i]));
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &alloc, &boardSets[i]));
            std::vector<VkWriteDescriptorSet> writes = {
                vks::initializers::writeDescriptorSet(sixFretSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(sixFretSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,&sixInfo),
                vks::initializers::writeDescriptorSet(guitarFxSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(guitarFxSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,&guitarFxInfo),
                vks::initializers::writeDescriptorSet(drumAtlasSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(drumAtlasSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,&drumInfo),
                vks::initializers::writeDescriptorSet(meterSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(meterSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,&meterInfo),
                vks::initializers::writeDescriptorSet(highwaySets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(highwaySets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &highwayInfo),
                vks::initializers::writeDescriptorSet(beatSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(beatSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &beatInfo),
                vks::initializers::writeDescriptorSet(atlasSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(atlasSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &atlasInfo),
                vks::initializers::writeDescriptorSet(boardSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
                vks::initializers::writeDescriptorSet(boardSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &boardInfo),
            };
            vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
        }
    }

    VkPipeline buildPipeline(const char* vert, const char* frag, bool textured, bool depthTest, bool premult, bool additive=false)
    {
        VkVertexInputBindingDescription binding;
        std::vector<VkVertexInputAttributeDescription> attribs;
        if (textured) {
            binding = vks::initializers::vertexInputBindingDescription(0, sizeof(TexVertex), VK_VERTEX_INPUT_RATE_VERTEX);
            attribs = {
                vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TexVertex, pos)),
                vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TexVertex, uv)),
                vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TexVertex, color)),
                vks::initializers::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(TexVertex, uvNext)),
                vks::initializers::vertexInputAttributeDescription(0, 4, VK_FORMAT_R32_SFLOAT, offsetof(TexVertex, frameBlend)),
            };
        } else {
            binding = vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
            attribs = {
                vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
                vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),
            };
        }
        VkPipelineVertexInputStateCreateInfo vinput = vks::initializers::pipelineVertexInputStateCreateInfo();
        vinput.vertexBindingDescriptionCount = 1; vinput.pVertexBindingDescriptions = &binding;
        vinput.vertexAttributeDescriptionCount = (uint32_t)attribs.size(); vinput.pVertexAttributeDescriptions = attribs.data();

        auto ia = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        auto rs = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        VkPipelineColorBlendAttachmentState ba = vks::initializers::pipelineColorBlendAttachmentState(0xf, premult ? VK_TRUE : VK_FALSE);
        if (premult) {
            ba.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  ba.dstColorBlendFactor = additive ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            ba.colorBlendOp = VK_BLEND_OP_ADD;
            ba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  ba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            ba.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        auto cb = vks::initializers::pipelineColorBlendStateCreateInfo(1, &ba);
        auto ds = vks::initializers::pipelineDepthStencilStateCreateInfo(depthTest ? VK_TRUE : VK_FALSE, depthTest ? VK_TRUE : VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        auto vp = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        auto ms = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState> dyn = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        auto ds2 = vks::initializers::pipelineDynamicStateCreateInfo(dyn);
        std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
            loadShader(getShadersPath() + vert, VK_SHADER_STAGE_VERTEX_BIT),
            loadShader(getShadersPath() + frag, VK_SHADER_STAGE_FRAGMENT_BIT),
        };
        VkPipelineRenderingCreateInfo rci{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChain.colorFormat,
            .depthAttachmentFormat = depthFormat, .stencilAttachmentFormat = depthFormat };
        VkGraphicsPipelineCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .pNext = &rci,
            .stageCount = (uint32_t)stages.size(), .pStages = stages.data(), .pVertexInputState = &vinput,
            .pInputAssemblyState = &ia, .pViewportState = &vp, .pRasterizationState = &rs,
            .pMultisampleState = &ms, .pDepthStencilState = &ds, .pColorBlendState = &cb,
            .pDynamicState = &ds2, .layout = pipelineLayout };
        VkPipeline pipe;
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &ci, nullptr, &pipe));
        return pipe;
    }

    void preparePipelines()
    {
        VkPipelineLayoutCreateInfo lci = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
        VkPushConstantRange meterRange{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(MeterParams)};
        lci.pushConstantRangeCount=1;lci.pPushConstantRanges=&meterRange;
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &lci, nullptr, &pipelineLayout));
        // PC FadeSpriteAdd1Intense: premultiplied source, additive destination.
        highwayGlowPipeline=buildPipeline("cloneheronative/tex.vert.spv","cloneheronative/tex.frag.spv",true,false,true,true);
        meterPipeline=buildPipeline("cloneheronative/tex.vert.spv","cloneheronative/meter.frag.spv",true,false,true);
        meterGlowPipeline=buildPipeline("cloneheronative/tex.vert.spv","cloneheronative/meter.frag.spv",true,false,true,true);
        spMeterPipeline=buildPipeline("cloneheronative/tex.vert.spv","cloneheronative/pc_spbar.frag.spv",true,false,true);
        texPipeline   = buildPipeline("cloneheronative/tex.vert.spv",     "cloneheronative/tex.frag.spv",     true,  false, true);  // sprites: premult, depth off
        sustainPipeline = buildPipeline("cloneheronative/sustain.vert.spv", "cloneheronative/sustain.frag.spv", true, false, true);
        // PC neck SpriteRenderer 17009 uses FadeSprite: One/OneMinusSrcAlpha,
        // ZWrite Off. Opaque replacement hid earlier layers through its fade.
        boardPipeline = buildPipeline("cloneheronative/tex.vert.spv",     "cloneheronative/tex.frag.spv",     true,  false, true);
    }

    ImFont* settingsFont{nullptr};
    ImFont* ratingFont{nullptr};
    ImFont* pauseHeadingFont{nullptr};
    ImFont* playerNameFont{nullptr};
    ImFont* songHeadingFont{nullptr};
    ImFont* resultsBoldFont{nullptr};
    ImFont* gameplayNameFont{nullptr};
    ImFont* countdownFont{nullptr};

    void prepare() override
    {
#if !defined(PS5_HOST_REFERENCE)
        if(!rhythm::persistent::prepare())fprintf(stderr,"[storage] Persistent data directory is unavailable: %d\n",errno);
#endif
        removedBundledSongs=rhythm::storage::removedBundles(CH_USERDATA "/removed-bundled-songs.cfg");
        if(FILE* preference=std::fopen(CH_USERDATA "/update-checks.cfg","rb")){int enabled=1;unsigned version=0;char magic[32]{};if(std::fscanf(preference,"%31s %u %d",magic,&version,&enabled)==3 && std::strcmp(magic,"ENCORE_UPDATES")==0 && version==1)autoUpdateChecks=enabled!=0;std::fclose(preference);}
        updateLiveQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="update-check-live";});
        rhythm::startDiagnostics(CH_USERDATA "/logs",buildVersion);
        if(ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="update-selftest";})){updateCheckStarted=true;updateLiveQa=true;(void)applicationUpdater.selfTest();}
        if(ps5.frameBudget && !*encore_application_resource_root() && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="runtime-update-selftest";})){updateCheckStarted=true;updateRuntimeQa=true;updateDialog=true;(void)applicationUpdater.selfTest(true);}
        rhythm::logDiagnostic("display width=%u height=%u frame_limit=%d",width,height,frameRateLimit);
        probeControllerAccess();
        storageLiveQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="storage-live";});
        storageScanTestHold=storageLiveQa && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="scan-hold";});
        loadSettings();
        rhythm::logDiagnostic("display refresh_millihz=%u target_millihz=%u",displayTiming.refreshMilliHz,displayTiming.targetMilliHz(frameRateLimit));
        loadProfiles();
        keyboardGridQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="keyboard-grid";});
        loadStallQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="load-stall";});
        keyboardRawQa=keyboardGridQa || (ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="keyboard-raw";}));
        slotInputQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.program.rfind("cloneheronative.slot",0)==0;});
        midiDiscoveryQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="discover-midi";});
        const bool usbQa=ps5.frameBudget && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="discover-usb" || p.action=="discover-midi";});
        if(!ps5.frameBudget || usbQa)usbControllers.start(getAssetPath()+"controller_profiles",CH_USERDATA "/controllers");
        if(!ps5.frameBudget || midiDiscoveryQa)rhythm::logDiagnostic("USB MIDI discovery started=%d",usbMidi.start());
#if !defined(PS5_HOST_REFERENCE)
        const std::string uiSoundPath = rhythm::assets::currentRoot() + "cloneheronative/sounds";
        if (!songAudio.loadUiSounds(uiSoundPath.c_str(),(getAssetPath()+"cloneheronative/sounds/menu_intro.wav").c_str()))
            fprintf(stderr, "[audio] PC UI sound assets could not be loaded\n");
#endif
        overlayFontPath = rhythm::assets::currentRoot() + "pc_menu/Lato-Regular.ttf";
        overlayFontSize = 22.0f;
        gameplayNameFont=ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-BoldItalic.ttf").c_str(),pc_gameplay_labels::nameSize);
        countdownFont=ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Heavy.ttf").c_str(),pc_gameplay_labels::countdownMaxSize);
        resultsBoldFont=ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Bold.ttf").c_str(),39.0f);
        playerNameFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Regular.ttf").c_str(),50.0f);
        songHeadingFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Semibold.ttf").c_str(),50.0f);
        pauseHeadingFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Light.ttf").c_str(), 100.0f);
        settingsFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Light.ttf").c_str(), 36.0f);
        ratingFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((rhythm::assets::currentRoot()+"pc_menu/Lato-Semibold.ttf").c_str(), 24.0f);
        inspectOutputResolution();
        VulkanExampleBase::prepare();
        inspectDisplayTiming();
        applyDisplayPacing();
        ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->Fonts.back();
        if (!prepareTextures()) { quit=true; return; }
        buildGeometry();
        prepareUniformBuffers();
        setupDescriptors();
        preparePipelines();
        std::srand(static_cast<unsigned>(getpid()) ^
                   static_cast<unsigned>(now_seconds() * 1000000.0));
        scanLocalSongs();
        // Charts/audio are prepared on demand; booting a menu needs no gameplay load.
#if !defined(PS5_HOST_REFERENCE)
        if (!audioOutputStarted) {
            applyAudioGains();
            audioOutputStarted = audio_start(&rhythm::AudioStemMixer::audioFill, &songAudio);
        }
#else
        // The renderer reference has no native audio device or intro cue.
        menuIntroStarted=menuIntroFinished=true;
        menuWidgetRevealAt=ImGui::GetTime();
#endif
        ui.visible = true;
        screen = Screen::MainMenu;
        restartSong();
        visibleNotes.reserve(chart.notes.size());
        spriteScratch.reserve(MAX_TEX_VERTS);
        sustainScratch.reserve(MAX_TEX_VERTS);
        boardScratch.reserve(6);
        playing = false;
        prepared = true;
    }

    void buildCommandBuffer()
    {
        profileDraws=profileBinds=profileVertices=0;
        if(renderProfileQa && !renderTimestampPool && vulkanDevice->queueFamilyProperties[vulkanDevice->queueFamilyIndices.graphics].timestampValidBits) {
            VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};info.queryType=VK_QUERY_TYPE_TIMESTAMP;info.queryCount=uint32_t(drawCmdBuffers.size()*2);
            if(vkCreateQueryPool(device,&info,nullptr,&renderTimestampPool)==VK_SUCCESS)renderTimestampPending.assign(drawCmdBuffers.size(),false);
        }
        if(renderTimestampPool && renderTimestampPending[currentBuffer]) {
            uint64_t values[2]{};
            if(vkGetQueryPoolResults(device,renderTimestampPool,currentBuffer*2,2,sizeof(values),values,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT)==VK_SUCCESS)
                profileGpuMs=double(values[1]-values[0])*vulkanDevice->properties.limits.timestampPeriod/1000000.;
        }
        VkCommandBuffer cmd = drawCmdBuffers[currentBuffer];
        VkCommandBufferBeginInfo bi = vks::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &bi));
        if(renderTimestampPool){vkCmdResetQueryPool(cmd,renderTimestampPool,currentBuffer*2,2);vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,renderTimestampPool,currentBuffer*2);}
        beginDynamicRendering(cmd);
        VkViewport vp = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        vkCmdSetViewport(cmd, 0, 1, &vp);
        VkRect2D sc = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(cmd, 0, 1, &sc);
        VkDeviceSize off[1] = { 0 };

        if (screen == Screen::Gameplay) {
          const unsigned seats=gameplaySeatMask?gameplaySeatMask:(1u<<gameplayPlayer);
          for(int seat=0;seat<PAD_PLAYERS;++seat)if(seats&(1u<<seat)) {
            selectGameplaySeat(seat);
            const auto viewport=gameplayViewport();
            vkCmdSetViewport(cmd,0,1,&viewport);
            VkRect2D clip{{int32_t(viewport.x),int32_t(viewport.y)},{uint32_t(viewport.width),uint32_t(viewport.height)}};
            vkCmdSetScissor(cmd,0,1,&clip);
            // Preserve the PC's ancestor SortingGroup before sorting its child
            // sprites. Flattening their local keys hides arrows behind the neck
            // and promotes multiplier ticks above unrelated notes.
            gameplayDraws=spriteDraws;
            gameplayDraws.push_back({pc_render_order::renderer(17009),
                GameplayDrawKind::Board,0,boardVertexCount});
            gameplayDraws.push_back({pc_render_order::at(pc_render_order::Layer::BeatLines),
                GameplayDrawKind::Beat,highwayVertexCount,beatVertexCount-highwayVertexCount});
            gameplayDraws.push_back({pc_render_order::at(pc_render_order::Layer::Sustains),
                GameplayDrawKind::Sustain,0,sustainVertexCount});
            for(size_t i=0;i<meterDraws.size();++i) {
                const auto& draw=meterDraws[i];
                gameplayDraws.push_back({pc_render_order::renderer(draw.renderer),
                    GameplayDrawKind::Meter,draw.first,6,false,i});
            }
            if(renderBatching)rhythm::stableDrawSort(gameplayDraws,pc_render_order::before);
            else std::stable_sort(gameplayDraws.begin(),gameplayDraws.end(),[](const auto& a,const auto& b) {
                return pc_render_order::before(a.key,b.key);
            });
            VkPipeline boundPipeline=VK_NULL_HANDLE;VkDescriptorSet boundSet=VK_NULL_HANDLE;VkBuffer boundBuffer=VK_NULL_HANDLE;
            for(size_t batchIndex=0;batchIndex<gameplayDraws.size();++batchIndex) {
                auto batch=gameplayDraws[batchIndex];
                if(!batch.count) continue;
                // Adjacent primitives retain their exact order. Only fold
                // identical state with contiguous vertices; meters carry
                // per-draw push constants and must remain separate.
                if(renderBatching && batch.kind!=GameplayDrawKind::Meter)while(batchIndex+1<gameplayDraws.size()) {
                    const auto& next=gameplayDraws[batchIndex+1];
                    if(next.kind!=batch.kind || next.additive!=batch.additive || next.first!=batch.first+batch.count)break;
                    batch.count+=next.count;++batchIndex;
                }
                VkPipeline pipeline=texPipeline;
                VkDescriptorSet set=atlasSets[currentBuffer];
                VkBuffer buffer=texBuffers[currentBuffer].buffer;
                switch(batch.kind) {
                case GameplayDrawKind::Board:
                    pipeline=boardPipeline;set=boardSets[currentBuffer];buffer=boardBuffers[currentBuffer].buffer;break;
                case GameplayDrawKind::Atlas:break;
                case GameplayDrawKind::GuitarFx:set=guitarFxSets[currentBuffer];if(batch.additive)pipeline=highwayGlowPipeline;break;
                case GameplayDrawKind::SixFret:set=sixFretSets[currentBuffer];break;
                case GameplayDrawKind::Drums:set=drumAtlasSets[currentBuffer];if(batch.additive)pipeline=highwayGlowPipeline;break;
                case GameplayDrawKind::Highway:
                    set=highwaySets[currentBuffer];
                    if(batch.additive) pipeline=highwayGlowPipeline;
                    break;
                case GameplayDrawKind::Beat:set=beatSets[currentBuffer];break;
                case GameplayDrawKind::Sustain:
                    pipeline=sustainPipeline;buffer=sustainBuffers[currentBuffer].buffer;break;
                case GameplayDrawKind::Meter: {
                    const auto& draw=meterDraws[batch.meterIndex];
                    pipeline=draw.sp?spMeterPipeline:draw.additive?meterGlowPipeline:meterPipeline;
                    set=meterSets[currentBuffer];buffer=meterBuffers[currentBuffer].buffer;
                    MeterParams params{glm::vec4(static_cast<float>(renderTime),starPower.fill(),
                        draw.numeralGlow?2.0f:starPower.active() || starPower.fill()>=0.5f?1.0f:0.0f,draw.fade?1.0f:0.0f),
                        glm::vec4(pc_meter_art::spElectricity[0],pc_meter_art::spElectricity[1],pc_meter_art::spElectricity[2],1)};
                    if(activeProfile.starPowerColor)for(unsigned k=0;k<3;++k)params.electricity[k]=selectedStarPowerColor()[k];
                    vkCmdPushConstants(cmd,pipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(params),&params);
                    break;
                }
                }
                if(!renderBatching || set!=boundSet){vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&set,0,nullptr);boundSet=set;++profileBinds;}
                if(!renderBatching || pipeline!=boundPipeline){vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);boundPipeline=pipeline;++profileBinds;}
                if(!renderBatching || buffer!=boundBuffer){vkCmdBindVertexBuffers(cmd,0,1,&buffer,off);boundBuffer=buffer;++profileBinds;}
                vkCmdDraw(cmd,batch.count,1,batch.first,0);++profileDraws;profileVertices+=batch.count;
            }
          }
          selectGameplaySeat(primaryGameplaySeat);
        }
        vkCmdSetViewport(cmd,0,1,&vp);vkCmdSetScissor(cmd,0,1,&sc);
        drawUI(cmd);
        endDynamicRendering(cmd);
        if(renderTimestampPool){vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,renderTimestampPool,currentBuffer*2+1);renderTimestampPending[currentBuffer]=true;}
        VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
    }

    void render() override
    {
        const double frameStart = now_seconds();
        if (!prepared) return;
        updateGameplay();
        profileUpdateMs=(now_seconds()-frameStart)*1000.;
        if(ps5.frameBudget && (multiplayerQa || (multiplayerQaLibraryLoaded && gameplaySeatMask)) && (multiplayerQaReplay || ps5.framesDrawn%15==0)) {
            if(FILE* trace=std::fopen("/app0/multiplayer-test.txt",ps5.framesDrawn<=10?"w":"a")) {
                const auto band=gameplayBandScore();
                for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
                    selectGameplaySeat(seat);
                    std::fprintf(trace,"frame %u seat %d source %d screen %d pause %d time %.17g notes %zu hits %u misses %u score %llu combo %u sp %.9f pro %d length %d speed %.3f result_page %d buffers %d",
                        unsigned(ps5.framesDrawn),seat,playerSeats.slotOfSeat[seat],int(screen),ui.visible,songTime,notes.size(),hits,misses,
                        (unsigned long long)score,combo,starPower.fill(),chartProDrums,activeProfile.highwayLength,noteSpeedScale,resultsSelection,
                        uniformBuffers[0].buffer && texBuffers[0].buffer && meterBuffers[0].buffer);
                    unsigned headHits=0,headMisses=0;
                    for(size_t i=0;i<chart.notes.size();++i){headHits+=gameplayEvaluator().state(i)==rhythm::NoteState::Hit;headMisses+=gameplayEvaluator().state(i)==rhythm::NoteState::Missed;}
                    std::fprintf(trace," sample %zu advance %d head_hits %u head_misses %u ordinary %llu solo %llu fret_bonus %llu base %llu health %.9g sp_ticks %lld sp_active %d band_total %llu band_ordinary %llu band_base %llu band_rank %u band_progress %.9g pause_owner %d instrument %d difficulty %d modifier %u failed %d\n",
                        multiplayerQaSample,multiplayerQaAdvanced,headHits,headMisses,(unsigned long long)(score-soloBonusTotal-fretBonusTotal),
                        (unsigned long long)soloBonusTotal,(unsigned long long)fretBonusTotal,(unsigned long long)chartBaseScore,gameplayHealth,
                        (long long)starPower.remainingTicks(),starPower.active(),(unsigned long long)band.total,(unsigned long long)band.ordinary,
                        (unsigned long long)band.base,scoreStarRank(band.ordinary,band.base),scoreStarProgress(band.ordinary,band.base),pauseOwner,
                        chartInstrument,chartDifficulty,unsigned(playerModifierMask[seat]),songFailed);
                }
                std::fclose(trace);selectGameplaySeat(primaryGameplaySeat);
            }
        }
        updatePcNoteAnimations();
        if(screen==Screen::Gameplay && !gameplaySeatMask)updateHighwayGlow();
        if(screen==Screen::Gameplay && gameplaySeatMask) {
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(gameplaySeatMask&(1u<<seat)) {
                selectGameplaySeat(seat);updateHighwayGlow();updateGameplayProjection();
            }
            selectGameplaySeat(primaryGameplaySeat);
        }
        // prepareFrame builds the UI: publish this frame's projection before it
        // projects meters, then write the GPU buffer after its fence is safe.
        if(screen==Screen::Gameplay)updateGameplayProjection();
        // Fret navigation belongs to this game. The shared sample backend maps
        // Square to ImGui's window switcher, which must not reach NewFrame here.
        auto& menuInput=ImGui::GetIO();
        menuInput.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);
        std::memset(menuInput.NavInputs,0,sizeof(menuInput.NavInputs));
        VulkanExampleBase::prepareFrame();
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.frameBudget && ps5.framesDrawn%30==0) {
            bool frameworkWindow=false;
            if(const auto* data=ImGui::GetDrawData()) for(int i=0;i<data->CmdListsCount;++i) {
                const char* owner=data->CmdLists[i]->_OwnerName;
                if(owner && (std::strstr(owner,"NavWindowingList") || std::strcmp(owner,"Vulkan Example")==0)) frameworkWindow=true;
            }
            if(FILE* trace=std::fopen("/app0/search-ui-test.txt",ps5.framesDrawn==0?"w":"a")) {
                std::fprintf(trace,"frame %u screen %d ime %d target %d field %d matches %zu debug %d flags %d ime_status %d close_stage %d query %s\n",
                    unsigned(ps5.framesDrawn),int(screen),searchKeyboardOpen?1:0,int(searchKeyboardTarget),int(songSearchField),
                    filteredSongIndices().size(),frameworkWindow?1:0,menuInput.ConfigFlags,
                    searchKeyboardOpen?searchKeyboardStatus():-1,keyboardCloseStage,songSearchQuery.c_str());
                std::fclose(trace);
            }
        }
#endif
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.frameBudget && (midiQa || midiDiscoveryQa) && ps5.framesDrawn%15==0)if(FILE* log=std::fopen("/app0/midi-setup-test.txt",ps5.framesDrawn<15?"w":"a")){
            std::fprintf(log,"frame %u screen %d page %d action %d mapped %zu type_page %d source %d linked %d companion %d\n",
                unsigned(ps5.framesDrawn),int(screen),int(midiWizard.page),int(midiWizard.action()),midiWizard.draft.mapping.bindings().size(),
                midiTypePage,midiSetupSource,midiSourceForSeat(0),midiNavSlot);std::fclose(log);
        }
        if(ps5.frameBudget && ps5.framesDrawn%15==0) if(FILE* log=std::fopen("/app0/controller-test.txt",ps5.framesDrawn==0?"w":"a")) {
            std::fprintf(log,"frame %u screen %d owner %d type %d stage %d index %u choice %d main %d reveal %.6f seats %d source1 %d source2 %d pro %d hits %u total %zu misses %u score %llu stars %u bot %d",
                unsigned(ps5.framesDrawn),int(screen),remapPlayer,playerRemaps[remapPlayer].wizard.type,int(playerRemaps[remapPlayer].wizard.page),playerRemaps[remapPlayer].wizard.index,playerRemaps[remapPlayer].wizard.row,mainSelection,versionNotesReveal,playerSeats.count,playerSeats.slotOfSeat[0],playerSeats.slotOfSeat[1],chartProDrums?1:0,unsigned(hits),notes.size(),unsigned(misses),static_cast<unsigned long long>(score),currentStarRank(),activeProfile.bot?1:0);
            std::fprintf(log," filter_ms %.3f",strumDebounceMs);
            std::fprintf(log," source3 %d source4 %d",playerSeats.slotOfSeat[2],playerSeats.slotOfSeat[3]);
            for(int player=0;player<PAD_PLAYERS;++player) {
                const auto& remap=playerRemaps[player];
                std::fprintf(log," active%d %d index%d %u stage%d %d review%d %d",player+1,remap.active?1:0,
                    player+1,remap.wizard.index,player+1,int(remap.wizard.page),player+1,remap.wizard.row);
                const auto& calibration=playerCalibrations[player];double reference=0;uint64_t epoch=0;
                const bool clock=calibrationReference(uint64_t(now_seconds()*1000000),reference,epoch);
                std::fprintf(log," calibration%d %d samples%d %u clock%d %d",player+1,int(calibration.phase),player+1,calibration.timing.count,player+1,clock?1:0);
            }
            std::fprintf(log,"\n");
            std::fclose(log);
        }
#endif
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.frameBudget && (storageQa || storageLiveQa) && ps5.framesDrawn%15==0)if(FILE* f=std::fopen("/app0/storage-test.txt",ps5.framesDrawn==0?"w":"a")) {
            unsigned external=0;for(const auto& song:localSongs)if(!song.externalRoot.empty())++external;
            std::fprintf(f,"frame %u screen %d total %zu external %u roots %zu busy %d copied %u duplicates %u failed %u popup %d popup_done %d usb_found %u usb_ready %u copy_page %d copy_choice %d copy_to_console %d plan_ready %d plan_copy %u plan_duplicates %u\n",unsigned(ps5.framesDrawn),int(screen),localSongs.size(),external,externalSongRoots.size(),storageThreadActive?1:0,storageThreadActive?0:storageCopyResult.copied,storageThreadActive?0:storageCopyResult.duplicates,storageThreadActive?0:storageCopyResult.failed,storagePopupActive?1:0,storagePopupComplete?1:0,storagePopupComplete?storagePopupFound:storageUsbFound.load(),storagePopupComplete?storagePopupLoaded:storageUsbLoaded.load(),int(songCopyPage),songCopyChoice,songCopyToConsole?1:0,songCopyPlanReady?1:0,songCopyPlanReady?songCopyPlan.toCopy:0,songCopyPlanReady?songCopyPlan.duplicates:0);
            std::fclose(f);
        }
#endif
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.frameBudget && ps5.framesDrawn%60==0 && std::any_of(ps5Presses.begin(),ps5Presses.end(),[](const auto& p){return p.action=="discover-usb";})) {
            const auto devices=usbControllers.snapshot();
            if(FILE* f=std::fopen("/app0/controller-usb-test.txt",ps5.framesDrawn==0?"w":"a")) {
                std::fprintf(f,"frame %u native_mask %u\n",unsigned(ps5.framesDrawn),pad_players());
                for(const auto& d:devices)if(d.present) {
                    std::fprintf(f,"device %04x:%04x streaming %d unknown %d bridge_owned %d length %u sequence %llu name %s\n",d.vid,d.pid,d.streaming,d.unknown,d.bridgeOwned,d.length,static_cast<unsigned long long>(d.sequence),d.deviceProfile?d.deviceProfile->name.c_str():"unmapped");
                    if(d.streaming && d.deviceProfile) {
                        const auto input=rhythm::controls::applyProfile(*d.deviceProfile,{},d.report.data(),d.length);
                        std::fprintf(f,"logical frets %u pads %u up %d down %d start %d report",input.frets,input.pads,input.strumUp,input.strumDown,input.start);
                        for(unsigned i=0;i<d.length;++i)std::fprintf(f," %02x",d.report[i]);
                        std::fprintf(f,"\n");
                    }
                }
                std::fclose(f);
            }
        }
#endif
        const double geometryStart=now_seconds();
        const unsigned renderSeats=screen==Screen::Gameplay?(gameplaySeatMask?gameplaySeatMask:(1u<<activeGameplaySeat)):0;
        for(int seat=0;seat<PAD_PLAYERS;++seat)if(renderSeats&(1u<<seat)) {
            selectGameplaySeat(seat);
        // prepareFrame has waited for this buffer's fence. Never rewrite a
        // descriptor still used by another frame or another player's highway.
        const int style=std::clamp(activeProfile.highwayStyle,0,int(rhythm::highwayStyles.size())-1);
        if(boardStyleIds[currentBuffer]!=style) {
            const auto& texture=style?highwayStyleTextures[style-1]:boardTex;
            VkDescriptorImageInfo image{texture.sampler,texture.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const auto write=vks::initializers::writeDescriptorSet(boardSets[currentBuffer],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,&image);
            vkUpdateDescriptorSets(device,1,&write,0,nullptr);
            boardStyleIds[currentBuffer]=style;
        }
        memcpy(uniformBuffers[currentBuffer].mapped,&uniformData,sizeof(uniformData));
        buildMeterFrame();
        if(!meterScratch.empty()) memcpy(meterBuffers[currentBuffer].mapped,meterScratch.data(),meterScratch.size()*sizeof(TexVertex));
        auto& tv = spriteScratch; buildTexFrame(tv);
        buildSustainFrame(sustainScratch);
        if (!sustainScratch.empty()) memcpy(sustainBuffers[currentBuffer].mapped, sustainScratch.data(), sustainScratch.size()*sizeof(TexVertex));
        if (!tv.empty())
            memcpy(texBuffers[currentBuffer].mapped, tv.data(), tv.size() * sizeof(TexVertex));
        auto& bv = boardScratch; buildBoardFrame(bv);
        memcpy(boardBuffers[currentBuffer].mapped, bv.data(), bv.size() * sizeof(TexVertex));
        }
        selectGameplaySeat(primaryGameplaySeat);
        profileGeometryMs=(now_seconds()-geometryStart)*1000.;
        const double commandStart=now_seconds();
        buildCommandBuffer();
        const double commandMs=(now_seconds()-commandStart)*1000.;
        if(renderProfileQa && ps5.framesDrawn>60 && screen==Screen::Gameplay)if(FILE* f=std::fopen("/app0/render-profile.txt","a")) {
            std::fprintf(f,"frame %u players %u batching %d update_ms %.6f geometry_ms %.6f command_ms %.6f gpu_ms %.6f draws %u binds %u vertices %u\n",unsigned(ps5.framesDrawn),unsigned(std::popcount(renderSeats)),renderBatching,profileUpdateMs,profileGeometryMs,commandMs,profileGpuMs,profileDraws,profileBinds,profileVertices);std::fclose(f);
        }
        VulkanExampleBase::submitFrame();
#if !defined(PS5_HOST_REFERENCE)
        if(ps5.framesDrawn>=120)encore_application_rendered();
#endif
        if(screen==Screen::Gameplay && !gameplayAudioPending)songStartGate.presented(now_seconds());
        applyDisplayPacing();
        // FIFO/VideoOut paces Auto and caps to exact display vblanks (including
        // 59.94/119.88). Sleep only if the driver's pacing call was refused.
        if (!displayPacingApplied && frameRateLimit != 0) {
            const double remaining = 1000.0 / displayTiming.targetMilliHz(frameRateLimit) - (now_seconds() - frameStart);
            if (remaining > 0.001)
                platform_sleep_us(static_cast<uint32_t>(remaining * 1000000.0));
        }
    }

    void OnUpdateUIOverlay(vks::UIOverlay* overlay) override
    {
        (void)overlay;
        if(screen!=Screen::MainMenu && screen!=Screen::ControllerSetup && screen!=Screen::MidiSetup) {
            versionNotesExpanded=false;versionNotesReveal=0;versionNotesScrollTime=0;
            versionNotesSlideElapsed=0;versionNotesExpanding=false;
            encoreFeaturesExpanded=false;encoreFeaturesReveal=0;encoreFeaturesScrollTime=0;
            encoreFeaturesSlideElapsed=0;encoreFeaturesExpanding=false;
        }
        if(screen==Screen::Results)flushInputDiagnostics();
        if (screen == Screen::MainMenu || screen==Screen::ControllerSetup || screen==Screen::MidiSetup) refreshMenuPreview();
        else if(screen==Screen::Gameplay && showSongDisplay && gameplaySongInfoValid)refreshSongAlbum(gameplaySongInfo);
        else if(screen==Screen::Results && gameplaySongInfoValid)refreshResultsIcon();
        else if((screen==Screen::SongSelect || screen==Screen::SongSearch || screen==Screen::PlayerSelect) && songSelection>=0 && songSelection<static_cast<int>(localSongs.size()))
            refreshSongAlbum(localSongs[static_cast<size_t>(songSelection)]);
        if(ps5.frameBudget && ps5.framesDrawn%30==0 && (screen==Screen::SongSelect || screen==Screen::SongSearch)) {
            if(FILE* log=std::fopen("/app0/song-art-test.txt",ps5.framesDrawn<=30?"w":"a")) {
                const bool selected=songSelection>=0 && songSelection<int(localSongs.size());
                const bool matches=selected && menuPreviewChartPath==localSongs[songSelection].chartPath && menuAlbumTexturePath==localSongs[songSelection].albumArtPath;
                std::fprintf(log,"frame %u matches %d visible %d title %s\n",unsigned(ps5.framesDrawn),matches?1:0,menuAlbumVisible?1:0,selected?localSongs[songSelection].title.c_str():"");std::fclose(log);
            }
        }
        ImGuiIO& io = ImGui::GetIO();
        const float fitScale = std::min(io.DisplaySize.x / 1920.0f, io.DisplaySize.y / 1080.0f);
        const float scale = fitScale * menuScale;
        const float ox = (io.DisplaySize.x - 1920.0f * scale) * 0.5f;
        const float oy = (io.DisplaySize.y - 1080.0f * scale) * 0.5f;
        const auto p = [&](float x, float y) { return ImVec2(ox + x * scale, oy + y * scale); };
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##clonehero_pc_menu", nullptr, flags);
        ImGui::SetWindowFontScale(scale);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        // Draw modal contents above all ordinary UI, with one extra 10% dim.
        // Per-player profile/remap panels stay non-modal and independent.
        const bool popupActive=updateDialog || instrumentKeyboard.open || searchKeyboardOpen || screen==Screen::SongCopyConfirm || screen==Screen::SongRemove ||
            screen==Screen::ControllerSetup || screen==Screen::MidiSetup || (screen==Screen::Gameplay && ui.visible) ||
            ((screen==Screen::Settings || screen==Screen::SettingsDetail) && settingsEditor==SettingsEditor::Choices);
        if(popupActive) {
            draw->ChannelsSplit(2);draw->ChannelsSetCurrent(1);
            draw->AddRectFilled(ImVec2(0,0),io.DisplaySize,IM_COL32(0,0,0,26));
            draw->ChannelsSetCurrent(0);
        }
        const auto popupLayer=[&](bool foreground){if(popupActive)draw->ChannelsSetCurrent(foreground?1:0);};
        if (screen != Screen::Gameplay) {
        if (menuBackgroundIndex == 13) {
            draw->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 255));
        } else if (menuBackgroundIds[menuBackgroundIndex]) {
            // The PC uses separate static, scrolling-UV, and Surfer-material
            // modes. Preserve each image's aspect ratio and approximate the
            // two animated material modes with a subtle bounded UV drift.
            static constexpr bool wideBackgrounds[] = {
                false, true, true, false, false, false, false, false, true, true, true, true, true, false
            };
            const float baseWidth = wideBackgrounds[menuBackgroundIndex] ? 16.0f / 18.0f : 1.0f;
            const float zoom = menuBackgroundAnimation == 1 ? 1.025f :
                               (menuBackgroundAnimation == 2 ? 1.045f : 1.0f);
            const float uvWidth = baseWidth / zoom, uvHeight = 1.0f / zoom;
            const double time = ImGui::GetTime();
            const float driftX = menuBackgroundAnimation == 2
                ? static_cast<float>(std::sin(time * 0.11) * baseWidth * 0.007) : 0.0f;
            const float driftY = menuBackgroundAnimation == 0 ? 0.0f :
                static_cast<float>(std::sin(time * (menuBackgroundAnimation == 1 ? 0.16 : 0.13)) * 0.006);
            const ImVec2 uv0((1.0f - uvWidth) * 0.5f + driftX,
                             (1.0f - uvHeight) * 0.5f + driftY);
            const ImVec2 uv1(uv0.x + uvWidth, uv0.y + uvHeight);
            draw->AddImage(menuBackgroundIds[menuBackgroundIndex], ImVec2(0, 0), io.DisplaySize, uv0, uv1);
        }
        if ((screen == Screen::MainMenu || screen==Screen::ControllerSetup || screen==Screen::MidiSetup) && menuLogoId) {
            // Keep the typographic title centered in the 553x360 layout slot. Preserve
            // the supplied artwork's aspect ratio within that slot.
            const float logoWidth=std::min(553.0f,360.0f*menuLogoAspect);
            const float logoHeight=logoWidth/menuLogoAspect;
            draw->AddImage(menuLogoId,p(960-logoWidth*.5f,495-logoHeight*.5f),
                p(960+logoWidth*.5f,495+logoHeight*.5f),menuLogoUV0,menuLogoUV1);
        }
        }
        // Keep menu panels square-cornered per the requested PS5 menu styling.
        auto nineSlice = [&](ImTextureID texture, float x, float y, float w, float h, ImU32 tint) {
            (void)texture;
            draw->AddRectFilled(p(x,y), p(x+w,y+h), tint);
        };
        auto buttonImage = [&](float x, float y, float w, float h, bool selected, bool enabled) {
            const ImU32 tint = enabled ? (selected ? IM_COL32(255,255,255,255) : IM_COL32(185,195,205,210)) : IM_COL32(110,115,125,120);
            nineSlice(menuButtonId, x, y, w, h, tint);
        };
        auto label = [&](const char* text, float x, float y, float w, float h, bool enabled, bool selected) {
            (void)selected;
            ImVec2 size = ImGui::CalcTextSize(text);
            ImU32 color = enabled ? IM_COL32(245,247,250,255) : IM_COL32(150,155,165,190);
            if (selected && enabled) color = IM_COL32(18,25,34,255);
            // Layout is in the PC scene's 1920x1080 units; ImGui reports text
            // dimensions in display pixels. Convert before centering the label.
            draw->AddText(p(x + 20, y + (h - size.y / scale) * 0.5f), color, text);
        };
        auto clickable = [&](int id, float x, float y, float w, float h) {
            if(screen==Screen::ControllerSetup || screen==Screen::MidiSetup)return false;
            if(updateDialog || screen==Screen::SongRemove)return false;
            if(screen==Screen::SongCopyConfirm && id!=9800 && id!=9801)return false;
            ImGui::SetCursorScreenPos(p(x, y));
            ImGui::InvisibleButton((std::string("##menu") + std::to_string(id)).c_str(), ImVec2(w * scale, h * scale));
            return !searchKeyboardOpen && !instrumentKeyboard.open && !gameplayReadyPending && ImGui::IsItemClicked();
        };
        // Recovered Song Select scene yaw is shared by Quickplay and Encore.
        const auto projectSongMenuPlanes = [&](int firstVertex) {
            for(int v=firstVertex;v<draw->VtxBuffer.Size;++v) {
                ImDrawVert& vertex=draw->VtxBuffer[v];
                const float x=(vertex.pos.x-ox)/scale,y=(vertex.pos.y-oy)/scale;
                const bool left=x<1400;
                const float edge=left?60.0f:1880.0f;
                const float sine=left?-0.08715574f:0.12186934f;
                const float cosine=left?0.9961947f:0.99254615f;
                const float factor=1100.0f/(1100.0f-(x-edge)*sine);
                vertex.pos=p(960+(edge+(x-edge)*cosine-960)*factor,540+(y-540)*factor);
            }
        };
        // Shared Press Start/version-panel timing, including direction reversals.
        const auto animateMenuPanel=[&](bool expanded,bool& expanding,float& elapsed,float& slide) {
            if(expanded!=expanding) {expanding=expanded;elapsed=0.0f;}
            elapsed=std::min(1.0f,elapsed+io.DeltaTime);
            const float t=elapsed,easing=t*t*(3.0f-2.0f*t),target=expanded?1.0f:0.0f;
            slide+=(target-slide)*easing;
            if(std::abs(target-slide)<0.001f) slide=target;
        };
        const auto drawPlayerCards = [&]() {
            static constexpr float playerCenters[PAD_PLAYERS] = {255,725,1195,1665};
            for (int i=0; i<PAD_PLAYERS; ++i) {
                auto& slot=profileMenu.slots[i];
                const bool expanded=profileMenu.open(i) || playerRemaps[i].active || playerCalibrations[i].active();
                animateMenuPanel(expanded,slot.expanding,slot.slideElapsed,slot.slide);
                const float x=playerCenters[i]-217.5f, y=978.0f-308.0f*slot.slide;
                const auto* profile=profileMenu.profile(i);
                const char* title=profile?profile->name.c_str():"PRESS START";
                if (expanded && slot.page==rhythm::ProfileMenu::Page::Profiles) title="Select Profile";
                if (slot.page==rhythm::ProfileMenu::Page::Name) title=slot.rename?"Rename Profile":"Create Profile";
                if (slot.page==rhythm::ProfileMenu::Page::Colors) title="Color Options";
                if(slot.page==rhythm::ProfileMenu::Page::Calibration)title="Calibrate Controller";
                if (slot.page==rhythm::ProfileMenu::Page::Delete) title="Delete Profile?";
                // Keep the profile card translucent in both collapsed and
                // expanded states so opening it does not look like the menu
                // wallpaper switched to black. Text and controls stay opaque.
                draw->AddRectFilled(p(x,y),p(x+435,1056),IM_COL32(8,13,23,217));
                const bool guestTitle=slot.profile==-1 && (!expanded || slot.page==rhythm::ProfileMenu::Page::Options);
                const ImU32 titleInk=guestTitle?guestNameInk(i):IM_COL32(245,247,250,255);
                const float titleSize=profile||expanded?26.0f:32.0f;
                const float titleWidth=ImGui::GetFont()->CalcTextSizeA(titleSize*scale,100000,0,title).x/scale;
                draw->PushClipRect(p(x+12,y),p(x+423,y+78),true);
                const ImTextureID icon=profile&&!expanded?playerControllerIcon(profile->controller):ImTextureID{};
                if (icon) {
                    const float gx=x+(435.0f-(42.0f+10.0f+titleWidth))*0.5f;
                    draw->AddImage(icon,p(gx,y+18),p(gx+42,y+60));
                    draw->AddText(ImGui::GetFont(),titleSize*scale,p(gx+52,y+(78-titleSize)*0.5f),titleInk,title);
                } else draw->AddText(ImGui::GetFont(),titleSize*scale,p(x+std::max(12.0f,(435-titleWidth)*0.5f),y+(78-titleSize)*0.5f),titleInk,title);
                draw->PopClipRect();
                if (clickable(600+i*100,x,y,435,78)) {if(playerCalibrations[i].active())closePlayerCalibration(i,0,"Calibration cancelled.");else if(playerRemaps[i].active)closeControllerRemap(i,0);else profileMenu.start(i);}
                if (!expanded) continue;
                draw->PushClipRect(p(x,y+78),p(x+435,1056),true);
                if(playerCalibrations[i].active()) {
                    auto& c=playerCalibrations[i];using Phase=PlayerCalibration::Phase;
                    const ImU32 white=IM_COL32(240,243,247,255),muted=IM_COL32(180,193,212,255);
                    const auto label=[&](const char* value,float dy,float size,ImU32 ink){
                        const auto* font=ImGui::GetFont();const float width=font->CalcTextSizeA(size,100000,0,value).x;
                        draw->AddText(font,size*scale,p(x+std::max(12.0f,(435-width)*.5f),y+dy),ink,value);
                    };
                    const auto hint=[&](int color,const char* value,float center,float dy){
                        const float width=ImGui::GetFont()->CalcTextSizeA(18,100000,0,value).x;
                        const float hx=x+center-(25+9+width)*.5f;
                        draw->AddImage(songSelectControlIds[color],p(hx,y+dy),p(hx+25,y+dy+25));
                        draw->AddText(ImGui::GetFont(),18*scale,p(hx+34,y+dy+3),white,value);
                    };
                    label(rhythm::MenuProfile::controllerNames[c.type],88,21,muted);
                    if(c.phase==Phase::TimingReview || c.phase==Phase::VelocityReview){
                        if(c.phase==Phase::TimingReview){
                            char value[100];
                            if(c.timing.valid)std::snprintf(value,sizeof(value),"Suggested offset: %+.0f ms",c.timing.offsetMs+audioOffsetMs);
                            else std::snprintf(value,sizeof(value),"Timing was inconsistent - retry");
                            label(value,125,22,c.timing.valid?white:IM_COL32(255,195,130,255));
                            std::snprintf(value,sizeof(value),"%u good hits  |  variation %.0f ms",c.timing.inliers,c.timing.spreadMs);
                            label(value,157,18,muted);
                        }else{label("Velocity calibration complete",125,22,white);label("Kick mappings are unchanged",157,18,muted);}
                        const char* choices[]{profileMenu.slots[i].profile>=0?"Save Player":"Apply to Guest","Retry","Cancel"};
                        for(int row=0;row<3;++row){
                            const float ry=y+196+row*35;const bool selected=c.row==row;
                            if(selected)draw->AddRectFilled(p(x+8,ry),p(x+427,ry+33),IM_COL32(208,212,214,255));
                            const ImU32 ink=selected?IM_COL32(15,18,24,255):white;
                            label(choices[row],ry-y+6,21,ink);
                            if(clickable(9400+i*10+row,x+8,ry,419,33)){
                                c.row=row;if(row==0)savePlayerCalibration(i,0);else if(row==1)beginPlayerCalibration(i,c.phase==Phase::VelocityReview);else closePlayerCalibration(i,0,"Calibration cancelled.");
                            }
                        }
                        hint(0,"Select",119,312);hint(1,"Cancel",316,312);
                    }else if(c.phase==Phase::Velocity){
                        static constexpr const char* names[]{"Kick","Red Pad","Yellow Pad","Yellow Cymbal","Blue Pad","Blue Cymbal","Green Pad","Green Cymbal","Orange Cymbal"};
                        const unsigned action=unsigned(c.velocity.action());
                        const int color=action==1?1:action==2||action==3?2:action==4||action==5?3:action==8?4:0;
                        label(c.velocity.page==rhythm::midi::SetupWizard::Page::Soft?"Strike softly":"Strike firmly",124,26,white);
                        draw->AddImage(songSelectControlIds[color],p(x+195,y+163),p(x+239,y+207));
                        label(names[std::min(action,8u)],216,25,white);
                        label("Use the same pad shown above",254,18,muted);
                        hint(2,"Skip pad",119,311);hint(1,"Cancel",316,311);
                    }else{
                        label(c.source>=0?"Tap a MIDI pad or kick to the clicks":c.type>=2?"Hit the kick to the clicks":"Strum or press Green to the clicks",124,21,white);
                        double now=0;uint64_t epoch=0;
                        const bool audio=calibrationReference(uint64_t(now_seconds()*1000000),now,epoch);
                        char value[96];
                        if(!audio || c.phase==Phase::WaitingAudio)std::snprintf(value,sizeof(value),"Starting audio...");
                        else if(now<c.timing.firstBeat)std::snprintf(value,sizeof(value),"Get ready: %d",int(std::ceil((c.timing.firstBeat-now)/rhythm::CalibrationBeatSeconds)));
                        else std::snprintf(value,sizeof(value),"%u / %u good hits",c.timing.count,rhythm::TimingCalibration::Samples);
                        label(value,178,29,white);
                        draw->AddRectFilled(p(x+35,y+229),p(x+400,y+242),IM_COL32(52,66,87,255));
                        draw->AddRectFilled(p(x+35,y+229),p(x+35+365*float(c.timing.count)/rhythm::TimingCalibration::Samples,y+242),IM_COL32(105,206,160,255));
                        const bool flash=audio && std::fmod(std::max(0.0,now),rhythm::CalibrationBeatSeconds)<.085;
                        draw->AddRectFilled(p(x+198,y+268),p(x+237,y+283),flash?IM_COL32(230,245,255,255):IM_COL32(57,73,94,255));
                        hint(1,"Cancel",217.5f,311);
                    }
                    const char* note=c.message.empty()?(profileMenu.slots[i].profile>=0?"Saved per player and instrument":"Guest changes last for this session"):c.message.c_str();
                    label(note,351,16,muted);
                } else if(playerRemaps[i].active) {

                    const auto& remap=playerRemaps[i];const auto& wizard=remap.wizard;
                    using Page=rhythm::controls::Remap::Page;
                    const ImU32 white=IM_COL32(240,243,247,255),muted=IM_COL32(184,194,210,255);
                    const auto label=[&](const char* value,float dy,float size,ImU32 ink) {
                        const ImFont* face=ImGui::GetFont();
                        const char* end=value+std::strlen(value);
                        while(value<end) {
                            const char* lineEnd=std::strchr(value,'\n');if(!lineEnd)lineEnd=end;
                            const char* stop=face->CalcWordWrapPositionA(size/face->FontSize,value,lineEnd,399);
                            if(stop<=value)stop=lineEnd;
                            const float width=face->CalcTextSizeA(size,100000,0,value,stop).x;
                            draw->AddText(face,size*scale,p(x+(435-width)*.5f,y+dy),ink,value,stop);
                            dy+=size+3;value=stop;
                            while(value<end && (*value==' ' || *value=='\n' || *value=='\r'))++value;
                        }
                    };
                    const auto hint=[&](int icon,const char* value,float center,float dy) {
                        const float width=icon>=5?48.0f:25.0f;
                        const float textWidth=ImGui::GetFont()->CalcTextSizeA(18,100000,0,value).x;
                        const float hx=center-(width+10+textWidth)*.5f;
                        draw->AddImage(songSelectControlIds[icon],p(x+hx,y+dy),p(x+hx+width,y+dy+25));
                        draw->AddText(ImGui::GetFont(),18*scale,p(x+hx+width+10,y+dy+3),white,value);
                    };
                    label("REMAP CONTROLLER",88,23,white);
                    if(wizard.page==Page::Review) {
                        constexpr const char* actions[]{"Save Player","Save Device","Retry Setup","Restore Defaults","Cancel"};
                        for(int row=0;row<5;++row) {
                            const float ry=y+123+row*33;const bool selected=wizard.row==row;
                            if(selected)draw->AddRectFilled(p(x+8,ry),p(x+427,ry+31),IM_COL32(208,212,214,255));
                            const ImU32 ink=!remapReviewEnabled(i,row)?IM_COL32(115,120,130,255):selected?IM_COL32(16,16,16,255):white;
                            label(actions[row],ry-y+5,21,ink);
                            if(clickable(9000+i*10+row,x+8,ry,419,31) && remapReviewEnabled(i,row)) {
                                playerRemaps[i].wizard.row=row;
                                const auto& navigation=playerRemaps[i].navigation;
                                updateControllerRemap(i,playerRemaps[i].raw?PAD_CROSS:navigation.keys[rhythm::controls::greenIndex(wizard.type)],0);
                            }
                        }
                        hint(0,"Confirm",119,300);hint(1,"Back",316,300);
                    } else {
                        const char* action=rhythm::controls::action(wizard.type,wizard.index);
                        if(remap.raw && !remap.calibrated)action="Release all inputs";
                        int actionIcon=-1;
                        if(!remap.raw || remap.calibrated) {
                            constexpr const char* colors[]{"Green","Red","Yellow","Blue","Orange"};
                            for(int color=0;color<5;++color)if(std::strncmp(action,colors[color],std::strlen(colors[color]))==0)actionIcon=color;
                            if(std::strcmp(action,"Start")==0)actionIcon=6;
                            if(std::strcmp(action,"Star Power")==0)actionIcon=5;
                        }
                        if(actionIcon>=0) {
                            const float iconWidth=actionIcon>=5?64.0f:44.0f;
                            const float iconHeight=actionIcon>=5?33.0f:44.0f;
                            const ImFont* face=ImGui::GetFont();
                            const float measured=face->CalcTextSizeA(28,100000,0,action).x;
                            const float textSize=28*std::min(1.0f,(399-iconWidth-12)/std::max(1.0f,measured));
                            const float textWidth=face->CalcTextSizeA(textSize,100000,0,action).x;
                            const float groupX=x+(435-iconWidth-12-textWidth)*.5f;
                            draw->AddImage(songSelectControlIds[actionIcon],p(groupX,y+156),p(groupX+iconWidth,y+156+iconHeight));
                            draw->AddText(face,textSize*scale,p(groupX+iconWidth+12,y+156+(iconHeight-textSize)*.5f),white,action);
                        } else label(action,161,32,white);
                        if(wizard.page==Page::Conflict) {
                            char conflict[128];std::snprintf(conflict,sizeof(conflict),"Already assigned: %s",rhythm::controls::action(wizard.type,wizard.duplicate));
                            label(conflict,213,21,muted);hint(0,"Swap",119,276);hint(1,"Try again",316,276);
                        } else label("Press, then release this input.",224,21,muted);
                        hint(6,"Review / cancel",217.5f,308);
                    }
                    if(!remap.message.empty())label(remap.message.c_str(),342,18,IM_COL32(255,192,133,255));
                } else if (slot.page==rhythm::ProfileMenu::Page::Name) {
                    draw->AddText(ImGui::GetFont(),26.0f*scale,p(x+18,y+96),IM_COL32(245,247,250,255),slot.name.empty()?"Enter profile name":slot.name.c_str());
                    draw->AddText(ImGui::GetFont(),22.0f*scale,p(x+18,y+156),IM_COL32(210,220,235,255),"Green: Edit name");
                    draw->AddText(ImGui::GetFont(),22.0f*scale,p(x+18,y+198),IM_COL32(210,220,235,255),"Red: Cancel");
                    if(clickable(610+i*100,x+12,y+86,411,120))(void)openSearchKeyboard(SearchKeyboardTarget::Profile,i);
                } else {
                    const auto rows=profileMenu.rows(i);
                    slot.row=std::clamp(slot.row,0,static_cast<int>(rows.size())-1);
                    const int first=std::clamp(slot.row-5,0,std::max(0,static_cast<int>(rows.size())-6));
                    for (int visible=0; visible<6 && first+visible<static_cast<int>(rows.size()); ++visible) {
                        const int row=first+visible; const float ry=y+78+visible*50.0f;
                        const bool selected=row==slot.row;
                        const bool enabled=slot.page!=rhythm::ProfileMenu::Page::Options||profileMenu.enabled(rows[row]);
                        if (selected) draw->AddRectFilled(p(x+5,ry),p(x+430,ry+48),IM_COL32(225,231,239,255));
                        const ImU32 ink=enabled?(selected?IM_COL32(12,18,26,255):IM_COL32(240,243,247,255)):(selected?IM_COL32(100,105,114,255):IM_COL32(130,137,149,255));
                        draw->AddText(ImGui::GetFont(),21.0f*scale,p(x+18,ry+14),ink,rows[row].c_str());
                        if (profile && (slot.page==rhythm::ProfileMenu::Page::Options || slot.page==rhythm::ProfileMenu::Page::Colors || slot.page==rhythm::ProfileMenu::Page::Calibration)) {
                            char value[32]{};
                            if (rows[row]=="Controller") {
                                const int controller=std::clamp(profile->controller,0,4);
                                std::snprintf(value,sizeof(value),"%s",rhythm::MenuProfile::controllerNames[controller]);
                                const ImTextureID controllerIcon=playerControllerIcon(controller);
                                const bool available=rhythm::controls::controllerTypeEnabled(controller);
                                const float measured=ImGui::GetFont()->CalcTextSizeA(20.0f,100000,0,value).x;
                                const float valueSize=20*std::min(1.0f,230/std::max(1.0f,measured));
                                const float vw=ImGui::GetFont()->CalcTextSizeA(valueSize,100000,0,value).x;
                                const float gx=x+415-(40+8+vw);
                                if (controllerIcon) draw->AddImage(controllerIcon,p(gx,ry+5),p(gx+40,ry+45),ImVec2(0,0),ImVec2(1,1),available?IM_COL32(255,255,255,255):IM_COL32(130,130,130,255));
                                draw->AddText(ImGui::GetFont(),valueSize*scale,p(gx+48,ry+(50-valueSize)*.5f),available?ink:IM_COL32(115,120,130,255),value);
                            } else {
                                if(rows[row]=="Input Offset") std::snprintf(value,sizeof(value),"%+.0f ms",profile->inputOffsetsMs[std::clamp(profile->controller,0,4)]);
                                else if(rows[row]=="Calibrate Controller") std::snprintf(value,sizeof(value),">");
                                else if(rows[row]=="Highway Style") std::snprintf(value,sizeof(value),"%s",rhythm::highwayStyles[std::clamp(profile->highwayStyle,0,int(rhythm::highwayStyles.size())-1)].name);
                                else if(rows[row]=="Highway Effects") std::snprintf(value,sizeof(value),"%s",profile->highwayEffects?"ON":"DEFAULT");
                                else if(rows[row]=="Note Color") std::snprintf(value,sizeof(value),"%s",rhythm::noteColorNames[std::clamp(profile->noteColor,0,9)]);
                                else if(rows[row]=="Star Power Color") std::snprintf(value,sizeof(value),"%s",rhythm::starPowerColorNames[std::clamp(profile->starPowerColor,0,5)]);
                                else if(rows[row]=="Color Options") std::snprintf(value,sizeof(value),">");
                                else if(rows[row]=="Tap Note Color") std::snprintf(value,sizeof(value),"%s",profile->purpleTapNotes?"PURPLE":"DEFAULT");
                                else if(rows[row]=="Hit Effect Color") std::snprintf(value,sizeof(value),"%s",profile->laneHitEffects?"LANE COLORS":"DEFAULT");
                                else if(rows[row]=="Kick Color") std::snprintf(value,sizeof(value),"%s",profile->usesPurpleKick()?"PURPLE":"ORANGE");
                                else if(rows[row]=="Note Speed") std::snprintf(value,sizeof(value),"%.1fx",profile->speed);
                                else if(rows[row]=="Highway Length") std::snprintf(value,sizeof(value),"%d%%",profile->highwayLength);
                                else if(rows[row]=="Lefty Flip") std::snprintf(value,sizeof(value),"%s",profile->lefty?"On":"Off");
                                else if(rows[row]=="Gamepad Mode") std::snprintf(value,sizeof(value),"%s",profile->gamepad?"On":"Off");
                                else if(rows[row]=="Bot") std::snprintf(value,sizeof(value),"%s",profile->bot?"On":"Off");
                                else if(rows[row]=="Show Accuracy") std::snprintf(value,sizeof(value),"%s",profile->showAccuracy?"On":"Off");
                                else if(rows[row]=="Display Name") std::snprintf(value,sizeof(value),"%s",profile->displayName?"On":"Off");
                                const float vw=ImGui::GetFont()->CalcTextSizeA(20.0f*scale,100000,0,value).x/scale;
                                draw->AddText(ImGui::GetFont(),20.0f*scale,p(x+415-vw,ry+15),ink,value);
                            }
                        }
                        if (clickable(660+i*100+visible,x+5,ry,425,48)) { slot.row=row; profileMenu.accept(i); }
                    }
                    if(slot.page==rhythm::ProfileMenu::Page::Calibration && rows[slot.row]=="Input Offset"){
                        draw->AddImage(songSelectControlIds[2],p(x+55,y+342),p(x+80,y+367));
                        draw->AddText(ImGui::GetFont(),18*scale,p(x+88,y+346),IM_COL32(235,240,247,255),"-1 ms");
                        draw->AddImage(songSelectControlIds[3],p(x+250,y+342),p(x+275,y+367));
                        draw->AddText(ImGui::GetFont(),18*scale,p(x+283,y+346),IM_COL32(235,240,247,255),"+1 ms");
                    }

                }
                draw->PopClipRect();
                if (!slot.message.empty()) draw->AddText(ImGui::GetFont(),17.0f*scale,p(x+10,y-25),IM_COL32(255,211,151,255),slot.message.c_str());
                playerJoined[i]=profileMenu.joined(i);
            }
        };
        const auto projectRightWidget=[&](ImVec2 position,float right) {
            const float x=(position.x-ox)/scale,y=(position.y-oy)/scale;
            const float perspective=2200.0f/(2200.0f+(right-x)*0.17364818f);
            return p(960.0f+(right+(x-right)*0.98480775f-960.0f)*perspective,
                     540.0f+(y-540.0f)*perspective);
        };
        constexpr float previewX=1476.0f,previewY=70.5f,previewWidth=391.5f,previewHeight=120.0f;
        const auto drawNowPlaying=[&](const LocalSong* previewSong,float progress,bool progressVisible,double scrollTime) {
            const ImFont* widgetFont=ImGui::GetFont();
            nineSlice(menuButtonId,previewX,previewY,previewWidth,previewHeight,IM_COL32(10,10,10,153));
            if(menuAlbumId&&menuAlbumVisible)
                draw->AddImage(menuAlbumId,p(previewX+15.0f,previewY+10.0f),p(previewX+105.0f,previewY+100.0f));
            const char* previewTitle=previewSong?previewSong->title.c_str():"No songs installed";
            const char* previewArtist=previewSong&&!previewSong->artist.empty()?previewSong->artist.c_str():"Unknown artist";
            const char* previewYear=previewSong&&!previewSong->year.empty()?previewSong->year.c_str():"";
            auto previewText=[&](const ImFont* font,const char* text,float y,float fontSize,ImU32 color) {
                constexpr float textX=previewX+120.0f,textWidth=previewWidth-135.0f;
                const float pixelWidth=textWidth*scale;
                const ImVec2 size=font->CalcTextSizeA(fontSize*scale,100000.0f,0.0f,text);
                const float overflow=std::max(0.0f,size.x-pixelWidth);
                float offset=0.0f;
                if(overflow>0.0f) {
                    constexpr float pause=1.0f,speed=24.0f;
                    const float travel=overflow/speed;
                    const float phase=std::fmod(static_cast<float>(scrollTime),pause*2.0f+travel);
                    if(phase>pause&&phase<pause+travel) offset=(phase-pause)*speed;
                    else if(phase>=pause+travel) offset=overflow;
                }
                const ImVec2 topLeft=p(textX,y);
                // Clip glyph geometry before the perspective transform so the
                // original axis-aligned GPU scissor cannot cut tilted text.
                const ImVec2 clipEnd=p(textX+textWidth,y+fontSize);
                const ImVec4 clip(topLeft.x,topLeft.y,clipEnd.x,clipEnd.y);
                draw->AddText(font,fontSize*scale,ImVec2(topLeft.x-offset,topLeft.y),color,text,nullptr,0.0f,&clip);
            };
            previewText(ratingFont?ratingFont:widgetFont,previewTitle,previewY+18.0f,23.0f,IM_COL32(245,247,250,255));
            previewText(widgetFont,previewArtist,previewY+49.0f,19.0f,IM_COL32(230,234,241,255));
            previewText(widgetFont,previewYear,previewY+73.0f,17.0f,IM_COL32(205,211,220,255));
            if(progressVisible) {
                const ImVec2 progressMin=p(previewX+13.9f,previewY+105.0f);
                const ImVec2 progressMax=p(previewX+377.5f,previewY+113.0f);
                draw->AddRectFilled(progressMin,progressMax,IM_COL32(90,108,132,255));
                const float fill=std::clamp(progress,0.0f,1.0f);
                if(fill>0.0f) draw->AddRectFilled(progressMin,
                    ImVec2(progressMin.x+(progressMax.x-progressMin.x)*fill,progressMax.y),IM_COL32(255,255,255,255));
            }

        };
        if (screen == Screen::MainMenu || screen==Screen::ControllerSetup || screen==Screen::MidiSetup) {
            const ImU32 menuOptionBackground=IM_COL32(9,14,25,128);
            const ImU32 widgetBackground=IM_COL32(10,10,10,255);
            static constexpr const char* entries[]={"QUICKPLAY","VERSUS","ONLINE (COMING SOON)","PRACTICE","SETTINGS","ENCORE BROWSER"};
            for(int i=0;i<6;++i) {
                const float rowY=282.0f+i*86.0f;
                constexpr float left=36.0f, width=455.0f, height=78.0f;
                const bool enabled=i!=2;
                const bool selected=enabled&&i==mainSelection;
                const ImU32 tint=selected?IM_COL32(214,218,220,255):menuOptionBackground;
                const ImU32 ink=!enabled?IM_COL32(128,134,144,255):(selected?IM_COL32(18,25,34,255):IM_COL32(238,241,245,255));
                const int rowVertexStart=draw->VtxBuffer.Size;
                draw->AddRectFilled(p(left,rowY),p(left+width,rowY+height),tint);
                const ImFont* buttonFont=settingsFont?settingsFont:ImGui::GetFont();
                constexpr float fontSize=43.0f;
                const float availableWidth=width-44.0f;
                const float measured=buttonFont->CalcTextSizeA(fontSize,1000000.0f,0.0f,entries[i]).x;
                const float size=fontSize*std::min(1.0f,availableWidth/measured);
                // Center the visible glyph bounds, rather than the font line box.
                float top=10000.0f,bottom=-10000.0f;
                for(const char* c=entries[i];*c;++c) {
                    const ImFontGlyph* glyph=buttonFont->FindGlyph(static_cast<ImWchar>(*c));
                    if(glyph&&glyph->Y1>glyph->Y0) {top=std::min(top,glyph->Y0);bottom=std::max(bottom,glyph->Y1);}
                }
                const float glyphScale=size/buttonFont->FontSize;
                const float textY=rowY+(height-(bottom-top)*glyphScale)*0.5f-top*glyphScale;
                draw->AddText(buttonFont,size*scale,p(left+22,textY),ink,entries[i]);
                // PC menu plane has a small inward pitch; text shares its panel transform.
                for(int v=rowVertexStart;v<draw->VtxBuffer.Size;++v) {
                    ImDrawVert& vertex=draw->VtxBuffer[v];
                    const float x=(vertex.pos.x-ox)/scale;
                    vertex.pos.y+=(x-left)/width*6.0f*(1.0f-rowY/850.0f)*scale;
                }
                if(enabled&&clickable(i,left,rowY,width,height)) {
                    mainSelection=i;
                    if(i==0||i==1||i==3) {
                        playMode=i==1?PlayMode::Versus:(i==3?PlayMode::Practice:PlayMode::Quickplay);
                        screen=Screen::SongSelect;
                    } else if(i==4) {
                        settingsCategory=settingsCategorySelection<5?settingsCategorySelection:0;
                        settingsCategorySelection=settingsCategory;
                        settingsSelection=0;
                        settingsEditor=SettingsEditor::None;
                        screen=Screen::Settings;
                    } else if(i==5) screen=Screen::Downloader;
                }
            }

            // Both inline banners share the PC News plane and animation. The
            // version header stays fixed; Features follows its revealed body.
            constexpr float notesX=1488.0f,notesW=354.0f;
            animateMenuPanel(versionNotesExpanded,versionNotesExpanding,versionNotesSlideElapsed,versionNotesReveal);
            animateMenuPanel(encoreFeaturesExpanded,encoreFeaturesExpanding,encoreFeaturesSlideElapsed,encoreFeaturesReveal);
            const float panelHeight=(476.0f-8.0f*versionNotesReveal)/std::max(1.0f,versionNotesReveal+encoreFeaturesReveal);
            const ImFont* widgetFont=ImGui::GetFont();
            for(int infoPanel=0;infoPanel<2;++infoPanel) {
            const bool features=infoPanel==1;
            const float notesY=features?388.0f+(panelHeight+8.0f)*versionNotesReveal:308.0f;
            const float reveal=features?encoreFeaturesReveal:versionNotesReveal;
            const bool expanded=features?encoreFeaturesExpanded:versionNotesExpanded;
            float& scrollTime=features?encoreFeaturesScrollTime:versionNotesScrollTime;
            // Both PC News Scroll View and Scrolling Song Preview use the same
            // +10-degree Y rotation. Keep each complete plane on its right pivot.
            const int notesVertexStart=draw->VtxBuffer.Size;
            draw->AddRectFilled(p(notesX,notesY),p(notesX+notesW,notesY+72),widgetBackground);
            draw->AddText(widgetFont,20.0f*scale,p(notesX+16,notesY+26),
                IM_COL32(245,247,250,255),features?"ENCORE FEATURES":"VERSION NOTES");
            draw->AddImage(songSelectControlIds[features?2:3],p(notesX+notesW-54,notesY+15),p(notesX+notesW-12,notesY+57));
            if(!features && mainSelection==6)
                draw->AddRect(p(notesX,notesY),p(notesX+notesW,notesY+72),
                    IM_COL32(233,236,241,245),0.0f,0,2.0f*scale);
            // Test the projected banner rather than its old axis-aligned bounds.
            const ImVec2 bannerCorners[]={
                projectRightWidget(p(notesX,notesY),notesX+notesW),
                projectRightWidget(p(notesX+notesW,notesY),notesX+notesW),
                projectRightWidget(p(notesX+notesW,notesY+72),notesX+notesW),
                projectRightWidget(p(notesX,notesY+72),notesX+notesW)};
            bool bannerHovered=true;
            for(int corner=0;corner<4;++corner) {
                const ImVec2 a=bannerCorners[corner],b=bannerCorners[(corner+1)%4];
                if((b.x-a.x)*(io.MousePos.y-a.y)-(b.y-a.y)*(io.MousePos.x-a.x)<0) bannerHovered=false;
            }
            if(bannerHovered && ImGui::IsMouseClicked(0)) {if(features)toggleEncoreFeatures();else toggleVersionNotes();}
            if(reveal>0) {
                const float panelY=notesY+80.0f;
                constexpr float padding=18.0f;
                constexpr float textWidth=notesW-padding*2.0f,fontSize=22.0f,headingSize=24.0f;
                struct Change { const char* heading;const char* body; };
                static constexpr Change changes[]={
                    {"1.5.2 - Standalone Windows builder",
                     "One Windows executable creates your personal install.\n"
                     "Folder and exFAT tools are included.\n"
                     "PKG tool downloads automatically on first use.\n"
                     "No separate desktop dependencies or ZIP extraction.\n"
                     "Your authorized PC assets are added locally.\n"
                     "Per-player, per-instrument timing calibration.\n"
                     "MIDI pad velocity can be recalibrated without remapping."},
                    {"1.5.1 - Personal install builder",
                     "Build your install on PC using your own authorized assets.\n"
                     "Choose a game folder, exFAT image or native homebrew PKG.\n"
                     "Personal builds prepare included assets automatically.\n"
                     "No separate asset transfer or import selection required.\n"
                     "Imported resources remain separate from code updates."},
                    {"1.5.0 - pursuaded",
                     "CH: Encore branding.\n"
                     "PC artwork, fonts and effects are no longer bundled.\n"
                     "Import an authorized personal asset pack at first launch.\n"
                     "Existing song libraries, profiles and scores are preserved."},
                    {"Beta 1.0.2 - Bring your own songs",
                     "Playable songs are no longer included.\n"
                     "Add charts with Encore Browser or an exFAT SONGS folder.\n"
                     "Existing internal and USB song libraries are preserved."},
                    {"Beta 1.0.1 - MIDI and native guitar input",
                     "Improved discovery of paired PS4 RB4 guitars.\n"
                     "Existing handles and delayed input publication supported.\n"
                     "RB4 hardware confirmation still pending.\n"
                     "Class-compliant USB MIDI 1.0 drum input.\n"
                     "4-Lane Pro and 5-Lane guided setup.\n"
                     "Keep a DualSense connected for menus and pause.\n"
                     "Map one or two kick pads without kick calibration.\n"
                     "Optional soft/hard pad and cymbal calibration.\n"
                     "Save mappings for a device or named player.\n"
                     "Repeated strikes retain independent timing.\n"
                     "Experimental / intermittent hardware support."},
                    {"Beta 1.0 - Updates and song management",
                     "Experimental native PS4 RB4 Stratocaster Bluetooth input.\n"
                     "RetroCultMods PS3-mode guitar menu controls.\n"
                     "Automatic release checks and verified update downloads.\n"
                     "Verified updates restart into a separate runtime.\n"
                     "Failed first launches restore the previous build.\n"
                     "Internal songs now use /data/homebrew/SONGS-VR.\n"
                     "Existing internal libraries migrate without overwriting.\n"
                     "Select and remove multiple installed or bundled songs.\n"
                     "Scores, profiles and bundled-song removal are preserved."},
                    {"Alpha v.39 - Startup and six-fret guitars",
                     "Main menu opening cue followed by fading-in song previews.\n"
                     "Fixed startup on installations without development songs.\n"
                     "Album art loads from the selected preview song.\n"
                     "Added early startup diagnostics for launch problems.\n"
                     "Six-fret lead, bass, rhythm and co-op tracks.\n"
                     "Automatic Guitar Hero Live PS4/PS3 USB presets.\n"
                     "Instrument choices follow the connected guitar type.\n"
                     "Tap ranges and full sustain lengths are preserved.\n"
                     "Subdued white open notes for six-fret tracks."},
                    {"Alpha v.38 - Drum menu controls",
                     "Drum kits use their D-pad and dedicated menu buttons.\n"
                     "Fixed Rock Band 4 face-button input in menus.\n"
                     "Older Rock Band cymbals no longer cause menu movement.\n"
                     "Existing gameplay remaps retain hardware navigation."},
                    {"Alpha v.37 - Five-lane drum highway",
                     "Dedicated five-lane drum highway and hit effects.\n"
                     "Yellow and orange notes use cymbal artwork.\n"
                     "Five-lane kits default to purple kicks.\n"
                     "Kick color choices remain saved per profile."},
                    {"Alpha v.36 - Solo HUD and local records",
                     "Solo percentage displays for each player.\n"
                     "Saved scores show stars, player and instrument.\n"
                     "Song selection uses full-mix widget previews.\n"
                     "Stronger filtering for repeated menu selections.\n"
                     "Optional solo highway effect uses a steady blue glow.\n"
                     "Automatic mappings for Riffmaster and other USB instruments.\n"
                     "New controller setup appears over the main menu.\n"
                     "Beta diagnostics save a bounded background log."},
                    {"Alpha v.35 - Preview fade-in",
                     "Song widget previews fade in smoothly over one second.\n"
                     "Full-song mixes retain their five-second fade-out.\n"
                     "Menu sound effects keep their normal volume."},
                    {"Alpha v.34 - Streaming and engine checks",
                     "Streamed song audio reduces memory and startup time.\n"
                     "Safer cancellation when changing songs or leaving Ready.\n"
                     "Audio read failures pause gameplay safely.\n"
                     "Corrected sustain release after overstrums.\n"
                     "Expanded tempo, sustain and drum regression checks.\n"
                     "Reduced repeated four-player rendering commands.\n"
                     "Song widget mixes fade out over their final five seconds."},
                    {"Alpha v.33 - Audio and multiplayer",
                     "Lower memory use and faster song preview preparation.\n"
                     "Preview clips preserve the complete instrument mix.\n"
                     "Full song audio is prepared safely before the countdown.\n"
                     "Corrected sustain scoring across input boundaries.\n"
                     "Verified four-player scoring and highway layouts."},
                    {"Alpha v.32 - Tester build",
                     "Improved fast-note and repeated-note recovery.\n"
                     "Checked alternate strums and drum/cymbal recovery.\n"
                     "Added resume rewind and missed-note sound volume.\n"
                     "Upcoming lyrics promote into the current line.\n"
                     "Faster menu browsing and cached difficulty selection.\n"
                     "Includes the original PC bundled songs.\n"
                     "Menu previews play for up to 60 seconds."},
                    {"Alpha v.31 - PS5 keyboard fix",
                     "Fixed PS5 keyboard cancellation and reopening.\n"
                     "Quickplay, Encore Browser and profile entry use the native PS5 input panel.\n"
                     "Guitar controllers retain the in-game keyboard.\n"
                     "Expanded Encore Features and set menu previews to 60 seconds."},
                    {"Alpha v.30 - Lyrics and drum chart compatibility",
                     "Timed lyrics with current and upcoming phrases.\n"
                     "Syllables highlight with the song clock.\n"
                     "Lyrics can be switched off in Video settings.\n"
                     "MIDI vocal phrases import automatically.\n"
                     "Corrected five-lane drum chord conversions.\n"
                     "Expert+ kicks remain separate from Expert drums."},
                    {"Alpha v.29 - Chart compatibility and timing",
                     "Corrected Star Power's fractional deployment deadline.\n"
                     "Solo displays follow the PC note-range timing.\n"
                     "Preserved five-lane MIDI drum pads and cymbals.\n"
                     "Existing MIDI imports refresh automatically.\n"
                     "Matched the PC chart clock's rounding at tick boundaries.\n"
                     "Corrected overlapping sustain payouts at multiplier changes.\n"
                     "Held tap/HOPO input now follows normal PC judging timing."},
                    {"Alpha v.28 - Four-player gameplay",
                     "Results actions work from any player's controller.\n"
                     "Random Song reloads every selected instrument and difficulty.\n"
                     "Confirmed restarts return every player to the countdown.\n"
                     "Corrected buffered HOPO priority and chart solo-end scoring."},
                    {"Alpha v.27 - Gameplay and player colors",
                     "Per-player highway styles and color options.\n"
                     "Optional danger, solo, streak and five-star highway pulses.\n"
                     "Tap sheen and transparent tails; consistent Star Power effects.\n"
                     "Corrected chord sustain scoring and Star Power measure timing.\n"
                     "Corrected chord health, buffered strums and exact sustain-release timing.\n"
                     "Solo markers read correctly from Windows-format charts.\n"
                     "Preserved MIDI drum dynamics and corrected pro-drum star scoring.\n"
                     "Corrected drum solo bonuses, rolls, fills and phrase lightning.\n"
                     "Song completion follows the full chart and audio timeline.\n"
                     "Faster MIDI rescans; retain custom song settings and end markers.\n"
                     "Improved held tap-note timing and restored open-note body.\n"
                     "Afterlife audio compatibility fix."},
                    {"Alpha v.26 - Menus and song loading",
                     "Encore Features panel on the main menu.\n\n"
                     "Compact controller keyboard with an opaque background and DualSense assistance.\n\n"
                     "Directional-pad and left-stick menu navigation.\n\n"
                     "Audio prepares while choosing instruments and difficulties, reusing decoded previews.\n\n"
                     "Ready waits for loading with a slim progress indicator.\n\n"
                     "Song timing starts after loading and first-frame preparation."},
                    {"Alpha v.25 - Input and performance",
                     "Direct USB input for supported guitars.\n\n"
                     "Instrument keyboard with fret and strum controls. DualSense uses the PS5 keyboard.\n\n"
                     "Automatic 120/60 FPS pacing with display capability checks.\n\n"
                     "Faster chart judgement with unchanged scoring and timing.\n\n"
                     "Timestamped USB input preserves brief fret, strum and drum strikes.\n\n"
                     "PC countdown formatting and bottom player-name placement.\n\n"
                     "Revised results statistics and multiplier-strip alignment."},
                    {"Alpha v.24 - Library update",
                     "USB song libraries with nested collection folders.\n\n"
                     "Scan widget with live progress.\n\n"
                     "Confirmed USB/console copies with song counts and duplicate checks.\n\n"
                     "Album-art fixes in the song list.\n\n"
                     "Improved MIDI chart compatibility.\n\n"
                     "Corrected held-note effect colors."},
                    {"Alpha v.23 - Core features",
                     "Player profiles and instrument remapping.\n\n"
                     "Encore song search and downloads.\n\n"
                     "Guitar and drum gameplay, audio stems and autoplay.\n\n"
                     "Scoring, star ranks, Star Power and sustain notes.\n\n"
                     "Song previews, settings and pause controls."}
                };
                static constexpr Change additions[]={
                    {"USB MIDI drums (experimental)","Class-compliant USB MIDI 1.0 kits can map 4-Lane Pro or 5-Lane layouts. Keep a DualSense connected for menus and pause. One/two kick pads, optional pad/cymbal velocity calibration and saved mappings. Hardware compatibility is experimental/intermittent."},
                    {"Five-lane drum highway","Red, yellow, blue, orange and green lanes. Yellow and orange use cymbals; five-lane kits default to muted purple kicks."},
                    {"Built-in Encore Browser","Browse song covers, charted instruments and charters. Download songs or queue several charts without leaving the game."},
                    {"Portable USB libraries","Use an exFAT-formatted drive with a SONGS folder at its root. Nested collections are detected; unplugging removes those songs from the list."},
                    {"Application updates","Check for releases at startup or in Settings. Confirm the verified update, then restart and play. Songs, profiles and scores are preserved."},
                    {"Shared song storage","Internal songs live in /data/homebrew/SONGS-VR. Older libraries move automatically. Select several installed songs and confirm removal together."},
                    {"Copy songs both ways","Copy USB songs to the console or console songs to USB. Confirm the song count before copying; existing charts are skipped."},
                    {"Beta tester diagnostics","A small rotating background log records important loading, display and controller events in userdata/logs/encore-test.log."},
                    {"Direct USB instruments","Supported USB rhythm controllers connect directly in game without a separate GameBridge payload."},
                    {"Player calibration","Timing tests and manual offsets are saved separately for each player and instrument. MIDI pad/cymbal velocity can be recalibrated without remapping kicks."},
                    {"Guided controller remapping","Set up an unfamiliar device from the player menu. Save mappings for that device or player and instrument; known mappings load automatically."},
                    {"Rhythm-controller menus","Browse with frets and strum controls, or a D-pad and stick. Settings use categories, select-to-edit lists and red to return."},
                    {"Controller-friendly text entry","Guitar controllers use the fret-and-strum keyboard. DualSense uses the native PS5 keyboard for search and profile names."},
                    {"Display-aware graphics","Automatic supported-output resolution and 60/120 FPS pacing. The 120 FPS option is disabled when the connected display does not support it."},
                    {"Per-player note palettes","Choose standard lane colors, a single color, purple, silver, gold or cycling RGB notes from Color Options in your player menu."},
                    {"Star Power palettes","Choose the default Star Power color or purple, white, gold, red or green, independently for each player."},
                    {"Tap notes and hit effects","Optional purple sheen and transparency for tap notes and their tails. Hit flames, sparks and sustain effects can follow the lane colors."},
                    {"Drum kick colors","Choose orange or muted purple kicks for each drum player, with matching hit and sustain effects."},
                    {"Optional highway pulses","Red near failure, purple at a 4x streak, a steady blue glow during solos and faint gold after five stars. Enable Highway Effects in your player's Color Options."},
                    {"Sixty-second menu previews","Play a dedicated preview or a full stem mix from a random section, up to 60 seconds. Recent-song protection avoids repeats; orange mutes or unmutes."},
                    {"Input filtering","Menu bounce protection also covers remapping and keyboard navigation. Gameplay has its own adjustable strum filter in Settings."},
                    {"Build notes in game","Expand Version History or Encore Features on the main menu. Long panels scroll automatically, and the current build appears in every menu."}
                };
                const Change* content=features?additions:changes;
                const size_t contentCount=features?std::size(additions):std::size(changes);
                const auto measure=[&](const char* text,float size) {
                    return widgetFont->CalcTextSizeA(size*scale,100000.0f,textWidth*scale,text).y/scale;
                };
                float contentHeight=0;
                for(size_t i=0;i<contentCount;++i)contentHeight+=measure(content[i].heading,headingSize)+6.0f+measure(content[i].body,fontSize)+22.0f;
                contentHeight-=22.0f;
                const float viewport=panelHeight-padding*2.0f;
                const float overflow=std::max(0.0f,contentHeight-viewport);
                bool profileOpen=false;
                for(int player=0;player<PAD_PLAYERS;++player) profileOpen=profileOpen || profileMenu.open(player);
                if(expanded && reveal>=1.0f && !profileOpen)scrollTime+=std::min(io.DeltaTime,0.1f);
                float offset=0;
                if(overflow>0) {
                    constexpr float pause=3.0f,speed=18.0f;
                    const float travel=overflow/speed;
                    const float phase=std::fmod(scrollTime,2.0f*(pause+travel));
                    if(phase<pause) offset=0;
                    else if(phase<pause+travel) offset=(phase-pause)*speed;
                    else if(phase<2.0f*pause+travel) offset=overflow;
                    else offset=overflow-(phase-2.0f*pause-travel)*speed;
                }
                const float bottom=panelY+panelHeight*reveal;
                draw->AddRectFilled(p(notesX,panelY),p(notesX+notesW,bottom),menuOptionBackground);
                const ImVec2 clipLo=p(notesX+padding,panelY+padding),clipHi=p(notesX+notesW-padding,bottom-padding);
                if(clipHi.y>clipLo.y) {
                    // Clip glyph geometry before projecting, so the GPU scissor
                    // cannot cut the tilted edges or leak scrolling text.
                    const ImVec4 clip(clipLo.x,clipLo.y,clipHi.x,clipHi.y);
                    float y=panelY+padding-offset;
                    for(size_t i=0;i<contentCount;++i) {
                        const auto& change=content[i];
                        draw->AddText(widgetFont,headingSize*scale,p(notesX+padding,y),IM_COL32(146,194,255,255),change.heading,nullptr,textWidth*scale,&clip);
                        y+=measure(change.heading,headingSize)+6.0f;
                        draw->AddText(widgetFont,fontSize*scale,p(notesX+padding,y),IM_COL32(238,241,245,255),change.body,nullptr,textWidth*scale,&clip);
                        y+=measure(change.body,fontSize)+22.0f;
                    }
                }
            }
            for(int v=notesVertexStart;v<draw->VtxBuffer.Size;++v)
                draw->VtxBuffer[v].pos=projectRightWidget(draw->VtxBuffer[v].pos,notesX+notesW);
            }

            // The preview metadata follows the currently previewed chart and
            // scrolls within the fixed PC-style text mask when a line is long.
            const int previewVertexStart=draw->VtxBuffer.Size;
            const LocalSong* previewSong=localSongs.empty()||menuMusicSelection<0?nullptr:&localSongs[static_cast<size_t>(
                std::clamp(menuMusicSelection,0,static_cast<int>(localSongs.size()-1)))];
            float previewProgress=0.0f;bool previewProgressVisible=false;
#if !defined(PS5_HOST_REFERENCE)
            previewProgressVisible=audioOutputStarted && previewSong && previewPlayingForMenu && previewClipLength>0 &&
                menuPreviewAudioChartPath==previewSong->chartPath;
            if(previewProgressVisible) previewProgress=static_cast<float>((songAudio.positionSeconds()-previewClipStart)/previewClipLength);
#endif
            drawNowPlaying(previewSong,previewProgress,previewProgressVisible,ImGui::GetTime()-menuPreviewChangedAt);
            constexpr float promptFontSize=18.0f,promptGap=10.0f;
            const float pressWidth=widgetFont->CalcTextSizeA(promptFontSize*scale,100000.0f,0.0f,"PRESS").x/scale;
            const float selectTextWidth=widgetFont->CalcTextSizeA(promptFontSize*scale,100000.0f,0.0f,"SELECT").x/scale;
            const float promptX=previewX+14.0f;
            const float selectX=promptX+pressWidth+promptGap;
            draw->AddText(widgetFont,promptFontSize*scale,p(promptX,previewY+135.0f),IM_COL32(230,234,241,255),"PRESS");
            draw->AddRectFilled(p(selectX,previewY+129.5f),p(selectX+72.0f,previewY+158.5f),widgetBackground);
            draw->AddText(widgetFont,promptFontSize*scale,
                p(selectX+(72.0f-selectTextWidth)*0.5f,previewY+135.0f),IM_COL32(255,255,255,255),"SELECT");
            draw->AddText(widgetFont,promptFontSize*scale,p(selectX+72.0f+promptGap,previewY+135.0f),
                IM_COL32(230,234,241,255),"TO PLAY");
            if(menuIntroFinished && clickable(500,previewX,previewY+122.0f,235.0f,44.0f)) {
                playMode=PlayMode::Quickplay;
                if(!openSongInPlayerSelect(menuMusicSelection,Screen::MainMenu)) screen=Screen::SongSelect;
            }
            const char* muteAction=menuPreviewMuted?"UNMUTE":"MUTE";
            const float muteTextWidth=widgetFont->CalcTextSizeA(18.0f*scale,100000,0,muteAction).x/scale;
            const float muteX=previewX+previewWidth-14.0f-32.0f-10.0f-muteTextWidth;
            draw->AddImage(songSelectControlIds[4],p(muteX,previewY+128),p(muteX+32,previewY+160));
            draw->AddText(widgetFont,18.0f*scale,p(muteX+42,previewY+135),IM_COL32(245,245,245,255),muteAction);
            if(menuIntroFinished && clickable(501,previewX+240,previewY+122,previewWidth-240,44)) toggleMenuPreviewMute();
            // PC Scrolling Song Preview quaternion: y=sin(5deg), w=cos(5deg).
            // Project its 10-degree yaw about the right edge into canvas space.
            // Camera distance is visually matched to mainmenu.webp; the scene
            // export preserves the rotation but not the complete canvas camera.
            for(int v=previewVertexStart;v<draw->VtxBuffer.Size;++v) {
                ImDrawVert& vertex=draw->VtxBuffer[v];
                vertex.pos=projectRightWidget(vertex.pos,previewX+previewWidth);
#if !defined(PS5_HOST_REFERENCE)
                const float opacity=menuIntroFinished?std::clamp(float((ImGui::GetTime()-menuWidgetRevealAt)/0.6),0.0f,1.0f):0.0f;
                vertex.col=(vertex.col & 0x00ffffffu) | (ImU32(float(vertex.col>>24)*opacity)<<24);
#endif
            }
            drawPlayerCards();
        } else if (screen == Screen::SongSelect) {
            const int songMenuVertexStart=draw->VtxBuffer.Size;
            ImFont* light=settingsFont?settingsFont:ImGui::GetFont();
            const ImU32 panel=IM_COL32(20,23,28,145), header=IM_COL32(18,18,18,235);
            const auto songText=[&](ImFont* font,float size,float x,float y,ImU32 ink,const std::string& value,float maxWidth) {
                const ImVec2 lo=p(x,y),hi=p(x+maxWidth,y+size+4);
                const ImVec4 clip(lo.x,lo.y,hi.x,hi.y);
                draw->AddText(font,size*scale,lo,ink,value.c_str(),nullptr,0.0f,&clip);
            };
            draw->AddRectFilled(p(60,55),p(1383,105),header);
            static constexpr const char* actions[]={"SELECT","BACK","SORT","SEARCH","NEXT SECTION","SEARCH OPTIONS"};
            constexpr float hintFontSize=18.0f;
            const auto textTop=[&](const char* value,float size) {
                const ImFont* font=ImGui::GetFont();float top=font->FontSize,bottom=0;
                for(const char* c=value;*c;++c) if(const auto* glyph=font->FindGlyph(*c)) {
                    top=std::min(top,glyph->Y0);bottom=std::max(bottom,glyph->Y1);
                }
                return 80.0f-(top+bottom)*.5f*size/font->FontSize;
            };
            std::array<float,6> groupWidths{};
            float totalGroupWidth=0;
            for(int control=0;control<6;++control) {
                const float width=ImGui::GetFont()->CalcTextSizeA(hintFontSize*scale,100000,0,actions[control]).x/scale;
                groupWidths[control]=(control<5?34.0f:48.0f)+10.0f+width;
                totalGroupWidth+=groupWidths[control];
            }
            const float gap=std::max(12.0f,(1170.0f-72.0f-totalGroupWidth)/5.0f);
            float hintX=72.0f;
            for(int control=0;control<6;++control) {
                const float x=hintX;
                if(control<5) draw->AddImage(songSelectControlIds[control],p(x,63),p(x+34,97));
                else draw->AddImage(songSelectControlIds[5],p(x,67),p(x+48,93));
                songText(ImGui::GetFont(),hintFontSize,x+(control<5?44:58),textTop(actions[control],hintFontSize),IM_COL32(238,242,249,255),actions[control],groupWidths[control]);
                if(control==3 && clickable(7100,x,55,groupWidths[control],50)) (void)beginLocalSongSearch();
                if(control==5 && clickable(7101,x,55,groupWidths[control],50)) openSongSearchOptions();
                hintX+=groupWidths[control]+gap;
            }
            const auto& filtered=filteredSongIndices();
            char countLabel[64];
            std::snprintf(countLabel,sizeof(countLabel),"Songs: %zu",filtered.size());
            constexpr float countFontSize=24.0f;
            const float countWidth=ImGui::GetFont()->CalcTextSizeA(countFontSize*scale,100000,0,countLabel).x/scale;
            songText(ImGui::GetFont(),countFontSize,1370-countWidth,textTop(countLabel,countFontSize),IM_COL32(238,242,249,255),countLabel,countWidth+2);
            if(filtered.empty()) {
                songText(ImGui::GetFont(),27,90,150,IM_COL32(239,243,250,255),
                    songSearchQuery.empty()?"No local charts found":"No songs match this search",1100);
            } else {
                auto current=std::find(filtered.begin(),filtered.end(),static_cast<size_t>(songSelection));
                const size_t selected=current==filtered.end()?0:static_cast<size_t>(current-filtered.begin());
                const auto key=[&](size_t index) {
                    const LocalSong& song=localSongs[filtered[index]];
                    const std::string* label=&song.title;
                    switch(songSortField) {
                    case SongSortField::Artist:label=&song.artist;break;
                    case SongSortField::Album:label=&song.album;break;
                    case SongSortField::Genre:label=&song.genre;break;
                    case SongSortField::Year:label=&song.year;break;
                    case SongSortField::Charter:label=&song.charter;break;
                    default:break;
                    }
                    for(unsigned char c:*label) if(std::isalnum(c)) return static_cast<char>(std::toupper(c));
                    return '#';
                };
                // Rebuild group counts only when the filtered library changes.
                const int groupField=static_cast<int>(songSearchField);
                if(songSectionCacheRevision!=songListRevision||songSectionCacheField!=groupField||
                   songSectionCacheQuery!=songSearchQuery||songSectionCounts.size()!=filtered.size()) {
                    songSectionCounts.assign(filtered.size(),0);
                    for(size_t start=0;start<filtered.size();) {
                        size_t end=start+1;
                        while(end<filtered.size()&&key(end)==key(start)) ++end;
                        for(size_t i=start;i<end;++i) songSectionCounts[i]=end-start;
                        start=end;
                    }
                    songSectionCacheRevision=songListRevision;
                    songSectionCacheField=groupField;
                    songSectionCacheQuery=songSearchQuery;
                }
                // Limit work to the visible window; never open chart/score files in rendering.
                const size_t first=selected>5?selected-5:0;
                int visible=0;
                for(size_t i=first;i<filtered.size()&&visible<13;++i) {
                    const bool groupStart=i==first||key(i)!=key(i-1);
                    if(groupStart&&visible<12) {
                        const float y=112+visible++*61.0f;
                        draw->AddRectFilled(p(60,y),p(1363,y+56),IM_COL32(8,8,8,245));
                        draw->AddImage(menuDetailIds[2],p(99,y+16),p(123,y+40));
                        std::string heading=std::to_string(songSectionCounts[i])+(songSectionCounts[i]==1?" SONG  ":" SONGS  ")+key(i);
                        const float hw=ImGui::GetFont()->CalcTextSizeA(31*scale,100000,0,heading.c_str()).x/scale;
                        songText(ImGui::GetFont(),31,542-hw,y+11,IM_COL32(242,244,248,255),heading,795);
                    }
                    const auto& song=localSongs[filtered[i]];
                    const float y=112+visible++*61.0f;
                    const bool chosen=i==selected;
                    const ImU32 ink=chosen?IM_COL32(24,24,24,255):IM_COL32(234,238,244,255);
                    draw->AddRectFilled(p(60,y),p(1363,y+56),chosen?IM_COL32(214,218,220,255):panel);
                    std::string artist=song.artist;
                    for(char& c:artist) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    const float aw=light->CalcTextSizeA(35*scale,100000,0,artist.c_str()).x/scale;
                    const ImVec2 al=p(82,y),ar=p(542,y+56);const ImVec4 ac(al.x,al.y,ar.x,ar.y);
                    draw->AddText(light,35*scale,p(542-aw,y+9),ink,artist.c_str(),nullptr,0,&ac);
                    const auto* best=songBest(song);
                    songText(ImGui::GetFont(),32,562,y+10,ink,song.title,best?395:775);
                    if(best) {
                        if(best->details)for(unsigned star=0;star<7;++star) {
                            const bool complete=star<best->stars;
                            const ImU32 color=chosen?(complete?IM_COL32(30,30,30,255):IM_COL32(120,120,120,120)):
                                (complete?IM_COL32(255,218,100,255):IM_COL32(90,90,90,100));
                            draw->AddImage(resultsUiIds[0],p(975+star*20,y+17),p(993+star*20,y+38),ImVec2(0,0),ImVec2(1,1),color);
                        }
                        char percent[24],points[32];
                        if(best->accuracyKnown)std::snprintf(percent,sizeof(percent),"%.2f%%",best->accuracyHundredths*.01);
                        else std::snprintf(percent,sizeof(percent),"--");
                        std::snprintf(points,sizeof(points),"%llu",static_cast<unsigned long long>(best->score));
                        songText(ImGui::GetFont(),22,1125,y+16,ink,percent,95);
                        songText(ImGui::GetFont(),22,1230,y+16,ink,points,118);
                    }
                    if(clickable(static_cast<int>(100+i),60,y,1303,56)) songSelection=static_cast<int>(filtered[i]);
                }
                if(filtered.size()>7) {
                    draw->AddRectFilled(p(1370,112),p(1382,943),IM_COL32(25,27,32,150));
                    const float amount=static_cast<float>(selected)/static_cast<float>(filtered.size()-1);
                    const float thumb=std::max(35.0f,831.0f*7.0f/static_cast<float>(filtered.size()));
                    const float y=112+(831-thumb)*amount;
                    draw->AddRectFilled(p(1370,y),p(1382,y+thumb),IM_COL32(214,218,222,255));
                }
                const auto& song=localSongs[filtered[selected]];
                const ImU32 ink=IM_COL32(236,239,245,255);
                draw->AddRectFilled(p(1415,90),p(1880,127),header);
                const float tw=ImGui::GetFont()->CalcTextSizeA(25*scale,100000,0,song.title.c_str()).x/scale;
                songText(ImGui::GetFont(),25,1415+std::max(14.0f,(465-tw)*0.5f),95,ink,song.title,437);
                draw->AddRectFilled(p(1415,133),p(1880,379),panel);
                if(menuAlbumId&&menuAlbumVisible) draw->AddImage(menuAlbumId,p(1427,145),p(1538,256));
                songText(ImGui::GetFont(),23,1557,145,ink,song.artist,310);
                songText(light,17,1557,176,IM_COL32(186,197,212,255),"artist",310);
                songText(ImGui::GetFont(),21,1557,210,ink,song.charter,165);
                songText(light,16,1557,239,IM_COL32(186,197,212,255),"CHARTER:",170);
                const unsigned seconds=static_cast<unsigned>(std::max(0.0,song.durationSeconds));
                char length[32];std::snprintf(length,sizeof(length),"%02u:%02u:%02u",seconds/3600,(seconds/60)%60,seconds%60);
                songText(ImGui::GetFont(),20,1740,210,ink,length,130);
                songText(light,16,1740,239,IM_COL32(186,197,212,255),"track length",130);
                const auto songTrackIcon = [&](size_t track) -> ImTextureID {
                    switch (track) {
                    case 0: case 2: return downloaderInstrumentIds[0];
                    case 1: return chartTrackExtraIds[0];
                    case 3: return downloaderInstrumentIds[1];
                    case 4: return downloaderInstrumentIds[2];
                    case 5: return downloaderInstrumentIds[3];
                    case 6: case 8: return chartTrackExtraIds[1];
                    case 7: return chartTrackExtraIds[2];
                    case 9: return chartTrackExtraIds[3];
                    default: return ImTextureID{};
                    }
                };

                size_t trackCount=0;
                for(size_t track=0;track<10;++track)
                    if((song.chartedInstrumentMask&(1u<<track))&&songTrackIcon(track)) ++trackCount;
                const float step=std::min(55.0f,437.0f/static_cast<float>(std::max<size_t>(1,trackCount)));
                size_t shown=0;
                for(size_t track=0;track<10;++track) {
                    if(!(song.chartedInstrumentMask&(1u<<track))) continue;
                    const ImTextureID icon=songTrackIcon(track);if(!icon)continue;
                    const float iconGroupWidth=44+(trackCount>0?static_cast<float>(trackCount-1)*step:0);
                    const float x=1415+(465-iconGroupWidth)*0.5f+shown++*step;
                    // PC backgroundOval: 53x100, scaled 0.91 horizontally.
                    // Both the rating and instrument live over this one oval.
                    // The extracted sprite is gray with alpha. Apply that gray as a UI
                    // tint to its alpha mask, avoiding sRGB decoding it twice.
                    draw->AddImage(menuDetailIds[4],p(x-2,273),p(x+46,373),ImVec2(0,0),ImVec2(1,1),IM_COL32(81,81,81,255));
                    draw->AddImage(menuDetailIds[0],p(x+4,281),p(x+40,317),ImVec2(0,0),ImVec2(1,1),IM_COL32(36,36,36,255));
                    const int rating=song.instrumentRatings[track];
                    // DifficultyDisplay: ((min(rating,18)-1)%6+1)/6,
                    // top-origin clockwise radial fill, hidden for nonpositive ratings.
                    if(rating>0) {
                        const int capped=std::min(rating,18);
                        const float amount=static_cast<float>((capped-1)%6+1)/6.0f;
                        const int segments=std::max(1,static_cast<int>(std::ceil(amount*48)));
                        const ImVec2 center=p(x+22,299);
                        const ImU32 fillColor=IM_COL32(210,82,12,255);
                        for(int segment=0;segment<segments;++segment) {
                            const float a=-1.57079633f+6.28318531f*amount*segment/segments;
                            const float b=-1.57079633f+6.28318531f*amount*(segment+1)/segments;
                            const ImVec2 ua(0.5f+0.5f*std::cos(a),0.5f+0.5f*std::sin(a));
                            const ImVec2 ub(0.5f+0.5f*std::cos(b),0.5f+0.5f*std::sin(b));
                            draw->AddImageQuad(menuDetailIds[6],center,
                                p(x+4+36*ua.x,281+36*ua.y),p(x+4+36*ub.x,281+36*ub.y),center,
                                ImVec2(0.5f,0.5f),ua,ub,ImVec2(0.5f,0.5f),fillColor);
                        }
                    }
                    draw->AddImage(menuDetailIds[1],p(x+4,281),p(x+40,317));
                    char value[12];std::snprintf(value,sizeof(value),"%d",rating);
                    const char* ratingText=rating>=0?value:"-";
                    ImFont* numberFace=ratingFont?ratingFont:ImGui::GetFont();
                    constexpr float numberSize=21.0f;
                    const float rw=numberFace->CalcTextSizeA(numberSize*scale,100000,0,ratingText).x/scale;
                    float glyphTop=1000,glyphBottom=-1000;
                    for(const char* c=ratingText;*c;++c) if(const auto* glyph=numberFace->FindGlyph(*c)) {
                        glyphTop=std::min(glyphTop,glyph->Y0);glyphBottom=std::max(glyphBottom,glyph->Y1);
                    }
                    const float glyphScale=numberSize/numberFace->FontSize;
                    const float nx=x+(44-rw)*0.5f;
                    const float ny=299-(glyphTop+glyphBottom)*glyphScale*0.5f;
                    draw->AddText(numberFace,numberSize*scale,p(nx+1,ny+1),IM_COL32(0,0,0,220),ratingText);
                    draw->AddText(numberFace,numberSize*scale,p(nx,ny),ink,ratingText);
                    draw->AddImage(icon,p(x+1,327),p(x+43,369));
                }
                draw->AddRectFilled(p(1415,385),p(1880,775),panel);
                draw->AddRectFilled(p(1415,781),p(1880,818),header);
                const float statsWidth=light->CalcTextSizeA(29*scale,100000,0,"Local Best").x/scale;
                songText(light,29,1415+(465-statsWidth)*0.5f,784,ink,"Local Best",440);
                draw->AddRectFilled(p(1415,824),p(1880,962),panel);
                if(const auto* best=songBest(song)) {
                    const ImTextureID icon=best->instrument==1?downloaderInstrumentIds[1]:best->instrument==2?rhythmInstrumentId:
                        best->instrument==3?playerControllerIcon(best->details?std::clamp(best->controller,2,4):2):playerControllerIcon(best->instrument>=4?1:0);
                    if(icon)draw->AddImage(icon,p(1433,842),p(1485,894));
                    char points[32];std::snprintf(points,sizeof(points),"%llu",static_cast<unsigned long long>(best->score));
                    songText(ImGui::GetFont(),25,1500,850,ink,best->details?best->player:"Unknown player",175);
                    songText(ImGui::GetFont(),25,1687,850,ink,points,180);
                    if(best->details)for(unsigned star=0;star<best->stars;++star)
                        draw->AddImage(resultsUiIds[0],p(1500+star*25,894),p(1522+star*25,919),ImVec2(0,0),ImVec2(1,1),IM_COL32(255,218,100,255));
                    char percent[32];if(best->accuracyKnown)std::snprintf(percent,sizeof(percent),"%.2f%%",best->accuracyHundredths*.01);else std::snprintf(percent,sizeof(percent),"--");
                    songText(ImGui::GetFont(),23,1715,894,ink,percent,140);
                    static constexpr const char* difficulties[]{"Easy","Medium","Hard","Expert"};
                    songText(light,19,1500,930,IM_COL32(186,197,212,255),difficulties[best->difficulty],350);
                } else {
                    songText(light,29,1470,870,ink,"NO SAVED SCORE",360);
                }
            }
            if(!songSearchQuery.empty())
                songText(ImGui::GetFont(),18,82,945,IM_COL32(224,234,247,255),std::string(songSearchFieldName(songSearchField))+": "+songSearchQuery,1150);
            if(!songSearchImeError.empty()) songText(ImGui::GetFont(),22,82,911,IM_COL32(255,186,145,255),songSearchImeError,1200);
            projectSongMenuPlanes(songMenuVertexStart);
            drawPlayerCards();
        } else if (screen == Screen::SongSearch) {
            ImFont* face=settingsFont?settingsFont:ImGui::GetFont();
            const auto text=[&](float x,float y,float size,ImU32 ink,const char* value) {
                draw->AddText(face,size*scale,p(x,y),ink,value);
            };
            draw->AddRectFilled(p(590,200),p(1330,267),IM_COL32(8,8,8,255));
            text(625,212,40,IM_COL32(245,245,245,255),"SEARCH OPTIONS");
            draw->AddRectFilled(p(590,275),p(1330,730),IM_COL32(22,22,22,217));
            for(int row=0;row<8;++row) {
                const float y=287+row*53.0f;
                const bool selected=row==songSearchKey;
                const ImU32 ink=selected?IM_COL32(16,16,16,255):IM_COL32(240,240,240,255);
                if(selected) draw->AddRectFilled(p(600,y),p(1320,y+49),IM_COL32(208,212,214,255));
                text(625,y+6,32,ink,row<6?songSearchFieldName(static_cast<SongSearchField>(row)):row==6?"Clear Search":"Back");
                if(clickable(7300+row,600,y,720,49)) {songSearchKey=row;acceptSongSearchOption();}
            }
            draw->AddImage(songSelectControlIds[0],p(610,750),p(654,794));text(668,755,29,IM_COL32(245,245,245,255),"Choose / Search");
            draw->AddImage(songSelectControlIds[1],p(1030,750),p(1074,794));text(1088,755,29,IM_COL32(245,245,245,255),"Back");
            if(!songSearchImeError.empty()) text(590,830,24,IM_COL32(255,186,145,255),songSearchImeError.c_str());
            drawPlayerCards();
        } else if (screen == Screen::PlayerSelect) {
            using rhythm::play_menu::Page;
            ImFont* face=settingsFont?settingsFont:ImGui::GetFont();
            ImFont* strong=ratingFont?ratingFont:face;
            const ImU32 panel=IM_COL32(0,0,0,70),banner=IM_COL32(27,27,27,255),white=IM_COL32(245,245,245,255),muted=IM_COL32(166,166,166,255);
            const auto text=[&](float x,float y,float size,float width,const char* value,ImU32 color,bool bold=false,bool center=false,const char* end=nullptr) {
                ImFont* font=bold?strong:face;
                const float measured=font->CalcTextSizeA(size,100000,0,value,end).x;
                const float actual=size*std::min(1.0f,width/std::max(1.0f,measured));
                const float w=font->CalcTextSizeA(actual,100000,0,value,end).x;
                draw->AddText(font,actual*scale,p(x+(center?(width-w)*.5f:0),y),color,value,end);
            };
            const auto difficultyBadge=[&](float bx,float by,int instrumentIndex) {
                const ImTextureID icon=playerInstrumentIcon(instrumentIndex);
                // DifficultyDisplay prefab: horizontal 100x48.23 oval,
                // 42px instrument at -25, rating ring at +25 (level1).
                draw->AddImageQuad(menuDetailIds[4],p(bx,by+1),p(bx+100,by+1),p(bx+100,by+49),p(bx,by+49),
                    ImVec2(1,0),ImVec2(1,1),ImVec2(0,1),ImVec2(0,0),IM_COL32(81,81,81,255));
                if(icon) draw->AddImage(icon,p(bx+4,by+4),p(bx+46,by+46));
                draw->AddImage(menuDetailIds[0],p(bx+54,by+4),p(bx+96,by+46),ImVec2(0,0),ImVec2(1,1),IM_COL32(36,36,36,255));
                constexpr int ratingTracks[]{0,3,2,4,6};
                const int level=localSongs[static_cast<size_t>(songSelection)].instrumentRatings[ratingTracks[instrumentIndex]];
                if(level>0) {
                    const float amount=float((std::min(level,18)-1)%6+1)/6.0f;
                    const int segments=std::max(1,int(std::ceil(amount*48)));
                    const ImVec2 center=p(bx+75,by+25);
                    for(int segment=0;segment<segments;++segment) {
                        const float a=-1.57079633f+6.28318531f*amount*segment/segments;
                        const float b=-1.57079633f+6.28318531f*amount*(segment+1)/segments;
                        const ImVec2 ua(.5f+.5f*std::cos(a),.5f+.5f*std::sin(a)),ub(.5f+.5f*std::cos(b),.5f+.5f*std::sin(b));
                        draw->AddImageQuad(menuDetailIds[6],center,p(bx+54+42*ua.x,by+4+42*ua.y),p(bx+54+42*ub.x,by+4+42*ub.y),center,
                            ImVec2(.5f,.5f),ua,ub,ImVec2(.5f,.5f),IM_COL32(210,82,12,255));
                    }
                }
                draw->AddImage(menuDetailIds[1],p(bx+54,by+4),p(bx+96,by+46));
                char rating[12];std::snprintf(rating,sizeof(rating),"%d",level);
                text(bx+54,by+13,23,42,level>=0?rating:"-",white,true,true);
            };
            if(!localSongs.empty()) {
                const auto& song=localSongs[static_cast<size_t>(songSelection)];
                ImFont* titleFace=songHeadingFont?songHeadingFont:strong;
                const float width=titleFace->CalcTextSizeA(50,100000,0,song.title.c_str()).x;
                const float size=50*std::min(1.0f,1500/std::max(1.0f,width));
                const float actual=titleFace->CalcTextSizeA(size,100000,0,song.title.c_str()).x;
                draw->AddText(titleFace,size*scale,p(960-actual*.5f,10),white,song.title.c_str());
                const std::string& phrase=song.loadingPhrase.empty()?song.artist:song.loadingPhrase;
                const char* first=phrase.c_str();const char* end=first+phrase.size();
                for(int line=0;line<3 && first<end;++line) {
                    const char* stop=face->CalcWordWrapPositionA(26.0f/face->FontSize,first,end,1500);
                    if(stop<=first) break;
                    text(210,76+line*30.0f,26,1500,first,white,false,true,stop);
                    first=stop;
                    while(first<end && (*first==' ' || *first=='\n' || *first=='\r')) ++first;
                }
            }
            // Keep the four controller/profile slots fixed at the footer. Only
            // assigned, connected player cards participate in the centered group.
            const uint32_t connected=connectedPlayerMask();
            int activeCount=0;
            for(int player=0;player<PAD_PLAYERS;++player)
                if(playerJoined[player] && (connected&(1u<<player))) ++activeCount;
            int ordinal=0;
            for(int player=0;player<PAD_PLAYERS;++player) {
                const bool joined=playerJoined[player] && (connected&(1u<<player));
                const auto* profile=profileMenu.profile(player);
                if(!joined) continue;
                const float x=rhythm::play_menu::cardLeft(ordinal++,activeCount);
                draw->AddRectFilled(p(x,254),p(x+450,326),banner);
                ImFont* nameFace=playerNameFont?playerNameFont:face;
                const char* name=profile?profile->name.c_str():"Guest";
                const float measured=nameFace->CalcTextSizeA(50,100000,0,name).x;
                const float size=50*std::min(1.0f,426/std::max(1.0f,measured));
                const float width=nameFace->CalcTextSizeA(size,100000,0,name).x;
                draw->AddText(nameFace,size*scale,p(x+(450-width)*.5f,263),profileMenu.slots[player].profile==-1?guestNameInk(player):white,name);
                const Page page=playerPage[player];
                const int row=playerMenuRow[player];
                const auto menuRow=[&](int index,float y,float height,const char* value,bool enabled=true,int instrumentIndex=-1) {
                    const bool selected=index==row;
                    if(selected) draw->AddRectFilled(p(x+6,y),p(x+444,y+height),IM_COL32(208,212,214,255));
                    const ImU32 ink=enabled?(selected?IM_COL32(16,16,16,255):white):(selected?IM_COL32(110,110,110,255):IM_COL32(170,170,170,255));
                    if(instrumentIndex>=0) {
                        difficultyBadge(x+15,y+(height-50)*.5f,instrumentIndex);
                        text(x+110,y+(height-34)*.5f,34,325,value,ink,selected,true);
                    } else text(x+20,y+(height-34)*.5f,34,410,value,ink);
                    if(!profileMenu.open(player) && clickable(750+player*16+index,x+6,y,438,height)) {
                        playerMenuRow[player]=index;acceptPlayerSelection(player);tryStartSelectedSong();
                    }
                };
                if(page==Page::Confirm || page==Page::Ready || page==Page::Modifiers) {
                    draw->AddRectFilled(p(x,326),p(x+450,page==Page::Modifiers?548.0f:592.0f),panel);
                    if(page!=Page::Modifiers) {
                        difficultyBadge(x+75,330,playerInstrument[player]);
                        text(x+185,338,35,240,rhythm::play_menu::difficulties[playerDifficulty[player]],white,false,true);
                    }
                    const float headerY=page==Page::Modifiers?335.0f:385.0f;
                    draw->AddRectFilled(p(x,headerY),p(x+450,headerY+40),banner);
                    text(x+12,headerY+3,29,426,page==Page::Modifiers?"CHOOSE MODIFIERS":"MODIFIERS",white,false,true);
                    int count=0;
                    for(int m=0;m<8;++m) if(playerModifierMask[player]&(1u<<m)) {
                        text(x+20,431.0f+count*19.0f,19,410,rhythm::play_menu::modifiers[m],muted);++count;
                    }
                    if(!count) text(x+12,466,28,426,"None",muted,false,true);
                    if(page==Page::Ready) {
                        draw->AddRectFilled(p(x,599),p(x+450,839),panel);
                        text(x+12,692,55,426,"Ready",white,false,true);
                        if(gameplayReadyPending) {
                            // The codecs expose completion, not byte progress.
                            // Match the preview bar without inventing a percent.
                            const float travel=0.5f+0.5f*std::sin(float(ImGui::GetTime())*2.5f);
                            draw->AddRectFilled(p(x+40,790),p(x+410,796),IM_COL32(90,108,132,255));
                            const float start=x+40+travel*296;
                            draw->AddRectFilled(p(start,790),p(start+74,796),white);
                        }
                    } else if(page==Page::Confirm) {
                        draw->AddRectFilled(p(x,599),p(x+450,839),panel);
                        for(int i=0;i<4;++i) menuRow(i,621.0f+i*50.0f,50,rhythm::play_menu::confirm[i],i!=0 || selectionAvailable(player));
                    } else {
                        draw->AddRectFilled(p(x,555),p(x+450,915),panel);
                        for(int m=0;m<8;++m) {
                            const float y=575.0f+m*40.0f;
                            if(m==row) draw->AddRectFilled(p(x+6,y),p(x+444,y+40),IM_COL32(208,212,214,255));
                            const ImU32 ink=m==row?IM_COL32(18,18,18,255):white;
                            const bool enabled=playerInstrument[player]!=3;
                            if(playerModifierMask[player]&(1u<<m))
                                draw->AddImage(menuDetailIds[7],p(x+15,y+6),p(x+43,y+34),ImVec2(0,0),ImVec2(1,1),ink);
                            text(x+64,y+6,26,370,rhythm::play_menu::modifiers[m],enabled?ink:muted);
                            if(!profileMenu.open(player) && clickable(750+player*16+m,x+6,y,438,40)) {playerMenuRow[player]=m;acceptPlayerSelection(player);}
                        }
                    }
                } else {
                    draw->AddRectFilled(p(x,326),p(x+450,839),panel);
                    draw->AddRectFilled(p(x,335),p(x+450,375),banner);
                    text(x+12,338,30,426,page==Page::Instrument?"CHOOSE INSTRUMENT":"CHOOSE DIFFICULTY",white,false,true);
                    const int count=rhythm::play_menu::rows(page);
                    for(int i=0;i<count;++i) {
                        const bool enabled=page==Page::Instrument?instrumentAvailable(player,i):
                            (selectedSongDifficulties[playerInstrument[player]]&(1u<<(3-i)));
                        menuRow(i,400.0f+i*(page==Page::Instrument?48.0f:60.0f),page==Page::Instrument?48.0f:60.0f,page==Page::Instrument?rhythm::play_menu::instruments[i]:rhythm::play_menu::difficulties[3-i],enabled,page==Page::Instrument?i:-1);
                    }
                }
                if(!selectionAvailable(player) && page!=Page::Instrument && page!=Page::Modifiers)
                    text(x+15,859,24,420,"No supported difficulty",muted,false,true);
            }
            if(gameplayReadyPending) {
                draw->AddImage(songSelectControlIds[1],p(856,925),p(898,967));text(907,931,27,250,"Cancel loading",white);
            } else {
                draw->AddImage(songSelectControlIds[0],p(630,925),p(672,967));text(681,931,27,150,"Select",white);
                draw->AddImage(songSelectControlIds[1],p(840,925),p(882,967));text(891,931,27,140,"Back",white);
                text(1060,931,27,300,"Strum: Navigate",white);
            }
            if(!playerSelectionMessage.empty()) text(55,185,26,1810,playerSelectionMessage.c_str(),IM_COL32(255,192,133,255));
            drawPlayerCards();
        } else if (screen == Screen::Online) {
            draw->AddRectFilled(p(500,260), p(1420,810), IM_COL32(5,9,17,225));
            draw->AddText(p(570,320), IM_COL32(255,255,255,255), "ONLINE");
            draw->AddText(p(570,420), IM_COL32(220,230,242,255), "Online play requires a server connection.");
            draw->AddText(p(570,470), IM_COL32(190,205,224,255), "Network lobby support is not available in this build.");
            draw->AddText(p(570,735), IM_COL32(210,215,225,255), "Circle: Back");
        } else if (screen == Screen::Settings || screen == Screen::SettingsDetail || screen==Screen::SongCopyConfirm || screen==Screen::SongRemove) {
            constexpr float choiceLeft=1186.0f, choiceRight=1830.0f;
            const bool categoryFocus=screen==Screen::Settings;
            const bool editing=settingsEditor!=SettingsEditor::None;
            const int settingsVertexStart=draw->VtxBuffer.Size;
            ImFont* face=settingsFont?settingsFont:ImGui::GetFont();
            const auto text=[&](float x,float y,float size,ImU32 color,const char* value) {
                draw->AddText(face,size*scale,p(x,y),color,value);
            };
            // PC reference: inactive rows/help transmit the wallpaper; headers,
            // selected rows and the control surfaces remain opaque. Alpha here
            // is visually matched: serialized Unity tint values are not recovered.
            const ImU32 panel=IM_COL32(22,22,22,153);
            const ImU32 solidPanel=IM_COL32(23,23,23,255);
            const ImU32 selectionPanel=IM_COL32(214,218,220,255);
            const ImU32 bannerPanel=IM_COL32(8,8,8,255);
            draw->AddRectFilled(p(30,135),p(607,210),bannerPanel);
            static const char* categories[] = { "GENERAL", "AUDIO", "VIDEO", "GAMEPLAY", "CONTROLLER", "BACK" };
            text(64,151,43,IM_COL32(250,250,250,255),"SETTINGS");
            for (int i=0; i<6; ++i) {
                const float y=220.0f+i*64.0f;
                const bool selected=categoryFocus && i==settingsCategorySelection;
                const ImU32 fill=selected?selectionPanel:panel;
                const ImU32 ink=selected?IM_COL32(18,24,33,255):IM_COL32(236,239,244,255);
                draw->AddRectFilled(p(30,y), p(607,y+58), fill);
                text(64,y+7,40,ink,categories[i]);
                if (!editing && clickable(60+i,30,y,577,58)) {
                    settingsCategorySelection=i;
                    if (i==5) screen=Screen::MainMenu;
                    else { settingsCategory=i; settingsSelection=0; screen=Screen::SettingsDetail; }
                }
            }
            if(!categoryFocus && settingsCategorySelection<5) {
                const float y=220.0f+settingsCategorySelection*64.0f;
                draw->AddRectFilled(p(30,y),p(37,y+58),IM_COL32(235,235,235,255));
            }
            draw->AddRectFilled(p(30,610),p(607,852),panel);
            draw->AddImage(menuDetailIds[3],p(64,637),p(96,675));
            const char* help="Choose a setting to see its description.";
            if (settingsCategorySelection==0) {
                static constexpr const char* generalHelp[]={
                    "Changes language. On PS5, the game follows the system language.",
                    "Scans all connected song libraries in the background.",
                    "Copies songs from USB to console storage after confirmation. Existing charts are skipped.",
                    "Copies console songs to USB after confirmation. Existing charts are skipped.",
                    "Select installed songs and confirm their removal together. Saved scores and profiles are kept.",
                    "Changes the initial song sorting method.",
                    "Makes search default to the current sorting field instead of song name.",
                    "Scans songs with duplicate charts. Scan Songs again after changing this option.",
                    "Changes the size of the native menu interface.",
                    "Restores the native settings to their defaults.",
                    "Checks the PS5 release channel. Confirm an update to download, verify and restart without replacing installation files.",
                    "Checks once after the main menu opening sound. Updates always require confirmation and never interrupt a song."
                };
                help=generalHelp[std::clamp(settingsSelection,0,11)];
            } else if (settingsCategorySelection==1) {
                if (settingsSelection==0) help="Sets the overall output level.";
                else if (settingsSelection==1) help="Sets the song level after a missed note.";
                else if (settingsSelection==2) help="Sets the level of game sounds and menu effects.";
                else if (settingsSelection==3) help="Sets the backing song level during gameplay.";
                else if (settingsSelection==4) help="Sets the main menu music level.";
                else if (settingsSelection==5) help="Sets song preview playback on the song selection screen.";
                else help="Sets the level of this instrument stem during gameplay.";
            } else if (settingsCategorySelection==2) {
                if (settingsSelection==0) help="Changes how quickly notes animate.";
                else if (settingsSelection==1) help="Changes how quickly Star Power notes animate.";
                else if (settingsSelection==2) help="Shows the current frame rate in game.";
                else if (settingsSelection==3) help="Shows the current song and artist during gameplay.";
                else if (settingsSelection==4) help="Uses the PC Star Power gem artwork.";
                else if (settingsSelection==5) help="Enables the highway movement feedback when notes are missed.";
                else if (settingsSelection==6) help="Cycles the PC menu background artwork.";
                else if (settingsSelection==7) help="Sets the menu background to static, animated, or black.";
                else if(settingsSelection==9) help="Shows timed lyrics when the chart includes vocal phrases.";
                else help=displayTiming.supports120()?"Auto uses 120 FPS on this output. 60 FPS presents every second vblank.":"This output supports 60 FPS. 120 FPS is unavailable. Capabilities are checked on each launch.";
            } else if (settingsCategorySelection==3) {
                if (settingsSelection==0) help="Changes the speed at which chart notes travel down the highway.";
                else if (settingsSelection==1) help="Changes the size of the note gems.";
                else if (settingsSelection==2) help="Sets the native hit timing window.";
                else if (settingsSelection==3) help="Displays a visual representation of the timing window.";
                else if (settingsSelection==4) help="Mutes the song track when a note is missed.";
                else if (settingsSelection==5) help="Enables gamepad play mode.";
                else if (settingsSelection==6) help="Displays a count-in before the song begins.";
                else if (settingsSelection==7) help="Sets the pause length before a practice section restarts.";
                else if (settingsSelection==8) help="Enables Star Power phrase notes and activation during practice.";
                else if(settingsSelection==9) help="Disables song failure.";
                else help="Guitar only: ignores repeated strums in the same direction within this interval. Alternating strums bypass it. 0 ms disables; PC default is 5 ms. Menu filtering is separate.";
            } else if (settingsCategorySelection==4) {
                if (settingsSelection==0) help="Chooses whether strums use vertical or horizontal D-pad directions.";
                else if (settingsSelection==1) help="Enables or disables controller vibration.";
                else if (settingsSelection==2) help="Sets controller vibration strength.";
                else help="Assigns the guitar frets and Star Power to DualSense buttons.";
            }
            else if (settingsCategorySelection==5) help="Return to the main menu.";
            if(categoryFocus && settingsCategorySelection<5)
                help=drumMenuNavigation()?"Use the D-pad to browse categories. Press Green to enter the settings on the right.":"Strum to browse categories. Press Green to enter the settings on the right.";
            else if(settingsEditor==SettingsEditor::Slider)
                help=drumMenuNavigation()?"Use the D-pad to adjust. Yellow decreases and Blue increases. Green saves; Red cancels.":"Strum to adjust. Yellow decreases and Blue increases. Green saves; Red cancels.";
            else if(settingsEditor==SettingsEditor::Choices)
                help=drumMenuNavigation()?"Use the D-pad to browse choices. Green applies your selection; Red cancels without changes.":"Strum to browse choices. Green applies your selection; Red cancels without changes.";
            draw->AddText(ImGui::GetFont(),22.0f*scale,p(64,697),IM_COL32(218,225,236,255),help,nullptr,505.0f*scale);
            draw->AddRectFilled(p(638,135),p(1880,210),bannerPanel);
            // PC header uses the extracted green/red navigation glyphs.
            const auto settingHint=[&](int fret,float x,const char* action) {
                constexpr float iconSize=42.0f;
                constexpr float textSize=22.0f;
                draw->AddImage(songSelectControlIds[static_cast<size_t>(fret)],
                    p(x,151),p(x+iconSize,151+iconSize));
                draw->AddText(face,textSize*scale,p(x+54,161),IM_COL32(245,245,245,255),action);
            };
            text(660,161,22,IM_COL32(245,245,245,255),
                drumMenuNavigation()?(settingsEditor==SettingsEditor::Slider?"D-PAD: Adjust":"D-PAD: Browse"):(settingsEditor==SettingsEditor::Slider?"STRUM: Adjust":"STRUM: Browse"));
            const char* greenAction=categoryFocus ? "Enter" :
                (editing ? "Save" : (settingsChoiceCount()>0 ? "Choose" :
                (settingsIsSlider() ? "Adjust" : "Select")));
            settingHint(0,905,greenAction);
            settingHint(1,1135,editing ? "Cancel" : (categoryFocus ? "Back" : "Categories"));
            if(settingsEditor==SettingsEditor::Slider) {
                settingHint(2,1430,"Decrease");
                settingHint(3,1650,"Increase");
            }
            const char* rows[14] = {};
            char values[14][64] = {};
            switch (settingsCategory) {
            case 0:
                rows[0]="Language";rows[1]="Scan Songs";
                rows[2]="COPY USB SONGS TO CONSOLE";rows[3]="COPY CONSOLE SONGS TO USB";
                rows[4]="REMOVE SONGS";rows[5]="Default Sort Filter";rows[6]="Default Search By Current Sort";
                rows[7]="Allow Duplicate Songs";rows[8]="Menu Scale";rows[9]="Restore Defaults";
                rows[10]="Check for Updates";rows[11]="Automatic Update Checks";
                std::snprintf(values[11],sizeof(values[11]),"%s",autoUpdateChecks?"On":"Off");
                snprintf(values[0],sizeof(values[0]),"System");
                snprintf(values[1],sizeof(values[1]),"%zu songs",localSongs.size());
                snprintf(values[5],sizeof(values[5]),"%s",songSortFieldName(songSortField));
                snprintf(values[6],sizeof(values[6]),"%s",defaultSearchByCurrentSort?"On":"Off");
                snprintf(values[7],sizeof(values[7]),"%s",allowDuplicateSongs?"On":"Off");
                snprintf(values[8],sizeof(values[8]),"%.0f%%",menuScale*100.0f);
                break;
            case 1:
                rows[0]="Master Volume"; rows[1]="Mute Volume"; rows[2]="Sound Effects Volume";
                rows[3]="Song Volume"; rows[4]="Menu Volume"; rows[5]="Preview Volume";
                rows[6]="Guitar Volume"; rows[7]="Bass Volume"; rows[8]="Rhythm Volume";
                rows[9]="Vocal Volume"; rows[10]="Drum Volume"; rows[11]="Keys Volume";
                rows[12]="Crowd Volume";rows[13]="Miss Sound Volume";
                snprintf(values[13],sizeof(values[13]),"%.0f%%",missSoundVolume*100.0f);
                snprintf(values[0],sizeof(values[0]),"%.0f%%",masterVolume*100.0f);
                snprintf(values[1],sizeof(values[1]),"%.0f%%",missVolume*100.0f);
                snprintf(values[2],sizeof(values[2]),"%.0f%%",effectsVolume*100.0f);
                snprintf(values[3],sizeof(values[3]),"%.0f%%",musicVolume*100.0f);
                snprintf(values[4],sizeof(values[4]),"%.0f%%",menuVolume*100.0f);
                snprintf(values[5],sizeof(values[5]),"%.0f%%",previewVolume*100.0f);
                for (size_t i=0;i<7;++i) snprintf(values[i+6],sizeof(values[i+6]),"%.0f%%",instrumentVolumes[i]*100.0f);
                break;
            case 2:
                rows[0]="Note Animation FPS"; rows[1]="Star Animation FPS"; rows[2]="Show FPS";
                rows[3]="Song Display"; rows[4]="Star Power Note Colors"; rows[5]="Highway Shake";
                rows[6]="Menu Background"; rows[7]="Background Animation"; rows[8]="Frame Rate Limit"; rows[9]="Lyrics";
                snprintf(values[0],sizeof(values[0]),"%d FPS",noteAnimationFps);
                snprintf(values[1],sizeof(values[1]),"%d FPS",starAnimationFps);
                snprintf(values[2],sizeof(values[2]),"%s",showFps?"On":"Off");
                snprintf(values[3],sizeof(values[3]),"%s",showSongDisplay?"On":"Off");
                snprintf(values[4],sizeof(values[4]),"%s",noteStarPowerColors?"On":"Off");
                snprintf(values[5],sizeof(values[5]),"%s",highwayShakeEnabled?"On":"Off");
                static constexpr const char* backgroundNames[] = {
                    "Surfer", "Rainbow", "Surfer Alt", "Classic", "Dark", "Light", "Autumn", "Alien",
                    "Blue Rays", "Grains", "Groovy", "Pastel Burst", "Spray", "Black"
                };
                snprintf(values[6],sizeof(values[6]),"%s",backgroundNames[menuBackgroundIndex]);
                snprintf(values[7],sizeof(values[7]),"%s",menuBackgroundAnimation==0?"Static":(menuBackgroundAnimation==1?"UV Scroll":"Surfer Animated"));
                snprintf(values[9],sizeof(values[9]),"%s",showLyrics?"On":"Off");
                snprintf(values[8],sizeof(values[8]),"%s",frameRateLimit==0?(displayTiming.supports120()?"Auto (120 FPS)":"Auto (60 FPS)"):(displayTiming.effectiveChoice(frameRateLimit)==2?"120 FPS":"60 FPS"));
                break;
            case 3:
                rows[0]="Note Speed"; rows[1]="Gem Size"; rows[2]="Timing Window";
                rows[3]="Show Hit Window"; rows[4]="Mute On Miss"; rows[5]="Gamepad Mode";
                rows[6]="Measure Countdown";
                snprintf(values[0],sizeof(values[0]),"%.1fx",noteSpeedScale);
                snprintf(values[1],sizeof(values[1]),"%d%%",gemSizePercent);
                snprintf(values[2],sizeof(values[2]),"%.0f ms",hitWindowMs);
                snprintf(values[3],sizeof(values[3]),"%s",showHitWindow?"On":"Off");
                snprintf(values[4],sizeof(values[4]),"%s",muteOnMiss?"On":"Off");
                snprintf(values[5],sizeof(values[5]),"%s",gamepadMode?"On":"Off");
                snprintf(values[6],sizeof(values[6]),"%s",measureCountdown?"On":"Off");
                rows[7]="Practice Start Delay";
                snprintf(values[7],sizeof(values[7]),"%d ms",practiceStartDelayMs);
            rows[8]="Practice Star Power";
            snprintf(values[8],sizeof(values[8]),"%s",practiceStarPowerEnabled?"On":"Off");
            rows[9]="No Fail";
            snprintf(values[9],sizeof(values[9]),"%s",noFail?"On":"Off");
            rows[10]="Double Strum Prevention";
            if(strumDebounceMs==0)snprintf(values[10],sizeof(values[10]),"Off (0 ms)");
            else snprintf(values[10],sizeof(values[10]),"%.0f ms",strumDebounceMs);
                break;
            case 4:
                rows[0]="Guitar Strum"; rows[1]="Controller Vibration"; rows[2]="Vibration Strength";
                snprintf(values[0],sizeof(values[0]),"%s",horizontalStrum?"D-pad Left / Right":"D-pad Up / Down");
                snprintf(values[1],sizeof(values[1]),"%s",controllerVibration?"On":"Off");
                snprintf(values[2],sizeof(values[2]),"%.0f%%",rumbleStrength*100.0f);
                break;
            }
            if(settingsEditor==SettingsEditor::Slider) {
                char* value=values[settingsSelection];
                if(settingsCategory==0 || settingsCategory==1 || settingsCategory==4)
                    snprintf(value,64,"%.0f%%",settingsSliderValue*100);
                else if(settingsCategory==2) snprintf(value,64,"%.0f FPS",settingsSliderValue);
                else if(settingsSelection==0) snprintf(value,64,"%.1fx",settingsSliderValue);
                else snprintf(value,64,settingsSelection==1?"%.0f%%":"%.0f ms",settingsSliderValue);
            }
            const int rowCount=settingsCategorySelection==5?0:settingsDetailRowCount();
            const int visibleRows = std::min(rowCount, 8);
            const int rowStart = std::clamp(settingsSelection - visibleRows + 1, 0, std::max(0, rowCount - visibleRows));
            for (int visible=0;visible<visibleRows;++visible) {
                const int i=rowStart+visible;
                const float y=220.0f+visible*74.0f;
                const bool selected=!categoryFocus && i==settingsSelection;
                const bool sectionButton=settingsCategory==4&&i==3;
                const ImU32 fill=selected?selectionPanel:(sectionButton?solidPanel:panel);
                const ImU32 ink=selected?IM_COL32(22,22,22,255):IM_COL32(210,213,216,255);
                draw->AddRectFilled(p(638,y),p(1860,y+67),fill);
                std::string caption=rows[i];
                for(char& c:caption) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                const float labelSize=caption.size()>30?25.0f:32.0f;
                const float labelWidth=face->CalcTextSizeA(labelSize*scale,100000,0,caption.c_str()).x/scale;
                text(1158-labelWidth,y+16,labelSize,ink,caption.c_str());
                const bool toggle=std::strcmp(values[i],"On")==0||std::strcmp(values[i],"Off")==0;
                const bool action=(settingsCategory==0&&(i==1||i==2||i==3||i==4||i==9||i==10))||(settingsCategory==4&&i==3);
                const bool slider=settingsCategory==1||(settingsCategory==0&&i==8)||
                    (settingsCategory==2&&i<2)||(settingsCategory==3&&(i<3||i==7||i==10))||(settingsCategory==4&&i==2);
                if(toggle) {
                    draw->AddRectFilled(p(1186,y+15),p(1262,y+53),IM_COL32(24,24,24,255));
                    const bool enabled=std::strcmp(values[i],"On")==0;
                    const float knob=enabled?1224.0f:1186.0f;
                    draw->AddRectFilled(p(knob,y+15),p(knob+38,y+53),IM_COL32(255,255,255,255));
                } else if(slider) {
                    // Use the same range for display and input, including pending edits.
                    const int savedSelection=settingsSelection;
                    settingsSelection=i;
                    float low,high,step;
                    settingsSliderRange(low,high,step);
                    const float value=(editing && i==savedSelection) ? settingsSliderValue : currentSettingsSlider();
                    settingsSelection=savedSelection;
                    const float amount=(value-low)/(high-low);
                    const float knob=1186+std::clamp(amount,0.0f,1.0f)*475;
                    draw->AddLine(p(1186,y+34),p(1715,y+34),IM_COL32(24,24,24,255),6*scale);
                    draw->AddRectFilled(p(knob,y+20),p(knob+28,y+48),IM_COL32(255,255,255,255));
                    text(1750,y+15,30,ink,values[i]);
                } else if(!action) {
                    draw->AddRectFilled(p(choiceLeft,y+9),p(choiceRight,y+59),IM_COL32(30,30,30,255));
                    text(1205,y+15,30,IM_COL32(246,246,246,255),values[i]);
                    if(!(settingsCategory==0 && i==0))
                        draw->AddImage(menuDetailIds[2],p(1788,y+23),p(1812,y+47));
                }
                if (!editing && clickable(70+visible,638,y,1222,67)) {
                    screen=Screen::SettingsDetail;
                    settingsSelection=i;
                    activateSettingsValue();
                }
            }
            if (rowCount>visibleRows) {
                draw->AddRectFilled(p(1870,220),p(1882,220+visibleRows*74.0f),IM_COL32(47,55,68,255));
                const float thumbH=visibleRows*74.0f*static_cast<float>(visibleRows)/static_cast<float>(rowCount);
                const float maxOffset=static_cast<float>(rowCount-visibleRows);
                const float thumbY=220.0f+(visibleRows*74.0f-thumbH)*(maxOffset>0?static_cast<float>(rowStart)/maxOffset:0.0f);
                draw->AddRectFilled(p(1870,thumbY),p(1882,thumbY+thumbH),IM_COL32(193,202,215,255));
            }
            if(settingsEditor==SettingsEditor::Choices) {
                popupLayer(true);
                const int count=settingsChoiceCount();
                const int shown=std::min(count,6);
                const int first=std::clamp(settingsChoice-shown+1,0,count-shown);
                const float height=shown*54.0f;
                const float rowY=220.0f+(settingsSelection-rowStart)*74.0f;
                // Open above lower rows so the selected control stays visible.
                const float below=rowY+67.0f;
                const float top=below+height<=852.0f ? below : std::max(220.0f,rowY-height);
                for(int n=0;n<shown;++n) {
                    const int choice=first+n;
                    const float y=top+n*54.0f;
                    const bool enabled=settingsChoiceEnabled(choice);
                    const bool selected=enabled && choice==settingsChoice;
                    draw->AddRectFilled(p(choiceLeft,y),p(choiceRight,y+54),selected?selectionPanel:solidPanel);
                    text(1205,y+9,30,!enabled?IM_COL32(115,115,115,255):selected?IM_COL32(20,20,20,255):IM_COL32(240,240,240,255),settingsChoiceName(choice));
                    if(choice==currentSettingsChoice()) text(1800,y+12,23,selected?IM_COL32(20,20,20,255):IM_COL32(240,240,240,255),"*");
                    if(enabled && clickable(190+n,choiceLeft,y,choiceRight-choiceLeft,54)) {
                        settingsChoice=choice;
                        confirmSettingsEditor();
                    }
                }
            }
            popupLayer(false);
            // Match the PC canvas perspective: both columns angle toward the center.
            // Transform text and controls together so they remain on their panels.
            for(int vertex=settingsVertexStart;vertex<draw->VtxBuffer.Size;++vertex) {
                ImDrawVert& v=draw->VtxBuffer[vertex];
                const float x=(v.pos.x-p(0,0).x)/scale;
                const float y=(v.pos.y-p(0,0).y)/scale;
                const float edge=x<620?(x-30)/577.0f:(1880-x)/1242.0f;
                v.pos.y+=edge*36.0f*(1.0f-y/940.0f)*scale;
            }
            drawPlayerCards();
        } else if (screen == Screen::Downloader) {
            // Borrow Song Select's scene planes, fonts and panels. Encore keeps
            // its own search/install actions; no local score records are invented.
            const int browserVertexStart=draw->VtxBuffer.Size;
            const ImU32 panel=IM_COL32(20,23,28,145),header=IM_COL32(18,18,18,235);
            const ImU32 topHeader=IM_COL32(4,5,7,252);
            const ImU32 ink=IM_COL32(236,239,245,255),muted=IM_COL32(197,205,216,255);
            const ImU32 selection=IM_COL32(214,218,220,255),dark=IM_COL32(24,24,24,255);
            const auto browserText = [&](float x,float y,float size,ImU32 color,const char* text) {
                draw->AddText(ImGui::GetFont(),size*scale,p(x,y),color,text);
            };
            const auto browserLine = [&](float x,float y,float size,ImU32 color,const std::string& text,float width,ImFont* face=nullptr) {
                const ImVec2 lo=p(x,y),hi=p(x+width,y+size+4);
                const ImVec4 clip(lo.x,lo.y,hi.x,hi.y);
                draw->AddText(face?face:ImGui::GetFont(),size*scale,lo,color,text.c_str(),nullptr,0,&clip);
            };
            const auto fittedText = [&](const char* text,float maxWidth,float fontSize,ImFont* face=nullptr) {
                std::string value = text ? text : "";
                const auto textWidth = [&](const std::string& candidate) {
                    return (face?face:ImGui::GetFont())->CalcTextSizeA(fontSize*scale,4096.0f,0.0f,candidate.c_str()).x;
                };
                if (textWidth(value) <= maxWidth*scale) return value;
                while (!value.empty() && textWidth(value + "…") > maxWidth*scale) {
                    size_t last=value.size()-1;
                    while(last>0 && (static_cast<unsigned char>(value[last])&0xc0u)==0x80u)--last;
                    value.resize(last);
                }
                return value + "…";
            };
            const auto centeredText = [&](ImFont* face,const char* text,float size,float x,float y,float width,float height,ImU32 color) {
                if(!face)face=ImGui::GetFont();
                const std::string value=fittedText(text,width-8,size,face);
                const float textWidth=face->CalcTextSizeA(size*scale,100000,0,value.c_str()).x/scale;
                float top=10000,bottom=-10000;
                // ASCII menu labels use visible glyph bounds, rather than the
                // font line box, to center different weights on the same axis.
                for(unsigned char ch:value)if(ch<128)if(const ImFontGlyph* glyph=face->FindGlyph(ch)) {
                    top=std::min(top,glyph->Y0);bottom=std::max(bottom,glyph->Y1);
                }
                const float textY=top<=bottom?y+height*0.5f-(top+bottom)*0.5f*(size/face->FontSize):y+(height-size)*0.5f;
                draw->AddText(face,size*scale,p(x+(width-textWidth)*0.5f,textY),color,value.c_str());
            };
            // Original OvalNoOutline has 25-pixel top/bottom sprite borders.
            // Slice those caps instead of stretching their curves with labels.
            const auto instrumentCapsule = [&](float x,float y,float width,float height) {
                const float cap=std::min(height*0.5f,25.0f*width/49.0f);
                const ImU32 tint=IM_COL32(81,81,81,255);
                draw->AddImage(menuDetailIds[4],p(x,y),p(x+width,y+cap),ImVec2(0,0),ImVec2(1,25.0f/97.0f),tint);
                draw->AddImage(menuDetailIds[4],p(x,y+cap),p(x+width,y+height-cap),ImVec2(0,25.0f/97.0f),ImVec2(1,72.0f/97.0f),tint);
                draw->AddImage(menuDetailIds[4],p(x,y+height-cap),p(x+width,y+height),ImVec2(0,72.0f/97.0f),ImVec2(1,1),tint);
            };
            const auto stripStyleTags = [](const std::string& value) {
                std::string plain;
                plain.reserve(value.size());
                bool inTag = false;
                for (char c : value) {
                    if (c == '<') { inTag = true; continue; }
                    if (c == '>' && inTag) { inTag = false; continue; }
                    if (!inTag) plain.push_back(c);
                }
                return plain;
            };
            const auto chartTrackIcon = [&](size_t track) -> ImTextureID {
                switch (track) {
                    case 0: case 2: return downloaderInstrumentIds[0];
                    case 1: return chartTrackExtraIds[0];
                    case 3: return downloaderInstrumentIds[1];
                    case 4: return downloaderInstrumentIds[2];
                    case 5: return downloaderInstrumentIds[3];
                    case 6: case 8: return chartTrackExtraIds[1];
                    case 7: return chartTrackExtraIds[2];
                    case 9: return chartTrackExtraIds[3];
                    default: return ImTextureID{};
                }
            };

            const auto drawTrackStrip = [&](const chorus::ChartResult& item,float x,float centerY,ImU32 tint) {
                const unsigned count=std::popcount(static_cast<unsigned>(item.chartedInstrumentMask&0x3ffu))+(item.hasVocals?1u:0u);
                const float step=std::min(34.0f,230.0f/static_cast<float>(std::max(1u,count)));
                const float size=std::min(30.0f,step-4.0f),y=centerY-size*0.5f;
                size_t shown=0;
                for(size_t track=0;track<10;++track) {
                    if(!(item.chartedInstrumentMask&(1u<<track)))continue;
                    const ImTextureID icon=chartTrackIcon(track);
                    const float at=x+static_cast<float>(shown++)*step;
                    if(icon) {
                        draw->AddImage(menuDetailIds[0],p(at,y),p(at+size,y+size),ImVec2(0,0),ImVec2(1,1),IM_COL32(25,25,25,255));
                        draw->AddImage(icon,p(at,y),p(at+size,y+size));
                    }
                }
                if(item.hasVocals)browserText(x+static_cast<float>(shown)*step,centerY-6,11,tint,"VOX");
            };
            draw->AddRectFilled(p(60,55),p(1383,105),topHeader);
            const char* primary=downloaderResultsFocus&&chorusDownloader.resultCount()?"INSTALL":"SEARCH";
            const char* queueAction=downloaderFilterFocus?"CHANGE":"QUEUE";
            const char* actions[]={primary,"BACK",queueAction,"FILTERS","FOCUS","SEARCH","PROFILE"};
            constexpr float hintStart=72,hintWidth=185,iconSlot=46,iconSize=34;
            for(int control=0;control<7;++control) {
                const float x=hintStart+static_cast<float>(control)*hintWidth;
                if(control<5)draw->AddImage(songSelectControlIds[control],p(x+(iconSlot-iconSize)*0.5f,63),p(x+(iconSlot+iconSize)*0.5f,97));
                else {
                    draw->AddRectFilled(p(x,70),p(x+iconSlot,90),IM_COL32(225,228,233,255));
                    centeredText(nullptr,control==5?"SELECT":"START",9,x,70,iconSlot,20,IM_COL32(12,16,22,255));
                }
                // Every glyph occupies the same slot; every caption has the
                // same size, gap and vertical center before plane projection.
                const float captionWidth=ImGui::GetFont()->CalcTextSizeA(16*scale,100000,0,actions[control]).x/scale;
                centeredText(nullptr,actions[control],16,x+52,55,captionWidth+8,50,ink);
            }
            draw->AddRectFilled(p(1415,90),p(1880,127),topHeader);
            centeredText(nullptr,"ENCORE BROWSER",25,1415,90,465,37,ink);
            const bool searchFocused=!downloaderResultsFocus&&!downloaderFilterFocus;
            draw->AddRectFilled(p(60,112),p(1383,174),searchFocused?selection:panel);
            const ImU32 queryInk=searchFocused?dark:ink;
            browserText(82,130,23,queryInk,"SEARCH:");
            browserLine(205,125,30,queryInk,downloaderQuery.empty()?"Songs, artists, albums...":downloaderQuery,1145);
            static constexpr const char* instrumentLabels[]={"Lead Guitar","Co-op Guitar","Rhythm","Bass","Drums","Keys","6-Fret Lead","6-Fret Co-op","6-Fret Rhythm","6-Fret Bass","Any"};
            static constexpr const char* difficultyLabels[]={"Expert","Hard","Medium","Easy","Any"};
            static constexpr const char* sortLabels[]={"Title","Artist","Album","Genre","Year","Charter","Length","Date Added"};
            static constexpr const char* drumLabels[]={"4-Lane","Pro 4-Lane","5-Lane"};
            static constexpr const char* searchFieldLabels[]={"All fields","Title","Artist","Album","Charter","Genre","Year"};
            char sortValue[40];
            std::snprintf(sortValue,sizeof(sortValue),"%s %s",sortLabels[downloaderSort],downloaderSortDescending?"DESC":"ASC");
            const char* values[]={instrumentLabels[downloaderInstrument],difficultyLabels[downloaderDifficulty],sortValue,drumLabels[downloaderDrumType],downloaderReviewedDrums?"Reviewed only":"All charts",searchFieldLabels[downloaderSearchField]};
            const char* names[]={"INSTRUMENT","DIFFICULTY","SORT BY","DRUM TYPE","DRUM REVIEW","SEARCH IN"};
            for(int filter=0;filter<6;++filter) {
                const float x=60+static_cast<float>(filter%3)*443,y=182+static_cast<float>(filter/3)*68;
                const bool selected=downloaderFilterFocus&&downloaderFilterSelection==filter;
                draw->AddRectFilled(p(x,y),p(x+437,y+62),selected?selection:panel);
                browserText(x+16,y+5,16,selected?dark:muted,names[filter]);
                browserLine(x+16,y+26,27,selected?dark:ink,values[filter],405);
            }
            const size_t resultCount=chorusDownloader.resultCount();
            draw->AddRectFilled(p(60,326),p(1383,367),topHeader);
            browserText(82,336,20,ink,"SONG");
            browserText(530,336,20,ink,"ARTIST");
            browserText(870,336,20,ink,"CHARTER");
            browserText(1120,336,20,ink,"INSTRUMENTS");
            char summary[80];
            std::snprintf(summary,sizeof(summary),"%zu CHARTS   /   PAGE %u",resultCount,chorusDownloader.page());

            const chorus::ChartResult* selectedResult=resultCount&&downloaderResult>=0?chorusDownloader.result(static_cast<size_t>(downloaderResult)):nullptr;
            if (selectedResult && !selectedResult->albumArtMd5.empty()) {
                (void)chorusDownloader.requestCover(static_cast<size_t>(downloaderResult));
                const char* path = chorusDownloader.coverPath(static_cast<size_t>(downloaderResult));
                if (path && *path && downloaderCoverLoadedPath != path && access(path, F_OK) == 0) {
                    std::vector<unsigned char> pixels;
                    int width = 0, height = 0;
                    if (loadRGBA(path, pixels, width, height) && width > 0 && height > 0) {
                        VK_CHECK_RESULT(vkDeviceWaitIdle(device));
                        Tex replacement{};
                        createTexture(replacement, pixels.data(), width, height,
                            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
                        updateTextureDescriptor(downloaderCoverId, replacement);
                        destroyTex(downloaderCoverTex);
                        downloaderCoverTex = replacement;
                        downloaderCoverLoadedPath = path;
                    }
                }
            }

            draw->AddRectFilled(p(1415,133),p(1880,453),panel);
            const bool hasCover=selectedResult&&!selectedResult->albumArtMd5.empty()&&downloaderCoverLoadedPath==selectedResult->albumArtPath&&downloaderCoverId;
            if(hasCover) draw->AddImage(downloaderCoverId,p(1497,143),p(1797,443));
            else if(selectedResult) {
                draw->AddRectFilled(p(1497,143),p(1797,443),IM_COL32(12,15,20,185));
                browserText(1530,279,18,muted,"ARTWORK UNAVAILABLE");
            }
            draw->AddRectFilled(p(1415,459),p(1880,562),panel);
            draw->AddRectFilled(p(1415,568),p(1880,618),panel);
            draw->AddRectFilled(p(1415,624),p(1880,661),topHeader);
            centeredText(nullptr,"INSTRUMENTS",23,1415,624,465,37,ink);
            constexpr size_t instrumentColumns=4;
            constexpr float instrumentCellWidth=110,instrumentRowHeight=96;
            constexpr float instrumentPanelTop=667,instrumentPadding=10,instrumentBadgeHeight=92;
            const size_t instrumentCount=selectedResult?
                static_cast<size_t>(std::popcount(static_cast<unsigned>(selectedResult->chartedInstrumentMask&0x3ffu)))+(selectedResult->hasVocals?1u:0u):0;
            const size_t instrumentRows=std::max<size_t>(2,(instrumentCount+instrumentColumns-1)/instrumentColumns);
            const float instrumentPanelBottom=instrumentPanelTop+2*instrumentPadding+
                static_cast<float>(instrumentRows-1)*instrumentRowHeight+instrumentBadgeHeight;
            draw->AddRectFilled(p(1415,instrumentPanelTop),p(1880,instrumentPanelBottom),panel);
            if(selectedResult) {
                centeredText(ratingFont,selectedResult->title.c_str(),30,1415,459,465,52,IM_COL32(255,255,255,255));
                centeredText(nullptr,selectedResult->artist.c_str(),26,1415,511,465,51,muted);
                const std::string charter=stripStyleTags(selectedResult->charter);
                const std::string charterLine=fittedText(("CHARTER: "+(charter.empty()?"Unknown":charter)).c_str(),425,24);
                browserText(1435,581,24,ink,charterLine.c_str());
                static constexpr const char* tracks[]={"Guitar","Co-op","Rhythm","Bass","Drums","Keys","6F Guitar","6F Co-op","6F Rhythm","6F Bass","Vocals"};
                static constexpr const char* levels[]={"Expert","Medium","Hard","Easy"};
                const size_t count=instrumentCount;
                size_t shown=0;
                for(size_t track=0;track<11;++track) {
                    if(track==10?!selectedResult->hasVocals:!(selectedResult->chartedInstrumentMask&(1u<<track)))continue;
                    constexpr size_t columns=instrumentColumns;
                    constexpr float cellWidth=instrumentCellWidth,rowHeight=instrumentRowHeight;
                    const size_t rowCount=std::min(columns,count-(shown/columns)*columns);
                    const float x=1415+(465-static_cast<float>(rowCount)*cellWidth)*0.5f+static_cast<float>(shown%columns)*cellWidth;
                    const float y=instrumentPanelTop+instrumentPadding+static_cast<float>(shown/columns)*rowHeight;
                    instrumentCapsule(x+25,y,60,72);
                    if(track==10)centeredText(nullptr,"VOX",18,x+25,y+4,60,42,ink);
                    else if(const ImTextureID icon=chartTrackIcon(track))draw->AddImage(icon,p(x+34,y+4),p(x+76,y+46));
                    centeredText(nullptr,tracks[track],16,x,y+74,cellWidth,18,ink);
                    std::string caption;
                    if(track<10) {
                        const unsigned mask=selectedResult->instrumentDifficultyMasks[track]&15u;
                        if(std::popcount(mask)==1)caption=levels[std::countr_zero(mask)];
                        else {
                            // API bits are Expert, Medium, Hard, Easy. Present
                            // the compact labels in ascending difficulty order.
                            constexpr unsigned bits[]={8,2,4,1};
                            constexpr const char* labels[]={"E","M","H","X"};
                            for(size_t level=0;level<4;++level)if(mask&bits[level]) {
                                if(!caption.empty())caption+=" ";
                                caption+=labels[level];
                            }
                        }
                    }
                    centeredText(nullptr,caption.c_str(),14,x+25,y+47,60,18,muted);
                    ++shown;
                }
            }
            if(resultCount==0) {
                const char* hint=chorusDownloader.state()==chorus::Downloader::State::Searching?"Searching Chorus Encore...":
                    chorusDownloader.state()==chorus::Downloader::State::Error?chorusDownloader.error():
                    chorusDownloader.state()==chorus::Downloader::State::Results?"No charts matched those keywords.":"Press Select to enter a search.";
                browserLine(82,406,29,ink,hint,1255);
            } else {
                constexpr size_t visibleResults=7;
                size_t first=static_cast<size_t>(std::max(0,downloaderResult-3));
                if(resultCount>visibleResults&&first+visibleResults>resultCount)first=resultCount-visibleResults;
                for(size_t i=first;i<std::min(resultCount,first+visibleResults);++i) {
                    const chorus::ChartResult* item=chorusDownloader.result(i);if(!item)continue;
                    const float y=375+static_cast<float>(i-first)*79;
                    const bool selected=static_cast<int>(i)==downloaderResult,marked=i<downloaderMarked.size()&&downloaderMarked[i];
                    // Keep the requested white song titles readable on selection.
                    draw->AddRectFilled(p(60,y),p(1383,y+73),selected?IM_COL32(42,52,65,245):panel);
                    if(selected)draw->AddRectFilled(p(60,y),p(64,y+73),IM_COL32(245,247,250,255));
                    ImFont* titleFace=ratingFont?ratingFont:ImGui::GetFont();
                    const std::string title=fittedText(item->title.c_str(),426,29,titleFace);
                    browserLine(82,y+22,29,IM_COL32(255,255,255,255),title,426,titleFace);
                    const std::string artist=fittedText(item->artist.c_str(),318,24);
                    browserLine(530,y+24.5f,24,IM_COL32(183,190,201,255),artist,318);
                    const std::string charter=stripStyleTags(item->charter);
                    const std::string credit=fittedText(charter.empty()?"Unknown":charter.c_str(),228,20);
                    browserLine(870,y+26.5f,20,IM_COL32(218,223,231,255),credit,228);
                    drawTrackStrip(*item,1120,y+36.5f,ink);
                    if(marked)browserText(1120,y+54,12,ink,"QUEUED");
                }
                if(resultCount>visibleResults) {
                    draw->AddRectFilled(p(1388,375),p(1398,928),IM_COL32(25,27,32,150));
                    const float h=std::max(35.0f,553*static_cast<float>(visibleResults)/static_cast<float>(resultCount));
                    const float y=375+(553-h)*static_cast<float>(downloaderResult)/static_cast<float>(resultCount-1);
                    draw->AddRectFilled(p(1388,y),p(1398,y+h),selection);
                }
            }
            const char* focus=downloaderResultsFocus?"RESULTS":downloaderFilterFocus?"FILTERS":"SEARCH";
            browserText(82,934,16,muted,downloaderFilterFocus?(drumMenuNavigation()?"D-PAD: SELECT FILTER    LEFT / RIGHT: CHANGE":"STRUM: SELECT FILTER    LEFT / RIGHT: CHANGE"):"STRUM / D-PAD: NAVIGATE");
            browserText(710,934,16,ink,focus);
            browserText(1050,934,16,muted,summary);
            if(!downloaderImeError.empty())browserLine(82,955,16,IM_COL32(255,155,145,255),downloaderImeError,1270);
            else if(chorusDownloader.state()==chorus::Downloader::State::Downloading) {
                draw->AddRectFilled(p(60,955),p(1383,966),header);
                draw->AddRectFilled(p(60,955),p(60+1323*static_cast<float>(chorusDownloader.progress()),966),selection);
            } else if(chorusDownloader.state()==chorus::Downloader::State::Complete)
                browserLine(82,955,16,ink,"Installed: "+std::string(chorusDownloader.installedPath()),1270);
            else if(chorusDownloader.state()==chorus::Downloader::State::Error&&resultCount)
                browserLine(82,955,16,IM_COL32(255,155,145,255),chorusDownloader.error(),1270);
            projectSongMenuPlanes(browserVertexStart);
            drawPlayerCards();
        } else if (screen == Screen::Results) {
            // PC level3 EndOfSong / PlayerStatsBox: metadata at top, centered
            // group score/stars, and a 450px player card with 435px stats pages.
            ImFont* regular=playerNameFont?playerNameFont:ImGui::GetFont();
            ImFont* bold=resultsBoldFont?resultsBoldFont:regular;
            ImFont* light=settingsFont?settingsFont:regular;
            const ImU32 white=IM_COL32(255,255,255,255),header=IM_COL32(27,27,27,255);
            const auto text=[&](ImFont* face,const std::string& value,float x,float y,float w,float h,float size,
                                ImU32 color=IM_COL32(255,255,255,255),int align=0,float spacing=0) {
                const auto measure=[&](float px) {
                    float width=0;size_t count=0;
                    for(const char* c=value.c_str();*c;) {
                        const unsigned char lead=static_cast<unsigned char>(*c);
                        const unsigned bytes=lead<0x80?1:lead<0xe0?2:lead<0xf0?3:4;
                        const char* end=c;for(unsigned i=0;i<bytes && *end;++i)++end;
                        width+=face->CalcTextSizeA(px,100000,0,c,end).x;++count;c=end;
                    }
                    return width+(count?count-1:0)*spacing*px/100.0f;
                };
                size*=std::min(1.0f,w/std::max(1.0f,measure(size)));
                const float width=measure(size);
                float pen=x+(align==1?(w-width)*.5f:align==2?w-width:0);
                // Source TMP middle/capline alignment: center the visible glyphs.
                float top=10000,bottom=-10000;
                for(unsigned char c:value)if(c>32 && c<128)if(const auto* g=face->FindGlyph(c)) {
                    top=std::min(top,g->Y0);bottom=std::max(bottom,g->Y1);
                }
                const float base=top<=bottom?y+h*.5f-(top+bottom)*.5f*size/face->FontSize:y+(h-size)*.5f;
                for(const char* c=value.c_str();*c;) {
                    const unsigned char lead=static_cast<unsigned char>(*c);
                    const unsigned bytes=lead<0x80?1:lead<0xe0?2:lead<0xf0?3:4;
                    const char* end=c;for(unsigned i=0;i<bytes && *end;++i)++end;
                    draw->AddText(face,size*scale,p(pen,base),color,c,end);
                    pen+=face->CalcTextSizeA(size,100000,0,c,end).x+spacing*size/100.0f;c=end;
                }
            };
            // Original 40px white-box sprites have 12px sliced borders.
            const auto panel=[&](ImTextureID image,float x,float y,float w,float h,ImU32 color) {
                const float xs[]{x,x+12,x+w-12,x+w},ys[]{y,y+12,y+h-12,y+h};
                constexpr float uv[]{0,.3f,.7f,1};
                for(int row=0;row<3;++row)for(int col=0;col<3;++col)
                    draw->AddImage(image,p(xs[col],ys[row]),p(xs[col+1],ys[row+1]),
                        ImVec2(uv[col],uv[row]),ImVec2(uv[col+1],uv[row+1]),color);
            };
            const auto countText=[](uint64_t value) {
                const auto raw=std::to_string(value);std::string out;
                for(size_t i=0;i<raw.size();++i){out+=raw[i];if((raw.size()-i-1)%3==0 && i+1<raw.size())out+=',';}
                return out;
            };
            const LocalSong* completed=gameplaySongInfoValid?&gameplaySongInfo:nullptr;
            if(completed) {
                draw->AddImage(resultsUiIds[7],p(50,70.7f),p(114,134.7f));
                text(bold,completed->title,137,41.4f,500,50,39,white,0,-3);
                text(bold,completed->artist,137,85.4f,500,40,30,white,0,-3);
                text(bold,stripRichTextTags(completed->charter),137,119.2f,500,40,30,white,0,-3);
            }
            text(regular,countText(gameplayBandScore().total),726.5f,25,467,88,72,white,1);
            const auto band=gameplayBandScore();
            const unsigned rank=scoreStarRank(band.ordinary,band.base);
            for(unsigned i=0;i<7;++i) {
                const float x=960+(float(i)-3)*52.4643f;
                draw->AddImage(resultsUiIds[0],p(x-32.175f,101.1f),p(x+32.175f,173.9f),
                    ImVec2(0,0),ImVec2(1,1),IM_COL32(27,27,27,168));
                if(i<rank)draw->AddImage(resultsUiIds[0],p(x-32.175f,97.6f),p(x+32.175f,170.4f));
            }
            if(activeProfile.bot || newHighScore || songFailed) {
                const char* message=songFailed?"Song Failed":activeProfile.bot?"Scores cannot be saved with a bot!":"New High Score!";
                text(regular,message,1371,79.2f,500,50,36,songFailed?IM_COL32(255,110,100,255):white,2);
            }
            const unsigned resultSeats=gameplaySeatMask?gameplaySeatMask:(1u<<gameplayPlayer);
            for(int seat=0;seat<PAD_PLAYERS;++seat)if(resultSeats&(1u<<seat)) {
                selectGameplaySeat(seat);
            const unsigned rank=currentStarRank();
            const float center=960+(float(gameplayOrdinal())-(gameplayPlayerCount()-1)*.5f)*470;
            const float x=center-225;
            constexpr float cardTop=196.38f,bodyTop=268.38f,bodyBottom=883.88f;
            panel(resultsUiIds[1],x,bodyTop,450,bodyBottom-bodyTop,IM_COL32(0,0,0,64));
            panel(resultsUiIds[2],x,cardTop,450,72,header);
            const std::string playerName=activeProfile.bot?activeProfile.name+" (Bot)":activeProfile.name;
            text(regular,playerName,x+10,cardTop+6,430,60,50,activeGuest?guestNameInk(gameplayPlayer):white,1);
            unsigned headHits=0;
            for(size_t i=0;i<chart.notes.size();++i)
                if(i+1<chordNoteOffsets.size() && chordNoteOffsets[i]<notes.size() && notes[chordNoteOffsets[i]].hit)++headHits;
            const unsigned total=static_cast<unsigned>(chart.notes.size());
            const unsigned missed=total-std::min(total,headHits);
            const double accuracy=total?100.0*headHits/total:0;
            char number[96];std::snprintf(number,sizeof(number),"%.2f",accuracy);
            std::string percent=number;while(percent.size()>1&&percent.back()=='0')percent.pop_back();
            if(!percent.empty()&&percent.back()=='.')percent.pop_back();percent+='%';
            static constexpr const char* difficulties[]{"Easy","Medium","Hard","Expert"};
            text(regular,difficulties[std::clamp(chartDifficulty,0,3)],x+5,291.38f,170,50,41.65f,white,1);
            const ImTextureID icon=chartInstrument==3?playerInstrumentIcon(activeProfile.controller>=2?activeProfile.controller:2):
                chartInstrument==1?downloaderInstrumentIds[1]:chartInstrument==2?rhythmInstrumentId:playerInstrumentIcon(chartInstrument>=4?1:0);
            if(icon)draw->AddImage(icon,p(x+183,274.38f),p(x+267,358.38f));
            const bool fullCombo=total && headHits==total && !resultStats.overstrums && !songFailed;
            if(fullCombo)draw->AddImage(resultsUiIds[resultStats.ghosts?4:5],p(x+320,276.38f),p(x+400,356.38f));
            else text(regular,percent,x+275,291.38f,170,50,41.65f,white,1);
            const float starsWidth=rank?rank*49.5f+(rank-1)*5:0;
            for(unsigned i=0;i<rank;++i) {
                const float left=center-starsWidth*.5f+i*54.5f;
                draw->AddImage(resultsUiIds[0],p(left,348.38f),p(left+49.5f,404.38f));
            }
            text(regular,countText(score),x+25,391.38f,400,50,40,white,1);
            const int sectionPages=std::max(1,(int(practiceMarks.size())+9)/10);
            const int pageCount=3+sectionPages;
            resultsSelection=std::clamp(resultsSelection,0,pageCount-1);
            const float target=resultsSelection*435.0f;
            resultsScroll+=(target-resultsScroll)*std::min(1.0f,float(frameTimer)*14.0f);
            if(std::abs(resultsScroll-target)<.1f)resultsScroll=target;
            draw->PushClipRect(p(x,448.93f),p(x+450,883.23f),true);
            const auto pageHeader=[&](float y,const char* name) {
                draw->AddRectFilled(p(x,y),p(x+450,y+40),header);
                text(light,name,x,y,450,40,30,white,1);
            };
            const auto row=[&](float y,unsigned index,const std::string& name,const std::string& value) {
                if(index&1)draw->AddRectFilled(p(x,y),p(x+450,y+37.5f),IM_COL32(0,0,0,64));
                text(regular,name,x+12.81f,y,319,37.5f,27,white,0,-3);
                text(regular,value,x+246.1f,y,191.4f,37.5f,27,white,2,-5);
            };
            for(int page=0;page<pageCount;++page) {
                const float y=448.93f+page*435-resultsScroll;
                if(y>883.23f || y+435<448.93f)continue;
                if(page==0) {
                    pageHeader(y,"PERFORMANCE");
                    const auto ordinary=score-soloBonusTotal-fretBonusTotal;
                    const float average=chartBaseScore?float(ordinary)/float(chartBaseScore):0;
                    std::snprintf(number,sizeof(number),"%.3fx",std::trunc(average*1000.0f)/1000.0f);
                    const std::string names[]{"Total Notes","Notes Hit","Notes Missed","Best Streak","Avg. Multiplier","Overstrums"};
                    const std::string values[]{countText(total),countText(headHits),countText(missed),countText(bestCombo),number,countText(resultStats.overstrums)};
                    for(unsigned i=0;i<6;++i)row(y+40+i*37.5f,i,names[i],values[i]);
                    unsigned used=0;
                    for(unsigned m=0;m<8;++m)if(playerModifierMask[gameplayPlayer]&(1u<<m)) {
                        const float yy=y+290+used*29;const char* name=rhythm::play_menu::modifiers[m];
                        const float ww=regular->CalcTextSizeA(22,100000,0,name).x+12;
                        draw->AddRectFilled(p(center-ww*.5f,yy),p(center+ww*.5f,yy+27),header);
                        text(regular,name,center-ww*.5f,yy,ww,27,22,white,1);++used;
                    }
                    if(!used) {
                        draw->AddRectFilled(p(center-58,y+337),p(center+58,y+364),header);
                        text(bold,"No Modifiers",center-58,y+337,116,27,20,white,1,-3);
                    }
                } else if(page==1) {
                    pageHeader(y,"STAR POWER");unsigned phrases=0;
                    for(const auto& phrase:starPhrases)phrases+=phrase.awarded;
                    row(y+40,0,"Phrases",countText(phrases)+"/"+countText(starPhrases.size()));
                    std::snprintf(number,sizeof(number),"%.2f",std::trunc(resultStats.barsFilled*100)/100);row(y+77.5f,1,"Bars Filled",number);
                    row(y+115,2,"Activations",countText(resultStats.activations));
                    std::snprintf(number,sizeof(number),"%.2fs",std::trunc(resultStats.spSeconds*100)/100);row(y+152.5f,3,"Time Used",number);
                    pageHeader(y+190,"MORE STATS");
                    row(y+230,0,chartInstrument==3?"Accents Hit":"Ghost Inputs",countText(chartInstrument==3?resultStats.accents:resultStats.ghosts));
                    if(chartInstrument==3)row(y+267.5f,1,"Ghosts Hit",countText(resultStats.ghostPads));
                    else row(y+267.5f,1,"Squeezed Notes","+"+countText(resultStats.positiveSqueezes)+"/-"+countText(resultStats.negativeSqueezes));
                } else if(page==2) {
                    pageHeader(y,"SCORING");
                    const char* names[]{"Notes","Combo Bonus","Star Power","Solo Bonus","Sustains","Clean Play Bonus","Squeeze Score"};
                    const uint64_t values[]{resultStats.notes,resultStats.combo,resultStats.starPower,soloBonusTotal,resultStats.sustains,fretBonusTotal};
                    for(unsigned i=0;i<6;++i)row(y+40+i*37.5f,i,names[i],countText(values[i]));
                    row(y+265,6,names[6],std::to_string(resultStats.squeezeScore));
                } else {
                    pageHeader(y,"SONG SECTIONS");
                    const size_t first=size_t(page-3)*10;
                    for(size_t i=first;i<std::min(practiceMarks.size(),first+10);++i) {
                        const double from=practiceMarks[i].time,to=i+1<practiceMarks.size()?practiceMarks[i+1].time:1e30;
                        unsigned totalIn=0,hitIn=0;
                        for(size_t head=0;head<chart.notes.size();++head)if(chart.notes[head].time>=from && chart.notes[head].time<to) {
                            ++totalIn;if(chordNoteOffsets[head]<notes.size()&&notes[chordNoteOffsets[head]].hit)++hitIn;
                        }
                        const float yy=y+40+float(i-first)*37.5f;
                        if((i-first)&1)draw->AddRectFilled(p(x,yy),p(x+450,yy+37.5f),IM_COL32(0,0,0,64));
                        text(regular,practiceMarks[i].name,x+12,yy,270,37.5f,27,white,0,-3);
                        std::snprintf(number,sizeof(number),"%.0f%%",totalIn?100.0*hitIn/totalIn:0);
                        text(regular,totalIn?number:"-",x+285,yy,78,37.5f,27,white,2,-5);
                        text(regular,countText(totalIn-hitIn),x+375,yy,62,37.5f,27,white,2,-5);
                    }
                }
            }
            draw->PopClipRect();
            if(resultsSelection+1<pageCount)draw->AddImage(resultsUiIds[3],p(center-27,889),p(center-2.425f,909.756f),ImVec2(0,0),ImVec2(1,1),IM_COL32(255,255,255,180));
            if(resultsSelection>0)draw->AddImage(resultsUiIds[3],p(center+3,889),p(center+27.575f,909.756f),ImVec2(0,1),ImVec2(1,0),IM_COL32(255,255,255,180));
            }
            selectGameplaySeat(primaryGameplaySeat);
            panel(resultsUiIds[1],440,942,1040,80,IM_COL32(0,0,0,64));
            static constexpr const char* actions[]{"Continue","Replay","Practice","Random Song"};
            static constexpr unsigned icons[]{0,2,3,4};
            for(int action=0;action<4;++action) {
                const float bx=458+action*250.0f;
                draw->AddImage(songSelectControlIds[icons[action]],p(bx,960),p(bx+44,1004));
                text(bold,actions[action],bx+49,958,194,48,26,white,0,-3);
                if(clickable(120+action,bx,950,246,64))performResultsAction(action);
            }
        } else if (ui.visible) {
            popupLayer(true);
            ImFont* face=settingsFont?settingsFont:ImGui::GetFont();
            const ImU32 white=IM_COL32(245,245,245,255),muted=IM_COL32(185,185,185,255);
            const auto text=[&](float x,float y,float size,float width,const char* value,ImU32 color,bool right=false) {
                const float measured=face->CalcTextSizeA(size,100000,0,value).x;
                const float actual=size*std::min(1.0f,width/std::max(1.0f,measured));
                const float w=face->CalcTextSizeA(actual,100000,0,value).x;
                draw->AddText(face,actual*scale,p(x+(right?width-w:0),y),color,value);
            };
            draw->AddRectFilled(p(0,0),p(1920,1080),IM_COL32(0,0,0,172));
            for(const auto& song:localSongs) if(song.chartPath==activeChartPath) {
                text(75,40,60,826,song.title.c_str(),white);
                text(75,115,45,826,song.artist.c_str(),muted);break;
            }
            draw->AddRectFilled(p(75,195),p(725,197.5f),IM_COL32(210,210,210,150));
            const char* heading=pausePage==PausePage::Main?"PAUSED":pausePage==PausePage::Options?"OPTIONS":
                pausePage==PausePage::Volume?"VOLUME":pausePage==PausePage::Calibration?"CALIBRATION":
                pausePage==PausePage::SongSettings?"SONG SETTINGS":pausePage==PausePage::Sections?"SECTION":
                pausePage==PausePage::ConfirmQuit?"CONFIRM":"PRACTICE";
            ImFont* titleFace=pauseHeadingFont?pauseHeadingFont:face;
            const float headingWidth=titleFace->CalcTextSizeA(100,100000,0,heading).x;
            const float headingSize=100*std::min(1.0f,1000/std::max(1.0f,headingWidth));
            const float actualWidth=titleFace->CalcTextSizeA(headingSize,100000,0,heading).x;
            draw->AddText(titleFace,headingSize*scale,p(1845-actualWidth,170),white,heading);
            draw->AddImage(songSelectControlIds[0],p(1440,315),p(1488,363));text(1493,322,30,110,"Select",white);
            draw->AddImage(songSelectControlIds[1],p(1650,315),p(1698,363));text(1703,322,30,140,"Back",white);
            const char* optionRows[]={"Volume","Calibration"};
            const char* volumeRows[]={"Master Volume","Guitar","Bass","Rhythm","Vocals","Drums","Keys","Song","Crowd","Drums Freestyle","Sound Effects","Miss Sound Volume"};
            const char* calibrationRows[]={"Audio Offset","Video Offset","Get Hardware Latency","Show Hit Window"};
            const char* songRows[]={"Song Offset","Video Start Time","End Events"};
            const char* practiceRows[]={"Set A Position","Set B Position","Song Speed","Restart Section","Clear A/B","Back"};
            const char* quitRows[]={"Yes","No"};
            const char* const* rows=playMode==PlayMode::Practice?rhythm::play_menu::practice:rhythm::play_menu::quickplay;
            if(pausePage==PausePage::Options) rows=optionRows;
            else if(pausePage==PausePage::Volume) rows=volumeRows;
            else if(pausePage==PausePage::Calibration) rows=calibrationRows;
            else if(pausePage==PausePage::SongSettings) rows=songRows;
            else if(pausePage==PausePage::PracticeSection) rows=practiceRows;
            else if(pausePage==PausePage::ConfirmQuit) rows=quitRows;
            if(pausePage==PausePage::ConfirmQuit) {
                const char* question=pendingPauseAction==PauseAction::Restart?"Are you sure you want to restart?":
                    pendingPauseAction==PauseAction::NewSong?"Choose a new song?":pendingPauseAction==PauseAction::Practice?"Switch to Practice?":
                    pendingPauseAction==PauseAction::Quickplay?"Switch to Quickplay?":"Are you sure you want to quit?";
                text(75,225,40,800,question,white);
            }
            if(pausePage==PausePage::Main && playMode==PlayMode::Practice) {
                char value[80];
                std::snprintf(value,sizeof(value),"A: %.2f s     B: %.2f s",practiceSectionASet?practiceSectionA:0.0,practiceSectionBSet?practiceSectionB:0.0);
                text(75,225,34,650,value,white);
                std::snprintf(value,sizeof(value),"Song Speed: %.0f%%",practiceRate*100.0f);
                text(75,285,32,650,value,muted);
            }
            const int rowCount=pausePageRowCount();
            const int first=pausePage==PausePage::Sections?std::max(0,pauseSelection-9):
                pausePage==PausePage::Volume?std::max(0,pauseSelection-10):0;
            for(int i=first;i<std::min(rowCount,first+(pausePage==PausePage::Sections?10:11));++i) {
                const float y=369.0f+(i-first)*54.0f;
                const bool selected=i==pauseSelection;
                const ImU32 ink=selected?IM_COL32(12,12,12,255):white;
                draw->AddRectFilled(p(1425,y),p(1845,y+50),selected?IM_COL32(224,224,224,255):IM_COL32(25,25,25,205));
                const char* row=pausePage==PausePage::Sections?(i<static_cast<int>(practiceMarks.size())?practiceMarks[i].name.c_str():"Back"):rows[i];
                const bool values=pausePage==PausePage::Volume || pausePage==PausePage::Calibration || pausePage==PausePage::SongSettings || pausePage==PausePage::PracticeSection;
                const bool disabled=(pausePage==PausePage::SongSettings && i>=1) || (pausePage==PausePage::Calibration && i==2) || (pausePage==PausePage::Volume && i==9);
                text(1445,y+7,34,values?210:380,row,disabled?IM_COL32(118,118,118,255):ink);
                if(values) {
                    char value[40]{};
                    if (pausePage == PausePage::Volume) {
                        float amount = masterVolume;
                        if (i >= 1 && i <= 6) amount = instrumentVolumes[static_cast<size_t>(i - 1)];
                        else if(i==7) amount=musicVolume;
                        else if(i==8) amount=instrumentVolumes[6];
                        else if(i==10) amount=effectsVolume;
                        else if(i==11) amount=missSoundVolume;
                        if(i==9) std::snprintf(value,sizeof(value),"Unavailable");
                        else std::snprintf(value,sizeof(value),"%.0f%%",amount * 100.0f);
                    } else if (pausePage == PausePage::Calibration && i == 0)
                        std::snprintf(value,sizeof(value),"%+.0f ms",audioOffsetMs);
                    else if (pausePage == PausePage::Calibration && i == 1)
                        std::snprintf(value,sizeof(value),"%+.0f ms",videoOffsetMs);
                    else if (pausePage == PausePage::Calibration && i == 2)
                        std::snprintf(value,sizeof(value),"Manual on PS5");
                    else if (pausePage == PausePage::Calibration && i == 3)
                        std::snprintf(value,sizeof(value),"%s",showHitWindow?"On":"Off");
                    else if (pausePage == PausePage::SongSettings && i == 0)
                        std::snprintf(value,sizeof(value),"%+.0f ms",chartOffsetSeconds * 1000.0);
                    else if (pausePage == PausePage::SongSettings && i >= 1)
                        std::snprintf(value,sizeof(value),"Unavailable");
                    else if (pausePage == PausePage::PracticeSection && i == 0)
                        std::snprintf(value,sizeof(value),practiceSectionASet?"%.2f s":"Not set",practiceSectionA);
                    else if (pausePage == PausePage::PracticeSection && i == 1)
                        std::snprintf(value,sizeof(value),practiceSectionBSet?"%.2f s":"Not set",practiceSectionB);
                    else if (pausePage == PausePage::PracticeSection && i == 2)
                        std::snprintf(value,sizeof(value),"%.0f%%",practiceRate*100.0f);
                    text(1660,y+10,28,165,value,ink,true);
                }
                if(clickable(30+i,1425,y,420,50)) {pauseSelection=i;activatePauseSelection();}
            }
            text(75,972,29,700,"Strum: Navigate     Start: Resume",muted);
            if(pausePage==PausePage::Volume || pausePage==PausePage::Calibration || pausePage==PausePage::SongSettings ||
               (pausePage==PausePage::Main && playMode==PlayMode::Practice)) {
                draw->AddImage(songSelectControlIds[2],p(1140,965),p(1188,1013));text(1198,974,29,220,"Decrease",white);
                draw->AddImage(songSelectControlIds[3],p(1495,965),p(1543,1013));text(1553,974,29,220,"Increase",white);
            }
        }
        if(screen==Screen::MidiSetup && midiSetupSource>=0 && midiSetupSource<4) {
            popupLayer(true);
            const ImU32 white=IM_COL32(245,245,245,255),muted=IM_COL32(205,211,220,255);
            const auto* face=settingsFont?settingsFont:ImGui::GetFont();
            const auto& device=midiSources[midiSetupSource];
            draw->AddRectFilled(p(400,150),p(1520,240),IM_COL32(8,8,8,255));
            draw->AddRectFilled(p(400,255),p(1520,925),IM_COL32(23,23,23,217));
            const auto heading=midiTypePage?"NEW CONTROLLER":"DRUMS (MIDI)";
            const auto extent=face->CalcTextSizeA(43*scale,10000,0,heading);
            draw->AddText(face,43*scale,ImVec2(p(960,172).x-extent.x*.5f,p(960,172).y),white,heading);
            char identity[140];std::snprintf(identity,sizeof(identity),"USB MIDI %04X:%04X  |  Experimental / intermittent support",device.vid,device.pid);
            draw->AddText(face,25*scale,p(450,280),muted,identity);
            draw->AddText(face,27*scale,p(450,326),white,"A DualSense controller is required for menu navigation.");
            draw->AddText(face,25*scale,p(450,365),muted,"MIDI can cause navigation issues. Kit hits only map pads and play notes.");
            char companion[90];
            std::snprintf(companion,sizeof(companion),midiNavSlot<0?"Connect a DualSense to continue.":"Companion controller: %d",midiNavSlot+1);
            draw->AddText(face,25*scale,p(450,409),IM_COL32(175,196,216,255),companion);
            using Page=rhythm::midi::SetupWizard::Page;
            const auto page=midiWizard.page;
            const bool capturing=!midiTypePage && (page==Page::Capture || page==Page::Soft || page==Page::Hard);
            if(capturing){
                static constexpr const char* actions[]{"Kick","Red pad","Yellow pad","Yellow cymbal","Blue pad","Blue cymbal","Green pad","Green cymbal","Orange cymbal"};
                const auto action=midiWizard.action();const unsigned a=unsigned(action);
                const char* prompt=midiWizard.secondKick()?"Second kick pad":actions[std::min(a,8u)];
                const auto titleSize=face->CalcTextSizeA(42*scale,10000,0,prompt);
                draw->AddText(face,42*scale,ImVec2(p(960,514).x-titleSize.x*.5f,p(960,514).y),white,prompt);
                const unsigned colors[]{4,1,2,2,3,3,0,0,4};
                draw->AddImage(songSelectControlIds[colors[std::min(a,8u)]],p(920,580),p(1000,660));
                const char* instruction=page==Page::Soft?"Strike this pad softly":page==Page::Hard?"Strike the same pad firmly":"Strike the requested pad once";
                const auto size=face->CalcTextSizeA(32*scale,10000,0,instruction);
                draw->AddText(face,32*scale,ImVec2(p(960,690).x-size.x*.5f,p(960,690).y),white,instruction);
                if(!device.streaming)draw->AddText(face,24*scale,p(450,760),IM_COL32(230,194,120,255),"Waiting for the MIDI interface. Reconnect the kit if no hits arrive.");
            }else{
                const char* rows[3]{};int count=2;
                if(midiTypePage){rows[0]=midiSavedMapping?"Drums (MIDI) - saved mapping":"Drums (MIDI)";rows[1]="Ignore for now";}
                else if(page==Page::Layout){rows[0]="4-Lane Pro";rows[1]="5-Lane";}
                else if(page==Page::KickPads){rows[0]="One kick pad - single or double pedal";rows[1]="Two kick pads";}
                else if(page==Page::OfferVelocity){rows[0]="Calibrate pad / cymbal velocity";rows[1]="Use standard MIDI velocity";}
                else {rows[0]="Save for this device and player";rows[1]="Save for this player only";rows[2]="Map again";count=3;}
                for(int row=0;row<count;++row){
                    const float y=485+row*68;const bool selected=row==midiChoice;
                    if(selected)draw->AddRectFilled(p(430,y),p(1490,y+60),IM_COL32(208,212,214,255));
                    draw->AddText(face,31*scale,p(470,y+12),selected?IM_COL32(16,16,16,255):white,rows[row]);
                }
                if(page==Page::OfferVelocity && !midiTypePage)
                    draw->AddText(face,25*scale,p(450,710),muted,"Soft / hard calibration is optional. Kicks are never calibrated.");
                if(page==Page::Review && !midiTypePage){
                    char summary[120];std::snprintf(summary,sizeof(summary),"%s  |  %s  |  %zu mapped inputs",
                        midiWizard.draft.mapping.layout()==rhythm::midi::Layout::Pro4?"4-Lane Pro":"5-Lane",
                        midiWizard.draft.twoKickPads?"Two kick pads":"One kick pad",midiWizard.draft.mapping.bindings().size());
                    draw->AddText(face,25*scale,p(450,730),muted,summary);
                }
            }
            const char* message=midiMessage.empty()?midiWizard.message:midiMessage.c_str();
            if(message && !midiTypePage)draw->AddText(face,23*scale,p(450,800),muted,message,nullptr,1010*scale);
            if(!capturing){draw->AddImage(songSelectControlIds[0],p(450,865),p(490,905));
                draw->AddText(face,26*scale,p(500,872),white,"Select");}
            draw->AddImage(songSelectControlIds[1],p(1090,865),p(1130,905));draw->AddText(face,26*scale,p(1140,872),white,"Cancel");
            if(!midiTypePage && (page==Page::Soft || page==Page::Hard)){
                draw->AddImage(songSelectControlIds[2],p(770,865),p(810,905));draw->AddText(face,26*scale,p(820,872),white,"Skip pad");
            }
            popupLayer(false);
        }
        if(screen==Screen::ControllerSetup) {
            popupLayer(true);
            const auto& u=usbInputs[unknownController];const ImFont* face=settingsFont?settingsFont:ImGui::GetFont();
            draw->AddRectFilled(p(400,150),p(1520,240),IM_COL32(8,8,8,255));
            draw->AddText(face,43*scale,p(460,172),IM_COL32(245,245,245,255),"NEW CONTROLLER");
            draw->AddRectFilled(p(400,255),p(1520,860),IM_COL32(23,23,23,217));
            char device[100];std::snprintf(device,sizeof(device),"USB %04X:%04X - choose the connected instrument",u.vid,u.pid);
            draw->AddText(face,27*scale,p(450,285),IM_COL32(205,211,220,255),device);
            for(int i=0;i<6;++i) {
                const float y=355+i*65;const bool selected=i==unknownControllerChoice;
                if(selected)draw->AddRectFilled(p(430,y),p(1490,y+60),IM_COL32(208,212,214,255));
                constexpr int order[]{0,2,4,3,1};const bool enabled=i==5 || rhythm::controls::controllerTypeEnabled(order[i]);
                const auto icon=i<5?playerControllerIcon(order[i]):ImTextureID{};
                if(icon)draw->AddImage(icon,p(455,y+5),p(505,y+55),ImVec2(0,0),ImVec2(1,1),enabled?IM_COL32(255,255,255,255):IM_COL32(130,130,130,255));
                draw->AddText(face,31*scale,p(530,y+12),!enabled?IM_COL32(115,120,130,255):selected?IM_COL32(16,16,16,255):IM_COL32(245,245,245,255),i<5?rhythm::MenuProfile::controllerNames[order[i]]:"Ignore for now");
            }
            draw->AddText(face,26*scale,p(450,800),IM_COL32(205,211,220,255),"Strum: Choose    Green: Map    Red: Ignore");
            popupLayer(false);

        }
        popupLayer(false);
        if (screen == Screen::Gameplay && !ui.visible) {
          const unsigned hudSeats=gameplaySeatMask?gameplaySeatMask:(1u<<gameplayPlayer);
          for(int seat=0;seat<PAD_PLAYERS;++seat)if(hudSeats&(1u<<seat)) {
            selectGameplaySeat(seat);
            draw->AddDrawCmd();const int commandStart=draw->CmdBuffer.Size-1;
            const int vertexStart=draw->VtxBuffer.Size;
            const auto hudLayout=rhythm::pcPlayerCamera(gameplayPlayerCount());
            const auto gameplayPoint=[&](float x,float y) {
                return ImVec2((io.DisplaySize.x-1920*fitScale)*.5f+x*fitScale,
                              (io.DisplaySize.y-1080*fitScale)*.5f+(y+hudLayout.uiOffset)*fitScale);
            };
            if(activeProfile.displayName || activeProfile.bot) {
                const std::string label=activeProfile.name+(activeProfile.bot?" (Bot)":"");
                const ImFont* font=gameplayNameFont?gameplayNameFont:ImGui::GetFont();
                const float fontSize=pc_gameplay_labels::nameSize*fitScale*hudLayout.uiScale;
                const auto extent=font->CalcTextSizeA(fontSize,100000,0,label.c_str());
                const auto center=gameplayPoint(960,1080-pc_gameplay_labels::nameBottom);
                draw->AddText(font,fontSize,ImVec2(center.x-extent.x*.5f,center.y-extent.y*.5f),
                    activeGuest?guestNameInk(gameplayPlayer):IM_COL32(255,255,255,255),label.c_str());
            }
            if(activeProfile.showAccuracy) {
                char text[48];
                const unsigned judged=hits+misses;
                snprintf(text,sizeof(text),"ACCURACY  %.2f%%",judged?100.0*hits/judged:100.0);
                const auto* font=ImGui::GetFont();const float size=18*fitScale*hudLayout.uiScale;
                const auto extent=font->CalcTextSizeA(size,100000,0,text);
                const auto center=gameplayPoint(960,800);
                draw->AddText(font,size,ImVec2(center.x-extent.x*.5f,center.y),IM_COL32(240,244,250,255),text);
            }
            for(const auto& solo:solos)if(songTime>=solo.startTime && songTime<solo.endTime) {
                // PC SoloCounter 0x1802BA820: hit parents / full solo length,
                // percentage starts at zero. Source canvas is centered, +165 Y.
                const unsigned percent=rhythm::pcSoloPercent(solo.hits,solo.total());
                char label[48];std::snprintf(label,sizeof(label),"SOLO  %u%%",percent);
                const auto* font=gameplayNameFont?gameplayNameFont:ImGui::GetFont();
                const float size=40*fitScale*hudLayout.uiScale;
                const auto extent=font->CalcTextSizeA(size,100000,0,label);
                const auto center=gameplayPoint(960,375);
                const ImVec2 origin(center.x-extent.x*.5f,center.y-extent.y*.5f);
                draw->AddText(font,size,ImVec2(origin.x+2*fitScale,origin.y+2*fitScale),IM_COL32(0,0,0,210),label);
                draw->AddText(font,size,origin,IM_COL32(255,255,255,255),label);
                break;
            }
            const bool practiceCountIn=practiceCountdownRemaining>0.0;
            const auto after=std::upper_bound(countdownRanges.begin(),countdownRanges.end(),songTime,
                [](double at,const rhythm::PcCountdown& range){return at<range.start;});
            const auto* range=after==countdownRanges.begin()?nullptr:&*(after-1);
            const bool inRest=range && songTime>=range->start && songTime<range->start+range->duration;
            if(practiceCountIn || inRest) {
                const double remaining=practiceCountIn?practiceCountdownRemaining:range->start+range->duration-songTime;
                const float alpha=practiceCountIn?1.0f:rhythm::pcCountdownAlpha(remaining);
                if(alpha>0) {
                    char countText[32];
                    if(!practiceCountIn && measureCountdown)std::snprintf(countText,sizeof(countText),"%d",rhythm::pcCountdownMeasures(*range,measureTimes,songTime));
                    else std::snprintf(countText,sizeof(countText),"%.0f",rhythm::pcCountdownSeconds(remaining));
                    const ImFont* font=countdownFont?countdownFont:ImGui::GetFont();
                    float size=pc_gameplay_labels::countdownMaxSize;
                    const float width=font->CalcTextSizeA(size,100000,0,countText).x;
                    size=std::clamp(size*std::min(1.0f,pc_gameplay_labels::countdownWidth/std::max(1.0f,width)),
                        pc_gameplay_labels::countdownMinSize,pc_gameplay_labels::countdownMaxSize)*pc_gameplay_labels::countdownScale*fitScale*hudLayout.uiScale;
                    const auto extent=font->CalcTextSizeA(size,100000,0,countText);
                    const auto baseline=gameplayPoint(960,540);
                    draw->AddText(font,size,ImVec2(baseline.x-extent.x*.5f,baseline.y-font->Ascent*(size/font->FontSize)),
                        IM_COL32(255,255,255,int(alpha*255)),countText);
                }
            }
            if(gameplayPlayerCount()>1) {
                const auto viewport=gameplayViewport();
                const float factor=viewport.height/float(height);
                for(int v=vertexStart;v<draw->VtxBuffer.Size;++v) {
                    auto& point=draw->VtxBuffer[v].pos;
                    point.x=viewport.x+viewport.width*.5f+(point.x-io.DisplaySize.x*.5f)*factor;
                    point.y=viewport.y+point.y*factor;
                }
                draw->AddDrawCmd();
                for(int command=commandStart;command+1<draw->CmdBuffer.Size;++command)
                    draw->CmdBuffer[command].ClipRect=ImVec4(viewport.x,viewport.y,viewport.x+viewport.width,viewport.y+viewport.height);
            }
          }
          selectGameplaySeat(primaryGameplaySeat);
        }
        if(screen==Screen::Gameplay && !ui.visible) {
            // ScoreManager.Update 0x180292C90 sums every engine's ordinary,
            // solo and clean-play points. StarProgress receives ordinary score.
            const auto band=gameplayBandScore();
            char rawScoreDigits[32], scoreDigits[48];
            std::snprintf(rawScoreDigits, sizeof(rawScoreDigits), "%llu", static_cast<unsigned long long>(band.total));
            size_t scoreOut = 0;
            const size_t rawLength = std::strlen(rawScoreDigits);
            for (size_t i = 0; i < rawLength && scoreOut + 2 < sizeof(scoreDigits); ++i) {
                scoreDigits[scoreOut++] = rawScoreDigits[i];
                const size_t remaining = rawLength - i - 1;
                if (remaining && remaining % 3 == 0) scoreDigits[scoreOut++] = ',';
            }
            scoreDigits[scoreOut] = '\0';
            // Exact PC HUD-local sprite bounds, colors and sorting. The
            // viewport anchor keeps the source group on the highway's right.
            const auto anchor=rhythm::pcScoreHudAnchor(gameplayPlayerCount());
            const auto hudPoint=[&](float x,float y) {return p(960+(anchor.x+x*4.25f)*108,540-(anchor.y+y*4.25f)*108);};
            const float starFill=scoreStarProgress(band.ordinary,band.base);
            double songEnd=notes.empty()?1.0:notes.back().timeSec+notes.back().sustainSec+2.0;
#if !defined(PS5_HOST_REFERENCE)
            if(songAudio.durationSeconds()>0) songEnd=songAudio.durationSeconds();
#endif
            const float songFill=static_cast<float>(std::clamp(songTime/std::max(0.001,songEnd),0.0,1.0));
            for(size_t i=0;i<std::size(pc_score::layers);++i) {
                const auto& layer=pc_score::layers[i];
                float x0=layer.bounds[0],y0=layer.bounds[1],x1=layer.bounds[2],y1=layer.bounds[3];
                const bool fill=layer.renderer==15930 || layer.renderer==15932;
                const bool end=layer.renderer==15926 || layer.renderer==15911;
                const float amount=(layer.renderer==15930 || layer.renderer==15926)?starFill:songFill;
                if(fill) x1=x0+(x1-x0)*amount;
                if(end) { const float shift=(1-amount)*(layer.renderer==15926?0.5616f:0.5936f);x0-=shift;x1-=shift; }
                const auto& c=layer.color;
                draw->AddImage(sourceScoreIds[i],hudPoint(x0,y1),hudPoint(x1,y0),
                    ImVec2(0,0),ImVec2(1,1),ImGui::ColorConvertFloat4ToU32(ImVec4(c[0],c[1],c[2],c[3])));
            }
            // Source score-font glyphs, right aligned; fit nine digits+commas.
            float digitX=1.565f;
            for(const char* c=scoreDigits+scoreOut;c!=scoreDigits;) {
                --c; const bool comma=*c==',';
                const unsigned glyph=comma?0u:static_cast<unsigned>(*c-'0')+1u;
                const float width=comma?0.021f:0.044f;
                digitX-=width;
                draw->AddImage(scoreGlyphIds[glyph],hudPoint(digitX,-0.183f),hudPoint(digitX+width,-0.245f));
            }
            const unsigned rank=scoreStarRank(band.ordinary,band.base);
            // StarProgress.Reset clears both numeral sprites until rank one.
            if(rank>0) {
                draw->AddImage(scoreGlyphIds[rank+1],hudPoint(1.677f,-0.185f),hudPoint(1.722f,-0.250f),
                    ImVec2(0,0),ImVec2(1,1),IM_COL32(255,197,29,210));
            }
        }
        if(screen==Screen::Gameplay && showLyrics && !lyricPhrases.empty()) {
            // Original level2 Lyrics canvas: centers y=135.5/191.5, current
            // 2x32pt Lato Regular, upcoming 32pt. Draw once for the band,
            // outside per-highway transforms, below the modal channel.
            const auto view=rhythm::pcLyricView(lyricPhrases,songClock.gameplayTime());
            const ImFont* face=playerNameFont?playerNameFont:ImGui::GetFont();
            // The Encore full-song widget occupies the PC lyric canvas's
            // right edge. Constrain the lyric strip to its free center area.
            const float width=showSongDisplay && gameplaySongInfoValid?900.0f:1480.0f;
            const auto line=[&](const rhythm::LyricPhrase& phrase,float y,float height,float size,float alpha,size_t highlight,float brightness) {
                const float measured=face->CalcTextSizeA(size,100000,0,phrase.text.c_str()).x;
                size*=std::min(1.0f,width/std::max(1.0f,measured));
                const auto extent=face->CalcTextSizeA(size,100000,0,phrase.text.c_str());
                const ImVec2 pos=p(960-extent.x*.5f,y-size*.5f);
                draw->AddRectFilled(p(960-width*.5f-16,y-height*.5f),p(960+width*.5f+16,y+height*.5f),IM_COL32(0,0,0,int(128*alpha)));
                draw->AddText(face,size*scale,pos,IM_COL32(int(brightness),int(brightness),int(brightness),int(255*alpha)),phrase.text.c_str());
                // PC's default karaoke tint is RGBA(255,163,0,255).
                if(highlight)draw->AddText(face,size*scale,pos,IM_COL32(255,163,0,int(255*alpha)),phrase.text.c_str(),phrase.text.c_str()+std::min(highlight,phrase.text.size()));
            };
            if(view.outgoing>=0)line(lyricPhrases[view.outgoing],135.5f-56*view.promotion,
                75,64,view.outgoingAlpha,lyricPhrases[view.outgoing].text.size(),255);
            if(view.current>=0)line(lyricPhrases[view.current],191.5f-56*view.promotion,
                35+40*view.promotion,32+32*view.promotion,view.alpha,view.highlighted,160+95*view.promotion);
            if(view.next>=0)line(lyricPhrases[view.next],191.5f,35,32,
                view.current>=0?std::clamp((view.promotion-.75f)*4,0.0f,1.0f):1.0f,0,160);
        }
        if (screen == Screen::Gameplay && !ui.visible && showSongDisplay && gameplaySongInfoValid) {
            const LocalSong& currentSong = gameplaySongInfo;
            double elapsed=std::max(0.0,songClock.gameplayTime());
            double duration=currentSong.durationSeconds;
#if !defined(PS5_HOST_REFERENCE)
            // Full synchronized audio, never the randomized menu preview window.
            if(songAudio.ready() && loadedAudioChartPath==activeChartPath) {
                elapsed=songAudio.positionSeconds();duration=songAudio.durationSeconds();
            }
#endif
            if(duration<=0 && !notes.empty()) duration=notes.back().timeSec+notes.back().sustainSec;
            const float progress=duration>0?static_cast<float>(elapsed/duration):0.0f;
            const int firstVertex=draw->VtxBuffer.Size;
            drawNowPlaying(&currentSong,progress,true,std::max(0.0,songClock.gameplayTime()));
            for(int v=firstVertex;v<draw->VtxBuffer.Size;++v)
                draw->VtxBuffer[v].pos=projectRightWidget(draw->VtxBuffer[v].pos,previewX+previewWidth);
        }
        if(!controllerToast.empty() && ImGui::GetTime()<controllerToastUntil) {
            const ImFont* font=settingsFont?settingsFont:ImGui::GetFont();
            const float width=font->CalcTextSizeA(24,100000,0,controllerToast.c_str()).x;
            draw->AddRectFilled(p(960-width*.5f-22,20),p(960+width*.5f+22,70),IM_COL32(8,13,23,230));
            draw->AddText(font,24*scale,p(960-width*.5f,32),IM_COL32(240,243,247,255),controllerToast.c_str());
        }
        if(updateDialog) {
            popupLayer(true);
            using State=rhythm::updates::UpdateState;
            const auto snapshot=applicationUpdater.snapshot();const auto state=snapshot.state;
            constexpr float padding=28,header=64,gap=8,maxWidth=800;
            const auto white=IM_COL32(245,245,245,255),muted=IM_COL32(195,200,210,255);
            auto* face=settingsFont?settingsFont:ImGui::GetFont();
            struct Line {const char* text;float size;ImU32 color;float height;};
            std::array<Line,4> lines{};unsigned count=0;
            const auto append=[&](const char* text,float size,ImU32 color){
                lines[count++]={text,size,color,0};
            };
            append(snapshot.release.title.empty()?buildVersion:snapshot.release.title.c_str(),28,white);
            append(snapshot.message.empty()?"Check for a new PS5 release.":snapshot.message.c_str(),26,white);
            if(state==State::Available)append("Updates apply automatically to folder, PKG and exFAT installs.",24,muted);
            if(state==State::Ready)append("Keep the console on while the game restarts.",24,muted);
            if(state==State::Downloaded && !snapshot.path.empty())append(snapshot.path.c_str(),22,muted);
            const bool busy=state==State::Downloading || state==State::Checking || state==State::Installing;
            const char* heading="APPLICATION UPDATE";
            const char* primary=busy?nullptr:state==State::Available?"Update":state==State::Ready?"Restart and play":state==State::Downloaded?"Done":"Check now";
            const char* secondary=state==State::Installing?nullptr:busy?"Cancel":state==State::Available?"Later":"Back";
            const auto textWidth=[&](const char* text,float size){return face->CalcTextSizeA(size*scale,100000,0,text).x/scale;};
            const float tw=textWidth(heading,32);
            float preferredWidth=tw;
            for(unsigned i=0;i<count;++i)preferredWidth=std::max(preferredWidth,textWidth(lines[i].text,lines[i].size));
            const float controlsWidth=(primary?44+textWidth(primary,24):0)+(secondary?44+textWidth(secondary,24):0)+(primary&&secondary?64:0);
            const float w=std::min(maxWidth,std::max(preferredWidth,controlsWidth)+padding*2),x=(1920-w)*.5f;
            float contentHeight=0;
            for(unsigned i=0;i<count;++i){
                auto& line=lines[i];
                line.height=face->CalcTextSizeA(line.size*scale,100000,(w-padding*2)*scale,line.text).y/scale;
                contentHeight+=line.height+(i?14:0);
            }
            const float bodyHeight=padding*2+contentHeight+(busy?30:0)+(state==State::Installing?0:54);
            constexpr float y=12; // Keep update text just below the top edge, above the logo.
            draw->AddRectFilled(p(x,y),p(x+w,y+header),IM_COL32(8,8,8,255));
            draw->AddRectFilled(p(x,y+header+gap),p(x+w,y+header+gap+bodyHeight),IM_COL32(22,22,22,153));
            draw->AddText(face,32*scale,p(960-tw*.5f,y+16),white,heading);
            float cursor=y+header+gap+padding;
            for(unsigned i=0;i<count;++i){
                const auto& line=lines[i];
                draw->AddText(face,line.size*scale,p(x+padding,cursor),line.color,line.text,nullptr,(w-padding*2)*scale);
                cursor+=line.height+14;
            }
            if(busy) {
                const float amount=state==State::Checking?.15f:float(applicationUpdater.progress());
                draw->AddRectFilled(p(x+padding,cursor+4),p(x+w-padding,cursor+14),IM_COL32(85,90,100,255));
                draw->AddRectFilled(p(x+padding,cursor+4),p(x+padding+(w-padding*2)*std::clamp(amount,0.0f,1.0f),cursor+14),white);
                cursor+=30;
            }
            const float buttonsY=y+header+gap+bodyHeight-padding-34;
            const auto hint=[&](int color,float hx,const char* label){draw->AddImage(songSelectControlIds[color],p(hx,buttonsY),p(hx+34,buttonsY+34));draw->AddText(face,24*scale,p(hx+44,buttonsY+3),white,label);};
            if(primary)hint(0,x+padding,primary);
            if(secondary)hint(1,x+w-padding-44-textWidth(secondary,24),secondary);
        }
        if(screen==Screen::SongRemove) {
            popupLayer(true);
            constexpr float x=370,y=155,w=1180;
            auto* face=settingsFont?settingsFont:ImGui::GetFont();
            const auto white=IM_COL32(245,245,245,255),muted=IM_COL32(190,195,205,255);
            draw->AddRectFilled(p(x,y),p(x+w,y+76),IM_COL32(8,8,8,255));
            draw->AddRectFilled(p(x,y+88),p(x+w,y+690),IM_COL32(22,22,22,153));
            const char* title=removalPage==1?"CONFIRM SONG REMOVAL":"REMOVE SONGS";
            const float titleWidth=face->CalcTextSizeA(34*scale,100000,0,title).x/scale;
            draw->AddText(face,34*scale,p(960-titleWidth*.5f,y+18),white,title);
            auto hint=[&](int color,float hx,const char* label){draw->AddImage(songSelectControlIds[color],p(hx,y+630),p(hx+34,y+664));draw->AddText(face,24*scale,p(hx+46,y+633),white,label);};
            if(removalPage==0) {
                char summary[96];std::snprintf(summary,sizeof(summary),"%u selected",removalCount());
                draw->AddText(face,27*scale,p(x+30,y+106),white,summary);
                if(removalChoices.empty())draw->AddText(face,28*scale,p(x+30,y+168),muted,"No installed songs are available.");
                const int first=std::max(0,removalCursor-5);
                for(int row=0;row<6 && first+row<int(removalChoices.size());++row) {
                    const int index=first+row;const auto& choice=removalChoices[index];const float ry=y+154+row*72;
                    if(index==removalCursor)draw->AddRectFilled(p(x+20,ry),p(x+w-20,ry+66),IM_COL32(70,80,95,220));
                    draw->AddText(face,28*scale,p(x+34,ry+15),white,removalSelected[index]?"[X]":"[ ]");
                    draw->AddText(face,28*scale,p(x+110,ry+4),white,choice.title.c_str(),nullptr,(w-290)*scale);
                    draw->AddText(face,22*scale,p(x+110,ry+37),muted,choice.artist.c_str(),nullptr,(w-290)*scale);
                    draw->AddText(face,20*scale,p(x+w-143,ry+21),muted,choice.bundled?"Bundled":"Installed");
                }
                hint(0,x+35,"Select");hint(2,x+350,"Review selection");hint(1,x+810,"Back");
            } else if(removalPage==1) {
                char question[140];std::snprintf(question,sizeof(question),"Remove %u selected song%s?",removalCount(),removalCount()==1?"":"s");
                draw->AddText(face,34*scale,p(x+36,y+145),white,question);
                draw->AddText(face,27*scale,p(x+36,y+230),muted,"Installed song folders will be deleted.\nSaved scores and player profiles are kept.",nullptr,(w-72)*scale);
                hint(0,x+70,"Confirm removal");hint(1,x+690,"Go back");
            } else if(removalPage==2) {
                draw->AddText(face,30*scale,p(x+36,y+165),white,"Removing selected songs...");
                const float amount=storageTotal?float(storageProgress.load())/storageTotal.load():0;
                draw->AddRectFilled(p(x+40,y+250),p(x+w-40,y+260),IM_COL32(85,90,100,255));
                draw->AddRectFilled(p(x+40,y+250),p(x+40+(w-80)*amount,y+260),white);
                hint(1,x+70,"Stop after current song");
            } else {
                draw->AddText(face,30*scale,p(x+36,y+160),white,storageResultText.c_str(),nullptr,(w-72)*scale);
                hint(0,x+70,"Done");hint(1,x+690,"Back");
            }
        }
        if(screen==Screen::SongCopyConfirm) {
            popupLayer(true);
            draw->AddRectFilled(ImVec2(0,0),io.DisplaySize,IM_COL32(0,0,0,65));
            const ImFont* face=ImGui::GetFont();
            const ImFont* heading=ratingFont?ratingFont:face;
            constexpr float x=460,y=250,w=1000;
            const auto white=IM_COL32(245,245,245,255),muted=IM_COL32(225,230,236,255);
            draw->AddRectFilled(p(x,y),p(x+w,y+82),IM_COL32(8,8,8,255));
            draw->AddRectFilled(p(x,y+94),p(x+w,y+480),IM_COL32(23,23,23,128));
            const char* title=songCopyToConsole?"COPY USB SONGS TO CONSOLE":"COPY CONSOLE SONGS TO USB";
            const float width=heading->CalcTextSizeA(36*scale,100000,0,title).x/scale;
            draw->AddText(heading,36*scale,p(960-width*.5f,y+21),white,title);
            const auto hint=[&](int fret,float hx,const char* label) {
                draw->AddImage(songSelectControlIds[fret],p(hx,y+429),p(hx+34,y+463));
                draw->AddText(face,24*scale,p(hx+46,y+434),white,label);
            };
            if(songCopyPage==SongCopyPage::Confirm) {
                draw->AddText(face,30*scale,p(x+40,y+123),white,songCopyToConsole?
                    "Copy songs from this USB drive to console storage?":"Copy console songs to this USB drive?");
                char drive[96];std::snprintf(drive,sizeof(drive),"%s",songCopyRoot.empty()?"No USB song library detected":"USB song library selected");
                if(externalSongRoots.size()>1)std::snprintf(drive,sizeof(drive),"USB drive %d of %zu",storageDrive+1,externalSongRoots.size());
                draw->AddText(face,26*scale,p(x+40,y+169),muted,drive);
                char count[180];
                if(songCopyRoot.empty())std::snprintf(count,sizeof(count),"Connect a USB song library to continue.");
                else if(!songCopyPlanReady)std::snprintf(count,sizeof(count),"Checking songs...");
                else if(!songCopyPlan.error.empty())std::snprintf(count,sizeof(count),"%s",songCopyPlan.error.c_str());
                else std::snprintf(count,sizeof(count),"%u song%s to copy    %u duplicate%s skipped",songCopyPlan.toCopy,songCopyPlan.toCopy==1?"":"s",songCopyPlan.duplicates,songCopyPlan.duplicates==1?"":"s");
                draw->AddText(face,28*scale,p(x+40,y+207),white,count);
                for(int i=0;i<2;++i) {
                    const float ry=y+257+i*64;const bool selected=songCopyChoice==i;
                    const bool enabled=i==1 || (songCopyPlanReady && songCopyPlan.error.empty() && songCopyPlan.toCopy>0);
                    draw->AddRectFilled(p(x+40,ry),p(x+w-40,ry+56),selected?IM_COL32(214,218,220,255):IM_COL32(35,35,35,0));
                    draw->AddText(face,31*scale,p(x+64,ry+10),enabled?(selected?IM_COL32(22,22,22,255):white):IM_COL32(130,135,140,255),i==0?"YES":"NO");
                    if(clickable(9800+i,x+40,ry,w-80,56)) {songCopyChoice=i;if(i==0)confirmSongCopy();else screen=Screen::SettingsDetail;}
                }
                if(!storageResultText.empty())draw->AddText(face,23*scale,p(x+40,y+387),IM_COL32(245,215,166,255),storageResultText.c_str(),nullptr,(w-80)*scale);
                hint(0,x+40,"Confirm");hint(1,x+260,"Back");
                if(externalSongRoots.size()>1){hint(2,x+460,"USB drive");hint(3,x+700,"USB drive");}
            } else {
                const bool copying=songCopyPage==SongCopyPage::Progress;
                draw->AddText(face,34*scale,p(x+40,y+134),white,copying?(storageCancel?"CANCELLING COPY...":"COPYING SONGS..."):"COPY COMPLETE");
                char status[180];
                if(copying)std::snprintf(status,sizeof(status),"%u / %u songs",storageProgress.load(),storageTotal.load());
                else std::snprintf(status,sizeof(status),"Copied: %u    Already installed: %u    Failed: %u%s",storageCopyResult.copied,storageCopyResult.duplicates,storageCopyResult.failed,storageCopyResult.cancelled?"    Cancelled":"");
                draw->AddText(face,29*scale,p(x+40,y+201),white,status,nullptr,(w-80)*scale);
                if(copying) {
                    const auto total=storageTotal.load();const float amount=total?std::clamp(float(storageProgress.load())/total,0.0f,1.0f):0;
                    draw->AddRectFilled(p(x+40,y+283),p(x+w-40,y+292),IM_COL32(70,78,91,255));
                    draw->AddRectFilled(p(x+40,y+283),p(x+40+(w-80)*amount,y+292),white);
                    hint(1,x+40,"Cancel copy");
                } else {
                    if(!storageCopyResult.error.empty())draw->AddText(face,24*scale,p(x+40,y+265),muted,storageCopyResult.error.c_str(),nullptr,(w-80)*scale);
                    hint(0,x+40,"Done");hint(1,x+260,"Back");
                }
            }
        }
        popupLayer(false);
        // Shared menu chrome: every current and future Screen receives the
        // version automatically, including pause pages. Active gameplay hides it.
        const bool menuVisible=screen!=Screen::Gameplay||ui.visible;
        if(menuVisible) {
            const float size=21.0f*fitScale;
            const float width=ImGui::GetFont()->CalcTextSizeA(size,100000,0,buildVersion).x;
            draw->AddText(ImGui::GetFont(),size,
                ImVec2(io.DisplaySize.x-40.0f*fitScale-width,15.0f*fitScale),
                IM_COL32(224,230,239,255),buildVersion);
        }
        // USB scan notification mirrors the actual Now Playing geometry and
        // alpha. Only a current, unfinished USB scan draws it; no result timer.
        if(storagePopupActive && storageThreadActive && storageJob==StorageJob::Scan &&
           !storageDone.load(std::memory_order_acquire) && storageExternalSnapshot==externalSongRoots &&
           screen!=Screen::Gameplay && screen!=Screen::SongCopyConfirm) {
            constexpr float x=1920.0f-previewX-previewWidth,y=previewY;
            const int firstVertex=draw->VtxBuffer.Size;
            const ImFont* font=ImGui::GetFont();
            draw->AddRectFilled(p(x,y),p(x+previewWidth,y+previewHeight),IM_COL32(10,10,10,153));
            draw->AddText(ratingFont?ratingFont:font,23*scale,p(x+15,y+18),IM_COL32(245,247,250,255),"READING USB SONGS");
            char line[120];
            std::snprintf(line,sizeof(line),"%u detected / %u available",storageUsbFound.load(),storageUsbLoaded.load());
            draw->AddText(font,19*scale,p(x+15,y+49),IM_COL32(230,234,241,255),line);
            const unsigned total=storageTotal.load(),done=storageProgress.load();
            if(total)std::snprintf(line,sizeof(line),"Reading charts: %u / %u",done,total);
            else std::snprintf(line,sizeof(line),"Reading song folders...");
            draw->AddText(font,17*scale,p(x+15,y+73),IM_COL32(205,211,220,255),line);
            const float left=x+13.9f,right=x+377.5f;
            draw->AddRectFilled(p(left,y+105),p(right,y+113),IM_COL32(90,108,132,255));
            if(total) {
                const float progress=std::clamp(float(done)/total,0.0f,1.0f);
                draw->AddRectFilled(p(left,y+105),p(left+(right-left)*progress,y+113),IM_COL32(245,247,250,255));
            } else {
                // Folder discovery has no known total yet. Use a moving bar
                // segment until the scan can report measured chart progress.
                const float t=float(std::fmod(ImGui::GetTime()*.5,1.0));
                const float start=left+(right-left)*.75f*t;
                draw->AddRectFilled(p(start,y+105),p(start+(right-left)*.25f,y+113),IM_COL32(245,247,250,255));
            }
            const auto reflect=[&](ImVec2 v){return ImVec2(2*ox+1920.0f*scale-v.x,v.y);};
            for(int i=firstVertex;i<draw->VtxBuffer.Size;++i)
                draw->VtxBuffer[i].pos=reflect(projectRightWidget(reflect(draw->VtxBuffer[i].pos),1920.0f-x));
        }
        popupLayer(true);
        draw->AddText(settingsFont?settingsFont:ImGui::GetFont(),16*scale,p(24,14),IM_COL32(235,225,195,235),"BETA BUILD - WORK IN PROGRESS");
        popupLayer(false);
        if (showFps) {
            char fpsText[32];
            const int fps = frameTimer > 0.0f ? static_cast<int>(0.5f + 1.0f / frameTimer) : 0;
            snprintf(fpsText,sizeof(fpsText),"%d FPS",fps);
            draw->AddText(ImGui::GetFont(),14.0f*scale,p(1805,menuVisible?48.0f:24.0f),IM_COL32(245,247,250,230),fpsText);
        }
        if (screen == Screen::Gameplay && !ui.visible && showHitWindow) {
            char windowText[48];
            const auto upcoming=gameplayEvaluator().nextPendingNote();
            const bool precision=(playerModifierMask[gameplayPlayer] & (1u<<7))!=0;
            if((precision || chart.kind==rhythm::ChartKind::Drums) && upcoming)
                snprintf(windowText,sizeof(windowText),"%s WINDOW  +/- %.1f ms",precision?"PRECISION":"HIT",(chart.kind==rhythm::ChartKind::Drums?drumSession.noteWindow(*upcoming):session.noteWindow(*upcoming)).early*1000.0);
            else snprintf(windowText, sizeof(windowText), "HIT WINDOW  +/- %.0f ms", hitWindowMs);
            draw->AddText(p(40, 52), IM_COL32(224,232,244,220), windowText);
        }
        if (screen == Screen::Gameplay && playMode == PlayMode::Practice && !ui.visible) {
            char rateText[48];
            snprintf(rateText, sizeof(rateText), "PRACTICE SPEED  %d%%",
                     static_cast<int>(practiceRate * 100.0f + 0.5f));
            draw->AddText(p(38, 28), IM_COL32(245,247,250,235), rateText);
            draw->AddText(p(38, 58), IM_COL32(205,215,230,220),
                    horizontalStrum ? "Pause > Restart Section: Yellow / Blue changes speed" : "Strum left / right: Change speed");
        }
#if !defined(PS5_HOST_REFERENCE)
        if(searchKeyboardOpen && nativeImePreferred) {
            popupLayer(true);
            const float ks=fitScale,kx=(io.DisplaySize.x-1920*ks)*.5f,ky=(io.DisplaySize.y-1080*ks)*.5f;
            const auto kp=[&](float x,float y){return ImVec2(kx+x*ks,ky+y*ks);};
            const char* title=searchKeyboardTarget==SearchKeyboardTarget::Profile?"Profile name":
                searchKeyboardTarget==SearchKeyboardTarget::Encore?"Search Encore":"Search installed songs";
            draw->AddRectFilled(kp(500,422),kp(1420,540),IM_COL32(9,14,25,240));
            draw->AddRectFilled(kp(500,422),kp(1420,460),IM_COL32(8,8,8,255));
            draw->AddText(ImGui::GetFont(),25*ks,kp(516,429),IM_COL32(246,246,246,255),title);
            const auto value=searchKeyboardText();
            const char* shown=value.empty()?"Enter text":value.c_str();
            draw->PushClipRect(kp(516,465),kp(1404,531),true);
            const float tw=ImGui::GetFont()->CalcTextSizeA(30*ks,100000,0,shown).x/ks;
            draw->AddText(ImGui::GetFont(),30*ks,kp(516-std::max(0.0f,tw-888),480),
                value.empty()?IM_COL32(116,120,128,255):IM_COL32(246,246,246,255),shown);
            draw->PopClipRect();
        }
#endif
        if(instrumentKeyboard.open) {
            popupLayer(true);
            // Compact system-style keyboard. Only source fonts and existing
            // fret prompt sprites are used; layout is independent of menu zoom.
            const float ks=fitScale,kx=(io.DisplaySize.x-1920*ks)*.5f,ky=(io.DisplaySize.y-1080*ks)*.5f;
            const auto kp=[&](float x,float y){return ImVec2(kx+x*ks,ky+y*ks);};
            const auto label=[&](const char* value,float x,float y,float size,ImU32 ink) {
                draw->AddText(ImGui::GetFont(),size*ks,kp(x,y),ink,value);
            };
            constexpr float left=500,right=1420,bottom=1008;
            const auto& error=keyboardError(searchKeyboardTarget);
            const float top=592-(error.empty()?0.0f:24.0f);
            const ImU32 white=IM_COL32(246,246,246,255),muted=IM_COL32(185,190,200,255);
            draw->AddRectFilled(ImVec2(0,0),io.DisplaySize,IM_COL32(0,0,0,112));
            draw->AddRectFilled(kp(left,top),kp(right,bottom),IM_COL32(9,14,25,128),4*ks);
            draw->AddRectFilled(kp(left,top),kp(right,top+42),IM_COL32(8,8,8,255));
            const char* title=searchKeyboardTarget==SearchKeyboardTarget::Profile?"Profile name":
                searchKeyboardTarget==SearchKeyboardTarget::Encore?"Search Encore":"Search installed songs";
            label(title,left+16,top+12,25,white);
            char counter[40];std::snprintf(counter,sizeof(counter),"%zu / %zu",instrumentKeyboard.text.size(),instrumentKeyboard.limit);
            label(counter,right-120,top+15,19,muted);
            draw->AddRectFilled(kp(left+16,top+46),kp(right-16,top+94),IM_COL32(13,15,19,255),3*ks);
            draw->PushClipRect(kp(left+28,top+49),kp(right-28,top+92),true);
            const char* typed=instrumentKeyboard.text.empty()?"Enter text":instrumentKeyboard.text.c_str();
            const float tw=ImGui::GetFont()->CalcTextSizeA(28*ks,100000,0,typed).x/ks;
            const float tx=left+28-std::max(0.0f,tw-(right-left-64));
            label(typed,tx,top+55,28,instrumentKeyboard.text.empty()?IM_COL32(116,120,128,255):white);
            const float caretX=tx+(instrumentKeyboard.text.empty()?0.0f:tw);
            if(std::fmod(ImGui::GetTime(),1.0)<.6)draw->AddRectFilled(kp(caretX+3,top+55),kp(caretX+5,top+84),white);
            draw->PopClipRect();
            for(int key=0;key<rhythm::InstrumentKeyboard::keyCount;++key) {
                float x,y,w;
                constexpr float step=(right-left-32+4)/10;
                if(key<40){x=left+16+(key%10)*step;y=top+106+(key/10)*48.0f;w=step-4;}
                else {
                    x=left+16+rhythm::InstrumentKeyboard::actionColumns[key-40]*step;
                    y=top+298;w=rhythm::InstrumentKeyboard::actionWidths[key-40]*step-4;}
                const bool selected=instrumentKeyboard.key==key;
                draw->AddRectFilled(kp(x,y),kp(x+w,y+44),selected?IM_COL32(243,244,247,255):IM_COL32(8,13,23,64),3*ks);
                char character[2]{instrumentKeyboard.letter(key),0};
                const char* name=key<40?character:key==40?"Space":key==41?"Clear":key==42?"Done":"Cancel";
                const float size=key<40?28:22;
                const float width=ImGui::GetFont()->CalcTextSizeA(size*ks,100000,0,name).x/ks;
                label(name,x+(w-width)*.5f,y+(44-size)*.5f,size,selected?IM_COL32(16,18,24,255):white);
                ImGui::SetCursorScreenPos(kp(x,y));
                ImGui::InvisibleButton(("##instrument-key"+std::to_string(key)).c_str(),ImVec2(w*ks,44*ks));
                if(ImGui::IsItemClicked()){instrumentKeyboard.setKey(key);updateInstrumentKeyboard(PAD_CROSS);}
            }
            const auto prompt=[&](int fret,const char* value,float x) {
                draw->AddImage(songSelectControlIds[fret],kp(x,top+354),kp(x+24,top+378));
                label(value,x+32,top+356,19,white);
            };
            prompt(0,"Type",left+16);prompt(1,"Delete",left+166);prompt(2,"Left",left+326);
            prompt(3,"Right",left+476);prompt(4,instrumentKeyboard.upper?"Lowercase":"Uppercase",left+666);
            const auto systemPrompt=[&](ImTextureID icon,const char* value,float x) {
                draw->AddImage(icon,kp(x,top+380),kp(x+64,top+413));
                label(value,x+74,top+386,21,white);
            };
            systemPrompt(songSelectControlIds[6],"Done",left+300);
            systemPrompt(songSelectControlIds[5],"Cancel",left+480);
            if(!error.empty())label(error.c_str(),left+16,top+414,18,IM_COL32(255,185,135,255));
        }
        if(popupActive)draw->ChannelsMerge();
        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
};

VULKAN_EXAMPLE_MAIN()
